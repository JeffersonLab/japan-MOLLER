/*!
 * \file   QwDatabase.h
 * \brief  A class for handling connections to the Qweak database.
 *
 * \author Damon Spayde
 * \date   2010-01-07
 */

#ifndef QWDATABASE_HH
#define QWDATABASE_HH

// System headers
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <typeinfo>
#include <variant>

// Third Party Headers
#include <sqlpp11/sqlpp11.h>
#ifdef __USE_DATABASE_MYSQL__
#include <sqlpp11/mysql/mysql.h>
#endif
#ifdef __USE_DATABASE_SQLITE3__
#include <sqlpp11/sqlite3/sqlite3.h>
#endif
#ifdef __USE_DATABASE_POSTGRESQL__
#include <sqlpp11/postgresql/postgresql.h>
#endif

// ROOT headers
#include "TString.h"

// Qweak headers
#include "QwTypes.h"
#include "QwLog.h"
#include "QwColor.h"
#include "QwOptions.h"

// Forward declarations

/**
 *  \class QwDatabase
 *  \ingroup QwAnalysis
 *  \brief A database interface class
 *
 * This class provides the connection to the Qweak database to other objects
 * in the Qweak analyzer.  A static global object gQwDatabase is used to
 * provide these services.
 *
 */

class QwDatabase {
  private:
    enum EQwDBType {kQwDatabaseNone, kQwDatabaseMySQL, kQwDatabaseSQLite3, kQwDatabasePostgreSQL};
    EQwDBType fDBType = kQwDatabaseNone;  //!< Type of database backend to use

    // Define connection types conditionally
#ifdef __USE_DATABASE_SQLITE3__
    using SQLiteConnection = std::shared_ptr<sqlpp::sqlite3::connection>;
#endif
#ifdef __USE_DATABASE_MYSQL__
    using MySQLConnection = std::shared_ptr<sqlpp::mysql::connection>;
#endif
#ifdef __USE_DATABASE_POSTGRESQL__
    using PostgreSQLConnection = std::shared_ptr<sqlpp::postgresql::connection>;
#endif

    // Build the variant type conditionally - much simpler approach!
    using DatabaseConnection = std::variant<
      std::monostate  // Always include monostate as fallback
#ifdef __USE_DATABASE_SQLITE3__
      , SQLiteConnection
#endif
#ifdef __USE_DATABASE_MYSQL__
      , MySQLConnection
#endif
#ifdef __USE_DATABASE_POSTGRESQL__
      , PostgreSQLConnection
#endif
    >;

// Return type for QuerySelect that supports all possible result types
    template<typename Statement>
    using QuerySelectReturnType = std::variant<
      std::monostate  // Always include monostate as fallback
#ifdef __USE_DATABASE_SQLITE3__
      , decltype((*std::declval<SQLiteConnection>())(std::declval<Statement>()))
#endif
#ifdef __USE_DATABASE_MYSQL__
      , decltype((*std::declval<MySQLConnection>())(std::declval<Statement>()))
#endif
#ifdef __USE_DATABASE_POSTGRESQL__
      , decltype((*std::declval<PostgreSQLConnection>())(std::declval<Statement>()))
#endif
    >;

    DatabaseConnection fDBConnection;

  public:

    QwDatabase(const string &major = "00", const string &minor = "00", const string &point = "0000"); //!< Simple constructor
    QwDatabase(QwOptions &options, const string &major = "00", const string &minor = "00", const string &point = "0000"); //!< Constructor with QwOptions object

    virtual ~QwDatabase(); //!< Destructor

    void         SetAccessLevel(string accesslevel);  //!< Sets the access level flag based on string labels:  "off", "ro", "rw".

    Bool_t       AllowsReadAccess(){return (fAccessLevel==kQwDatabaseReadOnly || fAccessLevel==kQwDatabaseReadWrite);};
    Bool_t       AllowsWriteAccess(){return (fAccessLevel==kQwDatabaseReadWrite);};

    Bool_t       Connect();                    //!< Open a connection to the database using the predefined parameters.
    void         Disconnect() {
      std::visit([](auto& conn) {
        if (conn) {
          conn.reset();
        }
      }, fDBConnection);
    }; //<! Close an open database connection
    Bool_t       Connected() {
      return std::visit([](const auto& conn) -> bool {
        return conn != nullptr;
      }, fDBConnection);
    }
    const string GetServerVersion() {
      // FIXME (wdconinc) implement server_version();
      return "";
    }; //<! Get database server version
    static void  DefineOptions(QwOptions& options); //!< Defines available class options for QwOptions
    void ProcessOptions(QwOptions &options); //!< Processes the options contained in the QwOptions object.
    void ProcessOptions(const EQwDBType& dbtype, const TString& dbname, const TString& username, const TString& passwd, const TString& dbhost="localhost", const Int_t dbport = 0, const TString& accesslevel = "ro"); //!< Processes database options

    // Separate methods for different query types to avoid std::visit return type issues
    template<typename Statement>
    size_t QueryCount(const Statement& statement) {
      return std::visit([&statement](auto& connection) -> size_t {
        if (!connection) {
          throw std::runtime_error("Database not connected");
        }
        auto result = (*connection)(statement);
        size_t count = 0;
        for (const auto& row : result) {
          (void)row; // Suppress unused variable warning
          count++;
        }
        return count;
      }, fDBConnection);
    }
    
    template<typename Statement>
    bool QueryExists(const Statement& statement) {
      return QueryCount(statement) > 0;
    } //<! Generate a query to check existence in the database.
    
    template<typename Statement>
    auto QuerySelect(const Statement& statement) {
      // Use a lambda to handle the variant result in a type-safe way
      return std::visit([&statement](auto& connection) -> 
          std::variant<
            decltype((*std::declval<SQLiteConnection>())(std::declval<Statement>())),
            decltype((*std::declval<MySQLConnection>())(std::declval<Statement>()))
          > {
        if (!connection) {
          throw std::runtime_error("Database not connected");
        }
        using ResultType = decltype((*connection)(statement));
        using SQLiteResult = decltype((*std::declval<SQLiteConnection>())(std::declval<Statement>()));
        using MySQLResult = decltype((*std::declval<MySQLConnection>())(std::declval<Statement>()));
        using ReturnVariant = std::variant<SQLiteResult, MySQLResult>;
        
        return ReturnVariant{(*connection)(statement)};
      }, fDBConnection);
    } //<! Execute a SELECT statement and return the result.
    
    template<typename Statement>
    void QueryExecute(const Statement& statement) {
      std::visit([&statement](auto& connection) {
        if (!connection) {
          throw std::runtime_error("Database not connected");
        }
        (*connection)(statement);
      }, fDBConnection);
    } //<! Execute a statement without returning a result.
    
    template<typename InsertStatement>
    uint64_t QueryInsertAndGetId(const InsertStatement& statement) {
      return std::visit([&statement](auto& connection) -> uint64_t {
        if (!connection) {
          throw std::runtime_error("Database not connected");
        }
        auto result = (*connection)(statement);
        // For INSERT operations, some databases return the ID directly as uint64_t
        // while others return it as a result object with insert_id() method
        using connection_type = typename std::decay_t<decltype(*connection)>;
        if constexpr (std::is_same_v<connection_type, sqlpp::normal_connection<sqlpp::sqlite3::connection_base>>) {
          // SQLite might return the ID directly or have it in result
          if constexpr (std::is_integral_v<decltype(result)>) {
            return static_cast<uint64_t>(result);
          } else {
            return result.insert_id();
          }
        } else if constexpr (std::is_same_v<connection_type, sqlpp::normal_connection<sqlpp::mysql::connection_base>>) {
          // MySQL might return the ID directly or have it in result
          if constexpr (std::is_integral_v<decltype(result)>) {
            return static_cast<uint64_t>(result);
          } else {
            return result.insert_id();
          }
        }
        return 0;
      }, fDBConnection);
    } //<! Execute an INSERT statement and return the auto-increment ID.
    
    const string GetVersion();                             //! Return a full version string for the DB schema
    const string GetVersionMajor() {return fVersionMajor;} //<! fVersionMajor getter
    const string GetVersionMinor() {return fVersionMinor;} //<! fVersionMinor getter
    const string GetVersionPoint() {return fVersionPoint;} //<! fVersionPoint getter
    const string GetValidVersion();                        //<! Return a full required version string.

    void         PrintServerInfo();                        //<! Print Server Information

 private:
    enum EQwDBAccessLevel{kQwDatabaseOff, kQwDatabaseReadOnly, kQwDatabaseReadWrite};
 private:

    Bool_t       ValidateConnection();                  //!< Checks that given connection parameters result in a valid connection
    bool StoreDBVersion();  //!< Retrieve database schema version information from database

    // Do not allow compiler to automatically generated these functions
    QwDatabase(const QwDatabase& rhs);  //!< Copy Constructor (not implemented)
    QwDatabase& operator= (const QwDatabase& rhs); //!< Assignment operator (not implemented)
//    QwDatabase* operator& (); //!< Address-of operator (not implemented)
//    const QwDatabase* operator& () const; //!< Address-of operator (not implemented)

    EQwDBAccessLevel fAccessLevel;  //!< Access level of the database instance

    string fDatabase;        //!< Name of database to connect to
    string fDBServer;        //!< Name of server carrying DB to connect to
    string fDBUsername;      //!< Name of account to connect to DB server with
    string fDBPassword;      //!< DB account password
    UInt_t fDBPortNumber;    //!< Port number to connect to on server (mysql default port is 3306)
    Bool_t fValidConnection; //!< True if a valid connection was established using defined connection information

    string fVersionMajor;    //!< Major version number of current DB schema
    string fVersionMinor;    //!< Minor version number of current DB schema
    string fVersionPoint;    //!< Point version number of current DB schema
    const string kValidVersionMajor;
    const string kValidVersionMinor;
    const string kValidVersionPoint;

};

#endif
