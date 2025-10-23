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
std::map<string, unsigned int> QwParityDB::fMonitorIDs;
std::map<string, unsigned int> QwParityDB::fMainDetectorIDs;
std::map<string, unsigned int> QwParityDB::fLumiDetectorIDs;
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
 * Sets run number for subsequent database interactions.  Makes sure correct
 * entry exists in run table and retrieves run_id.
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
 * Sets run number for subsequent database interactions.  Makes sure correct
 * entry exists in run table and retrieves run_id.
 */
bool QwParityDB::SetRunNumber(const UInt_t runnum)
{

  QwDebug << "Made it into QwParityDB::SetRunNumber()" << QwLog::endl;

  try {

    auto c = GetScopedConnection();

    const auto& run = QwParitySchema::run{};
    auto query = sqlpp::select(sqlpp::all_of(run))
                 .from(run)
                 .where(run.run_number == runnum);

    auto results = QuerySelect(query);
    size_t result_count = CountResults(results);
    QwDebug << "Number of rows returned:  " << result_count << QwLog::endl;

    if (result_count != 1) {
      QwError << "Unable to find unique run number " << runnum << " in database." << QwLog::endl;
      QwError << "Run number query returned " << result_count << "rows." << QwLog::endl;
      QwError << "Please make sure that the database contains one unique entry for this run." << QwLog::endl;
      return false;
    }

    // Access first row from result
    UInt_t found_run_id = 0;
    ForFirstResult(results, [&](const auto& row) {
      found_run_id = row.run_id;
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
 * This function sets the fRunID for the run being replayed as determined by the QwEventBuffer class.
 */
UInt_t QwParityDB::SetRunID(QwEventBuffer& qwevt)
{

  // Check to see if run is already in database.  If so retrieve run ID and exit.
  try {
      auto c = GetScopedConnection();

      const auto& run = QwParitySchema::run{};
      auto query = sqlpp::select(sqlpp::all_of(run))
                   .from(run)
                   .where(run.run_number == qwevt.GetRunNumber());
      auto results = QuerySelect(query);

      size_t result_count = CountResults(results);
      UInt_t first_run_id = 0;
      UInt_t first_run_number = 0;
      ForFirstResult(results, [&](const auto& row) {
        first_run_id = row.run_id;
        first_run_number = row.run_number;
      });
      
      QwDebug << "QwParityDB::SetRunID => Number of rows returned:  " << result_count << QwLog::endl;

      // If there is more than one run in the DB with the same run number, then there will be trouble later on.  Catch and bomb out.
      if (result_count > 1) {
        QwError << "Unable to find unique run number " << qwevt.GetRunNumber() << " in database." << QwLog::endl;
        QwError << "Run number query returned " << result_count << "rows." << QwLog::endl;
        QwError << "Please make sure that the database contains one unique entry for this run." << QwLog::endl;
        return 0;
      }

      // Run already exists in database.  Pull run_id and move along.
      if (result_count == 1) {
        QwDebug << "QwParityDB::SetRunID => Run ID = " << first_run_id << QwLog::endl;

        fRunNumber = qwevt.GetRunNumber();
        fRunID     = first_run_id;
        return fRunID;
      }

      // If we reach here, run is not in database so insert pertinent data and retrieve run ID
      // Right now this does not insert start/stop times or info on number of events.
      QwParitySchema::row<QwParitySchema::run> run_row;
      run_row[run.run_number] = qwevt.GetRunNumber();
      run_row[run.run_type] = "good"; // qwevt.GetRunType(); RunType is the confused name because we have also a CODA run type.
      // Convert Unix timestamps to sqlpp11 datetime using chrono time_point
      // FIXME (wdconinc) verify conversion
      run_row[run.start_time] = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::from_time_t(qwevt.GetStartUnixTime()));
      run_row[run.end_time] = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::from_time_t(qwevt.GetEndUnixTime()));
      run_row[run.n_mps] = 0;
      run_row[run.n_qrt] = 0;
      // Set following quantities to 9999 as "uninitialized value".  DTS 8/3/2012
      run_row[run.slug] = 9999;
      run_row[run.wien_slug] = 9999;
      run_row[run.injector_slug] = 9999;
      run_row[run.comment] = "";

      QwDebug << "QwParityDB::SetRunID() => Executing sqlpp11 run insert" << QwLog::endl;
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
 * This is a getter for run_id in the run table.  Should be used in subsequent queries to retain key relationships between tables.
 */
UInt_t QwParityDB::GetRunID(QwEventBuffer& qwevt)
{
  // If the stored run number does not agree with the CODA run number
  // or if fRunID is not set, then retrieve data from database and update if necessary.

  // GetRunNumber() in QwEventBuffer returns Int_t, thus
  // we should convert it to UInt_t here. I think, it is OK.

  if (fRunID == 0 || fRunNumber != (UInt_t) qwevt.GetRunNumber() ) {
    fRunID = SetRunID(qwevt);
    QwDebug << "QwParityDB::GetRunID() set fRunID to " << fRunID << QwLog::endl;
    fRunletID = 0;
    fAnalysisID = 0;
  }

  return fRunID;

}

/*!
 * This function sets the fRunletID for the run being replayed as determined by the QwEventBuffer class.
 *
 * Runlets are differentiated by file segment number at the moment, not by event range or start/stop time.  This function will need to be altered if we opt to differentiate between runlets in a different way.
 */
UInt_t QwParityDB::SetRunletID(QwEventBuffer& qwevt)
{

  // Make sure 'run' table has been populated and retrieve run_id
  //  UInt_t runid = this->GetRunID(qwevt);

  // Check to see if runlet is already in database.  If so retrieve runlet_id and exit.
  try {
      auto c = GetScopedConnection();

      QwParitySchema::runlet runlet{};
      
      // Query is slightly different if file segments are being chained together for replay or not.
      if (qwevt.AreRunletsSplit()) {
        fSegmentNumber = qwevt.GetSegmentNumber();
        auto query = sqlpp::select(sqlpp::all_of(runlet))
                     .from(runlet)
                     .where(runlet.run_id == fRunID
                            and runlet.full_run == "false"
                            and runlet.segment_number == fSegmentNumber);
        auto results = QuerySelect(query);
        
        // Count results and process using helper functions
        size_t result_count = CountResults(results);
        UInt_t found_runlet_id = 0;
        ForFirstResult(results, [&](const auto& row) {
          found_runlet_id = row.runlet_id;
        });
        
        QwDebug << "QwParityDB::SetRunletID => Number of rows returned:  " << result_count << QwLog::endl;
        
        // If there is more than one run in the DB with the same runlet number, then there will be trouble later on.
        if (result_count > 1) {
          QwError << "Unable to find unique runlet number " << qwevt.GetRunNumber() << " in database." << QwLog::endl;
          QwError << "Run number query returned " << result_count << " rows." << QwLog::endl;
          QwError << "Please make sure that the database contains one unique entry for this run." << QwLog::endl;
          return 0;
        }
        
        // Run already exists in database.  Pull runlet_id and move along.
        if (result_count == 1) {
          QwDebug << "QwParityDB::SetRunletID => Runlet ID = " << found_runlet_id << QwLog::endl;
          fRunletID = found_runlet_id;
          return fRunletID;
        }
        
      } else {
        auto query = sqlpp::select(sqlpp::all_of(runlet))
                     .from(runlet)
                     .where(runlet.run_id == fRunID
                            and runlet.full_run == "true");
        auto results = QuerySelect(query);
        
        // Count results and process using helper functions
        size_t result_count = CountResults(results);
        UInt_t found_runlet_id = 0;
        ForFirstResult(results, [&](const auto& row) {
          found_runlet_id = row.runlet_id;
        });
        
        QwDebug << "QwParityDB::SetRunletID => Number of rows returned:  " << result_count << QwLog::endl;
        
        // If there is more than one run in the DB with the same runlet number, then there will be trouble later on.
        if (result_count > 1) {
          QwError << "Unable to find unique runlet number " << qwevt.GetRunNumber() << " in database." << QwLog::endl;
          QwError << "Run number query returned " << result_count << " rows." << QwLog::endl;
          QwError << "Please make sure that the database contains one unique entry for this run." << QwLog::endl;
          return 0;
        }
        
        // Run already exists in database.  Pull runlet_id and move along.
        if (result_count == 1) {
          QwDebug << "QwParityDB::SetRunletID => Runlet ID = " << found_runlet_id << QwLog::endl;
          fRunletID = found_runlet_id;
          return fRunletID;
        }
      }

      // If we reach here, runlet is not in database so insert pertinent data and retrieve run ID
      // Right now this does not insert start/stop times or info on number of events.
      QwParitySchema::row<QwParitySchema::runlet> runlet_row;
      runlet_row[runlet.run_id]      = fRunID;
      runlet_row[runlet.run_number]      = qwevt.GetRunNumber();
      // Note: start_time and end_time are nullable fields but we need to use proper sqlpp11 null types
      // runlet_row[runlet.start_time]      = sqlpp::null;
      // runlet_row[runlet.end_time]        = sqlpp::null;
      runlet_row[runlet.first_mps] = 0;
      runlet_row[runlet.last_mps]	= 0;

      // Handle segment_number based on runlet split condition
      if (qwevt.AreRunletsSplit()) {
        runlet_row[runlet.segment_number]  = fSegmentNumber;
        runlet_row[runlet.full_run] = "false";
        QwDebug << "QwParityDB::SetRunletID() => Executing sqlpp11 runlet insert (with segment)" << QwLog::endl;
      } else {
        // Note: segment_number is nullable, but row assignment might need special handling for null
        // For now, use 0 or another default value
        // runlet_row[runlet.segment_number]  = sqlpp::null;
        runlet_row[runlet.full_run] = "true";
        QwDebug << "QwParityDB::SetRunletID() => Executing sqlpp11 runlet insert (no segment)" << QwLog::endl;
      }

      auto insert_id = QueryInsertAndGetId(runlet_row.insert_into());
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
 * This is a getter for runlet_id in the runlet table.  Should be used in subsequent queries to retain key relationships between tables.
 */
UInt_t QwParityDB::GetRunletID(QwEventBuffer& qwevt)
{
  // If the stored run number does not agree with the CODA run number
  // or if fRunID is not set, then retrieve data from database and update if necessary.

  if (fRunletID == 0 || (qwevt.AreRunletsSplit() && fSegmentNumber!=qwevt.GetSegmentNumber()) || fRunNumber != (UInt_t) qwevt.GetRunNumber() ) {
    fRunletID = SetRunletID(qwevt);
    QwDebug << "QwParityDB::GetRunletID() set fRunletID to " << fRunletID << QwLog::endl;
    fAnalysisID = 0;
  }

  return fRunletID;

}

/*!
 * This is used to set the appropriate analysis_id for this run.  Must be a valid runlet_id in the runlet table before proceeding.  Will insert an entry into the analysis table if necessary.
 */
UInt_t QwParityDB::SetAnalysisID(QwEventBuffer& qwevt)
{
  // If there is already an analysis_id for this run, then let's bomb out.

  try {
    auto c = GetScopedConnection();

    const auto& analysis = QwParitySchema::analysis{};
    auto query = sqlpp::select(analysis.analysis_id)
                 .from(analysis)
                 .where(analysis.beam_mode == "nbm"
                        and analysis.slope_calculation == "off"
                        and analysis.slope_correction == "off"
                        and analysis.runlet_id == this->GetRunletID(qwevt));
    auto results = QuerySelect(query);
    size_t result_count = CountResults(results);
    if (result_count > 0) {
      QwError << "This runlet has already been analyzed by the engine!" << QwLog::endl;
      QwError << "The following analysis_id values already exist in the database:  ";
      ForEachResult(results, [&](const auto& row) {
        QwError << row.analysis_id << " ";
      });
      QwError << QwLog::endl;

      if (fDisableAnalysisCheck==false) {
        QwError << "Analysis of this run will now be terminated."  << QwLog::endl;
        // Note: return statement cannot be used in lambda to return from function
      } else {
        QwWarning << "Analysis will continue.  A duplicate entry with new analysis_id will be added to the analysis table." << QwLog::endl;
      }
    }
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    QwError << "Unable to determine if there are other database entries for this run.  Exiting." << QwLog::endl;
    return 0;
  }


  try {

    QwParitySchema::analysis analysis;
    QwParitySchema::row<QwParitySchema::analysis> analysis_row;

    analysis_row[analysis.runlet_id]  = GetRunletID(qwevt);
    analysis_row[analysis.seed_id] = 1;
  
    std::pair<UInt_t, UInt_t> event_range;
    event_range = qwevt.GetEventRange();

    analysis_row[analysis.time]        = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::system_clock::now());
    analysis_row[analysis.bf_checksum] = "empty"; // we will match this as a real one later
    analysis_row[analysis.beam_mode]   = "nbm";   // we will match this as a real one later
    analysis_row[analysis.n_mps]       = 0;       // we will match this as a real one later
    analysis_row[analysis.n_qrt]       = 4;       // we will match this as a real one later
    analysis_row[analysis.first_event] = event_range.first;
    analysis_row[analysis.last_event]  = event_range.second;
    analysis_row[analysis.segment]     = 0;       // we will match this as a real one later
    analysis_row[analysis.slope_calculation] = "off";  // we will match this as a real one later
    analysis_row[analysis.slope_correction]  = "off"; // we will match this as a real one later

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
        analysis_row[analysis.root_version] = str_val;
      } else if (str_var.BeginsWith("ROOT file creating time")) {
        analysis_row[analysis.root_file_time] = str_val;
      } else if (str_var.BeginsWith("ROOT file created on Hostname")) {
        analysis_row[analysis.root_file_host] = str_val;
      } else if (str_var.BeginsWith("ROOT file created by the user")) {
        analysis_row[analysis.root_file_user] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer Name")) {
        analysis_row[analysis.analyzer_name] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer Options")) {
        analysis_row[analysis.analyzer_argv] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer SVN Revision")) {
        analysis_row[analysis.analyzer_svn_rev] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer SVN Last Changed Revision")) {
        analysis_row[analysis.analyzer_svn_lc_rev] = str_val;
      } else if (str_var.BeginsWith("QwAnalyzer SVN URL")) {
        analysis_row[analysis.analyzer_svn_url] = str_val;
      } else if (str_var.BeginsWith("DAQ ROC flags when QwAnalyzer runs")) {
        analysis_row[analysis.roc_flags] = str_val;
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
    
    QwParitySchema::parameter_files parameter_files;
    QwParitySchema::row<QwParitySchema::parameter_files> parameter_file_row;
    parameter_file_row[parameter_files.analysis_id] = GetAnalysisID();

    param_file_list->Print();
    TIter next(param_file_list);
    TList *pfl_elem;
    while ((pfl_elem = (TList *) next())) {
      parameter_file_row[parameter_files.filename] = pfl_elem->GetName();
      QueryExecute(parameter_file_row.insert_into());
    }
    delete param_file_list;
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    delete param_file_list;
  }
}

/*!
 * This is a getter for analysis_id in the analysis table.  Required by all queries on cerenkov, beam, etc. tables.  Will return 0 if fRunID has not been successfully set.  If fAnalysisID is not set, then calls code to insert into analysis table and retrieve analysis_id.
 */
UInt_t QwParityDB::GetAnalysisID(QwEventBuffer& qwevt)
{
  // Sanity check to make sure not calling this before runlet_id has been retrieved.
  if (fRunletID == 0) {
    QwDebug << "QwParityDB::GetAnalysisID() : fRunletID must be set before proceeding.  Check to make sure run exists in database." << QwLog::endl;
    return 0;
  }

  if (fAnalysisID == 0 || fRunNumber != (UInt_t) qwevt.GetRunNumber()
      || (qwevt.AreRunletsSplit() && fSegmentNumber!=qwevt.GetSegmentNumber())) {
    fAnalysisID = SetAnalysisID(qwevt);
    QwDebug << "QwParityDB::GetAnalysisID() set fAnalysisID to " << fAnalysisID << QwLog::endl;
    if (fAnalysisID==0) {
      QwError << "QwParityDB::SetAnalysisID() unable to set valid fAnalysisID for this run.  Exiting." <<QwLog::endl;
      exit(1);
    }
  }

  return fAnalysisID;
}


/*
 * This function retrieves the monitor table key 'monitor_id' for a given beam monitor.
 */
UInt_t QwParityDB::GetMonitorID(const string& name, Bool_t zero_id_is_error)
{
  if (fMonitorIDs.size() == 0) {
    StoreMonitorIDs();
  }

  UInt_t monitor_id = fMonitorIDs[name];

  if (zero_id_is_error && monitor_id==0) {
    QwError << "QwParityDB::GetMonitorID() => Unable to determine valid ID for beam monitor " << name << QwLog::endl;

    if (fDBInsertMissingKeys) {
      QwWarning << "Inserting missing variable " << name << " into monitor table." << QwLog::endl;
      try {
        auto c = GetScopedConnection();

        QwParitySchema::monitor monitor;
        QwParitySchema::row<QwParitySchema::monitor> monitor_row;
        monitor_row[monitor.quantity] = name;
        monitor_row[monitor.title] = "unknown";

        auto insert_id = QueryInsertAndGetId(monitor_row.insert_into());

        if (insert_id != 0) {
          fMonitorIDs[name] = insert_id;
          monitor_id = insert_id;
          QwWarning << "Successfully inserted variable " << name << " into monitor table with ID " << insert_id << QwLog::endl;
        } else {
          QwError << "Failed to insert variable " << name << " into monitor table." << QwLog::endl;
        }
      }
      catch (const std::exception& er) {
        QwError << er.what() << QwLog::endl;
      }
    } else {
      QwError << "To enable automatic insertion of missing variables, set the option '--QwDatabase.insert-missing-keys'" << QwLog::endl;
    }
  }

  return monitor_id;

}

/*
 * Stores monitor table keys in an associative array indexed by monitor name.
 */
void QwParityDB::StoreMonitorIDs()
{
  try {
    auto c = GetScopedConnection();

    QwParitySchema::monitor monitor{};
    auto query = sqlpp::select(sqlpp::all_of(monitor)).from(monitor).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreMonitorID:  monitor_id = " << row.monitor_id << " quantity = " << row.quantity << QwLog::endl;
      QwParityDB::fMonitorIDs.insert(std::make_pair(row.quantity, row.monitor_id));
    });
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    exit(1);
  }
  return;
}

/*
 * This function retrieves the main_detector table key 'main_detector_id' for a given beam main_detector.
 */
UInt_t QwParityDB::GetMainDetectorID(const string& name, Bool_t zero_id_is_error)
{
  if (fMainDetectorIDs.size() == 0) {
    StoreMainDetectorIDs();
  }

  UInt_t main_detector_id = fMainDetectorIDs[name];

  if (zero_id_is_error && main_detector_id==0) {

    if (fDBInsertMissingKeys) {
      QwWarning << "Inserting missing variable " << name << " into main_detector table." << QwLog::endl;
      try {
        auto c = GetScopedConnection();

        QwParitySchema::main_detector main_detector;
        QwParitySchema::row<QwParitySchema::main_detector> main_detector_row;
        main_detector_row[main_detector.quantity] = name;
        main_detector_row[main_detector.title] = "unknown";

        auto insert_id = QueryInsertAndGetId(main_detector_row.insert_into());

        if (insert_id != 0) {
          fMainDetectorIDs[name] = insert_id;
          main_detector_id = insert_id;
          QwWarning << "Successfully inserted variable " << name << " into main_detector table with ID " << insert_id << QwLog::endl;
        } else {
          QwError << "Failed to insert variable " << name << " into main_detector table." << QwLog::endl;
        }
      }
      catch (const std::exception& er) {
        QwError << er.what() << QwLog::endl;
      }
    } else {
      QwError << "To enable automatic insertion of missing variables, set the option '--QwDatabase.insert-missing-keys'" << QwLog::endl;
    }
  }

  return main_detector_id;
}


/*
 * Stores main_detector table keys in an associative array indexed by main_detector name.
 */
void QwParityDB::StoreMainDetectorIDs()
{
  try {
    auto c = GetScopedConnection();

    QwParitySchema::main_detector main_detector{};
    auto query = sqlpp::select(sqlpp::all_of(main_detector)).from(main_detector).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreMainDetectorID:  main_detector_id = " << row.main_detector_id << " quantity = " << row.quantity << QwLog::endl;
      QwParityDB::fMainDetectorIDs.insert(std::make_pair(row.quantity, row.main_detector_id));
    });
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    exit(1);
  }
  return;
}


/*
 * This function retrieves the slow control detector table key 'sc_detector_id' for a given epics variable.
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
      QwWarning << "Inserting missing variable " << name << " into sc_detector table." << QwLog::endl;
      try {
        auto c = GetScopedConnection();

        QwParitySchema::sc_detector sc_detector;
        QwParitySchema::row<QwParitySchema::sc_detector> sc_detector_row;
        sc_detector_row[sc_detector.name] = name;
        sc_detector_row[sc_detector.units] = "unknown";
        sc_detector_row[sc_detector.comment] = "unknown";

        auto insert_id = QueryInsertAndGetId(sc_detector_row.insert_into());

        if (insert_id != 0) {
          fSlowControlDetectorIDs[name] = insert_id;
          sc_detector_id = insert_id;
          QwWarning << "Successfully inserted variable " << name << " into sc_detector table with ID " << insert_id << QwLog::endl;
        } else {
          QwError << "Failed to insert variable " << name << " into sc_detector table." << QwLog::endl;
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
 * Stores slow control detector table keys in an associative array indexed by slow_controls_data name.
 */
void QwParityDB::StoreSlowControlDetectorIDs()
{
  try {
    auto c = GetScopedConnection();

    QwParitySchema::sc_detector sc_detector{};
    auto query = sqlpp::select(sqlpp::all_of(sc_detector)).from(sc_detector).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreSlowControlDetectorID: sc_detector_id = " << row.sc_detector_id << " name = " << row.name << QwLog::endl;
      QwParityDB::fSlowControlDetectorIDs.insert(std::make_pair(row.name, row.sc_detector_id));
    });
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    exit(1);
  }
  return;
}

/*
 * Stores error_code table keys in an associative array indexed by error_code quantity.
 */
void QwParityDB::StoreErrorCodeIDs()
{
  try {
    auto c = GetScopedConnection();
    
    QwParitySchema::error_code error_code{};
    auto query = sqlpp::select(sqlpp::all_of(error_code)).from(error_code).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreErrorCodeID: error_code_id = " << row.error_code_id << " quantity = " << row.quantity << QwLog::endl;
      QwParityDB::fErrorCodeIDs.insert(std::make_pair(row.quantity, row.error_code_id));
    });
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    exit(1);
  }
  return;
}

/*
 * This function retrieves the lumi_detector table key 'lumi_detector_id' for a given beam lumi_detector.
 */
UInt_t QwParityDB::GetLumiDetectorID(const string& name, Bool_t zero_id_is_error)
{
  if (fLumiDetectorIDs.size() == 0) {
    StoreLumiDetectorIDs();
  }

  UInt_t lumi_detector_id = fLumiDetectorIDs[name];

  if (zero_id_is_error && lumi_detector_id==0) {
     QwError << "QwParityDB::GetLumiDetectorID() => Unable to determine valid ID for beam lumi_detector " << name << QwLog::endl;
  }

  return lumi_detector_id;
}

/*
 * Stores lumi_detector table keys in an associative array indexed by lumi_detector name.
 */
void QwParityDB::StoreLumiDetectorIDs()
{
  try {
    auto c = GetScopedConnection();

    QwParitySchema::lumi_detector lumi_detector{};
    auto query = sqlpp::select(sqlpp::all_of(lumi_detector)).from(lumi_detector).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreLumiDetectorID:  lumi_detector_id = " << row.lumi_detector_id << " quantity = " << row.quantity << QwLog::endl;
      QwParityDB::fLumiDetectorIDs.insert(std::make_pair(row.quantity, row.lumi_detector_id));
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

  string measurement_type = fMeasurementIDs[index];

  if (measurement_type.empty()) {
    QwError << "QwParityDB::GetMeasurementID() => Unable to determine valid ID for measurement type with " << index << QwLog::endl;
  }

  return measurement_type;
}

void QwParityDB::StoreMeasurementIDs()
{
  try {
    auto c = GetScopedConnection();

    QwParitySchema::measurement_type measurement_type{};
    auto query = sqlpp::select(sqlpp::all_of(measurement_type)).from(measurement_type).where(sqlpp::value(true));
    QuerySelectForEachResult(query, [&](const auto& row) {
      QwDebug << "StoreMeasurementID:  measurement_type = " << row.measurement_type_id << QwLog::endl;
      QwParityDB::fMeasurementIDs.push_back((std::string) row.measurement_type_id);
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
    ("QwParityDB.disable-analysis-check", 
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
  if (options.GetValue<bool>("QwParityDB.disable-analysis-check"))  
    fDisableAnalysisCheck=true;

  return;
}

#endif // #ifdef __USE_DATABASE__
