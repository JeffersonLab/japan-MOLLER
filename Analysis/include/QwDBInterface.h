/*
 * QwDBInterface.h
 *
 *  Created on: Dec 14, 2010
 *      Author: jhlee
 */

#ifndef QWDBINTERFACE_H_
#define QWDBINTERFACE_H_

// System headers
#include <iostream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <map>

// ROOT headers
#include "Rtypes.h"
#include "TString.h"

// Qweak headers
#include "QwLog.h"

// Third Party Headers
#ifdef __USE_DATABASE__
#include <sqlpp11/sqlpp11.h>
#ifdef __USE_DATABASE_MYSQL__
#include <sqlpp11/mysql/mysql.h>
#endif // __USE_DATABASE_MYSQL__
#ifdef __USE_DATABASE_SQLITE3__
#include <sqlpp11/sqlite3/sqlite3.h>
#endif // __USE_DATABASE_SQLITE3__
#ifdef __USE_DATABASE_POSTGRESQL__
#include <sqlpp11/postgresql/postgresql.h>
#endif // __USE_DATABASE_POSTGRESQL__
#endif // __USE_DATABASE__

// Forward declarations
class QwParityDB;

#ifdef __USE_DATABASE__
#include "QwParitySchema.h"
namespace QwParitySchema {
  // Since sqlpp11 is a schema definition library, we need to define
  // our own data holding structures for bulk insertions.
  // These structures must match the database table structure.
  // They are simple structs with public members only.
  // We use sqlpp11::cpp_value_type_of to extract the C++ type
  // from the sqlpp11 table definition.
  // This ensures that the types always match the database schema.
  // Note that these data holding structures are a subset of the full
  // set of tables, limited to those tables where we use bulk inserts.
  // Note that these data holding structures do not need to include
  // all columns, only those we actually use.
  struct beam_row {
    using table_type = QwParitySchema::beam;
    sqlpp::cpp_value_type_of<decltype(table_type{}.analysis_id)::_traits::_value_type> analysis_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.monitor_id)::_traits::_value_type> monitor_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.measurement_type_id)::_traits::_value_type> measurement_type_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.subblock)::_traits::_value_type> subblock;
    sqlpp::cpp_value_type_of<decltype(table_type{}.n)::_traits::_value_type> n;
    sqlpp::cpp_value_type_of<decltype(table_type{}.value)::_traits::_value_type> value;
    sqlpp::cpp_value_type_of<decltype(table_type{}.error)::_traits::_value_type> error;
  };

  struct beam_errors_row {
    using table_type = QwParitySchema::beam_errors;
    sqlpp::cpp_value_type_of<decltype(table_type{}.analysis_id)::_traits::_value_type> analysis_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.monitor_id)::_traits::_value_type> monitor_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.error_code_id)::_traits::_value_type> error_code_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.n)::_traits::_value_type> n;
  };

  struct lumi_data_row {
    using table_type = QwParitySchema::lumi_data;
    sqlpp::cpp_value_type_of<decltype(table_type{}.analysis_id)::_traits::_value_type> analysis_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.lumi_detector_id)::_traits::_value_type> lumi_detector_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.measurement_type_id)::_traits::_value_type> measurement_type_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.subblock)::_traits::_value_type> subblock;
    sqlpp::cpp_value_type_of<decltype(table_type{}.n)::_traits::_value_type> n;
    sqlpp::cpp_value_type_of<decltype(table_type{}.value)::_traits::_value_type> value;
    sqlpp::cpp_value_type_of<decltype(table_type{}.error)::_traits::_value_type> error;
  };

  struct lumi_errors_row {
    using table_type = QwParitySchema::lumi_errors;
    sqlpp::cpp_value_type_of<decltype(table_type{}.analysis_id)::_traits::_value_type> analysis_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.lumi_detector_id)::_traits::_value_type> lumi_detector_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.error_code_id)::_traits::_value_type> error_code_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.n)::_traits::_value_type> n;
  };

  struct md_data_row {
    using table_type = QwParitySchema::md_data;
    sqlpp::cpp_value_type_of<decltype(table_type{}.analysis_id)::_traits::_value_type> analysis_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.main_detector_id)::_traits::_value_type> main_detector_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.measurement_type_id)::_traits::_value_type> measurement_type_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.subblock)::_traits::_value_type> subblock;
    sqlpp::cpp_value_type_of<decltype(table_type{}.n)::_traits::_value_type> n;
    sqlpp::cpp_value_type_of<decltype(table_type{}.value)::_traits::_value_type> value;
    sqlpp::cpp_value_type_of<decltype(table_type{}.error)::_traits::_value_type> error;
  };

  struct md_errors_row {
    using table_type = QwParitySchema::md_errors;
    sqlpp::cpp_value_type_of<decltype(table_type{}.analysis_id)::_traits::_value_type> analysis_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.main_detector_id)::_traits::_value_type> main_detector_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.error_code_id)::_traits::_value_type> error_code_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.n)::_traits::_value_type> n;
  };

  struct general_errors_row {
    using table_type = QwParitySchema::general_errors;
    sqlpp::cpp_value_type_of<decltype(table_type{}.analysis_id)::_traits::_value_type> analysis_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.error_code_id)::_traits::_value_type> error_code_id;
    sqlpp::cpp_value_type_of<decltype(table_type{}.n)::_traits::_value_type> n;
  };

} // namespace QwParitySchema
#endif

// QwDBInterface  GetDBEntry(TString subname);

// I extend a DB interface for QwIntegrationPMT to all subsystem,
// because the table structure are the same in the lumi_data table,
// the md_data table, and the beam table of MySQL database.
// Now every device-specified action will be done in
// the FillDB(QwParityDB *db, TString datatype) of QwBeamLine,
// QwMainCerenkovDetector, and QwLumi class.

class QwDBInterface {
 public:
  enum EQwDBIDataTableType{kQwDBI_OtherTable, kQwDBI_BeamTable, 
			   kQwDBI_MDTable, kQwDBI_LumiTable};
 private:
  static std::map<TString, TString> fPrefix;

  UInt_t fAnalysisId;
  UInt_t fDeviceId;
  UInt_t fSubblock;
  UInt_t fN;
  Double_t fValue;
  Double_t fError;
  Char_t fMeasurementTypeId[4];

  TString fDeviceName;

 private:
  template <class T>
  T TypedDBClone();

 public:
  static TString DetermineMeasurementTypeID(TString type, TString suffix = "",
					    Bool_t forcediffs = kFALSE);

 public:

    QwDBInterface()
    : fAnalysisId(0),fDeviceId(0),fSubblock(0),fN(0),fValue(0.0),fError(0.0) {
      std::strcpy(fMeasurementTypeId, "");fDeviceName ="";
    }
    virtual ~QwDBInterface() { }

    void SetAnalysisID(UInt_t id) {fAnalysisId = id;};
    void SetDetectorName(TString &in) {fDeviceName = in;};
    void SetDeviceID(UInt_t id) {fDeviceId = id;};
    void SetMonitorID(QwParityDB *db);
    void SetMainDetectorID(QwParityDB *db);
    void SetLumiDetectorID(QwParityDB *db);
    EQwDBIDataTableType SetDetectorID(QwParityDB *db);
    void SetMeasurementTypeID(const TString& in) {
      std::strncpy(fMeasurementTypeId, in.Data(), 3);
      fMeasurementTypeId[3] = '\0';
    };
    void SetMeasurementTypeID(const char* in) {
      std::strncpy(fMeasurementTypeId, in, 3);
      fMeasurementTypeId[3] = '\0';
    };
    void SetSubblock(UInt_t in) {fSubblock = in;};
    void SetN(UInt_t in)        {fN = in;};
    void SetValue(Double_t in)  {fValue = in;};
    void SetError(Double_t in)  {fError = in;};

    TString GetDeviceName() {return fDeviceName;};

    void Reset() {
      fAnalysisId = 0;
      fDeviceId = 0;
      fSubblock = 0;
      fN = 0;
      fValue = 0.0;
      fError = 0.0;
      std::strcpy(fMeasurementTypeId,"");
      fDeviceName = "";
    };

    template <class T> inline
    void AddThisEntryToList(std::vector<T> &list);


    void PrintStatus(Bool_t print_flag) {
      if(print_flag) {
        QwMessage << std::setw(12)
                  << " AnalysisID " << fAnalysisId
                  << " Device :"    << std::setw(30) << fDeviceName
                  << ":" << std::setw(4) << fDeviceId
                  << " Subblock "   << fSubblock
                  << " n "          << fN
                  << " Type "       << fMeasurementTypeId
                  << " [ave, err] "
                  << " [" << std::setw(14) << fValue
                  << ","  << std::setw(14) << fError
                  << "]"
                  << QwLog::endl;
      }
    }
};

//
//  Template definitions for the QwDBInterface class.
//
//

template <class T>
inline void QwDBInterface::AddThisEntryToList(std::vector<T> &list)
{
  Bool_t okay = kTRUE;
  if (fAnalysisId == 0) {
    QwError << "QwDBInterface::AddDBEntryToList:  Analysis ID invalid; entry dropped"
            << QwLog::endl;
    okay = kFALSE;
  }
  if (fDeviceId == 0) {
    QwError << "QwDBInterface::AddDBEntryToList:  Device ID invalid; entry dropped"
            << QwLog::endl;
    okay = kFALSE;
  }
  if (okay) {
    T row = TypedDBClone<T>();
    // Note: analysis_id validation done above with fAnalysisId check
    list.push_back(row);
  }
  if (okay == kFALSE) {
    PrintStatus(kTRUE);
  };
}






class QwErrDBInterface {

 private:

  UInt_t fAnalysisId;
  UInt_t fDeviceId;
  UInt_t fErrorCodeId;
  UInt_t fN;

  TString fDeviceName;

  template <class T>
  T TypedDBClone();


 public:

    QwErrDBInterface()
    : fAnalysisId(0),fDeviceId(0),fErrorCodeId(0),fN(0) {
      fDeviceName ="";
    }
    virtual ~QwErrDBInterface() { }

    void SetAnalysisID(UInt_t id) {fAnalysisId = id;};
    void SetDeviceName(TString &in) {fDeviceName = in;};
    void SetDeviceID(UInt_t id) {fDeviceId = id;};

    void SetMonitorID(QwParityDB *db);
    void SetMainDetectorID(QwParityDB *db);
    void SetLumiDetectorID(QwParityDB *db);

    void SetErrorCodeId(UInt_t in) {fErrorCodeId = in;};
    void SetN(UInt_t in)        {fN = in;};

    TString GetDeviceName() {return fDeviceName;};

    void Reset() {
      fAnalysisId = 0;
      fDeviceId = 0;
      fErrorCodeId = 0;
      fN = 0;
      fDeviceName = "";
    };

    template <class T> inline
    void AddThisEntryToList(std::vector<T> &list);

    void PrintStatus(Bool_t print_flag) {
      if(print_flag) {
        QwMessage << std::setw(12)
                  << " AnalysisID " << fAnalysisId
                  << " Device :"    << std::setw(30) << fDeviceName
                  << ":" << std::setw(4) << fDeviceId
                  << " ErrorCode "   << fErrorCodeId
                  << " n "          << fN
                  << QwLog::endl;
      }
    }
};

template <class T>
inline void QwErrDBInterface::AddThisEntryToList(std::vector<T> &list)
{
  Bool_t okay = kTRUE;
  if (fAnalysisId == 0) {
    QwError << "QwErrDBInterface::AddDBEntryToList:  Analysis ID invalid; entry dropped"
            << QwLog::endl;
    okay = kFALSE;
  }
  if (fDeviceId == 0) {
    QwError << "QwErrDBInterface::AddDBEntryToList:  Device ID invalid; entry dropped"
            << QwLog::endl;
    okay = kFALSE;
  }
  if (okay) {
    T row = TypedDBClone<T>();
    // Note: analysis_id validation done above with fAnalysisId check
    list.push_back(row);
  }
  if (okay == kFALSE) {
    PrintStatus(kTRUE);
  };
}

#endif /* QWDBINTERFACE_H_ */
