/*!
 * \file   QwBlinder.cc
 * \brief  A class for blinding data, adapted from G0 blinder class.
 *
 * \author Peiqing Wang
 * \date   2010-04-14
 */

#include "QwBlinder.h"

// System headers
#include <string>
#include <limits>

#include "TMath.h"

// Qweak headers
#include "QwLog.h"
#ifdef __USE_DATABASE__
#include "QwParitySchema.h"
#include "QwParityDB.h"
#endif // __USE_DATABASE__
#include "QwVQWK_Channel.h"
#include "QwParameterFile.h"

///  Run table aliases for seed query
///  (these types must be defined outside function scope)
#ifdef __USE_DATABASE__
#ifdef __USE_SQLPP11__
SQLPP_ALIAS_PROVIDER(run_first);
SQLPP_ALIAS_PROVIDER(run_last);
#endif // __USE_SQLPP11__
#ifdef __USE_SQLPP23__
SQLPP_CREATE_NAME_TAG(run_first);
SQLPP_CREATE_NAME_TAG(run_last);
#endif // __USE_SQLPP23__
#endif // __USE_DATABASE__

///  Blinder event counter indices
enum EQwBlinderErrorCounterIndices{
  kBlinderCount_Blindable=0,
  kBlinderCount_NonBlindable,
  kBlinderCount_Transverse,
  kBlinderCount_Disabled,
  kBlinderCount_NoBeam,
  kBlinderCount_UnknownTarget,
  kBlinderCount_ChangedTarget,
  kBlinderCount_UndefinedWien,
  kBlinderCount_ChangedWien,
  kBlinderCount_UndefinedIHWP,
  kBlinderCount_ChangedIHWP,
  kBlinderCount_OtherFailure,
  kBlinderCount_NumCounters
};


//  String names of the blinding and Wien status values
const TString QwBlinder::fStatusName[4] = {"Indeterminate", "NotBlindable",
					   "Blindable", "BlindableFail"};

// Maximum blinding asymmetry for additive blinding
const Double_t QwBlinder::kDefaultMaximumBlindingAsymmetry = 0.150; // ppm
const Double_t QwBlinder::kDefaultMaximumBlindingFactor = 0.0; // [fraction]

// Default seed, associated with seed_id 0
const TString QwBlinder::kDefaultSeed = "Default seed, should not be used!";

//**************************************************//
void QwBlinder::DefineOptions(QwOptions &options){
  options.AddOptions("Blinder")("blinder.force-target-blindable", po::value<bool>()->default_bool_value(false),
		       "Forces the blinder to interpret the target as being in a blindable position");
  options.AddOptions("Blinder")("blinder.force-target-out", po::value<bool>()->default_bool_value(false),
		       "Forces the blinder to interpret the target position as target-out");
  options.AddOptions("Blinder")("blinder.beam-current-threshold", po::value<double>()->default_value(2.5),
		       "Beam current in microamps below which data will not be blinded");
}


/**
 * Default constructor using optional database connection and blinding strategy
 * @param blinding_strategy Blinding strategy
 */
QwBlinder::QwBlinder(const EQwBlindingStrategy blinding_strategy):
  fTargetBlindability_firstread(kIndeterminate),
  fTargetBlindability(kIndeterminate),
  fTargetPositionForced(kFALSE),
  //
  fWienMode_firstread(kWienIndeterminate),
  fWienMode(kWienIndeterminate),
  fIHWPPolarity_firstread(0),
  fIHWPPolarity(0),
  fSpinDirectionForced(kFALSE),
  //
  fBeamCurrentThreshold(1.0),
  fBeamIsPresent(kFALSE),
  fBlindingStrategy(blinding_strategy),
  fBlindingOffset(0.0),
  fBlindingOffset_Base(0.0),
  fBlindingFactor(1.0),
  //
  fMaximumBlindingAsymmetry(kDefaultMaximumBlindingAsymmetry),
  fMaximumBlindingFactor(kDefaultMaximumBlindingFactor)
{
  // Set up the blinder with seed_id 0
  fSeed = kDefaultSeed;
  fSeedID = 0;

  fCREXTargetIndex  = -1;

  Int_t tgt_index;

  // Read parameter file
  QwParameterFile blinder("blinder.map");
  if (blinder.FileHasVariablePair("=", "seed", fSeed))
    QwVerbose << "Using seed from file: " << fSeed << QwLog::endl;
  if (blinder.FileHasVariablePair("=", "max_asymmetry", fMaximumBlindingAsymmetry))
    QwVerbose << "Using blinding box: " << fMaximumBlindingAsymmetry << " ppm" << QwLog::endl;
  if (blinder.FileHasVariablePair("=", "max_factor", fMaximumBlindingFactor))
    QwVerbose << "Using blinding factor: " << fMaximumBlindingFactor << QwLog::endl;
  if (blinder.FileHasVariablePair("=", "crex_target_index", tgt_index)){
    if (tgt_index>=kCREXTgtIndexMin && tgt_index<=kCREXTgtIndexMax){
      fCREXTargetIndex = tgt_index;
    } else {
      QwError << "Invalid CREX target index for blindable events!  Exiting!"
	      << QwLog::endl;
      exit(100);
    }
  }
  QwMessage << "What is the blindable CREX target position (-1 means we're using the PREX positions)? " << fCREXTargetIndex << QwLog::endl;
  if (fCREXTargetIndex>=kCREXTgtIndexMin){
    fSeed.Prepend(TString("[Using CREX positions!]  "));
    QwMessage << "Updated the seed string: " << fSeed << QwLog::endl;
  }
  std::string strategy;
  if (blinder.FileHasVariablePair("=", "strategy", strategy)) {
    std::transform(strategy.begin(), strategy.end(), strategy.begin(), ::tolower);
    QwVerbose << "Using blinding strategy from file: " << strategy << QwLog::endl;
    if (strategy == "disabled") fBlindingStrategy = kDisabled;
    else if (strategy == "additive") fBlindingStrategy = kAdditive;
    else if (strategy == "multiplicative") fBlindingStrategy = kMultiplicative;
    else if (strategy == "additivemultiplicative") fBlindingStrategy = kAdditiveMultiplicative;
    else QwWarning << "Blinding strategy " << strategy << " not recognized" << QwLog::endl;
  }

  std::string spin_direction;
  if (blinder.FileHasVariablePair("=", "force-spin-direction", spin_direction)) {
    std::transform(spin_direction.begin(), spin_direction.end(), spin_direction.begin(), ::tolower);
    if (spin_direction == "spin-forward"){
      QwWarning << "QwBlinder::QwBlinder:  Spin direction forced with force-spin-direction==spin-forward" << QwLog::endl;
      SetWienState(kWienForward);
      SetIHWPPolarity(+1);
      fSpinDirectionForced = kTRUE;
    } else if (spin_direction == "spin-backward"){
      QwWarning << "QwBlinder::QwBlinder:  Spin direction forced with force-spin-direction==spin-backward" << QwLog::endl;
      SetWienState(kWienBackward);
      SetIHWPPolarity(+1);
      fSpinDirectionForced = kTRUE;
    } else if (spin_direction == "spin-vertical"){
      QwWarning << "QwBlinder::QwBlinder:  Spin direction forced with force-spin-direction==spin-vertical" << QwLog::endl;
      SetWienState(kWienVertTrans);
      SetIHWPPolarity(+1);
      fSpinDirectionForced = kTRUE;
    } else if (spin_direction == "spin-horizontal"){
      QwWarning << "QwBlinder::QwBlinder:  Spin direction forced with force-spin-direction==spin-horizontal" << QwLog::endl;
      SetWienState(kWienHorizTrans);
      SetIHWPPolarity(+1);
      fSpinDirectionForced = kTRUE;
    } else {
      QwError << "QwBlinder::QwBlinder:  Unrecognized option given to force-spin-direction in blinder.map; "
	      << "force-spin-direction==" << spin_direction << ".  Exit and correct the file."
	      << QwLog::endl;
      exit(10);
    }
  }

  std::string target_type;
  if (blinder.FileHasVariablePair("=", "force-target-type", target_type)) {
    std::transform(target_type.begin(), target_type.end(), target_type.begin(), ::tolower);
    if (target_type == "target-blindable"){
      QwWarning << "QwBlinder::QwBlinder:  Target position forced with force-target-type==target-blindable" << QwLog::endl;
      fTargetPositionForced = kTRUE;
      SetTargetBlindability(QwBlinder::kBlindable);
    } else if (target_type == "target-out"){
      QwWarning << "QwBlinder::QwBlinder:  Target position forced with force-target-type==target-out" << QwLog::endl;
      fTargetPositionForced = kTRUE;
      SetTargetBlindability(QwBlinder::kNotBlindable);
    } else {
      QwError << "QwBlinder::QwBlinder:  Unrecognized option given to force-target-type in blinder.map; "
	      << "force-target-type==" << target_type << ".  Exit and correct the file."
	      << QwLog::endl;
      exit(10);
    }
  }

  // Initialize blinder from seed
  InitBlinders(0);
  // Calculate set of test values
  InitTestValues(10);

  if (fSpinDirectionForced){
    if (fWienMode == kWienForward){
      fBlindingOffset = fBlindingOffset_Base;
    } else if (fWienMode == kWienBackward){
      fBlindingOffset = -1 * fBlindingOffset_Base;
    } else {
      fBlindingOffset = 0.0;
    }
  }

  // Resize counters
  fPatternCounters.resize(kBlinderCount_NumCounters);
  fPairCounters.resize(kBlinderCount_NumCounters);
}


/**
 * Destructor checks the validity of the blinding and unblinding
 */
QwBlinder::~QwBlinder()
{
  // Check the blinded values
  PrintFinalValues();
}


/**
 * Update the blinder status with new external information
 * @param options Qweak option handler
 */
void QwBlinder::ProcessOptions(QwOptions& options)
{
  if (options.GetValue<bool>("blinder.force-target-out")
      && options.GetValue<bool>("blinder.force-target-blindable")){
    QwError << "QwBlinder::ProcessOptions:  Both blinder.force-target-blindable and blinder.force-target-out are set.  "
	    << "Only one can be in force at one time.  Exit and choose one option."
	    << QwLog::endl;
    exit(10);
  } else if (options.GetValue<bool>("blinder.force-target-blindable")){
    QwWarning << "QwBlinder::ProcessOptions:  Target position forced with blinder.force-target-blindable." << QwLog::endl;
    fTargetPositionForced = kTRUE;
    SetTargetBlindability(QwBlinder::kBlindable);
  } else if (options.GetValue<bool>("blinder.force-target-out")){
    QwWarning << "QwBlinder::ProcessOptions:  Target position forced with blinder.force-target-out." << QwLog::endl;
    fTargetPositionForced = kTRUE;
    SetTargetBlindability(QwBlinder::kNotBlindable);
  }

  fBeamCurrentThreshold = options.GetValue<double>("blinder.beam-current-threshold");
}

#ifdef __USE_DATABASE__
/**
 * Update the blinder status with new external information
 *
 * @param db Database connection
 */
void QwBlinder::Update(QwParityDB* db)
{
  //  Update the seed ID then tell us if it has changed.
  UInt_t old_seed_id = fSeedID;
  ReadSeed(db);
  // If the blinding seed has changed, re-initialize the blinder
  if (fSeedID != old_seed_id ||
      (fSeedID==0 && fSeed!=kDefaultSeed) ) {
    QwWarning << "Changing blinder seed to " << fSeedID
              << " from " << old_seed_id << "." << QwLog::endl;
    InitBlinders(fSeedID);
    InitTestValues(10);
  }
}
#endif // __USE_DATABASE__

/**
 * Update the blinder status using a random number
 */
void QwBlinder::Update()
{
  //  Update the seed ID then tell us if it has changed.
  UInt_t old_seed_id = fSeedID;
  ReadRandomSeed();
  //  Force the target to blindable, Wien to be forward,
  //  and IHWP polarity to be +1
  SetTargetBlindability(QwBlinder::kBlindable);
  SetWienState(kWienForward);
  SetIHWPPolarity(+1);
  // If the blinding seed has changed, re-initialize the blinder
  if (fSeedID != old_seed_id ||
      (fSeedID==0 && fSeed!=kDefaultSeed) ) {
    QwWarning << "Changing blinder seed to " << fSeedID
              << " from " << old_seed_id << "." << QwLog::endl;
    InitBlinders(fSeedID);
    InitTestValues(10);
  }
}

/**
 * Update the blinder status with new external information
 *
 * @param detectors Current subsystem array
 */
void QwBlinder::Update(const QwSubsystemArrayParity& detectors)
{
  // Check for the target blindability flag
  if (fBlindingStrategy != kDisabled && fTargetBlindability==kBlindable) {
    
    // Check that the current on target is above acceptable limit
    Bool_t tmp_beam = kFALSE;
    const VQwHardwareChannel* q_targ = detectors.RequestExternalPointer("q_targ");
    if (q_targ != nullptr) {
      if (q_targ->GetValue() > fBeamCurrentThreshold) {
        tmp_beam = kTRUE;
      }
    }

    fBeamIsPresent &= tmp_beam;
  }
}

/**
 * Update the blinder status with new external information
 *
 * @param epics Current EPICS event
 */
void QwBlinder::Update(const QwEPICSEvent& epics)
{
  // Check for the target information
  // Position:
  //     QWtgt_name == "HYDROGEN-CELL" || QWTGTPOS > 350
  // Temperature:
  //     QWT_miA < 22.0 && QWT_moA < 22.0
  // Pressure:
  //     QW_PT3 in [20,35] && QW_PT4 in [20,35]
  if (fBlindingStrategy != kDisabled && !(fTargetPositionForced) ) {
    //TString  position  = epics.GetDataString("QWtgt_name");
    Double_t tgt_pos   = epics.GetDataValue("pcrex90BDSPOS.VAL");
    QwDebug << "Target parameters used by the blinder: "
      //	  << "QWtgt_name=" << position << " "
	    << "QWTGTPOS=" << tgt_pos << " "
	    << QwLog::endl;

    //
    // ****  Target index 1:  Beginning of CREX
    if (fCREXTargetIndex==1 &&
	(tgt_pos>14.5e6 && tgt_pos<18.0e6) ){
      //  Calcium-48 target position
      SetTargetBlindability(QwBlinder::kBlindable);

    } else if (fCREXTargetIndex==1 &&
	       ( (tgt_pos>-1.0e3 && tgt_pos<14.5e6)
		 || (tgt_pos>18.0e6 && tgt_pos<61.e6) ) ){
      //  Reasonable non-calcium-48 target positions
      SetTargetBlindability(QwBlinder::kNotBlindable);


      //
      // ****  Target index 2:  After 20January change in target location
    } else if (fCREXTargetIndex==2 &&
	(tgt_pos>11.5e6 && tgt_pos<14.5e6) ){
      //  Calcium-48 target position (old Ca-40 position)
      SetTargetBlindability(QwBlinder::kBlindable);

    } else if (fCREXTargetIndex==2 &&
	       ( (tgt_pos>-1.0e3 && tgt_pos<11.5e6)
		 || (tgt_pos>14.5e6 && tgt_pos<61.e6) ) ){
      //  Reasonable non-calcium-48 target positions
      SetTargetBlindability(QwBlinder::kNotBlindable);

      //
      //  ****  Target index -1:  These are the PREX positions
    } else if ( fCREXTargetIndex==-1 &&
		(/*  Target positions before 1 August 2019.*/
		 ( (tgt_pos > 3e6 && tgt_pos < 6.9e6)
		   || (tgt_pos > 7.3e6 && tgt_pos < 7.7e6))
		 /*  Target positions after 1 August 2019.*/
		 ||( (tgt_pos>30.e6 && tgt_pos<69e6)
		     || (tgt_pos>73e6 && tgt_pos<78e6)
		     ) ) ){
      //  Lead-208 target positions
      SetTargetBlindability(QwBlinder::kBlindable);

    } else if  ( fCREXTargetIndex==-1 &&
		 (/*  Target positions before 1 August 2019.*/
		  ((tgt_pos > -1e3 && tgt_pos < 3e6)
		   || (tgt_pos > 6.8e6 && tgt_pos < 7.2e6)
		   || (tgt_pos > 7.7e6 && tgt_pos < 10e6))
		  /*  Target positions after 1 August 2019.*/
		  || ( (tgt_pos>17e6 && tgt_pos<30e6)
		       || (tgt_pos>69e6 && tgt_pos<73e6)
		       || (tgt_pos>78e6 && tgt_pos<90e6)
		       )
		  ) ){
      //  Positions are not lead-208 targets.
      SetTargetBlindability(QwBlinder::kNotBlindable);

    } else {
      SetTargetBlindability(QwBlinder::kIndeterminate);
      QwWarning << "Target parameters used by the blinder are indeterminate: "
	//  << "QWtgt_name=" << position << " "
		<< "QWTGTPOS=" << tgt_pos << " "
		<< QwLog::endl;
    } // End of tests on target positions
  }
  // Check for the beam polarity information
  //     IGL1I00DI24_24M         Beam Half-wave plate Read(off=out)
  //
  if (fBlindingStrategy != kDisabled && !(fSpinDirectionForced) &&
      (fTargetBlindability == QwBlinder::kBlindable) ) {
    //  Use the EPICS class functions to determine the
    //  Wien mode and IHWP polarity.
    SetWienState(epics.DetermineWienMode());
    SetIHWPPolarity(epics.DetermineIHWPPolarity());

    if (fWienMode == kWienForward){
      fBlindingOffset = fBlindingOffset_Base * fIHWPPolarity;
    } else if (fWienMode == kWienBackward){
      fBlindingOffset = -1 * fBlindingOffset_Base * fIHWPPolarity;
    } else {
      fBlindingOffset = 0.0;
    }
  }
}

/*!-----------------------------------------------------------
 *------------------------------------------------------------
 * Function to read the seed in from the database.
 *
 * Parameters:
 *
 * Return: Int_t
 *
 *------------------------------------------------------------
 *------------------------------------------------------------*/
#ifdef __USE_DATABASE__
Int_t QwBlinder::ReadSeed(QwParityDB* db)
{
  // Return unchanged if no database specified
  if (! db) {
    QwWarning << "QwBlinder::ReadSeed(): No database specified" << QwLog::endl;
    fSeedID = 0;
    fSeed   = "Default seed, No database specified";
    return 0;
  }
  if (! db->AllowsReadAccess()){
    QwDebug << "QwBlinder::ReadSeed(): Database access is turned off.  Don't update the blinder." << QwLog::endl;
    return 0;
  }

  // Try to connect to the database
  try {
    auto c = db->GetScopedConnection();

    QwError << "QwBlinder::ReadSeed  db->GetRunNumber() returns "
	    <<  db->GetRunNumber() << QwLog::endl;

    // Convert to sqlpp11 query with JOINs
    QwParitySchema::seeds seeds{};
    QwParitySchema::run first_run{};
    QwParitySchema::run last_run{};

    // Create aliases for the run table
    auto rf_alias = first_run.as(run_first);
    auto rl_alias = last_run.as(run_last);
    auto query = sqlpp::select(seeds.seed_id, seeds.seed)
                 .from(seeds
                       .join(rf_alias).on(seeds.first_run_id == rf_alias.run_id)
                       .join(rl_alias).on(seeds.last_run_id == rl_alias.run_id))
                 .where(rf_alias.run_number <= db->GetRunNumber()
                        and rl_alias.run_number >= db->GetRunNumber()
                        and seeds.seed_id > 2);

    QwError << "QwBlinder::ReadSeed executing sqlpp11 query for run number "
            << db->GetRunNumber() << QwLog::endl;

    auto results = c->QuerySelect(query);
    size_t result_count = c->CountResults(results);
    if (result_count == 1) {
      // Analyze the single result using database-agnostic interface
      c->ForFirstResult(results, [this](const auto& row) {
        // Process first (and only) row
        fSeedID = row.seed_id;
        if (!is_null(row.seed)) {
          fSeed = row.seed.value();
        } else {
          QwError << "QwBlinder::ReadSeed(): Seed value came back NULL from the database." << QwLog::endl;
          fSeedID = 0;
          fSeed = kDefaultSeed;
        }
      });

      std::cout << "QwBlinder::ReadSeed():  Successfully read "
        << Form("the fSeed with ID %d from the database.", fSeedID)
        << std::endl;

    } else {
      // Error Condition.
      // There should be one and only one seed_id for each seed.
      fSeedID = 0;
      fSeed   = Form("ERROR:  There should be one and only one seed_id for each seed, but this had %zu.",
                     result_count);
      std::cerr << "QwBlinder::ReadSeed(): "<<fSeed<<std::endl;
    }
  } catch (const std::exception& er) {
    //  We were unable to open the connection.
    fSeedID = 0;
    fSeed   = "ERROR:  Unable to open the connection to the database.";
    QwError << "QwBlinder::ReadSeed(): Unable to open connection to database: " << er.what() << QwLog::endl;
  }

  return fSeedID;
}
#endif // __USE_DATABASE__

/*!-----------------------------------------------------------
 *------------------------------------------------------------
 * Function to read the seed string generated utilizing a random number generator
 *
 * Parameters: none
 *
 * Return: Int_t
 *
 *------------------------------------------------------------
 *------------------------------------------------------------*/
Int_t QwBlinder::ReadRandomSeed()
{
  static const Char_t alphanum[] =
    "0123456789"
    "!@#$%^&*"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  Int_t strLen = sizeof(alphanum) - 1;
  const size_t length = 20;
  Char_t randomchar[length];
  // Initialize random number generator.
  srand(time(0));
  //get  a "random" positive integer
  
  for (size_t i = 0; i < length; ++i) {
    randomchar[i] = alphanum[rand() % strLen];
  }
  fSeedID=rand();
  TString frandomSeed(randomchar, length);
  fSeed=frandomSeed;//a random string
  return fSeedID;
}

/*!-----------------------------------------------------------
 *------------------------------------------------------------
 * Function to read the seed in from the database.
 *
 * Parameters:
 *	seed_id = ID number of seed to use (0 = most recent seed)
 *
 * Return: Int_t
 *
 *------------------------------------------------------------
 *------------------------------------------------------------*/
#ifdef __USE_DATABASE__
Int_t QwBlinder::ReadSeed(QwParityDB* db, const UInt_t seed_id)
{
  // Return unchanged if no database specified
  if (! db) {
    QwWarning << "QwBlinder::ReadSeed(): No database specified" << QwLog::endl;
    fSeedID = 0;
    fSeed   = "Default seed, No database specified";
    return 0;
  }

  // Try to connect to the database
  try {
    auto c = db->GetScopedConnection();

    // Convert to sqlpp11 query
    QwParitySchema::seeds seeds{};
    if (fSeedID > 0) {
      // Use specified seed
      auto query = sqlpp::select(sqlpp::all_of(seeds))
                   .from(seeds)
                   .where(seeds.seed_id == seed_id);
      auto results = db->QuerySelect(query);

      // Process results using database-agnostic interface
      UInt_t found_seed_id = 0;
      TString found_seed = "";
      size_t result_count = db->CountResults(results);

      db->ForFirstResult(results, [&](const auto& row) {
        found_seed_id = row.seed_id;
        if (!is_null(row.seed)) {
          found_seed = row.seed.value();
        } else {
          QwError << "QwBlinder::ReadSeed(): Seed value came back NULL from the database." << QwLog::endl;
          found_seed_id = 0;
          found_seed = kDefaultSeed;
        }
      });

      // Process result for specified seed
      if (result_count == 1) {
        fSeedID = found_seed_id;
        fSeed = found_seed;
        std::cout << "QwBlinder::ReadSeed():  Successfully read "
        << Form("the fSeed with ID %d from the database.", fSeedID)
        << std::endl;
      } else {
        fSeedID = 0;
        fSeed = Form("ERROR:  There should be one and only one seed_id for each seed, but this had %zu.",
                     result_count);
        std::cerr << "QwBlinder::ReadSeed(): " << fSeed << std::endl;
      }
    } else {
      // Use most recent seed
      auto query = sqlpp::select(sqlpp::all_of(seeds))
                   .from(seeds)
                   .order_by(seeds.seed_id.desc())
                   .limit(1u)
                   .where(sqlpp::value(true));
      auto results = db->QuerySelect(query);

      // Process results using database-agnostic interface
      UInt_t found_seed_id2 = 0;
      TString found_seed2 = "";

      size_t result_count2 = db->CountResults(results);

      db->ForFirstResult(results, [&](const auto& row) {
        found_seed_id2 = row.seed_id;
        if (!is_null(row.seed)) {
          found_seed2 = row.seed.value();
        } else {
          QwError << "QwBlinder::ReadSeed(): Seed value came back NULL from the database." << QwLog::endl;
          found_seed_id2 = 0;
          found_seed2 = kDefaultSeed;
        }
      });

      // Process result for most recent seed
      if (result_count2 == 1) {
        fSeedID = found_seed_id2;
        fSeed = found_seed2;
        std::cout << "QwBlinder::ReadSeed():  Successfully read "
        << Form("the fSeed with ID %d from the database.", fSeedID)
        << std::endl;
      } else {
        fSeedID = 0;
        fSeed = Form("ERROR:  There should be one and only one seed_id for each seed, but this had %zu.",
                     result_count2);
        std::cerr << "QwBlinder::ReadSeed(): " << fSeed << std::endl;
      }
    }
  } catch (const std::exception& er) {

    //  We were unable to open the connection.
    fSeedID = 0;
    fSeed   = "ERROR:  Unable to open the connection to the database.";
    QwError << "QwBlinder::ReadSeed(): Unable to open connection to database: " << er.what() << QwLog::endl;
  }

  return fSeedID;
}
#endif // __USE_DATABASE__


/**
 * Initialize the blinder parameters
 */
void QwBlinder::InitBlinders(const UInt_t seed_id)
{
  // If the blinding strategy is disabled
  if (fBlindingStrategy == kDisabled) {

      fSeed = kDefaultSeed;
      fSeedID = 0;
      fBlindingFactor = 1.0;
      fBlindingOffset = 0.0;
      fBlindingOffset_Base = 0.0;
      QwWarning << "Blinding parameters have been disabled!"<< QwLog::endl;

  // Else blinding is enabled
  } else {

    Int_t finalseed = UseMD5(fSeed);

    Double_t newtempout;
    if ((finalseed & 0x80000000) == 0x80000000) {
      newtempout = -1.0 * (finalseed & 0x7FFFFFFF);
    } else {
      newtempout =  1.0 * (finalseed & 0x7FFFFFFF);
    }


    /// The blinding constants are determined in two steps.
    ///
    /// First, the blinding asymmetry (offset) is determined.  It is
    /// generated from a signed number between +/- 0.244948974 that
    /// is squared to get a number between +/- 0.06 ppm.
    static Double_t maximum_asymmetry_sqrt = sqrt(fMaximumBlindingAsymmetry);
    Double_t tmp1 = maximum_asymmetry_sqrt * (newtempout / Int_t(0x7FFFFFFF));
    fBlindingOffset = tmp1 * fabs(tmp1) * 0.000001;

    //  Do another little calculation to round off the blinding asymmetry
    Double_t tmp2;
    tmp1 = fBlindingOffset * 4;    // Exactly shifts by two binary places
    tmp2 = tmp1 + fBlindingOffset; // Rounds 5*fBlindingOffset
    fBlindingOffset = tmp2 - tmp1; // fBlindingOffset has been rounded.

    //  Set the base blinding offset.
    fBlindingOffset_Base = fBlindingOffset;

    /// Secondly, the multiplicative blinding factor is determined.  This
    /// number is generated from the blinding asymmetry between, say, 0.9 and 1.1
    /// by an oscillating but uniformly distributed sawtooth function.
    fBlindingFactor = 1.0;
    if (fMaximumBlindingAsymmetry > 0.0) {
      /// TODO:  This section of InitBlinders doesn't calculate a reasonable fBlindingFactor but we don't use it for anything.
      fBlindingFactor  = 1.0 + fmod(30.0 * fBlindingOffset, fMaximumBlindingAsymmetry);
      fBlindingFactor /= (fMaximumBlindingAsymmetry > 0.0 ? fMaximumBlindingAsymmetry : 1.0);
    }

    QwMessage << "Blinding parameters have been calculated."<< QwLog::endl;

  }

  // Generate checksum
  TString hex_string;
  hex_string.Form("%.16llx%.16llx", *(ULong64_t*)(&fBlindingFactor), *(ULong64_t*)(&fBlindingOffset));
  fDigest = GenerateDigest(hex_string);
  fChecksum = "";
  for (size_t i = 0; i < fDigest.size(); i++)
    fChecksum += string(Form("%.2x",fDigest[i]));
}

#ifdef __USE_DATABASE__
void  QwBlinder::WriteFinalValuesToDB(QwParityDB* db)
{
  WriteChecksum(db);
  if (! CheckTestValues()) {
    QwError << "QwBlinder::WriteFinalValuesToDB():  "
            << "Blinded test values have changed; may be a problem in the analysis!!!"
            << QwLog::endl;
  }
  WriteTestValues(db);
}
#endif // __USE_DATABASE__


/**
 * Generate a set of test values of similar size as measured asymmetries
 * @param n Number of test values
 */
void QwBlinder::InitTestValues(const int n)
{
  // Use the stored seed to get a pseudorandom number
  Int_t finalseed = UsePseudorandom(fSeed);

  fTestValues.clear();
  fBlindTestValues.clear();
  fUnBlindTestValues.clear();

  Double_t tmp_offset = fBlindingOffset;
  fBlindingOffset = fBlindingOffset_Base;
  // For each test case
  for (int i = 0; i < n; i++) {

    // Generate a pseudorandom number
    for (Int_t j = 0; j < 16; j++) {
      finalseed &= 0x7FFFFFFF;
      if ((finalseed & 0x800000) == 0x800000) {
        finalseed = ((finalseed ^ 0x00000d) << 1) | 0x1;
      } else {
        finalseed <<= 1;
      }
    }

    // Mask out the low digits of the finalseed, multiply by two,
    // divide by the mask value, subtract from 1, and divide result by
    // 1.0e6 to get a range of about -1000 to +1000 ppb.
    Int_t mask = 0xFFFFFF;
    Double_t tempval = (1.0 - 2.0*(finalseed&mask)/mask) / (1.0e6);

    // Store the test values
    fTestValues.push_back(tempval);
    BlindValue(tempval);
    fBlindTestValues.push_back(tempval);
    UnBlindValue(tempval);
    fUnBlindTestValues.push_back(tempval);
  }
  fBlindingOffset = tmp_offset;
  QwMessage << "QwBlinder::InitTestValues(): A total of " << fTestValues.size()
            << " test values have been calculated successfully." << QwLog::endl;
}

/**
 * Use string manipulation to get a number from the seed string
 * @param barestring Seed string
 * @return Integer number
 */
Int_t QwBlinder::UseStringManip(const TString& barestring)
{
  std::vector<UInt_t> choppedwords;
  UInt_t tmpword = 0;
  Int_t finalseed = 0;

  for (Int_t i = 0; i < barestring.Length(); i++)
    {
      if (i % 4 == 0) tmpword = 0;
      tmpword |= (char(barestring[i]))<<(24-8*(i%4));
      if (i%4 == 3 || i == barestring.Length()-1)
        {
          choppedwords.push_back(tmpword);
          finalseed ^= (tmpword);
        }
    }
  for (Int_t i=0; i<64; i++)
    {
      finalseed &= 0x7FFFFFFF;
      if ((finalseed & 0x800000) == 0x800000)
        {
          finalseed = ((finalseed^0x00000d)<<1) | 0x1;
        }
      else
        {
          finalseed<<=1;
        }
    }
  if ((finalseed&0x80000000) == 0x80000000)
    {
      finalseed = -1 * (finalseed&0x7FFFFFFF);
    }
  else
    {
      finalseed = (finalseed&0x7FFFFFFF);
    }
  return finalseed;
}


/**
 * Use pseudo-random number generator to get a number from the seed string
 * @param barestring Seed string
 * @return Integer number
 */
Int_t QwBlinder::UsePseudorandom(const TString& barestring)
{
  ULong64_t finalseed;
  Int_t bitcount;
  Int_t tempout = 0;

  //  This is an attempt to build a portable 64-bit constant
  ULong64_t longmask = (0x7FFFFFFF);
  longmask<<=32;
  longmask|=0xFFFFFFFF;

  finalseed = 0;
  bitcount  = 0;
  for (Int_t i=0; i<barestring.Length(); i++)
    {
      if ( ((barestring[i])&0xC0)!=0 && ((barestring[i])&0xC0)!=0xC0)
        {
          finalseed = ((finalseed&longmask)<<1) | (((barestring[i])&0x40)>>6);
          bitcount++;
        }
      if ( ((barestring[i])&0x30)!=0 && ((barestring[i])&0x30)!=0x30)
        {
          finalseed = ((finalseed&longmask)<<1) | (((barestring[i])&0x10)>>4);
          bitcount++;
        }
      if ( ((barestring[i])&0xC)!=0 && ((barestring[i])&0xC)!=0xC)
        {
          finalseed = ((finalseed&longmask)<<1) | (((barestring[i])&0x4)>>2);
          bitcount++;
        }
      if ( ((barestring[i])&0x3)!=0 && ((barestring[i])&0x3)!=0x3)
        {
          finalseed = ((finalseed&longmask)<<1) | ((barestring[i])&0x1);
          bitcount++;
        }
    }
  for (Int_t i=0; i<(192-bitcount); i++)
    {
      if ((finalseed & 0x800000) == 0x800000)
        {
          finalseed = ((finalseed^0x00000d)<<1) | 0x1;
        }
      else
        {
          finalseed<<=1;
        }
    }
  tempout = (finalseed&0xFFFFFFFF) ^ ((finalseed>>32)&0xFFFFFFFF);
  if ((tempout&0x80000000) == 0x80000000)
    {
      tempout = -1 * (tempout&0x7FFFFFFF);
    }
  else
    {
      tempout =  1 * (tempout&0x7FFFFFFF);
    }
  return tempout;
}


/**
 * Use an MD5 checksum to get a number from the seed string
 * @param barestring Seed string
 * @return Integer number
 */
Int_t QwBlinder::UseMD5(const TString& barestring)
{
  Int_t temp = 0;
  Int_t tempout = 0;

  std::vector<UChar_t> digest = GenerateDigest(barestring);
  for (size_t i = 0; i < digest.size(); i++)
    {
      Int_t j = i%4;
      if (j == 0)
        {
          temp = 0;
        }
      temp |= (digest[i])<<(24-(j*8));
      if ( (j==3) || (i==(digest.size()-1)))
        {
          tempout ^= temp;
        }
    }

  if ((tempout & 0x80000000) == 0x80000000) {
      tempout = -1 * (tempout & 0x7FFFFFFF);
  } else {
      tempout = (tempout & 0x7FFFFFFF);
  }

  return tempout;
}


/*!-----------------------------------------------------------
 *------------------------------------------------------------
 * Function to write the checksum into the analysis table
 *
 * Parameters: void
 *
 * Return: void
 *
 * Note:  This function assumes that the analysis table has already
 *        been filled for the run.
 *------------------------------------------------------------
 *------------------------------------------------------------*/
#ifdef __USE_DATABASE__
void QwBlinder::WriteChecksum(QwParityDB* db)
{
  //----------------------------------------------------------
  // Construct and execute sqlpp11 UPDATE query
  //----------------------------------------------------------
  QwParitySchema::analysis analysis{};
  auto update_query = sqlpp::update(analysis)
                      .set(analysis.seed_id = fSeedID,
                           analysis.bf_checksum = fChecksum)
                      .where(analysis.analysis_id == db->GetAnalysisID());
  //----------------------------------------------------------
  // Execute SQL
  //----------------------------------------------------------
  auto c = db->GetScopedConnection();
  db->QueryExecute(update_query);
} //End QwBlinder::WriteChecksum

/*!------------------------------------------------------------
 *------------------------------------------------------------
 * Function to write the test values into the database
 *
 * Parameters: void
 *
 * Return: void
 *------------------------------------------------------------
 *------------------------------------------------------------*/
void QwBlinder::WriteTestValues(QwParityDB* db)
{
  //----------------------------------------------------------
  // Use sqlpp11 INSERT for bf_test table
  //----------------------------------------------------------
  QwParitySchema::bf_test bf_test{};

  //----------------------------------------------------------
  // Insert test values using sqlpp11
  //----------------------------------------------------------
  // Loop over all test values
  for (size_t i = 0; i < fTestValues.size(); i++)
    {
      auto insert_query = sqlpp::insert_into(bf_test)
                          .set(bf_test.analysis_id = db->GetAnalysisID(),
                               bf_test.test_number = static_cast<int>(i),
                               bf_test.test_value = fBlindTestValues[i]);

      // Execute SQL
      auto c = db->GetScopedConnection();
      db->QueryExecute(insert_query);
    }
}
#endif // __USE_DATABASE__

/*!--------------------------------------------------------------
 *  This routines checks to see if the stored fBlindTestValues[i]
 *  match a recomputed blinded test value.  The values are cast
 *  into floats, and their difference must be less than a change
 *  of the least-significant-bit of fBlindTestValues[i].
 *--------------------------------------------------------------*/

Bool_t QwBlinder::CheckTestValues()
{
  Bool_t status = kTRUE;

  Double_t tmp_offset = fBlindingOffset;
  fBlindingOffset = fBlindingOffset_Base;
  double epsilon = std::numeric_limits<double>::epsilon();
  for (size_t i = 0; i < fTestValues.size(); i++) {

    /// First test: compare a blinded value with a second computation
    double checkval = fBlindTestValues[i];
    UnBlindValue(checkval);

    double test1 = fTestValues[i];
    double test2 = checkval;
    if ((test1 - test2) <= -epsilon || (test1 - test2) >= epsilon) {
      QwError << "QwBlinder::CheckTestValues():  Unblinded test value "
              << i
              << " does not agree with original test value, "
              << "with a difference of "
              << (test1 - test2)
	      << " (epsilon==" << epsilon << ")"
	      << "." << QwLog::endl;
      status = kFALSE;
    }

    /// Second test: compare the unblinded value with the original value
    test1 = fUnBlindTestValues[i];
    test2 = fTestValues[i];
    if ((test1 - test2) <= -epsilon || (test1 - test2) >= epsilon) {
      QwError << "QwBlinder::CheckTestValues():  Unblinded test value "
              << i
              << " does not agree with original test value, "
              << "with a difference of "
              << (test1 - test2) << "." << QwLog::endl;
      status = kFALSE;
    }
  }
  fBlindingOffset = tmp_offset;
  return status;
}


/**
 * Generate an MD5 digest of the blinding parameters
 * @param input Input string
 * @return MD5 digest of the input string
 */
std::vector<UChar_t> QwBlinder::GenerateDigest(const TString& input) const
{
  // Initialize MD5 checksum array
  const UInt_t length = 16;
  UChar_t value[length];
  for (UInt_t i = 0; i < length; i++)
    value[i] = 0;

  // Calculate MD5 checksum from input and store in md5_value
  TMD5 md5;
  md5.Update((UChar_t*) input.Data(), input.Length());
  md5.Final(value);

  // Copy the MD5 checksum in an STL vector
  std::vector<UChar_t> output;
  for (UInt_t i = 0; i < length; i++)
    output.push_back(value[i]);

  return output;
}


/**
 * Print a summary of the blinding/unblinding test
 */
void QwBlinder::PrintFinalValues(Int_t kVerbosity)
{
  Int_t total_count = 0;
  for (size_t i=0; i<kBlinderCount_NumCounters; i++){
    total_count += fPatternCounters.at(i);
  }
  if (total_count<=0) return;

  QwMessage << "QwBlinder::PrintFinalValues():  Begin summary"    << QwLog::endl;
  QwMessage << "================================================" << QwLog::endl;
  PrintCountersValues(fPatternCounters, "Patterns");
  if(kVerbosity==1){
    QwMessage << "================================================" << QwLog::endl;
    PrintCountersValues(fPairCounters, "Pairs");
  }
  QwMessage << "================================================" << QwLog::endl;
  QwMessage << "The blinding parameters checksum for seed ID "
            << fSeedID << " is:" << QwLog::endl;
  QwMessage << fChecksum << QwLog::endl;
  QwMessage << "================================================" << QwLog::endl;
  CheckTestValues();
  double epsilon = std::numeric_limits<double>::epsilon();
  TString diff;
  QwMessage << "The test results are:" << QwLog::endl;
  QwMessage << std::setw(8)  << "Index"
            << std::setw(16) << "Original value"
            << std::setw(16) << "Blinded value"
            << std::setw(22) << "Orig.-Unblind value"
            << QwLog::endl;
  for (size_t i = 0; i < fTestValues.size(); i++) {
    if ((fTestValues[i]-fUnBlindTestValues[i]) < -epsilon
	|| (fTestValues[i]-fUnBlindTestValues[i]) > epsilon )
      diff = Form("% 9g ppb", fTestValues[i]-fUnBlindTestValues[i]*1e9);
    else
      diff = "epsilon";
    QwMessage << std::setw(8)  << i
              << std::setw(16) << Form(" [CENSORED]")
	      << std::setw(16) << Form("% 9.3f ppb",fBlindTestValues[i]*1e9)
              << std::setw(22) << diff
              << QwLog::endl;
  }
  QwMessage << "================================================" << QwLog::endl;
  QwMessage << "QwBlinder::PrintFinalValues():  End of summary"   << QwLog::endl;
}

void QwBlinder::PrintCountersValues(std::vector<Int_t> fCounters, TString counter_type)
{
  QwMessage << "Blinder Passed " << counter_type  << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " with blinding disabled: " << fCounters.at(kBlinderCount_Disabled) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " on a non-blindable target: " << fCounters.at(kBlinderCount_NonBlindable) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " with transverse beam: " << fCounters.at(kBlinderCount_Transverse) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " on blindable target with beam present: " << fCounters.at(kBlinderCount_Blindable) << QwLog::endl;
  QwMessage << "Blinder Failed " << counter_type  << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " with unknown target position: " << fCounters.at(kBlinderCount_UnknownTarget) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " with changed target position: " << fCounters.at(kBlinderCount_ChangedTarget) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " with an undefined Wien setting: " << fCounters.at(kBlinderCount_UndefinedWien) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " with a changed Wien setting: " << fCounters.at(kBlinderCount_ChangedWien) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " with an undefined IHWP setting: " << fCounters.at(kBlinderCount_UndefinedIHWP) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " with a changed IHWP setting: " << fCounters.at(kBlinderCount_ChangedIHWP) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " on blindable target with no beam: " << fCounters.at(kBlinderCount_NoBeam) << QwLog::endl;
  QwMessage << "\t" << counter_type
	    << " with other blinding failure: " << fCounters.at(kBlinderCount_OtherFailure) << QwLog::endl;

}


/**
 * Write the blinding parameters to the database
 *
 * For each analyzed run the database contains a digest of the blinding parameters
 * and a number of blinded test entries.
 */
#ifdef __USE_DATABASE__
void QwBlinder::FillDB(QwParityDB *db, TString datatype)
{
  QwDebug << " --------------------------------------------------------------- " << QwLog::endl;
  QwDebug << "                         QwBlinder::FillDB                       " << QwLog::endl;
  QwDebug << " --------------------------------------------------------------- " << QwLog::endl;

  // Get the analysis ID
  UInt_t analysis_id = db->GetAnalysisID();

  // Fill the test values for database insertion
  if (! CheckTestValues()) {
    QwError << "QwBlinder::FillDB():  "
            << "Blinded test values have changed; "
	    << "may be a problem in the analysis!!!"
            << QwLog::endl;
  }


  // Connect to the database
  auto c = db->GetScopedConnection();

  // Modify the seed_id and bf_checksum in the analysis table
  try {
    QwParitySchema::analysis analysis{};

    auto update_query = sqlpp::update(analysis)
                        .set(analysis.seed_id = fSeedID,
                             analysis.bf_checksum = fChecksum)
                        .where(analysis.analysis_id == analysis_id);

    QwDebug << "Updating analysis table with blinder information" << QwLog::endl;
    db->QueryExecute(update_query);

  } catch (const std::exception& err) {
    QwError << "Failed to update analysis table: " << err.what() << QwLog::endl;
  }

  // Add the bf_test rows
  try {
    if (fTestValues.size() > 0) {
      QwParitySchema::bf_test bf_test{};
      for (size_t i = 0; i < fTestValues.size(); i++) {
        auto insert_query = sqlpp::insert_into(bf_test)
                            .set(bf_test.analysis_id = analysis_id,
                                 bf_test.test_number = static_cast<int>(i),
                                 bf_test.test_value = fBlindTestValues[i]);

        db->QueryExecute(insert_query);
      }
      QwDebug << "Inserted " << fTestValues.size() << " bf_test entries" << QwLog::endl;
    } else {
      QwMessage << "QwBlinder::FillDB(): No bf_test entries to write."
		<< QwLog::endl;
    }
  } catch (const std::exception& err) {
    QwError << "Failed to insert bf_test entries: " << err.what() << QwLog::endl;
  }
}

void QwBlinder::FillErrDB(QwParityDB *db, TString datatype)
{
  QwDebug << " --------------------------------------------------------------- " << QwLog::endl;
  QwDebug << "                     QwBlinder::FillErrDB                        " << QwLog::endl;
  QwDebug << " --------------------------------------------------------------- " << QwLog::endl;

  UInt_t analysis_id = db->GetAnalysisID();
  QwParitySchema::general_errors general_errors{};

  auto c = db->GetScopedConnection();

  try {
    // Insert error counter entries for each blinder counter type
    for (size_t index = 0; index < kBlinderCount_NumCounters; index++) {
      if (fPatternCounters.at(index) > 0) {  // Only insert non-zero counters
        auto insert_query = sqlpp::insert_into(general_errors)
                            .set(general_errors.analysis_id = analysis_id,
                                 general_errors.error_code_id = index + 20,  // error codes 20+
                                 general_errors.n = fPatternCounters.at(index));

        db->QueryExecute(insert_query);
      }
    }
    QwDebug << "Inserted blinder error counters for analysis " << analysis_id << QwLog::endl;

  } catch (const std::exception& err) {
    QwError << "Failed to insert blinder error counters: " << err.what() << QwLog::endl;
  }
};
#endif // __USE_DATABASE__


void QwBlinder::SetTargetBlindability(QwBlinder::EQwBlinderStatus status)
{
  fTargetBlindability = status;
  if (fTargetBlindability_firstread == QwBlinder::kIndeterminate
      && fTargetBlindability != QwBlinder::kIndeterminate){
    fTargetBlindability_firstread = fTargetBlindability;
    QwMessage << "QwBlinder:  First set target blindability to "
	      << fStatusName[fTargetBlindability] << QwLog::endl;
  }
}

void QwBlinder::SetWienState(EQwWienMode wienmode)
{
  fWienMode = wienmode;
  if (fWienMode_firstread == kWienIndeterminate
      && fWienMode != kWienIndeterminate){
    fWienMode_firstread = fWienMode;
    QwMessage << "QwBlinder:  First set Wien state to "
	      << WienModeName(fWienMode) << QwLog::endl;
  }
}

void QwBlinder::SetIHWPPolarity(Int_t ihwppolarity)
{
  fIHWPPolarity = ihwppolarity;
  if (fIHWPPolarity_firstread == 0 && fIHWPPolarity != 0){
    fIHWPPolarity_firstread = fIHWPPolarity;
    QwMessage << "QwBlinder:  First set IHWP state to "
	      << fIHWPPolarity << QwLog::endl;
  }
}


QwBlinder::EQwBlinderStatus QwBlinder::CheckBlindability(std::vector<Int_t> &fCounters)
{
  EQwBlinderStatus status = QwBlinder::kBlindableFail;
  if (fBlindingStrategy == kDisabled) {
    status = QwBlinder::kNotBlindable;
    fCounters.at(kBlinderCount_Disabled)++;
  } else if (fTargetBlindability == QwBlinder::kIndeterminate) {
    QwDebug  << "QwBlinder::CheckBlindability:  The target blindability is not determined.  "
	     << "Fail this pattern." << QwLog::endl;
    status = QwBlinder::kBlindableFail;
    fCounters.at(kBlinderCount_UnknownTarget)++;
  } else if (fTargetBlindability!=fTargetBlindability_firstread
	     && !fTargetPositionForced) {
    QwDebug << "QwBlinder::CheckBlindability:  The target blindability has changed.  "
	    << "Fail this pattern." << QwLog::endl;
    status = QwBlinder::kBlindableFail;
    fCounters.at(kBlinderCount_ChangedTarget)++;
  } else if (fTargetBlindability==kNotBlindable) {
    //  This isn't a blindable target, so don't do anything.
    status = QwBlinder::kNotBlindable;
    fCounters.at(kBlinderCount_NonBlindable)++;
  } else if (fTargetBlindability==kBlindable &&
	     fWienMode != fWienMode_firstread) {
    //  Wien status changed.  Fail
    status = QwBlinder::kBlindableFail;
    fCounters.at(kBlinderCount_ChangedWien)++;
  } else if (fTargetBlindability==kBlindable &&
	     fIHWPPolarity != fIHWPPolarity_firstread) {
    //  IHWP status changed.  Fail
    status = QwBlinder::kBlindableFail;
    fCounters.at(kBlinderCount_ChangedIHWP)++;
  } else if (fTargetBlindability==kBlindable &&
	     fWienMode == kWienIndeterminate) {
    //  Wien status isn't determined.  Fail.
    status = QwBlinder::kBlindableFail;
    fCounters.at(kBlinderCount_UndefinedWien)++;
  } else if (fTargetBlindability==kBlindable &&
	     fIHWPPolarity==0) {
    //  IHWP status isn't determined.  Fail.
    status = QwBlinder::kBlindableFail;
    fCounters.at(kBlinderCount_UndefinedIHWP)++;
  } else if (fTargetBlindability==kBlindable &&
	    (fWienMode == kWienVertTrans || fWienMode == kWienHorizTrans)) {
    //  We don't have longitudinal beam, so don't blind.
    status = QwBlinder::kNotBlindable;
    fCounters.at(kBlinderCount_Transverse)++;
  } else if (fTargetBlindability==kBlindable
	     && fBeamIsPresent) {
    //  This is a blindable target and the beam is sufficient.
    status = QwBlinder::kBlindable;
    fCounters.at(kBlinderCount_Blindable)++;
  } else if (fTargetBlindability==kBlindable
	     && (! fBeamIsPresent) ) {
    //  This is a blindable target but there is insufficient beam present
    status = QwBlinder::kNotBlindable;
    fCounters.at(kBlinderCount_NoBeam)++;
  } else {
    QwError << "QwBlinder::CheckBlindability:  The pattern blindability is unclear.  "
	     << "Fail this pattern." << QwLog::endl;
    status = QwBlinder::kBlindableFail;
    fCounters.at(kBlinderCount_OtherFailure)++;
  }
  //
  fBlinderIsOkay = (status != QwBlinder::kBlindableFail);

  return status;
}
