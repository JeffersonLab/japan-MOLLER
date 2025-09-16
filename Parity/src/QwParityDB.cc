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
    
    //  Write from the datebase
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

    Connect();

    const auto& run_table = QwParitySchema::run{};
    auto query = sqlpp::select(sqlpp::all_of(run_table))
                 .from(run_table)
                 .where(run_table.run_number == runnum);
    auto result = this->QuerySelect(query);

    size_t result_count = 0;
    std::visit([&](auto& res) {
      for (const auto& row : res) {
        result_count++;
      }
    }, result);
    QwDebug << "Number of rows returned:  " << result_count << QwLog::endl;

    if (result_count != 1) {
      QwError << "Unable to find unique run number " << runnum << " in database." << QwLog::endl;
      QwError << "Run number query returned " << result_count << "rows." << QwLog::endl;
      QwError << "Please make sure that the database contains one unique entry for this run." << QwLog::endl;
      return false;
    }

    // Access first row from result
    UInt_t found_run_id = 0;
    std::visit([&](auto& res) {
      for (const auto& row : res) {
        found_run_id = row.run_id;
        break;
      }
    }, result);
    QwDebug << "Run ID = " << found_run_id << QwLog::endl;

    fRunNumber = runnum;
    fRunID = found_run_id;

    Disconnect();
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
  try
    {
      Connect();

      const auto& run_table = QwParitySchema::run{};
      auto query = sqlpp::select(sqlpp::all_of(run_table))
                   .from(run_table)
                   .where(run_table.run_number == qwevt.GetRunNumber());
      auto result = this->QuerySelect(query);
      
      size_t result_count = 0;
      UInt_t first_run_id = 0;
      UInt_t first_run_number = 0;
      std::visit([&](auto& res) {
        for (const auto& row : res) {
          if (result_count == 0) {
            first_run_id = row.run_id;
            first_run_number = row.run_number;
          }
          result_count++;
        }
      }, result);
      
      QwDebug << "QwParityDB::SetRunID => Number of rows returned:  " << result_count << QwLog::endl;

      // If there is more than one run in the DB with the same run number, then there will be trouble later on.  Catch and bomb out.
      if (result_count > 1)
      {
        QwError << "Unable to find unique run number " << qwevt.GetRunNumber() << " in database." << QwLog::endl;
        QwError << "Run number query returned " << result_count << "rows." << QwLog::endl;
        QwError << "Please make sure that the database contains one unique entry for this run." << QwLog::endl;
        Disconnect();
        return 0;
      }

      // Run already exists in database.  Pull run_id and move along.
      if (result_count == 1)
      {
        QwDebug << "QwParityDB::SetRunID => Run ID = " << first_run_id << QwLog::endl;

        fRunNumber = qwevt.GetRunNumber();
        fRunID     = first_run_id;
        Disconnect();
        return fRunID;
      }
      Disconnect();
    }
  catch (const std::exception& er)
    {

      QwError << er.what() << QwLog::endl;
      Disconnect();
      return 0;
    }

  // Run is not in database so insert pertinent data and retrieve run ID
  // Right now this does not insert start/stop times or info on number of events.
  try
    {

      Connect();
      const auto& run_table = QwParitySchema::run{};
      auto insert_query = sqlpp::insert_into(run_table).set(
        run_table.run_number = qwevt.GetRunNumber(),
        run_table.run_type = "good", // qwevt.GetRunType(); RunType is the confused name because we have also a CODA run type.
        // Convert Unix timestamps to sqlpp11 datetime using chrono time_point
        run_table.start_time = std::chrono::system_clock::from_time_t(qwevt.GetStartUnixTime()),
        run_table.end_time = std::chrono::system_clock::from_time_t(qwevt.GetEndUnixTime()),
        run_table.n_mps = 0,
        run_table.n_qrt = 0,
        // Set following quantities to 9999 as "uninitialized value".  DTS 8/3/2012
        run_table.slug = 9999,
        run_table.wien_slug = 9999,
        run_table.injector_slug = 9999,
        run_table.comment = ""
      );
        
      QwDebug << "QwParityDB::SetRunID() => Executing sqlpp11 run insert" << QwLog::endl;
      auto insert_id = this->QueryInsertAndGetId(insert_query);
      
      if (insert_id != 0)
      {
        fRunNumber = qwevt.GetRunNumber();
        fRunID     = insert_id;
      }
      Disconnect();
      return fRunID;
    }
  catch (const std::exception& er)
    {
      QwError << er.what() << QwLog::endl;
      Disconnect();
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
     QwDebug << "QwParityDB::GetRunID() set fRunID to " << SetRunID(qwevt) << QwLog::endl;
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
  try
    {
      Connect();
      // Convert MySQL++ SELECT query to sqlpp11
      QwParitySchema::runlet runlet_table{};
      
      // Query is slightly different if file segments are being chained together for replay or not.
      if (qwevt.AreRunletsSplit()) {
        fSegmentNumber = qwevt.GetSegmentNumber();
        auto query = sqlpp::select(sqlpp::all_of(runlet_table))
                     .from(runlet_table)
                     .where(runlet_table.run_id == fRunID 
                            and runlet_table.full_run == "false" 
                            and runlet_table.segment_number == fSegmentNumber);
        auto res = this->QuerySelect(query);
        
        // Count results and process using visitor pattern  
        size_t result_count = 0;
        UInt_t found_runlet_id = 0;
        std::visit([&](auto& result) {
          for (const auto& row : result) {
            result_count++;
            if (result_count == 1) {
              found_runlet_id = row.runlet_id;
            }
          }
        }, res);
        
        QwDebug << "QwParityDB::SetRunletID => Number of rows returned:  " << result_count << QwLog::endl;
        
        // If there is more than one run in the DB with the same runlet number, then there will be trouble later on.
        if (result_count > 1) {
          QwError << "Unable to find unique runlet number " << qwevt.GetRunNumber() << " in database." << QwLog::endl;
          QwError << "Run number query returned " << result_count << " rows." << QwLog::endl;
          QwError << "Please make sure that the database contains one unique entry for this run." << QwLog::endl;
          Disconnect();
          return 0;
        }
        
        // Run already exists in database.  Pull runlet_id and move along.
        if (result_count == 1) {
          QwDebug << "QwParityDB::SetRunletID => Runlet ID = " << found_runlet_id << QwLog::endl;
          fRunletID = found_runlet_id;
          Disconnect();
          return fRunletID;
        }
        
      } else {
        auto query = sqlpp::select(sqlpp::all_of(runlet_table))
                     .from(runlet_table)
                     .where(runlet_table.run_id == fRunID 
                            and runlet_table.full_run == "true");
        auto res = this->QuerySelect(query);
        
        // Count results and process using visitor pattern  
        size_t result_count = 0;
        UInt_t found_runlet_id = 0;
        std::visit([&](auto& result) {
          for (const auto& row : result) {
            result_count++;
            if (result_count == 1) {
              found_runlet_id = row.runlet_id;
            }
          }
        }, res);
        
        QwDebug << "QwParityDB::SetRunletID => Number of rows returned:  " << result_count << QwLog::endl;
        
        // If there is more than one run in the DB with the same runlet number, then there will be trouble later on.
        if (result_count > 1) {
          QwError << "Unable to find unique runlet number " << qwevt.GetRunNumber() << " in database." << QwLog::endl;
          QwError << "Run number query returned " << result_count << " rows." << QwLog::endl;
          QwError << "Please make sure that the database contains one unique entry for this run." << QwLog::endl;
          Disconnect();
          return 0;
        }
        
        // Run already exists in database.  Pull runlet_id and move along.
        if (result_count == 1) {
          QwDebug << "QwParityDB::SetRunletID => Runlet ID = " << found_runlet_id << QwLog::endl;
          fRunletID = found_runlet_id;
          Disconnect();
          return fRunletID;
        }
      }
      Disconnect();
    }
  catch (const std::exception& er)
    {

      QwError << er.what() << QwLog::endl;
      Disconnect();
      return 0;
    }

  // Runlet is not in database so insert pertinent data and retrieve run ID
  // Right now this does not insert start/stop times or info on number of events.
  try
    {
      Connect();

      // Convert MySQL++ insert to sqlpp11 for runlet table
      QwParitySchema::runlet runlet_table{};
      uint64_t insert_id = 0;
      
      // Handle segment_number based on runlet split condition
      if (qwevt.AreRunletsSplit()) {
        auto insert_query = sqlpp::insert_into(runlet_table)
                           .set(runlet_table.run_id = fRunID,
                                runlet_table.run_number = qwevt.GetRunNumber(),
                                runlet_table.start_time = sqlpp::null,
                                runlet_table.end_time = sqlpp::null,
                                runlet_table.first_mps = 0,
                                runlet_table.last_mps = 0,
                                runlet_table.segment_number = fSegmentNumber,
                                runlet_table.full_run = "false");
        
        QwDebug << "QwParityDB::SetRunletID() => Executing sqlpp11 runlet insert (with segment)" << QwLog::endl;
        insert_id = this->QueryInsertAndGetId(insert_query);
      } else {
        auto insert_query = sqlpp::insert_into(runlet_table)
                           .set(runlet_table.run_id = fRunID,
                                runlet_table.run_number = qwevt.GetRunNumber(),
                                runlet_table.start_time = sqlpp::null,
                                runlet_table.end_time = sqlpp::null,
                                runlet_table.first_mps = 0,
                                runlet_table.last_mps = 0,
                                runlet_table.segment_number = sqlpp::null,
                                runlet_table.full_run = "true");
        
        QwDebug << "QwParityDB::SetRunletID() => Executing sqlpp11 runlet insert (no segment)" << QwLog::endl;
        insert_id = this->QueryInsertAndGetId(insert_query);
      }
      
      
      if (insert_id != 0)
      {
        fRunletID = insert_id;
      }
      Disconnect();
      return fRunletID;
    }
  catch (const std::exception& er)
    {
      QwError << er.what() << QwLog::endl;
      Disconnect();
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
     QwDebug << "QwParityDB::GetRunletID() set fRunletID to " << SetRunletID(qwevt) << QwLog::endl;
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
    Connect();

    const auto& analysis_table = QwParitySchema::analysis{};
    auto query = sqlpp::select(analysis_table.analysis_id)
                 .from(analysis_table)
                 .where(analysis_table.beam_mode == "nbm"
                        and analysis_table.slope_calculation == "off"
                        and analysis_table.slope_correction == "off"
                        and analysis_table.runlet_id == this->GetRunletID(qwevt));
    auto result = this->QuerySelect(query);

    std::visit([&](auto& res) {
      if (!res.empty()) {
        QwError << "This runlet has already been analyzed by the engine!" << QwLog::endl;
        QwError << "The following analysis_id values already exist in the database:  ";
        for (const auto& row : res) {
          QwError << row.analysis_id << " ";
        }
        QwError << QwLog::endl;

        if (fDisableAnalysisCheck==false) {
          QwError << "Analysis of this run will now be terminated."  << QwLog::endl;
          // Note: return statement cannot be used in lambda to return from function
        } else {
          QwWarning << "Analysis will continue.  A duplicate entry with new analysis_id will be added to the analysis table." << QwLog::endl;
        }
      }
    }, result);

    Disconnect();
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    QwError << "Unable to determine if there are other database entries for this run.  Exiting." << QwLog::endl;
    Disconnect();
    return 0;
  }


  try {

    // Declare analysis variables for INSERT
    UInt_t runlet_id = GetRunletID(qwevt);
    UInt_t seed_id = 1;

    std::pair<UInt_t, UInt_t> event_range;
    event_range = qwevt.GetEventRange();

    // Use sqlpp11 current timestamp with std::chrono
    auto analysis_time = std::chrono::system_clock::now();
    std::string bf_checksum = "empty"; // we will match this as a real one later
    std::string beam_mode = "nbm";   // we will match this as a real one later
    UInt_t n_mps = 0;       // we will match this as a real one later
    UInt_t n_qrt = 4;       // we will match this as a real one later
    UInt_t first_event = event_range.first;
    UInt_t last_event = event_range.second;
    UInt_t segment = 0;       // we will match this as a real one later
    std::string slope_calculation = "off";  // we will match this as a real one later
    std::string slope_correction = "off"; // we will match this as a real one later
    
    // Variables for run condition parsing
    std::string root_version = "";
    std::string root_file_time = "";
    std::string root_file_host = "";
    std::string root_file_user = "";
    std::string analyzer_name = "";
    std::string analyzer_argv = "";
    std::string analyzer_svn_rev = "";
    std::string analyzer_svn_lc_rev = "";
    std::string analyzer_svn_url = "";
    std::string roc_flags = "";

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
      if (str_var.EqualTo("root_version"))  {
        root_version = str_val;
      } else if (str_var.EqualTo("root_file_time"))  {
        root_file_time = str_val;
      } else if (str_var.EqualTo("root_file_host"))  {
        root_file_host = str_val;
      } else if (str_var.EqualTo("root_file_user"))  {
        root_file_user = str_val;
      } else if (str_var.EqualTo("analyzer_name"))  {
        analyzer_name = str_val;
      } else if (str_var.EqualTo("analyzer_argv"))  {
        analyzer_argv = str_val;
      } else if (str_var.EqualTo("analyzer_svn_rev"))  {
        analyzer_svn_rev = str_val;
      } else if (str_var.EqualTo("analyzer_svn_lc_rev"))  {
        analyzer_svn_lc_rev = str_val;
      } else if (str_var.EqualTo("analyzer_svn_url"))  {
        analyzer_svn_url = str_val;
      } else if (str_var.EqualTo("roc_flags"))  {
        roc_flags = str_val;
      }
    }
    
    Connect();
    // Convert MySQL++ insert to sqlpp11 for analysis table
    QwParitySchema::analysis analysis_table{};
    auto insert_query = sqlpp::insert_into(analysis_table)
                        .set(analysis_table.runlet_id = runlet_id,
                             analysis_table.seed_id = seed_id,
                             analysis_table.time = analysis_time,
                             analysis_table.bf_checksum = bf_checksum,
                             analysis_table.beam_mode = beam_mode,
                             analysis_table.n_mps = n_mps,
                             analysis_table.n_qrt = n_qrt,
                             analysis_table.first_event = first_event,
                             analysis_table.last_event = last_event,
                             analysis_table.segment = segment,
                             analysis_table.slope_calculation = slope_calculation,
                             analysis_table.slope_correction = slope_correction,
                             analysis_table.root_version = root_version,
                             analysis_table.root_file_time = root_file_time,
                             analysis_table.root_file_host = root_file_host,
                             analysis_table.root_file_user = root_file_user,
                             analysis_table.analyzer_name = analyzer_name,
                             analysis_table.analyzer_argv = analyzer_argv,
                             analysis_table.analyzer_svn_rev = analyzer_svn_rev,
                             analysis_table.analyzer_svn_lc_rev = analyzer_svn_lc_rev,
                             analysis_table.analyzer_svn_url = analyzer_svn_url,
                             analysis_table.roc_flags = roc_flags);
    
    auto insert_id = this->QueryInsertAndGetId(insert_query);
    
    if (insert_id != 0)
    {
      fAnalysisID = insert_id;
    }

    Disconnect();
    return fAnalysisID;
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    Disconnect();
    return 0;
  }


}

void QwParityDB::FillParameterFiles(QwSubsystemArrayParity& subsys){
  TList* param_file_list = subsys.GetParamFileNameList("mapfiles");
  try {
    Connect();
    
    // Convert MySQL++ insert loop to sqlpp11
    QwParitySchema::parameter_files param_files_table{};
    UInt_t analysis_id = GetAnalysisID();

    param_file_list->Print();
    TIter next(param_file_list);
    TList *pfl_elem;
    while ((pfl_elem = (TList *) next())) {
      auto insert_query = sqlpp::insert_into(param_files_table)
                          .set(param_files_table.analysis_id = analysis_id,
                               param_files_table.filename = pfl_elem->GetName());
      this->QueryExecute(insert_query);
    }

    Disconnect();

    delete param_file_list;
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    Disconnect();
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
    QwDebug << "QwParityDB::GetAnalysisID() set fAnalysisID to " << SetAnalysisID(qwevt) << QwLog::endl;
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
    //    monitor_id = 6; // only for QwMockDataAnalysis
    QwError << "QwParityDB::GetMonitorID() => Unable to determine valid ID for beam monitor " << name << QwLog::endl;
  }

  return monitor_id;

}

/*
 * Stores monitor table keys in an associative array indexed by monitor name.
 */
void QwParityDB::StoreMonitorIDs()
{
  try {

    Connect();
    // Convert MySQL++ for_each to sqlpp11 select with range-based iteration
    QwParitySchema::monitor monitor_table{};
    auto query = sqlpp::select(sqlpp::all_of(monitor_table)).from(monitor_table).unconditionally();
    auto res = this->QuerySelect(query);
    
    std::visit([&](auto& result) {
      for (const auto& row : result) {
        QwDebug << "StoreMonitorID:  monitor_id = " << row.monitor_id << " quantity = " << row.quantity << QwLog::endl;
        QwParityDB::fMonitorIDs.insert(std::make_pair(row.quantity, row.monitor_id));
      }
    }, res);

//    QwDebug<< "QwParityDB::SetAnalysisID() => Analysis Insert Query = " << query.str() << QwLog::endl;
    Disconnect();
  }

  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    Disconnect();
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
    //    main_detector_id = 19; // only for QwMockDataAnalysis
    QwError << "QwParityDB::GetMainDetectorID() => Unable to determine valid ID for beam main_detector " << name << QwLog::endl;
  }

  return main_detector_id;

}


/*
 * Stores main_detector table keys in an associative array indexed by main_detector name.
 */
void QwParityDB::StoreMainDetectorIDs()
{

  try {
    Connect();

    // Convert MySQL++ for_each to sqlpp11 select with range-based iteration for main_detector table
    QwParitySchema::main_detector main_detector_table{};
    auto query = sqlpp::select(sqlpp::all_of(main_detector_table)).from(main_detector_table).unconditionally();
    auto res = this->QuerySelect(query);
    
    std::visit([&](auto& result) {
      for (const auto& row : result) {
        QwDebug << "StoreMainDetectorID:  main_detector_id = " << row.main_detector_id << " quantity = " << row.quantity << QwLog::endl;
        QwParityDB::fMainDetectorIDs.insert(std::make_pair(row.quantity, row.main_detector_id));
      }
    }, res);

//    QwDebug<< "QwParityDB::SetAnalysisID() => Analysis Insert Query = " << query.str() << QwLog::endl;

    Disconnect();
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    Disconnect();
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
    Connect();
    
    // Convert MySQL++ for_each to sqlpp11 select for sc_detector table
    QwParitySchema::sc_detector sc_detector_table{};
    auto query = sqlpp::select(sqlpp::all_of(sc_detector_table)).from(sc_detector_table).unconditionally();
    auto res = this->QuerySelect(query);
    
    std::visit([&](auto& result) {
      for (const auto& row : result) {
        QwDebug << "StoreSlowControlDetectorID: sc_detector_id = " << row.sc_detector_id << " name = " << row.name << QwLog::endl;
        QwParityDB::fSlowControlDetectorIDs.insert(std::make_pair(row.name, row.sc_detector_id));
      }
    }, res);

//    QwDebug<< "QwParityDB::SetAnalysisID() => Analysis Insert Query = " << query.str() << QwLog::endl;

    Disconnect();
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    Disconnect();
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
    Connect();
    
    // Convert MySQL++ for_each to sqlpp11 select for error_code table
    QwParitySchema::error_code error_code_table{};
    auto query = sqlpp::select(sqlpp::all_of(error_code_table)).from(error_code_table).unconditionally();
    auto res = this->QuerySelect(query);
    
    std::visit([&](auto& result) {
      for (const auto& row : result) {
        QwDebug << "StoreErrorCodeID: error_code_id = " << row.error_code_id << " quantity = " << row.quantity << QwLog::endl;
        QwParityDB::fErrorCodeIDs.insert(std::make_pair(row.quantity, row.error_code_id));
      }
    }, res);

    Disconnect();
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    Disconnect();
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
    Connect();

    // Convert MySQL++ for_each to sqlpp11 select for lumi_detector table
    QwParitySchema::lumi_detector lumi_detector_table{};
    auto query = sqlpp::select(sqlpp::all_of(lumi_detector_table)).from(lumi_detector_table).unconditionally();
    auto res = this->QuerySelect(query);
    
    std::visit([&](auto& result) {
      for (const auto& row : result) {
        QwDebug << "StoreLumiDetectorID:  lumi_detector_id = " << row.lumi_detector_id << " quantity = " << row.quantity << QwLog::endl;
        QwParityDB::fLumiDetectorIDs.insert(std::make_pair(row.quantity, row.lumi_detector_id));
      }
    }, res);

    Disconnect();
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    Disconnect();
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
    Connect();

    // Convert MySQL++ for_each to sqlpp11 select for measurement_type table
    QwParitySchema::measurement_type measurement_type_table{};
    auto query = sqlpp::select(sqlpp::all_of(measurement_type_table)).from(measurement_type_table).unconditionally();
    auto res = this->QuerySelect(query);
    
    std::visit([&](auto& result) {
      for (const auto& row : result) {
        QwDebug << "StoreMeasurementID:  measurement_type = " << row.measurement_type_id << QwLog::endl;
        QwParityDB::fMeasurementIDs.push_back((std::string) row.measurement_type_id);
      }
    }, res);

    Disconnect();
  }
  catch (const std::exception& er) {
    QwError << er.what() << QwLog::endl;
    Disconnect();
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
