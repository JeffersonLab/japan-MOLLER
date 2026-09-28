/*!
 * \file   QwParityDB.cc
 * \brief  A class for handling connections to the Qweak database.
 *
 * \author Damon Spayde
 * \date   2010-01-07
 */

#ifdef __USE_DATABASE__
#include "QwParityDB.h"
#include "QwParitySchema.h"

// System headers

// Qweak headers
#include "QwEventBuffer.h"
#include "QwRunCondition.h"
using namespace QwParitySchema;

// Definition of static class members in QwParityDB
std::map<string, unsigned int> QwParityDB::fDetectorIDs;
std::map<string, unsigned int> QwParityDB::fDetectorTypeIDs;
std::vector<string>            QwParityDB::fMeasurementIDs;
std::map<string, unsigned int> QwParityDB::fSlowControlDetectorIDs;// for epics
std::map<string, unsigned char> QwParityDB::fErrorCodeIDs;

// Functor classes for MySQL++ for_each() have been removed
// They have been replaced with direct sqlpp11 SELECT queries and range-based for loops


/*! The simple constructor initializes member fields.  This class is not
 * used to establish the database connection.  It sets up a
 * mysqlpp::Connection() object that has exception throwing disabled.
 */
//QwDatabase::QwDatabase() : Connection(false)
QwParityDB::QwParityDB() : QwDatabase("01", "04", "0000")
{
  QwDebug << "Greetings from QwParityDB simple constructor." << QwLog::endl;
  // Initialize member fields

  fRunNumber         = 0;
  fRunID             = 0;
  fRunletID          = 0;
  fAnalysisID        = 0;
  fSegmentNumber     = -1;
  fDisableAnalysisCheck = false;

}

/*! The constructor initializes member fields using the values in
 *  the QwOptions object.
 * @param options  The QwOptions object.
 */
QwParityDB::QwParityDB(QwOptions &options) : QwDatabase(options, "01", "04", "0000")
{
  QwDebug << "Greetings from QwParityDB extended constructor." << QwLog::endl;

  // Initialize member fields
  fRunNumber         = 0;
  fRunID             = 0;
  fRunletID          = 0;
  fAnalysisID        = 0;
  fSegmentNumber     = -1;
  fDisableAnalysisCheck = false;

  ProcessAdditionalOptions(options);

}

/*! The destructor says "Good-bye World!"
 */
QwParityDB::~QwParityDB()
{
  QwDebug << "QwParityDB::~QwParityDB() : Good-bye World from QwParityDB destructor!" << QwLog::endl;
  if( Connected() ) Disconnect();
}

/*!
 * Sets Run number for subsequent database interactions.  Makes sure correct
 * entry exists in Run table and retrieves run_id.
 */
void QwParityDB::SetupOneRun(QwEventBuffer& qwevt)
{
  if (this->AllowsReadAccess()) {
    UInt_t run_id      = this->GetRunID(qwevt);
    UInt_t runlet_id   = this->GetRunletID(qwevt);
    UInt_t analysis_id = this->GetAnalysisID(qwevt);

    //  Write from the database
    QwMessage << "QwParityDB::SetupOneRun::"
	      << " Run Number "  << QwColor(Qw::kBoldMagenta) << qwevt.GetRunNumber() << QwColor(Qw::kNormal)
	      << " Run ID "      << QwColor(Qw::kBoldMagenta) << run_id << QwColor(Qw::kNormal)
	      << " Runlet ID "   << QwColor(Qw::kBoldMagenta) << runlet_id << QwColor(Qw::kNormal)
	      << " Analysis ID " << QwColor(Qw::kBoldMagenta) << analysis_id
	      << QwColor(Qw::kNormal)
	      << QwLog::endl;
  }
}

/*!
 * Sets Run number for subsequent database interactions.  Makes sure correct
 * entry exists in Run table and retrieves run_id.
 */
bool QwParityDB::SetRunNumber(const UInt_t runnum)
{

  QwDebug << "Made it into QwParityDB::SetRunNumber()" << QwLog::endl;

  try {

    auto c = GetScopedConnection();

    const auto& DataTakingPeriod = QwParitySchema::DataTakingPeriod{};
    auto query = sqlpp::select(sqlpp::all_of(DataTakingPeriod))
                 .from(DataTakingPeriod)
                 .where(DataTakingPeriod.periodType == "run"
                        and DataTakingPeriod.runNumber == runnum);

    auto results = QuerySelect(query);
    size_t result_count = CountResults(results);
    QwDebug << "Number of rows returned:  " << result_count << QwLog::endl;

    if (result_count != 1) {
      QwError << "Unable to find unique Run number " << runnum << " in database." << QwLog::endl;
      QwError << "Run number query returned " << result_count << "rows." << QwLog::endl;
      QwError << "Please make sure that the database contains one unique entry for this Run." << QwLog::endl;
      return false;
    }

    // Access first row from result
    UInt_t found_run_id = 0;
    ForFirstResult(results, [&](const auto& row) {
      found_run_id = row.periodId;
    });
    QwDebug << "Run ID = " << found_run_id << QwLog::endl;

    fRunNumber = runnum;
    fRunID = found_run_id;
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    return false;
  }

  return true;

}

/*!
 * This function sets the fRunID for the Run being replayed as determined by the QwEventBuffer class.
 */
UInt_t QwParityDB::SetRunID(QwEventBuffer& qwevt)
{

  // Check to see if Run is already in database.  If so retrieve Run ID and exit.
  try {
      auto c = GetScopedConnection();

      const auto& DataTakingPeriod = QwParitySchema::DataTakingPeriod{};
      auto query = sqlpp::select(sqlpp::all_of(DataTakingPeriod))
                   .from(DataTakingPeriod)
                   .where(DataTakingPeriod.periodType == "run"
                          and DataTakingPeriod.runNumber == qwevt.GetRunNumber());
      auto results = QuerySelect(query);

      size_t result_count = CountResults(results);
      UInt_t first_run_id = 0;
      UInt_t first_run_number = 0;
      ForFirstResult(results, [&](const auto& row) {
        first_run_id = row.periodId;
        first_run_number = row.runNumber;
      });

      QwDebug << "QwParityDB::SetRunID => Number of rows returned:  " << result_count << QwLog::endl;

      // If there is more than one Run in the DB with the same Run number, then there will be trouble later on.  Catch and bomb out.
      if (result_count > 1) {
        QwError << "Unable to find unique Run number " << qwevt.GetRunNumber() << " in database." << QwLog::endl;
        QwError << "Run number query returned " << result_count << "rows." << QwLog::endl;
        QwError << "Please make sure that the database contains one unique entry for this Run." << QwLog::endl;
        return 0;
      }

      // Run already exists in database.  Pull run_id and move along.
      if (result_count == 1) {
        QwDebug << "QwParityDB::SetRunID => Run ID = " << first_run_id << QwLog::endl;

        fRunNumber = qwevt.GetRunNumber();
        fRunID     = first_run_id;
        return fRunID;
      }

      // If we reach here, Run is not in database so insert pertinent data and retrieve Run ID
      // Right now this does not insert start/stop times or info on number of events.
      QwParitySchema::row<QwParitySchema::DataTakingPeriod> run_row;
      run_row[DataTakingPeriod.periodType] = "run";
      run_row[DataTakingPeriod.runNumber] = qwevt.GetRunNumber();
      run_row[DataTakingPeriod.goodForId] = 1;
      run_row[DataTakingPeriod.dataQuality] = "good";
      run_row[DataTakingPeriod.slugNumber] = 9999;
      run_row[DataTakingPeriod.wienNumber] = 9999;
      run_row[DataTakingPeriod.fullRun] = true;
      // Convert Unix timestamps to sqlpp11 datetime using chrono time_point
      // FIXME (wdconinc) verify conversion
      run_row[DataTakingPeriod.startTime] = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::from_time_t(qwevt.GetStartUnixTime()));
      run_row[DataTakingPeriod.endTime] = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::from_time_t(qwevt.GetEndUnixTime()));
      run_row[DataTakingPeriod.numMinipulses] = 0;
      run_row[DataTakingPeriod.numQuartets] = 0;
      run_row[DataTakingPeriod.comment] = "";

      QwDebug << "QwParityDB::SetRunID() => Executing sqlpp11 DataTakingPeriod(run) insert" << QwLog::endl;
      auto insert_id = QueryInsertAndGetId(run_row.insert_into());

      if (insert_id != 0) {
        fRunNumber = qwevt.GetRunNumber();
        fRunID     = insert_id;
      }
      return fRunID;
  }
  catch (const std::exception& er) {
      QwError << er.what() << QwLog::endl;
      return 0;
  }

}

/*!
 * This is a getter for run_id in the Run table.  Should be used in subsequent queries to retain key relationships between tables.
 */
UInt_t QwParityDB::GetRunID(QwEventBuffer& qwevt)
{
  // If the stored Run number does not agree with the CODA Run number
  // or if fRunID is not set, then retrieve data from database and update if necessary.

  // GetRunNumber() in QwEventBuffer returns Int_t, thus
  // we should convert it to UInt_t here. I think, it is OK.

  if (fRunID == 0 || fRunNumber != (UInt_t) qwevt.GetRunNumber() ) {
     QwDebug << "QwParityDB::GetRunID() set fRunID to " << SetRunID(qwevt) << QwLog::endl;
     fRunletID = 0;
     fAnalysisID = 0;
  }

  return fRunID;

}

/*!
 * This function sets the fRunletID for the Run being replayed as determined by the QwEventBuffer class.
 *
 * Runlets are differentiated by file segment number at the moment, not by event range or start/stop time.  This function will need to be altered if we opt to differentiate between runlets in a different way.
 */
UInt_t QwParityDB::SetRunletID(QwEventBuffer& qwevt)
{

  // Make sure 'Run' table has been populated and retrieve run_id
  //  UInt_t runid = this->GetRunID(qwevt);

  // Check to see if Runlet is already in database.  If so retrieve runlet_id and exit.
  try {
      auto c = GetScopedConnection();

      QwParitySchema::DataTakingPeriod DataTakingPeriod{};

      // Query is slightly different if file segments are being chained together for replay or not.
      if (qwevt.AreRunletsSplit()) {
        fSegmentNumber = qwevt.GetSegmentNumber();
        auto query = sqlpp::select(sqlpp::all_of(DataTakingPeriod))
                     .from(DataTakingPeriod)
                     .where(DataTakingPeriod.periodType == "runlet"
                            and DataTakingPeriod.parentId == fRunID
                            and DataTakingPeriod.fullRun == false
                            and DataTakingPeriod.segmentNumber == fSegmentNumber);
        auto results = QuerySelect(query);

        // Count results and process using helper functions
        size_t result_count = CountResults(results);
        UInt_t found_runlet_id = 0;
        ForFirstResult(results, [&](const auto& row) {
          found_runlet_id = row.periodId;
        });

        QwDebug << "QwParityDB::SetRunletID => Number of rows returned:  " << result_count << QwLog::endl;

        // If there is more than one Run in the DB with the same Runlet number, then there will be trouble later on.
        if (result_count > 1) {
          QwError << "Unable to find unique Runlet number " << qwevt.GetRunNumber() << " in database." << QwLog::endl;
          QwError << "Run number query returned " << result_count << " rows." << QwLog::endl;
          QwError << "Please make sure that the database contains one unique entry for this Run." << QwLog::endl;
          return 0;
        }

        // Run already exists in database.  Pull runlet_id and move along.
        if (result_count == 1) {
          QwDebug << "QwParityDB::SetRunletID => Runlet ID = " << found_runlet_id << QwLog::endl;
          fRunletID = found_runlet_id;
          return fRunletID;
        }

      } else {
        auto query = sqlpp::select(sqlpp::all_of(DataTakingPeriod))
                     .from(DataTakingPeriod)
                     .where(DataTakingPeriod.periodType == "runlet"
                            and DataTakingPeriod.parentId == fRunID
                            and DataTakingPeriod.fullRun == true);
        auto results = QuerySelect(query);

        // Count results and process using helper functions
        size_t result_count = CountResults(results);
        UInt_t found_runlet_id = 0;
        ForFirstResult(results, [&](const auto& row) {
          found_runlet_id = row.periodId;
        });

        QwDebug << "QwParityDB::SetRunletID => Number of rows returned:  " << result_count << QwLog::endl;

        // If there is more than one Run in the DB with the same Runlet number, then there will be trouble later on.
        if (result_count > 1) {
          QwError << "Unable to find unique Runlet number " << qwevt.GetRunNumber() << " in database." << QwLog::endl;
          QwError << "Run number query returned " << result_count << " rows." << QwLog::endl;
          QwError << "Please make sure that the database contains one unique entry for this Run." << QwLog::endl;
          return 0;
        }

        // Run already exists in database.  Pull runlet_id and move along.
        if (result_count == 1) {
          QwDebug << "QwParityDB::SetRunletID => Runlet ID = " << found_runlet_id << QwLog::endl;
          fRunletID = found_runlet_id;
          return fRunletID;
        }
      }

      // If we reach here, Runlet is not in database so insert pertinent data and retrieve Run ID
      // Right now this does not insert start/stop times or info on number of events.
      QwParitySchema::DataTakingPeriod runlet_table{};
      QwParitySchema::row<QwParitySchema::DataTakingPeriod> row;
      row[runlet_table.periodType] = "runlet";
      row[runlet_table.parentId] = fRunID;
      row[runlet_table.runNumber] = qwevt.GetRunNumber();
      row[runlet_table.goodForId] = 1;
      row[runlet_table.dataQuality] = "good";
      row[runlet_table.slugNumber] = 9999;
      row[runlet_table.wienNumber] = 9999;
      row[runlet_table.firstMinipulses] = 0;
      row[runlet_table.lastMinipulses] = 0;
      row[runlet_table.numMinipulses] = 0;
      row[runlet_table.numQuartets] = 0;

      // Handle segment_number based on Runlet split condition
      if (qwevt.AreRunletsSplit()) {
        row[runlet_table.segmentNumber]  = fSegmentNumber;
        row[runlet_table.fullRun] = false;
        QwDebug << "QwParityDB::SetRunletID() => Executing sqlpp11 DataTakingPeriod(runlet) insert (with segment)" << QwLog::endl;
      } else {
        // Note: segment_number is nullable, but row assignment might need special handling for null
        // For now, use 0 or another default value
        // row[runlet_table.segmentNumber]  = sqlpp::null;
        row[runlet_table.fullRun] = true;
        QwDebug << "QwParityDB::SetRunletID() => Executing sqlpp11 DataTakingPeriod(runlet) insert (no segment)" << QwLog::endl;
      }

      auto insert_id = QueryInsertAndGetId(row.insert_into());
      if (insert_id != 0) {
        fRunletID = insert_id;
      }
      return fRunletID;
  }
  catch (const std::exception& er) {
      QwError << er.what() << QwLog::endl;
      return 0;
  }

}

/*!
 * This is a getter for runlet_id in the Runlet table.  Should be used in subsequent queries to retain key relationships between tables.
 */
UInt_t QwParityDB::GetRunletID(QwEventBuffer& qwevt)
{
  // If the stored Run number does not agree with the CODA Run number
  // or if fRunID is not set, then retrieve data from database and update if necessary.

  if (fRunletID == 0 || (qwevt.AreRunletsSplit() && fSegmentNumber!=qwevt.GetSegmentNumber()) || fRunNumber != (UInt_t) qwevt.GetRunNumber() ) {
     QwDebug << "QwParityDB::GetRunletID() set fRunletID to " << SetRunletID(qwevt) << QwLog::endl;
     fAnalysisID = 0;
  }

  return fRunletID;

}

/*!
 * This is used to set the appropriate analysis_id for this Run.  Must be a valid runlet_id in the Runlet table before proceeding.  Will insert an entry into the Analysis table if necessary.
 */
UInt_t QwParityDB::SetAnalysisID(QwEventBuffer& qwevt)
{
  // If there is already an analysis_id for this Run, then let's bomb out.

  try {
    auto c = GetScopedConnection();

    const auto& Analysis = QwParitySchema::Analysis{};
    auto query = sqlpp::select(Analysis.analysisId)
                 .from(Analysis)
                 .where(Analysis.beamMode == "nbm"
                        and Analysis.periodId == this->GetRunletID(qwevt));
    auto results = QuerySelect(query);
    size_t result_count = CountResults(results);
    if (result_count > 0) {
      QwError << "This Runlet has already been analyzed by the engine!" << QwLog::endl;
      QwError << "The following analysis_id values already exist in the database:  ";
      ForEachResult(results, [&](const auto& row) {
        QwError << row.analysisId << " ";
      });
      QwError << QwLog::endl;

      if (fDisableAnalysisCheck==false) {
        QwError << "Analysis of this Run will now be terminated."  << QwLog::endl;
        // Note: return statement cannot be used in lambda to return from function
      } else {
        QwWarning << "Analysis will continue.  A duplicate entry with new analysis_id will be added to the Analysis table." << QwLog::endl;
      }
    }
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    QwError << "Unable to determine if there are other database entries for this Run.  Exiting." << QwLog::endl;
    return 0;
  }


  try {

    QwParitySchema::Analysis Analysis;
    QwParitySchema::row<QwParitySchema::Analysis> analysis_row;

    analysis_row[Analysis.periodId]  = GetRunletID(qwevt);
    analysis_row[Analysis.eeSeedId] = 1;
    analysis_row[Analysis.epSeedId] = 1;

    analysis_row[Analysis.time]        = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    analysis_row[Analysis.eeBfChecksum] = "empty"; // updated after blinder step
    analysis_row[Analysis.epBfChecksum] = "empty"; // updated after blinder step
    analysis_row[Analysis.beamMode]   = "nbm";   // we will match this as a real one later
    analysis_row[Analysis.regressionStatus] = false;
    analysis_row[Analysis.rntupleEnabled] = false;

    // Analyzer Information Parsing
    QwRunCondition run_condition(
      gQwOptions.GetArgc(),
      gQwOptions.GetArgv(),
      "run_condition"
    );

    run_condition.Get()->Print();

    TIter next(run_condition.Get());
    TObjString *obj_str;
    TString str_val, str_var;
    Ssiz_t location;

    // Iterate over each entry in run_condition
    while ((obj_str = (TObjString *) next())) {
      QwMessage << obj_str->GetName() << QwLog::endl;

      // Store string contents for parsing
      str_var = str_val = obj_str->GetString();
      location = str_val.First(":"); // The first : separates variable from value
      location = location + 2; // Value text starts two characters after :
      str_val.Remove(0,location); //str_val stores value to go in DB

      // Decision tree to figure out which variable to store in
      if (str_var.BeginsWith("ROOT Version")) {
        analysis_row[Analysis.rootVersion] = str_val;
      } else if (str_var.BeginsWith("ROOT file creating time")) {
        analysis_row[Analysis.rootFileTime] = str_val;
      } else if (str_var.BeginsWith("ROOT file created on Hostname")) {
        analysis_row[Analysis.rootFileHost] = str_val;
      } else if (str_var.BeginsWith("ROOT file created by the user")) {
        analysis_row[Analysis.rootFileUser] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer Name")) {
        analysis_row[Analysis.analyzerName] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer Options")) {
        analysis_row[Analysis.analyzerArgv] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer SVN Revision")) {
        analysis_row[Analysis.analyzerSvnRev] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer SVN Last Changed Revision")) {
        analysis_row[Analysis.analyzerSvnLcRev] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer SVN URL")) {
        analysis_row[Analysis.analyzerSvnUrl] = str_val;
      } else if (str_var.BeginsWith("DAQ ROC flags when QwAnalyzer runs")) {
        analysis_row[Analysis.rocFlags] = str_val;
      } else {
      }
    }
    auto c = GetScopedConnection();

    auto insert_id = QueryInsertAndGetId(analysis_row.insert_into());

    if (insert_id != 0)
    {
      fAnalysisID = insert_id;
    }
    return fAnalysisID;
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    return 0;
  }


}

void QwParityDB::FillParameterFiles(QwSubsystemArrayParity& subsys){
  TList* param_file_list = subsys.GetParamFileNameList("mapfiles");
  try {
    auto c = GetScopedConnection();
    QwParitySchema::Analysis Analysis;

    param_file_list->Print();
    TIter next(param_file_list);
    TList *pfl_elem;
    std::vector<std::string> files;
    while ((pfl_elem = (TList *) next())) {
      files.emplace_back(pfl_elem->GetName());
    }

    auto escape_json = [](const std::string& input) {
      std::string out;
      out.reserve(input.size());
      for (char ch : input) {
        if (ch == '\\' || ch == '"') out.push_back('\\');
        out.push_back(ch);
      }
      return out;
    };
    std::string json = "[";
    for (size_t i = 0; i < files.size(); ++i) {
      if (i) json += ",";
      json += "\"";
      json += escape_json(files[i]);
      json += "\"";
    }
    json += "]";

    auto update_query = sqlpp::update(Analysis)
      .set(Analysis.parameterFiles = json)
      .where(Analysis.analysisId == GetAnalysisID());
    QueryExecute(update_query);
    delete param_file_list;
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    delete param_file_list;
  }
}

/*!
 * This is a getter for analysis_id in the Analysis table.  Required by all queries on cerenkov, beam, etc. tables.  Will return 0 if fRunID has not been successfully set.  If fAnalysisID is not set, then calls code to insert into Analysis table and retrieve analysis_id.
 */
UInt_t QwParityDB::GetAnalysisID(QwEventBuffer& qwevt)
{
  // Sanity check to make sure not calling this before runlet_id has been retrieved.
  if (fRunletID == 0) {
    QwDebug << "QwParityDB::GetAnalysisID() : fRunletID must be set before proceeding.  Check to make sure Run exists in database." << QwLog::endl;
    return 0;
  }

  if (fAnalysisID == 0 || fRunNumber != (UInt_t) qwevt.GetRunNumber()
      || (qwevt.AreRunletsSplit() && fSegmentNumber!=qwevt.GetSegmentNumber())) {
    QwDebug << "QwParityDB::GetAnalysisID() set fAnalysisID to " << SetAnalysisID(qwevt) << QwLog::endl;
    if (fAnalysisID==0) {
      QwError << "QwParityDB::SetAnalysisID() unable to set valid fAnalysisID for this Run.  Exiting." <<QwLog::endl;
      exit(1);
    }
  }

  return fAnalysisID;
}


/*
 * This function retrieves the monitor table key 'monitor_id' for a given beam monitor.
 */
/*
 * This function retrieves the Detector table key 'detector_id' for a given Detector name.
 */
UInt_t QwParityDB::GetDetectorID(const string& name, Bool_t zero_id_is_error)
{
  if (fDetectorIDs.size() == 0) {
    StoreDetectorIDs();
  }

  UInt_t QwParityDB::GetDetectorTypeID(const string& detector_type)
  {
    if (fDetectorTypeIDs.empty()) {
      StoreDetectorTypeIDs();
    }
    auto it = fDetectorTypeIDs.find(detector_type);
    if (it == fDetectorTypeIDs.end()) {
      QwError << "QwParityDB::GetDetectorTypeID() => Unable to determine detector_type_id for " << detector_type << QwLog::endl;
      return 0;
    }
    return it->second;
  }

  UInt_t detector_id = fDetectorIDs[name];

  if (zero_id_is_error && detector_id==0) {

    if (fDBInsertMissingKeys) {
      QwWarning << "Inserting missing variable " << name << " into Detector table." << QwLog::endl;
      try {
        auto c = GetScopedConnection();

        QwParitySchema::Detector Detector;
        QwParitySchema::DetectorType DetectorType;
        QwParitySchema::row<QwParitySchema::Detector> detector_row;
        UInt_t default_detector_type_id = 0;
        auto detector_type_query = sqlpp::select(DetectorType.detectorTypeId)
          .from(DetectorType)
          .where(DetectorType.detector == "md");
        auto detector_type_results = QuerySelect(detector_type_query);
        ForFirstResult(detector_type_results, [&](const auto& row) {
          default_detector_type_id = row.detectorTypeId;
        });
        if (default_detector_type_id == 0) {
          QwError << "Failed to resolve detector_type_id for detector='md'; cannot insert detector " << name << QwLog::endl;
          return 0;
        }
        detector_row[Detector.name] = name;
        detector_row[Detector.description] = "unknown";
        detector_row[Detector.detectorTypeId] = default_detector_type_id;  // default to main detector type

        auto insert_id = QueryInsertAndGetId(detector_row.insert_into());

        if (insert_id != 0) {
          fDetectorIDs[name] = insert_id;
          detector_id = insert_id;
          QwWarning << "Successfully inserted variable " << name << " into Detector table with ID " << insert_id << QwLog::endl;
        } else {
          QwError << "Failed to insert variable " << name << " into Detector table." << QwLog::endl;
        }
      }
      catch (const std::exception& er) {
        QwError << er.what() << QwLog::endl;
      }
    } else {
      QwError << "QwParityDB::GetDetectorID() => Unable to determine valid ID for Detector " << name << QwLog::endl;
      QwError << "To enable automatic insertion of missing variables, set the option '--QwDatabase.insert-missing-keys'" << QwLog::endl;
    }
  }

  return detector_id;
}


/*
 * Stores Detector table keys in an associative array indexed by Detector name.
 */
void QwParityDB::StoreDetectorIDs()
{
  try {
    auto c = GetScopedConnection();

    QwParitySchema::Detector Detector{};
    auto query = sqlpp::select(sqlpp::all_of(Detector)).from(Detector).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreDetectorID:  detector_id = " << row.detectorId << " name = " << row.name << QwLog::endl;
      QwParityDB::fDetectorIDs.insert(std::make_pair(row.name, row.detectorId));
    });
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    exit(1);
  }
  return;
}

void QwParityDB::StoreDetectorTypeIDs()
{
  try {
    auto c = GetScopedConnection();
    QwParitySchema::DetectorType DetectorType{};
    auto query = sqlpp::select(sqlpp::all_of(DetectorType)).from(DetectorType).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreDetectorTypeID: detector_type_id = " << row.detectorTypeId
              << " detector = " << row.detector << QwLog::endl;
      QwParityDB::fDetectorTypeIDs.insert(std::make_pair(row.detector, row.detectorTypeId));
    });
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    exit(1);
  }
}


/*
 * This function retrieves the slow control Detector table key 'sc_detector_id' for a given epics variable.
 */
UInt_t QwParityDB::GetSlowControlDetectorID(const string& name)
{
  if (fSlowControlDetectorIDs.size() == 0) {
    StoreSlowControlDetectorIDs();
  }

  UInt_t sc_detector_id = fSlowControlDetectorIDs[name];

  if (sc_detector_id==0) {
    QwError << "QwParityDB::GetSlowControlDetectorID() => Unable to determine valid ID for the epics variable " << name << QwLog::endl;

    if (fDBInsertMissingKeys) {
      QwWarning << "Inserting missing variable " << name << " into SlowControlDetector table." << QwLog::endl;
      try {
        auto c = GetScopedConnection();

        QwParitySchema::SlowControlDetector SlowControlDetector;
        QwParitySchema::row<QwParitySchema::SlowControlDetector> sc_detector_row;
        sc_detector_row[SlowControlDetector.name] = name;
        sc_detector_row[SlowControlDetector.units] = "unknown";
        sc_detector_row[SlowControlDetector.comment] = "unknown";

        auto insert_id = QueryInsertAndGetId(sc_detector_row.insert_into());

        if (insert_id != 0) {
          fSlowControlDetectorIDs[name] = insert_id;
          sc_detector_id = insert_id;
          QwWarning << "Successfully inserted variable " << name << " into SlowControlDetector table with ID " << insert_id << QwLog::endl;
        } else {
          QwError << "Failed to insert variable " << name << " into SlowControlDetector table." << QwLog::endl;
        }
      }
      catch (const std::exception& er) {
        QwError << er.what() << QwLog::endl;
      }
    } else {
      QwError << "To enable automatic insertion of missing variables, set the option '--QwDatabase.insert-missing-keys'" << QwLog::endl;
    }
  }

  return sc_detector_id;

}

/*
 * This function retrieves the error code table key 'error_code_id' for a given error code name.
 */
UInt_t QwParityDB::GetErrorCodeID(const string& name)
{
  if (fErrorCodeIDs.size() == 0) {
    StoreErrorCodeIDs();
  }

  UInt_t error_code_id = fErrorCodeIDs[name];

  if (error_code_id==0) {
    QwError << "QwParityDB::GetErrorCodeID() => Unable to determine valid ID for the error code " << name << QwLog::endl;
  }

  return error_code_id;

}

/*
 * Stores slow control Detector table keys in an associative array indexed by slow_controls_data name.
 */
void QwParityDB::StoreSlowControlDetectorIDs()
{
  try {
    auto c = GetScopedConnection();

    QwParitySchema::SlowControlDetector SlowControlDetector{};
    auto query = sqlpp::select(sqlpp::all_of(SlowControlDetector)).from(SlowControlDetector).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreSlowControlDetectorID: slow_control_detector_id = " << row.slowControlDetectorId << " name = " << row.name << QwLog::endl;
      QwParityDB::fSlowControlDetectorIDs.insert(std::make_pair(row.name, row.slowControlDetectorId));
    });
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    exit(1);
  }
  return;
}

/*
 * Stores ErrorCode table keys in an associative array indexed by ErrorCode quantity.
 */
void QwParityDB::StoreErrorCodeIDs()
{
  try {
    auto c = GetScopedConnection();

    QwParitySchema::ErrorCode ErrorCode{};
    auto query = sqlpp::select(sqlpp::all_of(ErrorCode)).from(ErrorCode).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreErrorCodeID: error_code_id = " << row.errorCodeId << " description = " << row.description << QwLog::endl;
      QwParityDB::fErrorCodeIDs.insert(std::make_pair(row.description, row.errorCodeId));
    });
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    exit(1);
  }
  return;
}

const string QwParityDB::GetMeasurementID(const Int_t index)
{
  if (fMeasurementIDs.size() == 0) {
    StoreMeasurementIDs();
  }

  string MeasurementType = fMeasurementIDs[index];

  if (MeasurementType.empty()) {
    QwError << "QwParityDB::GetMeasurementID() => Unable to determine valid ID for measurement type with " << index << QwLog::endl;
  }

  return MeasurementType;
}

void QwParityDB::StoreMeasurementIDs()
{
  try {
    auto c = GetScopedConnection();

    QwParitySchema::MeasurementType MeasurementType{};
    auto query = sqlpp::select(sqlpp::all_of(MeasurementType)).from(MeasurementType).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreMeasurementID:  MeasurementType = " << row.measurementTypeId << QwLog::endl;
      QwParityDB::fMeasurementIDs.push_back((std::string) row.measurementTypeId);
    });
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    exit(1);
  }
  return;
}

/*!
 * Defines configuration options for QwParityDB class using QwOptions
 * functionality.
 *
 * Should apparently by called by QwOptions::DefineOptions() in
 * QwParityOptions.h
 */
void QwParityDB::DefineAdditionalOptions(QwOptions& options)
{
  // Specify command line options for use by QwParityDB
  options.AddOptions("Parity Analyzer Database options")
    ("QwParityDB.disable-Analysis-check",
     po::value<bool>()->default_bool_value(false),
     "disable check of pre-existing analysis_id");
}

/*!
 * Loads the configuration options for QwParityDB class into this instance of
 * QwParityDB from the QwOptions object.
 * @param options Options object
 */
void QwParityDB::ProcessAdditionalOptions(QwOptions &options)
{
  if (options.GetValue<bool>("QwParityDB.disable-Analysis-check"))
    fDisableAnalysisCheck=true;

  return;
}

#endif // #ifdef __USE_DATABASE__
