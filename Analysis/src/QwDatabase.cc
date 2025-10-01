/*!
 * \file   QwDatabase.cc
 * \brief  A class for handling connections to the Qweak database.
 *
 * \author Damon Spayde
 * \date   2010-01-07
 */


#include "QwDatabase.h"

// System headers

// Third-party headers
#include <sqlpp11/select.h>
#include <sqlpp11/functions.h>

// Qweak headers
#include "QwParitySchema.h"

// QwScopedConnection implementation
QwScopedConnection::QwScopedConnection(QwDatabase* db) 
    : fDatabase(db), fConnected(false) {
    if (fDatabase) {
        fConnected = fDatabase->Connect();
        if (!fConnected) {
            QwError << "QwScopedConnection: Failed to establish database connection" << QwLog::endl;
        }
    }
}

QwScopedConnection::~QwScopedConnection() {
    if (fDatabase && fConnected) {
        fDatabase->Disconnect();
        fConnected = false;
    }
}

QwScopedConnection::QwScopedConnection(QwScopedConnection&& other) noexcept
    : fDatabase(other.fDatabase), fConnected(other.fConnected) {
    other.fDatabase = nullptr;
    other.fConnected = false;
}

QwScopedConnection& QwScopedConnection::operator=(QwScopedConnection&& other) noexcept {
    if (this != &other) {
        // Clean up current connection
        if (fDatabase && fConnected) {
            fDatabase->Disconnect();
        }
        
        // Move from other
        fDatabase = other.fDatabase;
        fConnected = other.fConnected;
        
        // Clear other
        other.fDatabase = nullptr;
        other.fConnected = false;
    }
    return *this;
}

bool QwScopedConnection::IsConnected() const {
    return fConnected && fDatabase && fDatabase->Connected();
}


/*! The simple constructor initializes member fields.  This class is not
 * used to establish the database connection.
 */
//QwDatabase::QwDatabase() : Connection(false)
QwDatabase::QwDatabase(const string &major, const string &minor, const string &point) : kValidVersionMajor(major), kValidVersionMinor(minor), kValidVersionPoint(point)
{
  QwDebug << "Greetings from QwDatabase simple constructor." << QwLog::endl;

  // Initialize member fields
  fDatabase=fDBServer=fDBUsername=fDBPassword="";
  fVersionMajor=fVersionMinor=fVersionPoint="";

  fAccessLevel = kQwDatabaseOff;

  fDBPortNumber      = 0;
  fValidConnection   = false;

}

/*! The constructor initializes member fields using the values in
 *  the QwOptions object.
 * @param options  The QwOptions object.
 * @param major Major version number
 * @param minor Minor version number
 * @param point Point revision number
 */
QwDatabase::QwDatabase(QwOptions &options, const string &major, const string &minor, const string &point) : kValidVersionMajor(major), kValidVersionMinor(minor), kValidVersionPoint(point)
{
  QwDebug << "Greetings from QwDatabase extended constructor." << QwLog::endl;

  // Initialize member fields
  fDatabase=fDBServer=fDBUsername=fDBPassword="";
  fVersionMajor=fVersionMinor=fVersionPoint="";

  fAccessLevel = kQwDatabaseOff;

  fDBPortNumber      = 0;
  fValidConnection   = false;

  ProcessOptions(options);

}

/*! The destructor says "Good-bye World!"
 */
QwDatabase::~QwDatabase()
{
  QwDebug << "QwDatabase::~QwDatabase() : Good-bye World from QwDatabase destructor!" << QwLog::endl;
  if (Connected()) Disconnect();
}

/*! This function is used to load the connection information for the
 * database.  It tests the connection to make sure it is valid and causes a
 * program exit if no valid connection can be formed.
 *
 * It is called the first time Connect() is called.
 */
Bool_t QwDatabase::ValidateConnection()
{
  QwDebug << "Entering QwDatabase::ValidateConnection()..." << QwLog::endl;

  // Bool_t status;
  //
  // Retrieve options if they haven't already been filled.
  //
  //   if (fDatabase.empty()){
  //     status = ProcessOptions(gQwOptions);
  //     if (!status) return status;
  //   }

  //  Check values.
  if (fAccessLevel!=kQwDatabaseOff){
    if (fDatabase.empty()){
      QwError << "QwDatabase::ValidateConnection() : No database supplied.  Unable to connect." << QwLog::endl;
      fValidConnection=false;
    }
#ifdef __USE_DATABASE_SQLITE3__
    if (fDBType != kQwDatabaseSQLite3) {
#else
    // If SQLite3 is not available, all databases require username/password
    if (true) {
#endif
      if (fDBUsername.empty()){
        QwError << "QwDatabase::ValidateConnection() : No database username supplied.  Unable to connect." << QwLog::endl;
        fValidConnection=false;
      }
      if (fDBPassword.empty()){
        QwError << "QwDatabase::ValidateConnection() : No database password supplied.  Unable to connect." << QwLog::endl;
        fValidConnection=false;
      }
      if (fDBServer.empty()){
        QwMessage << "QwDatabase::ValidateConnection() : No database server supplied.  Attempting localhost." << QwLog::endl;
        fDBServer = "localhost";
      }
    }
    //
    // Try to connect with given information
    //
    try {
      // FIXME (wdconinc) duplication with Connect
      switch(fDBType) 
      {
#ifdef __USE_DATABASE_MYSQL__
        case kQwDatabaseMySQL: {
          QwDebug << "QwDatabase::ValidateConnection() : Using MySQL backend." << QwLog::endl;
          sqlpp::mysql::connection_config config;
          config.host = fDBServer;
          config.user = fDBUsername;
          config.password = fDBPassword;
          config.database = fDatabase;
          config.port = fDBPortNumber;
          config.debug = fDBDebug;
          fDBConnection = std::make_shared<sqlpp::mysql::connection>(config);
          break;
        }
#endif
#ifdef __USE_DATABASE_SQLITE3__
        case kQwDatabaseSQLite3: {
          QwDebug << "QwDatabase::ValidateConnection() : Using SQLite3 backend." << QwLog::endl;
          sqlpp::sqlite3::connection_config config;
          config.path_to_database = fDatabase;
          config.password = fDBPassword;
          // FIXME (wdconinc) use proper access flags
          config.flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
          config.debug = fDBDebug;
          fDBConnection = std::make_shared<sqlpp::sqlite3::connection>(config);
          break;
        }
#endif
#ifdef __USE_DATABASE_POSTGRESQL__
        case kQwDatabasePostgreSQL: {
          QwDebug << "QwDatabase::ValidateConnection() : Using PostgreSQL backend." << QwLog::endl;
          sqlpp::postgresql::connection_config config;
          config.host = fDBServer;
          config.user = fDBUsername;
          config.password = fDBPassword;
          config.database = fDatabase;
          config.port = fDBPortNumber;
          config.debug = fDBDebug;
          fDBConnection = std::make_shared<sqlpp::postgresql::connection>(config);
          break;
        }
#endif
        default: {
          QwError << "QwDatabase::ValidateConnection() : Unsupported database type." << QwLog::endl;
          fAccessLevel = kQwDatabaseOff;
          return false;
        }
      }
    } catch (std::exception const& e) {
      QwError << "QwDatabase::ValidateConnection() : " << QwLog::endl;
      QwError << e.what() << " while validating connection" << QwLog::endl;
      QwError << "Database name = " << fDatabase <<QwLog::endl;
      QwError << "Database server = " << fDBServer <<QwLog::endl;
      QwError << "Database username = " << fDBUsername <<QwLog::endl;
      QwError << "Database port = " << fDBPortNumber <<QwLog::endl;
      QwError << "Continuing without database." << QwLog::endl;
      QwWarning << "Might have left database connection dangling..." << QwLog::endl;
      fAccessLevel = kQwDatabaseOff;
      return kFALSE;
    }

    QwDebug << "QwDatabase::ValidateConnection() : Made it past connect() call."  << QwLog::endl;

    // Get database schema version information
    if (StoreDBVersion()) {
      fValidConnection = true;
      // Success!
      QwMessage << "QwDatabase::ValidateConnection() : Successfully connected to requested database." << QwLog::endl;
    } else {
      QwError << "QwDatabase::ValidateConnection() : Unsuccessfully connected to requested database." << QwLog::endl;
      // Connection was bad so clear the member variables
      fValidConnection = false;
      fDatabase.clear();
      fDBServer.clear();
      fDBUsername.clear();
      fDBPassword.clear();
      fDBPortNumber=0;
    }
    Disconnect();
  }

  // Check to make sure database and QwDatabase schema versions match up.
  if (fAccessLevel == kQwDatabaseReadWrite && 
      (fVersionMajor != kValidVersionMajor ||
       fVersionMinor != kValidVersionMinor ||
       fVersionPoint <  kValidVersionPoint)) {
    fValidConnection = false;
    QwError << "QwDatabase::ValidConnection() : Connected database schema inconsistent with current version of analyzer." << QwLog::endl;
    QwError << "  Database version is " << this->GetVersion() << QwLog::endl;
    QwError << "  Required database version is " << this->GetValidVersion() << QwLog::endl;
    QwError << "  Please connect to a database supporting the required schema version." << QwLog::endl;
    exit(1);
  }

  QwDebug << "QwDatabase::ValidateConnection() : Exiting successfully." << QwLog::endl;
  return fValidConnection;
}


/*! This function is used to initiate a database connection.
 */
bool QwDatabase::Connect()
{
  /* Open a connection to the database using the predefined parameters.
   * Must call QwDatabase::ConnectionInfo() first.
   */

  //  Return flase, if we're not using the DB.
  if (fAccessLevel==kQwDatabaseOff) return false;

  // Make sure not already connected
  if (Connected()) return true;

  // If never connected before, then make sure connection parameters form
  // valid connection
  if (!fValidConnection) {
    ValidateConnection();
  }

  if (fValidConnection) {
    // FIXME (wdconinc) duplication with ValidateConnection
    try {
      switch(fDBType) 
      {
#ifdef __USE_DATABASE_MYSQL__
        case kQwDatabaseMySQL: {
          QwDebug << "QwDatabase::ValidateConnection() : Using MySQL backend." << QwLog::endl;
          sqlpp::mysql::connection_config config;
          config.host = fDBServer;
          config.user = fDBUsername;
          config.password = fDBPassword;
          config.database = fDatabase;
          config.port = fDBPortNumber;
          fDBConnection = std::make_shared<sqlpp::mysql::connection>(config);
          break;
        }
#endif
#ifdef __USE_DATABASE_SQLITE3__
        case kQwDatabaseSQLite3: {
          QwDebug << "QwDatabase::ValidateConnection() : Using SQLite3 backend." << QwLog::endl;
          sqlpp::sqlite3::connection_config config;
          config.path_to_database = fDatabase;
          config.password = fDBPassword;
          // FIXME (wdconinc) use proper access flags
          config.flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
          config.debug = fDBDebug;
          fDBConnection = std::make_shared<sqlpp::sqlite3::connection>(config);
          break;
        }
#endif
#ifdef __USE_DATABASE_POSTGRESQL__
        case kQwDatabasePostgreSQL: {
          QwDebug << "QwDatabase::ValidateConnection() : Using PostgreSQL backend." << QwLog::endl;
          sqlpp::postgresql::connection_config config;
          config.host = fDBServer;
          config.user = fDBUsername;
          config.password = fDBPassword;
          config.database = fDatabase;
          config.port = fDBPortNumber;
          fDBConnection = std::make_shared<sqlpp::postgresql::connection>(config);
          break;
        }
#endif
        default: {
          QwError << "QwDatabase::ValidateConnection() : Unsupported database type." << QwLog::endl;
          return false;
        }
      }
    } catch (const std::exception& e) {
      QwError << "QwDatabase::Connect() : " << QwLog::endl;
      QwError << e.what() << " while connecting to database" << QwLog::endl;
      fValidConnection = false;
      return false;
    }
    return true;
  } else {
    QwError << "QwDatabase::Connect() : Must establish valid connection to database." << QwLog::endl;
    return false;
  }
}

/*!
 * Defines configuration options for QwDatabase class using QwOptions
 * functionality.
 *
 * Should apparently by called by QwOptions::DefineOptions() in
 * QwParityOptions.h
 */
void QwDatabase::DefineOptions(QwOptions& options)
{
  // Specify command line options for use by QwDatabase
  // FIXME (wdconinc) add database type option
  options.AddOptions("Database options")("QwDatabase.accesslevel", po::value<string>(), "database access level (OFF,RO,RW)");
  options.AddOptions("Database options")("QwDatabase.dbname", po::value<string>(), "database name or path");
  options.AddOptions("Database options")("QwDatabase.dbserver", po::value<string>(), "database server name");
  options.AddOptions("Database options")("QwDatabase.dbusername", po::value<string>(), "database username");
  options.AddOptions("Database options")("QwDatabase.dbpassword", po::value<string>(), "database password");
  options.AddOptions("Database options")("QwDatabase.dbport", po::value<int>()->default_value(0), "database server port number (defaults to standard mysql port)");
  options.AddOptions("Database options")("QwDatabase.debug", po::value<bool>()->default_value(false), "enable database debug output (default false)");
  options.AddOptions("Database options")("QwDatabase.insert-missing-keys", po::value<bool>()->default_value(false), "insert missing keys into the database (default false)");

  std::stringstream dbtypes;
  dbtypes << "none";
#ifdef __USE_DATABASE_SQLITE3__
  dbtypes << ",sqlite3";
#endif // __USE_DATABASE_SQLITE3__
#ifdef __USE_DATABASE_MYSQL__
  dbtypes << ",mysql";
#endif // __USE_DATABASE_MYSQL__
#ifdef __USE_DATABASE_POSTGRESQL__
  dbtypes << ",postgresql";
#endif // __USE_DATABASE_POSTGRESQL__
  std::stringstream desc;
  desc << "database type (" << dbtypes.str() << ")";
  options.AddOptions("Database options")("QwDatabase.dbtype", po::value<string>()->default_value("none"), desc.str().c_str());
}

/*!
 * Loads the configuration options for QwDatabase class into this instance of
 * QwDatabase from the QwOptions object.
 * @param options Options object
 */
void QwDatabase::ProcessOptions(QwOptions &options)
{
  if (options.HasValue("QwDatabase.accesslevel")) {
    string access = options.GetValue<string>("QwDatabase.accesslevel");
    SetAccessLevel(access);
  }
  else {
    QwWarning << "QwDatabase::ProcessOptions : No access level specified; database access is OFF" << QwLog::endl;
    fAccessLevel = kQwDatabaseOff;
  }
  if (options.HasValue("QwDatabase.dbtype")) {
    string dbtype = options.GetValue<string>("QwDatabase.dbtype");
    fDBType = kQwDatabaseNone;
#ifdef __USE_DATABASE_MYSQL__
    if (dbtype == "mysql") {
      fDBType = kQwDatabaseMySQL;
    }
#endif
#ifdef __USE_DATABASE_SQLITE3__
    if (dbtype == "sqlite3") {
      fDBType = kQwDatabaseSQLite3;
    }
#endif
#ifdef __USE_DATABASE_POSTGRESQL__
    if (dbtype == "postgresql") {
      fDBType = kQwDatabasePostgreSQL;
    }
#endif
    if (fDBType == kQwDatabaseNone) {
      QwWarning << "QwDatabase::ProcessOptions : Unrecognized database type \"" << dbtype << "\"; using none" << QwLog::endl;
    }
  } else {
    QwMessage << "QwDatabase::ProcessOptions : No database type specified" << QwLog::endl;
    fDBType = kQwDatabaseNone;
  }
  if (options.HasValue("QwDatabase.dbport")) {
    fDBPortNumber = options.GetValue<int>("QwDatabase.dbport");
  }
  if (options.HasValue("QwDatabase.dbname")) {
    fDatabase = options.GetValue<string>("QwDatabase.dbname");
  }
  if (options.HasValue("QwDatabase.dbusername")) {
    fDBUsername = options.GetValue<string>("QwDatabase.dbusername");
  }
  if (options.HasValue("QwDatabase.dbpassword")) {
    fDBPassword = options.GetValue<string>("QwDatabase.dbpassword");
  }
  if (options.HasValue("QwDatabase.dbserver")) {
    fDBServer = options.GetValue<string>("QwDatabase.dbserver");
  }
  if (options.HasValue("QwDatabase.debug")) {
    fDBDebug = options.GetValue<bool>("QwDatabase.debug");
  }
  if (options.HasValue("QwDatabase.insert-missing-keys")) {
    fDBInsertMissingKeys = options.GetValue<bool>("QwDatabase.insert-missing-keys");
  }
}

void QwDatabase::ProcessOptions(const EQwDBType& dbtype, const TString& dbname, const TString& username, const TString& passwd, const TString& dbhost, const Int_t dbport, const TString& accesslevel)
{
  SetAccessLevel(static_cast<string>(accesslevel));
  fDBType = dbtype;
  fDatabase = dbname;
  fDBUsername = username;
  fDBPassword = passwd;
  fDBServer = dbhost;
  fDBPortNumber = dbport;
}

void QwDatabase::SetAccessLevel(string accesslevel)
{
  TString level = accesslevel.c_str();
  level.ToLower();
  if (level=="off")     fAccessLevel = kQwDatabaseOff;
  else if (level=="ro") fAccessLevel = kQwDatabaseReadOnly;
  else if (level=="rw") fAccessLevel = kQwDatabaseReadWrite;
  else{
    QwWarning << "QwDatabase::SetAccessLevel  : Unrecognized access level \""
	      << accesslevel << "\"; setting database access OFF"
	      << QwLog::endl;
    fAccessLevel = kQwDatabaseOff;
  }
  return;
}

/*!
 * This function prints the server information.
 */
void QwDatabase::PrintServerInfo()
{
  if (fValidConnection)
    {
      printf("\nQwDatabase MySQL ");
      printf("%s v%s %s -----------------\n", BOLD, GetServerVersion().c_str(), NORMAL);
      printf("Database server : %s%10s%s",    RED,  fDBServer.c_str(),   NORMAL);
      printf(" name   : %s%12s%s",            BLUE, fDatabase.c_str(),   NORMAL);
      printf(" user   : %s%6s%s",             RED,  fDBUsername.c_str(), NORMAL);
      printf(" port   : %s%6d%s\n",           BLUE, fDBPortNumber,       NORMAL);
      // FIXME (wdconinc) implement server_status();
      //printf(" %s\n\n", server_status().c_str());
    }
  else
    {
      printf("There is no connection.\n");
    }

  return;
}


const string QwDatabase::GetVersion(){
  string version = fVersionMajor + "." + fVersionMinor + "." + fVersionPoint;
  return version;
}

const string QwDatabase::GetValidVersion(){
  string version = kValidVersionMajor + "." + kValidVersionMinor + "." + kValidVersionPoint;
  return version;
}

/*
 * Retrieves database schema version information from database.
 * Returns true if successful, false otherwise.
 */
bool QwDatabase::StoreDBVersion()
{
  try
    {
      QwParitySchema::db_schema db_schema;
      size_t record_count = QueryCount(
          sqlpp::select(sqlpp::all_of(db_schema))
          .from(db_schema)
          .unconditionally()
      );
      QwDebug << "QwDatabase::StoreDBVersion => Number of rows returned:  " << record_count << QwLog::endl;

      // If there is more than one run in the DB with the same run number, then there will be trouble later on.  Catch and bomb out.
      if (record_count > 1)
      {
        QwError << "Unable to find unique schema version in database." << QwLog::endl;
        QwError << "Schema query returned " << record_count << " rows." << QwLog::endl;
        QwError << "Please make sure that db_schema contains one unique." << QwLog::endl;
        Disconnect();
        return false;
      }

      // Run already exists in database.  Pull run_id and move along.
      if (record_count == 1)
      {
        auto results = QuerySelect(
            sqlpp::select(sqlpp::all_of(db_schema))
            .from(db_schema)
            .unconditionally()
        );
        ForFirstResult(
          results,
          [this](const auto& row) {
            QwDebug << "QwDatabase::StoreDBVersion => db_schema_id = " << row.db_schema_id << QwLog::endl;
            this->fVersionMajor = row.major_release_number;
            this->fVersionMinor = row.minor_release_number;
            this->fVersionPoint = row.point_release_number;
          }
        );
        Disconnect();
      }
    }
  catch (const std::exception& er)
    {
      QwError << er.what() << QwLog::endl;
      Disconnect();
      return false;
    }

    return true;

}
