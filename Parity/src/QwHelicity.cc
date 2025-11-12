/**********************************************************\
* File: QwHelicity.C                                      *
*                                                         *
* Contributor: Arindam Sen (asen@jlab.org)                *
* Time-stamp:  November, 2025                             *
\**********************************************************/

#include "QwHelicity.h"

// System headers
#include <stdexcept>

// ROOT headers
#include "TRegexp.h"
#include "TMath.h"

// Qweak headers
#include "QwRootFile.h"

// Qweak headers
#include "QwHistogramHelper.h"
#include "QwLog.h"

extern QwHistogramHelper gQwHists;

//**************************************************//
/// Default helicity bit pattern of 0x69 represents a -++-+--+ octet
/// (event polarity listed in reverse time order), where the LSB
/// of the bit pattern is the first event of the pattern.
//**************************************************//

/// Constructor with name
QwHelicity::QwHelicity(const TString& name)
: VQwSubsystem(name),
  QwHelicityBase(name),
  fInputReg_HelPlus(kDefaultInputReg_HelPlus),
  fInputReg_HelMinus(kDefaultInputReg_HelMinus),
  fInputReg_PatternSync(kDefaultInputReg_PatternSync),
  fInputReg_PairSync(0)
{

  fInputReg_FakeMPS = kDefaultInputReg_FakeMPS;
}

//**************************************************//
QwHelicity::QwHelicity(const QwHelicity& source)
: VQwSubsystem(source.GetName()),
  QwHelicityBase(source.GetName()),
  fInputReg_HelPlus(source.fInputReg_HelPlus),
  fInputReg_HelMinus(source.fInputReg_HelMinus),
  fInputReg_PatternSync(source.fInputReg_PatternSync),
  fInputReg_PairSync(source.fInputReg_PairSync)
{
  fHelicityBitPattern = source.fHelicityBitPattern; 
  ClearErrorCounters();
  // Default helicity delay to two patterns.
  fHelicityDelay = 2;
  // Default the EventType flags to HelPlus=1 and HelMinus=4
  // These are only used in Moller decoding mode.
  kEventTypeHelPlus  = 4;
  kEventTypeHelMinus = 1;
  //
  fEventNumberOld=-1; fEventNumber=-1;
  fPatternPhaseNumberOld=-1; fPatternPhaseNumber=-1;
  fPatternNumberOld=-1;  fPatternNumber=-1;
  kUserbit=-1;
  fActualPatternPolarity=kUndefinedHelicity;
  fDelayedPatternPolarity=kUndefinedHelicity;
  fHelicityReported=kUndefinedHelicity;
  fHelicityActual=kUndefinedHelicity;
  fHelicityDelayed=kUndefinedHelicity;
  fHelicityBitPlus=kFALSE;
  fHelicityBitMinus=kFALSE;
  fGoodHelicity=kFALSE;
  fGoodPattern=kFALSE;
  fHelicityDecodingMode=-1;

  this->fWord.resize(source.fWord.size());
  for(size_t i=0;i<this->fWord.size();i++)
    {
      this->fWord[i].fWordName=source.fWord[i].fWordName;
      this->fWord[i].fModuleType=source.fWord[i].fModuleType;
      this->fWord[i].fWordType=source.fWord[i].fWordType;
    }
  fNumMissedGates = source.fNumMissedGates;
  fNumMissedEventBlocks = source.fNumMissedEventBlocks;
  fNumMultSyncErrors = source.fNumMultSyncErrors;
  fNumHelicityErrors = source.fNumHelicityErrors;
  fEventNumberFirst = source.fEventNumberFirst;
  fPatternNumberFirst = source.fPatternNumberFirst;
  fEventType = source.fEventType;
  fIgnoreHelicity = source.fIgnoreHelicity;
  fRandBits = source.fRandBits;
  fUsePredictor = source.fUsePredictor;
  fHelicityInfoOK = source.fHelicityInfoOK;
  fPatternPhaseOffset = source.fPatternPhaseOffset;
  fMinPatternPhase = source.fMinPatternPhase;
  fMaxPatternPhase = source.fMaxPatternPhase;
  fHelicityDelay = source.fHelicityDelay;
  iseed_Delayed = source.iseed_Delayed;
  iseed_Actual = source.iseed_Actual;
  n_ranbits = source.n_ranbits;
  fEventNumber = source.fEventNumber;
  fEventNumberOld = source.fEventNumberOld;
  fPatternPhaseNumber = source.fPatternPhaseNumber;
  fPatternPhaseNumberOld = source.fPatternPhaseNumberOld;
  fPatternNumber = source.fPatternNumber;
  fPatternNumberOld = source.fPatternNumberOld;

  this->kUserbit = source.kUserbit;
  this->fIgnoreHelicity = source.fIgnoreHelicity;
}

//**************************************************//
void QwHelicity::DefineOptions(QwOptions &options)
{
  options.AddOptions("Helicity options")
      ("helicity.seed", po::value<int>(),
          "Number of bits in random seed");
  options.AddOptions("Helicity options")
      ("helicity.bitpattern", po::value<std::string>(),
          "Helicity bit pattern: 0x1 (pair), 0x9 (quartet), 0x69 (octet), 0x666999 (hexo-quad), 0x66669999 (octo-quad)");
  options.AddOptions("Helicity options")
      ("helicity.patternoffset", po::value<int>(),
          "Set 1 when pattern starts with 1 or 0 when starts with 0");
  options.AddOptions("Helicity options")
      ("helicity.patternphase", po::value<int>(),
          "Maximum pattern phase");
  options.AddOptions("Helicity options")
      ("helicity.delay", po::value<int>(),
          "Default delay is 2 patterns, set at the helicity map file.");
  options.AddOptions("Helicity options")
      ("helicity.toggle-mode", po::value<bool>()->default_bool_value(false),
          "Activates helicity toggle-mode, overriding the 'delay', 'patternphase', 'bitpattern', and 'seed' options.");
}

//**************************************************//

void QwHelicity::ProcessOptions(QwOptions &options)
{
  // Read the cmd options and override channel map settings
  QwMessage << "QwHelicity::ProcessOptions" << QwLog::endl;
  if (options.HasValue("helicity.patternoffset")) {
    if (options.GetValue<int>("helicity.patternoffset") == 1
     || options.GetValue<int>("helicity.patternoffset") == 0) {
      fPatternPhaseOffset = options.GetValue<int>("helicity.patternoffset");
      QwMessage << " Pattern Phase Offset = " << fPatternPhaseOffset << QwLog::endl;
    } else QwError << "Pattern phase offset should be 0 or 1!" << QwLog::endl;
  }

  if (options.HasValue("helicity.patternphase")) {
    if (options.GetValue<int>("helicity.patternphase") % 2 == 0) {
      fMaxPatternPhase = options.GetValue<int>("helicity.patternphase");
      QwMessage << " Maximum Pattern Phase = " << fMaxPatternPhase << QwLog::endl;
    } else QwError << "Pattern phase should be an even integer!" << QwLog::endl;
  }

  if (options.HasValue("helicity.seed")) {
    if (options.GetValue<int>("helicity.seed") == 24
     || options.GetValue<int>("helicity.seed") == 30) {
      QwMessage << " Random Bits = " << options.GetValue<int>("helicity.seed") << QwLog::endl;
      fRandBits = options.GetValue<int>("helicity.seed");
    } else QwError << "Number of random seed bits should be 24 or 30!" << QwLog::endl;
  }

  if (options.HasValue("helicity.delay")) {
    QwMessage << " Helicity Delay = " << options.GetValue<int>("helicity.delay") << QwLog::endl;
    SetHelicityDelay(options.GetValue<int>("helicity.delay"));
  }

  if (options.HasValue("helicity.bitpattern")) {
    QwMessage << " Helicity Pattern ="
	      << options.GetValue<std::string>("helicity.bitpattern")
	      << QwLog::endl;
    std::string hex = options.GetValue<std::string>("helicity.bitpattern");
    SetHelicityBitPattern(hex);
  } else {
  }

  if (options.GetValue<bool>("helicity.toggle-mode")) {
    fHelicityDelay   = 0;
    fUsePredictor    = kFALSE;
    fMaxPatternPhase = 2;
    fHelicityBitPattern = kDefaultHelicityBitPattern;
  }

  //  If we have the default Helicity Bit Pattern & a large fMaxPatternPhase,
  //  try to recompute the Helicity Bit Pattern.
  if (fMaxPatternPhase > 8 && fHelicityBitPattern == kDefaultHelicityBitPattern) {
  }

  //  Here we're going to try to get the "online" option which
  //  is defined by QwEventBuffer.
  if (options.HasValue("online")){
    fSuppressMPSErrorMsgs = options.GetValue<bool>("online");
  } else {
    fSuppressMPSErrorMsgs = kFALSE;
  }
}


void QwHelicity::ClearEventData()
{
  SetDataLoaded(kFALSE);
  for (size_t i=0;i<fWord.size();i++)
    fWord[i].ClearEventData();

  /**Reset data by setting the old event number, pattern number and pattern phase
     to the values of the previous event.*/
  if (fEventNumberFirst==-1 && fEventNumberOld!= -1){
    fEventNumberFirst = fEventNumberOld;
  }
  if (fPatternNumberFirst==-1 && fPatternNumberOld!=-1
      && fPatternNumber==fPatternNumberOld+1){
    fPatternNumberFirst = fPatternNumberOld;
  }

  fEventNumberOld = fEventNumber;
  fPatternNumberOld = fPatternNumber;
  fPatternPhaseNumberOld = fPatternPhaseNumber;

  //fIgnoreHelicity = kFALSE;

  /**Clear out helicity variables */
  fHelicityReported = kUndefinedHelicity;
  fHelicityActual = kUndefinedHelicity;
  fHelicityDelayed= kUndefinedHelicity;
  fHelicityBitPlus = kFALSE;
  fHelicityBitMinus = kFALSE;
  // be careful: it is not that I forgot to reset fActualPatternPolarity
  // or fDelayedPatternPolarity. One doesn't want to do that here.
  /** Set the new event number and pattern number to -1. If we are not reading these correctly
      from the data stream, -1 will allow us to identify that.*/
  fEventNumber = -1;
  fPatternPhaseNumber = -1;
  fPatternNumber = -1;
  return;
}

Int_t QwHelicity::ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words)
{
  //stub function
  // QwError << " this function QwHelicity::ProcessConfigurationBuffer does nothing yet " << QwLog::endl;
  return 0;
}

Int_t QwHelicity::LoadInputParameters(TString pedestalfile)
{
  return 0;
}


Bool_t QwHelicity::ApplySingleEventCuts(){
  //impose single event cuts //Paul's modifications

  return kTRUE;
}

/*!
 * \brief Process helicity information from userbit configuration data.
 *
 * This is a complex function (~80 lines) that extracts helicity information
 * from userbit data for injector tests and special configurations. It handles:
 *
 * Userbit Decoding:
 * - Extracts 3-bit userbit pattern from bits 28-30 of userbit word
 * - Decodes quartet synchronization bit (bit 3) for pattern timing
 * - Decodes helicity bit (bit 2) for spin state determination
 * - Manages scaler offset calculations for event counting
 *
 * Event Counting Logic:
 * - Increments event numbers based on scaler counter ratios
 * - Handles missed events when scaler offset > 1 (indicates DAQ issues)
 * - Maintains pattern phase and pattern number synchronization
 * - Resets quartet phase on quartet sync bit assertion
 *
 * Helicity State Management:
 * - Sets fHelicityBitPlus/fHelicityBitMinus based on userbit helicity bit
 * - Updates fHelicityReported for downstream processing
 * - Maintains helicity predictor state for data quality monitoring
 *
 * Error Recovery:
 * - Detects missed events through scaler offset analysis
 * - Resets helicity predictor when event sequence is uncertain
 * - Provides debug output for missed event scenarios
 *
 * Pattern Synchronization:
 * - Manages quartet boundaries using sync bits
 * - Handles pattern phase wraparound at maximum phase
 * - Maintains continuous event numbering across pattern boundaries
 *
 * \note This mode is primarily used for injector testing and is not the
 * standard helicity decoding method for production Qweak data analysis.
 *
 * \warning Missed events (scaler offset > 1) will reset the helicity
 * predictor and may affect downstream helicity-dependent analyses.
 */
void QwHelicity::ProcessEventUserbitMode()
{

  /** In this version of the code, the helicity is extracted for a userbit configuration.
      This is not what we plan to have for Qweak but it was done for injector tests and
      so is useful to have as another option to get helicity information. */

  Bool_t ldebug=kFALSE;
  UInt_t userbits;
  static UInt_t lastuserbits  = 0xFF;
  UInt_t scaleroffset=fWord[kScalerCounter].fValue/32;

  if(scaleroffset==1 || scaleroffset==0) {
    userbits = (fWord[kUserbit].fValue & 0xE0000000)>>28;

    //  Now fake the input register, MPS counter, QRT counter, and QRT phase.
    fEventNumber=fEventNumberOld+1;

    lastuserbits = userbits;

    if (lastuserbits==0xFF) {
      fPatternPhaseNumber    = fMinPatternPhase;
    } else {
      if ((lastuserbits & 0x8) == 0x8) {
	//  Quartet bit is set.
	fPatternPhaseNumber    = fMinPatternPhase;  // Reset the QRT phase
	fPatternNumber=fPatternNumberOld+1;     // Increment the QRT counter
      } else {
	fPatternPhaseNumber=fPatternPhaseNumberOld+1;       // Increment the QRT phase
      }

      fHelicityReported=0;

      if ((lastuserbits & 0x4) == 0x4){ //  Helicity bit is set.
	fHelicityReported    |= 1; // Set the InputReg HEL+ bit.
	fHelicityBitPlus=kTRUE;
	fHelicityBitMinus=kFALSE;
      } else {
	fHelicityReported    |= 0; // Set the InputReg HEL- bit.
	fHelicityBitPlus=kFALSE;
	fHelicityBitMinus=kTRUE;
      }
    }
  } else {
    QwError << " QwHelicity::ProcessEvent finding a missed read event in the scaler" << QwLog::endl;
    if(ldebug) {
      std::cout << " QwHelicity::ProcessEvent :" << scaleroffset << " events were missed \n";
      std::cout << " before manipulation \n";
      Print();
    }
    //there was more than one event since the last reading of the scalers
    //ie we should read only one event at the time,
    //if not something is wrong
    fEventNumber=fEventNumberOld+scaleroffset;
    Int_t localphase=fPatternPhaseNumberOld;
    Int_t localpatternnumber=fPatternNumberOld;
    for (UInt_t i=0;i<scaleroffset;i++) {
      fPatternPhaseNumber=localphase+1;
      if(fPatternPhaseNumber>fMaxPatternPhase) {
	fPatternNumber=localpatternnumber+fPatternPhaseNumber/fMaxPatternPhase;
	fPatternPhaseNumber=fPatternPhaseNumber-fMaxPatternPhase;
	localpatternnumber=fPatternNumber;
      }
      localphase=fPatternPhaseNumber;
    }
    //Reset helicity predictor because we are not sure of what we are doing
    fHelicityReported=-1;
    ResetPredictor();
    if(ldebug) {
      std::cout << " after manipulation \n";
      Print();
    }
  }
  return;
}


void QwHelicity::ProcessEventInputRegisterMode()
{
  static Bool_t firstevent   = kTRUE;
  static Bool_t firstpattern = kTRUE;
  static Bool_t fake_the_counters=kFALSE;
  UInt_t thisinputregister=fWord[kInputRegister].fValue;

  if (firstpattern){
    //  If any of the special counters are negative or zero, setup to
    //  generate the counters internally.
    fake_the_counters |= (kPatternCounter<=0)
      || ( kMpsCounter<=0) || (kPatternPhase<=0);
  }

  if (CheckIORegisterMask(thisinputregister,fInputReg_FakeMPS))
    fIgnoreHelicity = kTRUE;
  else
    fIgnoreHelicity = kFALSE;

  /** If we get junk for the mps and pattern information from the run
      we can enable fake counters for mps, pattern number and pattern
      phase to get the job done.
  */
  if (!fake_the_counters){
    /**
       In the Input Register Mode,
       the event number is obtained straight from the wordkMPSCounter.
    */
    fEventNumber=fWord[kMpsCounter].fValue;
    // When we have the minimum phase from the pattern phase word
    // and the input register minimum phase bit is set
    // we can select the second pattern as below.
    if(fWord[kPatternPhase].fValue - fPatternPhaseOffset == 0)
      if (firstpattern && CheckIORegisterMask(thisinputregister,fInputReg_PatternSync)){
	firstpattern   = kFALSE;
      }

    // If firstpattern is still TRUE, we are still searching for the first
    // pattern of the data stream. So set the pattern number = 0
    if (firstpattern)
      fPatternNumber      = -1;
    else {
      fPatternNumber      = fWord[kPatternCounter].fValue;
      fPatternPhaseNumber = fWord[kPatternPhase].fValue - fPatternPhaseOffset + fMinPatternPhase;
    }
  } else {
    //  Use internal variables for all the counters.
    fEventNumber = fEventNumberOld+1;
    if (CheckIORegisterMask(thisinputregister,fInputReg_PatternSync)) {
      fPatternPhaseNumber = fMinPatternPhase;
      fPatternNumber      = fPatternNumberOld + 1;
    } else  {
      fPatternPhaseNumber = fPatternPhaseNumberOld + 1;
      fPatternNumber      = fPatternNumberOld;
    }
  }


  if (firstevent){
    firstevent = kFALSE;
  } else if(fEventNumber!=(fEventNumberOld+1)){
    Int_t nummissed(fEventNumber - (fEventNumberOld+1));
    if (!fSuppressMPSErrorMsgs){
      QwError << "QwHelicity::ProcessEvent read event# ("
	      << fEventNumber << ") is not  old_event#+1; missed "
	      << nummissed << " gates" << QwLog::endl;
    }
    fNumMissedGates += nummissed;
    fNumMissedEventBlocks++;
    fErrorFlag = kErrorFlag_Helicity + kGlobalCut + kEventCutMode3;
  }

  if (CheckIORegisterMask(thisinputregister,fInputReg_PatternSync) && fPatternPhaseNumber != fMinPatternPhase){
    //  Quartet bit is set.
    QwError << "QwHelicity::ProcessEvent:  The Multiplet Sync bit is set, but the Pattern Phase is ("
	    << fPatternPhaseNumber << ") not "
	    << fMinPatternPhase << "!  Please check the fPatternPhaseOffset in the helicity map file." << QwLog::endl;
    fNumMultSyncErrors++;
    fErrorFlag = kErrorFlag_Helicity + kGlobalCut + kEventCutMode3;
  }

  fHelicityReported=0;

  /**
     Extract the reported helicity from the input register for each event.
  */

  if (CheckIORegisterMask(thisinputregister,fInputReg_HelPlus)
      && CheckIORegisterMask(thisinputregister,fInputReg_HelMinus) ){
    //  Both helicity bits are set.
    QwError << "QwHelicity::ProcessEvent:  Both the H+ and H- bits are set: thisinputregister=="
	    << thisinputregister << QwLog::endl;
    fHelicityReported = kUndefinedHelicity;
    fHelicityBitPlus  = kFALSE;
    fHelicityBitMinus = kFALSE;
  } else if (CheckIORegisterMask(thisinputregister,fInputReg_HelPlus)){ //  HelPlus bit is set.
    fHelicityReported    |= 1; // Set the InputReg HEL+ bit.
    fHelicityBitPlus  = kTRUE;
    fHelicityBitMinus = kFALSE;
  } else {
    fHelicityReported    |= 0; // Set the InputReg HEL- bit.
    fHelicityBitPlus  = kFALSE;
    fHelicityBitMinus = kTRUE;
  }

  return;
}

void QwHelicity::ProcessEventInputMollerMode()
{
  static Bool_t firstpattern = kTRUE;

  if(firstpattern && fWord[kPatternCounter].fValue > fPatternNumberOld){
    firstpattern = kFALSE;
  }

  fEventNumber=fWord[kMpsCounter].fValue;
  if(fEventNumber!=(fEventNumberOld+1)){
    Int_t nummissed(fEventNumber - (fEventNumberOld+1));
    QwError << "QwHelicity::ProcessEvent read event# ("
	    << fEventNumber << ") is not  old_event#+1; missed "
	    << nummissed << " gates" << QwLog::endl;
    fNumMissedGates += nummissed;
    fNumMissedEventBlocks++;
  }
  if (firstpattern){
    fPatternNumber      = -1;
    fPatternPhaseNumber = fMinPatternPhase;
  } else {
    fPatternNumber = fWord[kPatternCounter].fValue;
    if (fPatternNumber > fPatternNumberOld){
      //  We are at a new pattern!
      fPatternPhaseNumber  = fMinPatternPhase;
    } else {
      fPatternPhaseNumber  = fPatternPhaseNumberOld + 1;
    }
  }

  if (fEventType == kEventTypeHelPlus)       fHelicityReported=1;
  else if (fEventType == kEventTypeHelMinus) fHelicityReported=0;
  //  fHelicityReported = (fEventType == 1 ? 0 : 1);

  if (fHelicityReported == 1){
    fHelicityBitPlus=kTRUE;
    fHelicityBitMinus=kFALSE;
  } else {
    fHelicityBitPlus=kFALSE;
    fHelicityBitMinus=kTRUE;
  }
  return;
}


void  QwHelicity::ProcessEvent()
{
  Bool_t ldebug = kFALSE;
  fErrorFlag = 0;

  if (! HasDataLoaded()) return;

  switch (fHelicityDecodingMode)
    {
    case kHelUserbitMode :
      ProcessEventUserbitMode();
      break;
    case kHelInputRegisterMode :
      ProcessEventInputRegisterMode();
      break;
    case kHelInputMollerMode :
      ProcessEventInputMollerMode();
      break;
    default:
      QwError << "QwHelicity::ProcessEvent no instructions on how to decode the helicity !!!!" << QwLog::endl;
      abort();
      break;
    }

  if(fHelicityBitPlus==fHelicityBitMinus)
    fHelicityReported=-1;

  // Predict helicity if delay is non zero.
  if(fUsePredictor && !fIgnoreHelicity){
    PredictHelicity();
  } else {
    // Else use the reported helicity values.
    fHelicityActual  = fHelicityReported;
    fHelicityDelayed = fHelicityReported;

    if(fPatternPhaseNumber== fMinPatternPhase){
      fPreviousPatternPolarity = fActualPatternPolarity;
      fActualPatternPolarity   = fHelicityReported;
      fDelayedPatternPolarity  = fHelicityReported;
    }

  }

  if(ldebug){
    std::cout<<"\nevent number= "<<fEventNumber<<std::endl;
    std::cout<<"pattern number = "<<fPatternNumber<<std::endl;
    std::cout<<"pattern phase = "<<fPatternPhaseNumber<<std::endl;
    std::cout<<"max pattern phase = "<<fMaxPatternPhase<<std::endl;
    std::cout<<"min pattern phase = "<<fMinPatternPhase<<std::endl;
  }

  return;
}


void QwHelicity::EncodeEventData(std::vector<UInt_t> &buffer)
{
  std::vector<UInt_t> localbuffer;
  localbuffer.clear();

  // Userbit mode
  switch (fHelicityDecodingMode) {
  case kHelUserbitMode: {
    UInt_t userbit = 0x0;
    if (fPatternPhaseNumber == fMinPatternPhase) userbit |= 0x80000000;
    if (fHelicityDelayed == 1)    userbit |= 0x40000000;

    // Write the words to the buffer
    localbuffer.push_back(0x1); // cleandata
    localbuffer.push_back(0xa); // scandata1
    localbuffer.push_back(0xa); // scandata2
    localbuffer.push_back(0x0); // scalerheader
    localbuffer.push_back(0x20); // scalercounter (32)
    localbuffer.push_back(userbit); // userbit

    for (int i = 0; i < 64; i++) localbuffer.push_back(0x0); // (not used)
    break;
  }
  case kHelInputRegisterMode: {
    UInt_t input_register = 0x0;
    if (fHelicityDelayed == 1) input_register |= fInputReg_HelPlus;
    if (fHelicityDelayed == 0) input_register |= fInputReg_HelMinus;
    if (fPatternPhaseNumber == fMinPatternPhase) input_register |= fInputReg_PatternSync;

    // Write the words to the buffer
    localbuffer.push_back(input_register); // input_register
    localbuffer.push_back(0x0); // output_register
    localbuffer.push_back(fEventNumber); // mps_counter
    localbuffer.push_back(fPatternNumber); // pat_counter
    localbuffer.push_back(fPatternPhaseNumber - fMinPatternPhase + fPatternPhaseOffset); // pat_phase

    for (int i = 0; i < 17; i++) localbuffer.push_back(0x0); // (not used)
    break;
  }
  default:
    QwWarning << "QwHelicity::EncodeEventData: Unsupported helicity encoding!" << QwLog::endl;
    break;
  }

  // If there is element data, generate the subbank header
  std::vector<UInt_t> subbankheader;
  std::vector<UInt_t> rocheader;
  if (localbuffer.size() > 0) {

    // Form CODA subbank header
    subbankheader.clear();
    subbankheader.push_back(localbuffer.size() + 1);	// subbank size
    subbankheader.push_back((fCurrentBank_ID << 16) | (0x01 << 8) | (1 & 0xff));
		// subbank tag | subbank type | event number

    // Form CODA bank/roc header
    rocheader.clear();
    rocheader.push_back(subbankheader.size() + localbuffer.size() + 1);	// bank/roc size
    rocheader.push_back((fCurrentROC_ID << 16) | (0x10 << 8) | (1 & 0xff));
		// bank tag == ROC | bank type | event number

    // Add bank header, subbank header and element data to output buffer
    buffer.insert(buffer.end(), rocheader.begin(), rocheader.end());
    buffer.insert(buffer.end(), subbankheader.begin(), subbankheader.end());
    buffer.insert(buffer.end(), localbuffer.begin(), localbuffer.end());
  }
}

Int_t QwHelicity::LoadChannelMap(TString mapfile)
{
  Bool_t ldebug=kFALSE;

  Int_t wordsofar=0;
  Int_t bankindex=-1;

  fPatternPhaseOffset=1;//Phase number offset is set to 1 by default and will be set to 0 if phase number starts from 0


  // Default value for random seed is 30 bits
  fRandBits = 30;


  QwParameterFile mapstr(mapfile.Data());  //Open the file
  fDetectorMaps.insert(mapstr.GetParamFileNameContents());
  mapstr.EnableGreediness();
  mapstr.SetCommentChars("!");

  UInt_t value = 0;
  TString valuestr;

  while (mapstr.ReadNextLine()){
    RegisterRocBankMarker(mapstr);

    if (mapstr.PopValue("patternphase",value)) {
      fMaxPatternPhase=value;
      BuildHelicityBitPattern(fMaxPatternPhase);
    }
    if (mapstr.PopValue("patternbits",valuestr)) {
      SetHelicityBitPattern(valuestr);
    }
    if (mapstr.PopValue("inputregmask_fakemps",value)) {
      fInputReg_FakeMPS = value;
    }
    if (mapstr.PopValue("inputregmask_helicity",value)) {
      fInputReg_HelPlus  = value;
      fInputReg_HelMinus = 0;
    }
    if (mapstr.PopValue("inputregmask_helplus",value)) {
      fInputReg_HelPlus = value;
    }
    if (mapstr.PopValue("inputregmask_helminus",value)) {
      fInputReg_HelMinus = value;
    }
    if (mapstr.PopValue("inputregmask_pattsync",value)) {
      fInputReg_PatternSync = value;
    }
    if (mapstr.PopValue("inputregmask_pairsync",value)) {
      fInputReg_PairSync = value;
    }
    if (mapstr.PopValue("fakempsbit",value)) {
      fInputReg_FakeMPS = value;
      QwWarning << " fInputReg_FakeMPS 0x" << std::hex << fInputReg_FakeMPS << std::dec << QwLog::endl;
    }
    if (mapstr.PopValue("numberpatternsdelayed",value)) {
      SetHelicityDelay(value);
    }
    if (mapstr.PopValue("randseedbits",value)) {
      if (value==24 || value==30)
	fRandBits = value;
    }
    if (mapstr.PopValue("patternphaseoffset",value)) {
      fPatternPhaseOffset=value;
    }
    if (mapstr.PopValue("helpluseventtype",value)) {
      kEventTypeHelPlus = value;
    }
    if (mapstr.PopValue("helminuseventtype",value)) {
      kEventTypeHelMinus = value;
    }
    if (mapstr.PopValue("helicitydecodingmode",valuestr)) {
      if (valuestr=="InputRegisterMode") {
	QwMessage << " **** Input Register Mode **** " << QwLog::endl;
	fHelicityDecodingMode=kHelInputRegisterMode;
      } else if (valuestr=="UserbitMode"){
	QwMessage << " **** Userbit Mode **** " << QwLog::endl;
	fHelicityDecodingMode=kHelUserbitMode;
      } else if (valuestr=="HelLocalyMadeUp"){
	QwMessage << "**** Helicity Locally Made Up ****" << QwLog::endl;
	fHelicityDecodingMode=kHelLocalyMadeUp;
      } else if (valuestr=="InputMollerMode") {
	QwMessage << "**** Input Moller Mode ****" << QwLog::endl;
	fHelicityDecodingMode=kHelInputMollerMode;
      } else {
	QwError  << "The helicity decoding mode read in file " << mapfile
		 << " is not recognized in function QwHelicity::LoadChannelMap \n"
		 << " Quitting this execution." << QwLog::endl;
      }
    }

    if(bankindex!=GetSubbankIndex(fCurrentROC_ID,fCurrentBank_ID)) {
      bankindex=GetSubbankIndex(fCurrentROC_ID,fCurrentBank_ID);
      if ((bankindex+1)>0){
	UInt_t numbanks = UInt_t(bankindex+1);
	if (fWordsPerSubbank.size()<numbanks){
	  fWordsPerSubbank.resize(numbanks,
				  std::pair<Int_t, Int_t>(fWord.size(),fWord.size()));
	}
      }
      wordsofar=0;
    }
    mapstr.TrimWhitespace();   // Get rid of leading and trailing spaces.
    if (mapstr.LineIsEmpty())  continue;

    //  Break this line into tokens to process it.
    TString modtype = mapstr.GetTypedNextToken<TString>();	// module type
    Int_t modnum = mapstr.GetTypedNextToken<Int_t>();	//slot number
    /* Int_t channum = */ mapstr.GetTypedNextToken<Int_t>();	//channel number /* unused */
    TString dettype = mapstr.GetTypedNextToken<TString>();	//type-purpose of the detector
    dettype.ToLower();
    TString namech  = mapstr.GetTypedNextToken<TString>();  //name of the detector
    namech.ToLower();
    TString keyword = mapstr.GetTypedNextToken<TString>();
    keyword.ToLower();
    // Notice that "namech" and "keyword" are now forced to lower-case.

    if(modtype=="SKIP"){
      if (modnum<=0) wordsofar+=1;
      else           wordsofar+=modnum;
    } else if(modtype!="WORD"|| dettype!="helicitydata") {
      QwError << "QwHelicity::LoadChannelMap:  Unknown detector type: "
	      << dettype  << ", the detector " << namech << " will not be decoded "
	      << QwLog::endl;
      continue;
    } else {
      QwWord localword;
      localword.fSubbankIndex=bankindex;
      localword.fWordInSubbank=wordsofar;
      wordsofar+=1;
      // I assume that one data = one word here. But it is not always the case, for
      // example the triumf adc gives 6 words per channel
      localword.fModuleType=modtype;
      localword.fWordName=namech;
      localword.fWordType=dettype;
      fWord.push_back(localword);
      fWordsPerSubbank.at(bankindex).second = fWord.size();

      // Notice that "namech" is in lower-case, so these checks
      // should all be in lower-case
      switch (fHelicityDecodingMode)
	{
	case kHelUserbitMode :
	  if(namech.Contains("userbit")) kUserbit=fWord.size()-1;
	  if(namech.Contains("scalercounter")) kScalerCounter=fWord.size()-1;
	  break;
	case kHelInputRegisterMode :
	  if(namech.Contains("input_register")) kInputRegister= fWord.size()-1;
	  if(namech.Contains("mps_counter")) kMpsCounter= fWord.size()-1;
	  if(namech.Contains("pat_counter")) kPatternCounter= fWord.size()-1;
	  if(namech.Contains("pat_phase")) kPatternPhase= fWord.size()-1;
	  break;
	case kHelInputMollerMode :
	  if(namech.Contains("mps_counter")) {
	    kMpsCounter= fWord.size()-1;
	  }
	  if(namech.Contains("pat_counter")) {
	    kPatternCounter = fWord.size()-1;
	  }
	  break;
	}
    }
  }


  if(ldebug) {
    std::cout << "Done with Load map channel \n";
    for(size_t i=0;i<fWord.size();i++)
      fWord[i].PrintID();
    std::cout << " kUserbit=" << kUserbit << "\n";
  }
  ldebug=kFALSE;

  if (fHelicityDecodingMode==kHelInputMollerMode){
    // Check to be sure kEventTypeHelPlus and kEventTypeHelMinus are both defined and not equal
    if (kEventTypeHelPlus != kEventTypeHelMinus
	&& kEventTypeHelPlus>0 && kEventTypeHelPlus<15
	&& kEventTypeHelMinus>0 && kEventTypeHelMinus<15) {
      // Everything is okay
      QwDebug << "QwHelicity::LoadChannelMap:"
	      << "  We are in Moller Helicity Mode, with HelPlusEventType = "
	      << kEventTypeHelPlus
	      << "and HelMinusEventType = " << kEventTypeHelMinus
	      << QwLog::endl;
    } else {
      QwError << "QwHelicity::LoadChannelMap:"
	      << "  We are in Moller Helicity Mode, and the HelPlus and HelMinus event types are not set properly."
	      << "  HelPlusEventType = "  << kEventTypeHelPlus
	      << ", HelMinusEventType = " << kEventTypeHelMinus
	      << ".  Please correct the helicity map file!"
	      << QwLog::endl;
      exit(65);
    }
  }

std::cout << fHelicityBitPattern.size() << std::endl;

  mapstr.Close(); // Close the file (ifstream)
  return 0;
}


Int_t QwHelicity::LoadEventCuts(TString filename){
  return 0;
}

Int_t QwHelicity::ProcessEvBuffer(UInt_t event_type, const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words)
{
  Bool_t lkDEBUG = kFALSE;

  if (((0x1 << (event_type - 1)) & this->GetEventTypeMask()) == 0)
    return 0;
  fEventType = event_type;

  Int_t index = GetSubbankIndex(roc_id,bank_id);
  if (index >= 0 && num_words > 0) {
    SetDataLoaded(kTRUE);
    //  We want to process this ROC.  Begin loopilooping through the data.
    QwDebug << "QwHelicity::ProcessEvBuffer:  "
	    << "Begin processing ROC" << roc_id
	    << " and subbank " << bank_id
	    << " number of words=" << num_words << QwLog::endl;

    for(Int_t i=fWordsPerSubbank.at(index).first; i<fWordsPerSubbank.at(index).second; i++) {
      if(fWord[i].fWordInSubbank+1<= (Int_t) num_words) {
	fWord[i].fValue=buffer[fWord[i].fWordInSubbank];
      } else {
	QwWarning << "QwHelicity::ProcessEvBuffer:  There is not enough word in the buffer to read data for "
		  << fWord[i].fWordName << QwLog::endl;
	QwWarning << "QwHelicity::ProcessEvBuffer:  Words in this buffer:" << num_words
		  << " trying to read word number =" << fWord[i].fWordInSubbank << QwLog::endl;
      }
    }
    if(lkDEBUG) {
      QwDebug << "QwHelicity::ProcessEvBuffer:  Done with Processing this event" << QwLog::endl;
      for(size_t i=0;i<fWord.size();i++) {
	std::cout << "QwHelicity::ProcessEvBuffer:  word number = " << i << " ";
	fWord[i].Print();
      }
    }
  }
  lkDEBUG=kFALSE;
  return 0;
}


void  QwHelicity::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  SetHistoTreeSave(prefix);
  if (folder != NULL) folder->cd();
  TString basename;
  size_t index=0;

  if(fHistoType==kHelNoSave)
    {
      //do nothing
    }
  else if(fHistoType==kHelSavePattern)
    {
      fHistograms.resize(1+fWord.size(), NULL);
      basename="pattern_polarity";
      fHistograms[index]   = gQwHists.Construct1DHist(basename);
      index+=1;
      for (size_t i=0; i<fWord.size(); i++){
	basename="hel_"+fWord[i].fWordName;
	fHistograms[index]   = gQwHists.Construct1DHist(basename);
	index+=1;
      }
    }
  else if(fHistoType==kHelSaveMPS)
    {
      fHistograms.resize(4+fWord.size(), NULL);
      //eventnumber, patternnumber, helicity, patternphase + fWord.size
      basename=prefix+"delta_event_number";
      fHistograms[index]   = gQwHists.Construct1DHist(basename);
      index+=1;
      basename=prefix+"delta_pattern_number";
      fHistograms[index]   = gQwHists.Construct1DHist(basename);
      index+=1;
      basename=prefix+"pattern_phase";
      fHistograms[index]   = gQwHists.Construct1DHist(basename);
      index+=1;
      basename=prefix+"helicity";
      fHistograms[index]   = gQwHists.Construct1DHist(basename);
      index+=1;
      for (size_t i=0; i<fWord.size(); i++){
	basename=prefix+fWord[i].fWordName;
	fHistograms[index]   = gQwHists.Construct1DHist(basename);
	index+=1;
      }
    }
  else
    QwError << "QwHelicity::ConstructHistograms this prefix--" << prefix << "-- is not unknown:: no histo created" << QwLog::endl;

  return;
}

