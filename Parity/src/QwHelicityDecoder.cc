/**********************************************************\
* File: QwHelicityDecoder.C                               *
* Implemented the helicity data decoder                   *
* Contributor: Arindam Sen (asen@jlab.org)                *
* Time-stamp:                                             *
\**********************************************************/

#include "QwHelicityDecoder.h"

// System headers
#include <stdexcept>

// ROOT headers
#include "TRegexp.h"
#include "TMath.h"

// Qweak headers
#include "QwHistogramHelper.h"
#ifdef __USE_DATABASE__
#define MYSQLPP_SSQLS_NO_STATICS
#include "QwParitySSQLS.h"
#include "QwParityDB.h"
#endif // __USE_DATABASE__
#include "QwLog.h"

extern QwHistogramHelper gQwHists;
const Int_t QwHelicityDecoder::fNumDecoderWords=14;

//**************************************************//

// Register this subsystem with the factory
RegisterSubsystemFactory(QwHelicityDecoder);

const std::vector<UInt_t> QwHelicityDecoder::kDefaultHelicityBitPattern{0x69};

/// constructor

//**************************************************//

QwHelicityDecoder::QwHelicityDecoder(const TString& name)
: VQwSubsystem(name),
  QwHelicityBase(name)
{
}

//**************************************************//
QwHelicityDecoder::QwHelicityDecoder(const QwHelicityDecoder& source)
: VQwSubsystem(source.GetName()),
  QwHelicityBase(source.GetName())
{
}

//**************************************************//
void QwHelicityDecoder::DefineOptions(QwOptions &options)
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

void QwHelicityDecoder::ProcessOptions(QwOptions &options)
{
 // Read the cmd options and override channel map settings
  QwMessage << "QwHelicityDecoder::ProcessOptions" << QwLog::endl;
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
    //UInt_t bits = QwParameterFile::GetUInt(hex);
    SetHelicityBitPattern(hex);
  } else {
    BuildHelicityBitPattern(fMaxPatternPhase);
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
    BuildHelicityBitPattern(fMaxPatternPhase);
  }

  //  Here we're going to try to get the "online" option which
  //  is defined by QwEventBuffer.
  if (options.HasValue("online")){
    fSuppressMPSErrorMsgs = options.GetValue<bool>("online");
  } else {
    fSuppressMPSErrorMsgs = kFALSE;
  }
}

void QwHelicityDecoder::ClearEventData()
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

  /**Clear out helicity variables */
  fHelicityReported = kUndefinedHelicity;
  fHelicityActual = kUndefinedHelicity;
  fHelicityDelayed= kUndefinedHelicity;
  fHelicityBitPlus = kFALSE;
  fHelicityBitMinus = kFALSE;
  fEventNumber = -1;
  fPatternPhaseNumber = -1;
  fPatternNumber = -1;
  return;
}

Int_t QwHelicityDecoder::ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words)
{
  return 0;
}

Int_t QwHelicityDecoder::LoadInputParameters(TString pedestalfile)
{
  return 0;
}


Bool_t QwHelicityDecoder::ApplySingleEventCuts(){

  return kTRUE;
}

void  QwHelicityDecoder::ProcessEvent()
{
  Bool_t ldebug = kTRUE;
  fErrorFlag = 0;

  if (! HasDataLoaded()) return;

  static Bool_t firstpattern = kTRUE;

  if(firstpattern && fPatternNumber > fPatternNumberOld){
    firstpattern = kFALSE;
  }
  
  if(fEventNumber!=(fEventNumberOld+1)){
    Int_t nummissed(fEventNumber - (fEventNumberOld+1));
    QwError << "QwHelicityDecoder::ProcessEvent read event# ("
	    << fEventNumber << ") is not  old_event#+1; missed "
	    << nummissed << " gates" << QwLog::endl;
    fNumMissedGates += nummissed;
    fNumMissedEventBlocks++;
  }
  
  fHelicityReported=fBit_Helicity;

  if (fHelicityReported == 1){
    fHelicityBitPlus=kTRUE;
    fHelicityBitMinus=kFALSE;
  } else {
    fHelicityBitPlus=kFALSE;
    fHelicityBitMinus=kTRUE;
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
  }
  
  fPatternSeed = fSeed_Reported & 0x7fffffff;  

  if(ldebug){
    std::cout<<"\nevent number= "<<fEventNumber<<std::endl;
    std::cout<<"pattern number = "<<fPatternNumber<<std::endl;
    std::cout<<"pattern phase = "<<fPatternPhaseNumber<<std::endl;
    std::cout<<"max pattern phase = "<<fMaxPatternPhase<<std::endl;
    std::cout<<"min pattern phase = "<<fMinPatternPhase<<std::endl;


  }
  return;
}


void QwHelicityDecoder::EncodeEventData(std::vector<UInt_t> &buffer)
{
}

void QwHelicityDecoder::Print() const
{
  QwOut << "===========================\n"
	<< "This event: Event#, Pattern#, PatternPhase#="
	<< fEventNumber << ", "
	<< fPatternNumber << ", "
	<< fPatternPhaseNumber << QwLog::endl;
  QwOut << "Previous event: Event#, Pattern#, PatternPhase#="
	<< fEventNumberOld << ", "
	<< fPatternNumberOld << ", "
	<< fPatternPhaseNumberOld << QwLog::endl;
  QwOut << "delta = \n(fEventNumberOld)-(fMaxPatternPhase)x(fPatternNumberOld)-(fPatternPhaseNumberOld)= "
	<< ((fEventNumberOld)-(fMaxPatternPhase)*(fPatternNumberOld)-(fPatternPhaseNumberOld)) << QwLog::endl;
  QwOut << "Helicity Reported, Delayed, Actual ="
	<< fHelicityReported << ","
	<< fHelicityDelayed << ","
	<< fHelicityActual << QwLog::endl;
  QwOut << "===" << QwLog::endl;
  return;
}


Int_t QwHelicityDecoder::LoadChannelMap(TString mapfile)
{
  UInt_t value = 0;
  TString valuestr;

  QwParameterFile mapstr(mapfile.Data());  //Open the file
  fDetectorMaps.insert(mapstr.GetParamFileNameContents());
  mapstr.EnableGreediness();
  mapstr.SetCommentChars("!");

  while (mapstr.ReadNextLine()){
    //  This class doesn't have any normal decode lines in the map file,
    //  so there is nothing to do in this loop.  We just need the one
    //  call to "ReadNextLine" to populate the list of key/vale pairs.

    // std::cout << "in the loop: " << mapstr.GetLine() << std::endl;   
    // RegisterRocBankMarker(mapstr);
  }
  //std::cout << "finished loop" << std::endl;

  //  Call "RegisterRocBankMarker" to record the ROC/Bank our data is in.
  RegisterRocBankMarker(mapstr);

  //  If we have other key/values we want to use, we'd need to go though 
  //  them here to see if they were found in the mapfile.
  if (mapstr.PopValue("patternphase",value)) {
    fMaxPatternPhase=value;
    //QwMessage << " fMaxPatternPhase " << fMaxPatternPhase << QwLog::endl;
  }


  //PrintInfo();
  return 0;
}


Int_t QwHelicityDecoder::LoadEventCuts(TString filename){

  return 0;
}

Int_t QwHelicityDecoder::ProcessEvBuffer(UInt_t event_type, const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words)
{

  Bool_t lkDEBUG = kFALSE;
  //std::cout << " roc id = " << roc_id << "bank id = " << bank_id << std::endl; 
  Int_t index = GetSubbankIndex(roc_id,bank_id);
  //std::cout << "index = " << index << std::endl;
  if (index >= 0 && num_words > 0) {
    SetDataLoaded(kTRUE);
 // lkDEBUG=kTRUE;

  uint32_t type_last = 15;       /* initialize to type FILLER WORD */
  uint32_t time_last = 0;
  uint32_t decoder_index = 0;
  uint32_t num_decoder_words = 1;
  
  uint32_t slot_id_ev_hd = 0;
  uint32_t slot_id_dnv = 0;
  uint32_t slot_id_fill = 0;
   
  uint32_t new_type;
  uint32_t type;
  uint32_t slot_id_hd;
  uint32_t mod_id_hd;
  uint32_t slot_id_tr;
  uint32_t n_evts;
  uint32_t blk_num;
  uint32_t n_words;
  uint32_t evt_num_1;
  uint32_t trig_time;
  uint32_t time_now=0;
  uint32_t time_1;
  uint32_t time_2;

  uint32_t data;

  if (num_words<fNumDecoderWords+4){
    std::cerr << "Not enough words in the bank:"<< std::endl;
    return kFALSE;
    
  } else 
    for (size_t i=0; i<num_words; i++){

      data = buffer[i];
      if(decoder_index)             /* decoder type data word - set by decoder header word */
	{
	  type = 16;        /*  set decoder data words as type 16 */
	  new_type = 0;
	  if(decoder_index < num_decoder_words)
	    {
	      FillHDVariables(data, decoder_index - 1);
	      if(lkDEBUG)
		printf("%8X - decoder data(%d) = %d\n", data, (decoder_index - 1),
		       data);
	      decoder_index++;
	    }
	  else                      /* last decoder word */

	    {
	      FillHDVariables(data, decoder_index - 1);
	      if(lkDEBUG)
		printf("%8X - decoder data(%d) = %d\n", data, (decoder_index - 1),
		       data);
	      decoder_index = 0;
	      num_decoder_words = 1;
	    }
	}
      else                          /* normal typed word */
	{
	  if(data & 0x80000000)     /* data type defining word */
	    {
	      new_type = 1;
	      type = (data & 0x78000000) >> 27;
	    }
	  else                      /* data type continuation word */
	    {
	      new_type = 0;
	      type = type_last;
	    }
	  
	  switch (type)
	    {
	    case 0:         /* BLOCK HEADER */
	      slot_id_hd = (data & 0x7C00000) >> 22;
	      mod_id_hd = (data & 0x3C0000) >> 18;
	      n_evts = (data & 0x000FF);
	      blk_num = (data & 0x3FF00) >> 8;
	      if(lkDEBUG)
		printf
		  ("%8X - BLOCK HEADER - slot = %d  id = %d  n_evts = %d  n_blk = %d\n",
		   data, slot_id_hd, mod_id_hd, n_evts,
		   blk_num);
	      break;
	      
	    case 1:         /* BLOCK TRAILER */
	      slot_id_tr = (data & 0x7C00000) >> 22;
	      n_words = (data & 0x3FFFFF);
	      if(lkDEBUG)
		printf("%8X - BLOCK TRAILER - slot = %d   n_words = %d\n",
		       data, slot_id_tr, n_words);
	      break;
	      
	    case 2:         /* EVENT HEADER */
	      if(new_type)
		{
		  slot_id_ev_hd = (data & 0x07C00000) >> 22;
		  evt_num_1 = (data & 0x00000FFF);
		  trig_time = (data & 0x003FF000) >> 12;
		  if(lkDEBUG)
		    printf
		      ("%8X - EVENT HEADER - slot = %d  evt_num = %d  trig_time = %d (%X)\n",
		       data, slot_id_ev_hd, evt_num_1, trig_time,
		       trig_time);
		}
	      break;
	      
	    case 3:         /* TRIGGER TIME */
	      
	      if(new_type)
		{
		  time_1 = (data & 0x7FFFFFF);
		  if(lkDEBUG)
		    printf("%8X - TRIGGER TIME 1 - time = %X\n", data,
			   time_1);
		  time_now = 1;
		  time_last = 1;
		}
	      else
		{
		  if(time_last == 1)
		    {
		      time_2 = (data & 0xFFFFF);
		      if(lkDEBUG)
			printf("%8X - TRIGGER TIME 2 - time = %X\n", data,
			       time_2);
		      time_now = 2;
		    }
		  else if(lkDEBUG)
		    printf("%8X - TRIGGER TIME - (ERROR)\n", data);

		  time_last = time_now;
		}
          break;
	  
	    case 4:         /* UNDEFINED TYPE */
	      if(lkDEBUG)
		printf("%8X - UNDEFINED TYPE = %d\n", data, type);
	      break;
	      
	    case 5:         /* UNDEFINED TYPE */
	      if(lkDEBUG)
		printf("%8X - UNDEFINED TYPE = %d\n", data, type);
	      break;
	      
	    case 6:         /* UNDEFINED TYPE */
	      if(lkDEBUG)
		printf("%8X - UNDEFINED TYPE = %d\n", data, type);
	      break;
	      
	    case 7:         /* UNDEFINED TYPE */
	      if(lkDEBUG)
		printf("%8X - UNDEFINED TYPE = %d\n", data, type);
	      break;

	    case 8:         /* DECODER HEADER */
	      num_decoder_words = (data & 0x3F);    /* number of decoder words to follow */
	      decoder_index = 1;    /* identify next word as a decoder data word */
	      if(lkDEBUG)
		printf("%8X - DECODER HEADER = %d  (NUM DECODER WORDS = %d)\n",
		       data, type, num_decoder_words);
	      break;
	      
	    case 9:         /* UNDEFINED TYPE */
	      if(lkDEBUG)
		printf("%8X - UNDEFINED TYPE = %d\n", data, type);
	      break;
	      
	    case 10:                /* UNDEFINED TYPE */
	      if(lkDEBUG)
		printf("%8X - UNDEFINED TYPE = %d\n", data, type);
	      break;
	      
	    case 11:                /* UNDEFINED TYPE */
	      if(lkDEBUG)
		printf("%8X - UNDEFINED TYPE = %d\n", data, type);
	      break;
	      
	    case 12:                /* UNDEFINED TYPE */
	      if(lkDEBUG)
		printf("%8X - UNDEFINED TYPE = %d\n", data, type);
	      break;
	      
	    case 13:                /* END OF EVENT */
	      if(lkDEBUG)
		printf("%8X - END OF EVENT = %d\n", data, type);
	      break;
	      
	    case 14:                /* DATA NOT VALID (no data available) */
	      slot_id_dnv = (data & 0x7C00000) >> 22;
	      if(lkDEBUG)
		printf("%8X - DATA NOT VALID = %d  slot = %d\n", data,
		       type, slot_id_dnv);
	      break;
	      
	    case 15:                /* FILLER WORD */
	      slot_id_fill = (data & 0x7C00000) >> 22;
	      if(lkDEBUG)
		printf("%8X - FILLER WORD = %d  slot = %d\n", data, type,
		       slot_id_fill);
	      break;
	    }
	  
	  type_last = type; /* save type of current data word */
	  
	}
    }
  }    
  return 0;
}

void QwHelicityDecoder::FillHDVariables(uint32_t data, uint32_t index) {

  switch (index) {
  case 0:
    fSeed_Reported=data;
    break;
  case 1:
    fNum_TStable_Fall=data;
    break;
  case 2:
    fEventNumber=data;
    break;
  case 3:
    fPatternNumber=data;
    break;
  case 4:
    fNum_Pair_Sync=data;
    break;
  case 5:
    fTime_since_TStable=data;
    break;
  case 6:
    fTime_since_TSettle=data;
    break;
  case 7:
    fLast_Duration_TStable=data;
    break;
  case 8:
    fLast_Duration_TSettle=data;
    break;
  case 9:
    fPatternPhaseNumber = ((data>>8) & 0xff)+1;
    fEventPolarity      = (data>>5) & 0x1;
    fReportedPatternHel = (data>>4) & 0x1;
    fBit_Helicity       = (data>>3) & 0x1;
    fBit_PairSync       = (data>>2) & 0x1;
    fBit_PatSync        = (data>>1) & 0x1;
    fBit_TStable        = data      & 0x1;
    break;
  case 10:
    fEvtHistory_PatSync=data;
    break;
  case 11:
    fEvtHistory_PairSync=data;
    break;
  case 12:
    fEvtHistory_ReportedHelicity=data;
    break;
  case 13:
    fPatHistory_ReportedHelicity=data;
    break;
  }
}

void QwHelicityDecoder::SetFirstBits(UInt_t nbits, UInt_t seed)
{
  // This gives the predictor a quick start
  UShort_t firstbits[nbits];
  for (unsigned int i = 0; i < nbits; i++) firstbits[i] = (seed >> i) & 0x1;
  // Set delayed seed
  iseed_Delayed = GetRandomSeed(firstbits);
  // Progress actual seed by the helicity delay
  iseed_Actual = iseed_Delayed;
  for (int i = 0; i < fHelicityDelay; i++) GetRandbit(iseed_Actual);
}

void QwHelicityDecoder::SetHistoTreeSave(const TString &prefix)
{
 Ssiz_t len;
  if (TRegexp("diff_").Index(prefix,&len) == 0
   || TRegexp("asym[1-9]*_").Index(prefix,&len) == 0)
    fHistoType = kHelNoSave;
  else if (TRegexp("yield_").Index(prefix,&len) == 0)
    fHistoType = kHelSavePattern;
  else
    fHistoType = kHelSaveMPS;

}

void  QwHelicityDecoder::ConstructHistograms(TDirectory *folder, TString &prefix)
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
    QwError << "QwHelicityDecoder::ConstructHistograms this prefix--" << prefix << "-- is not unknown:: no histo created" << QwLog::endl;

  return;
}

void  QwHelicityDecoder::FillHistograms()
{
  if (fHistograms.size()==0){return;}
  size_t index=0;
  if(fHistoType==kHelNoSave)
    {
      //do nothing
    }
  else if(fHistoType==kHelSavePattern)
    {
      QwDebug << "QwHelicityDecoder::FillHistograms helicity info " << QwLog::endl;
      QwDebug << "QwHelicityDecoder::FillHistograms  pattern polarity=" << fActualPatternPolarity << QwLog::endl;
      if (fHistograms[index]!=NULL)
        fHistograms[index]->Fill(fActualPatternPolarity);
      index+=1;
      
      for (size_t i=0; i<fWord.size(); i++){
        if (fHistograms[index]!=NULL)
          fHistograms[index]->Fill(fWord[i].fValue);
        index+=1;       
        QwDebug << "QwHelicityDecoder::FillHistograms " << fWord[i].fWordName << "=" << fWord[i].fValue << QwLog::endl;
      }
    }
  else if(fHistoType==kHelSaveMPS)
    {
      QwDebug << "QwHelicityDecoder::FillHistograms mps info " << QwLog::endl;
      if (fHistograms[index]!=NULL)
        fHistograms[index]->Fill(fEventNumber-fEventNumberOld);
      index+=1;
      if (fHistograms[index]!=NULL)
        fHistograms[index]->Fill(fPatternNumber-fPatternNumberOld);
      index+=1;
      if (fHistograms[index]!=NULL)
        fHistograms[index]->Fill(fPatternPhaseNumber);
      index+=1;
      if (fHistograms[index]!=NULL)
        fHistograms[index]->Fill(fHelicityActual);
      index+=1;
      for (size_t i=0; i<fWord.size(); i++){
        if (fHistograms[index]!=NULL)
          fHistograms[index]->Fill(fWord[i].fValue);
        index+=1;
        QwDebug << "QwHelicityDecoder::FillHistograms " << fWord[i].fWordName << "=" << fWord[i].fValue << QwLog::endl;
      }
    }

  return;
}


void  QwHelicityDecoder::ConstructBranchAndVector(TTree *tree, TString &prefix, std::vector<Double_t> &values)
{
SetHistoTreeSave(prefix);
  fTreeArrayIndex  = values.size();
  TString basename;
  if(fHistoType==kHelNoSave)
    {
      //do nothing
    }
  else if(fHistoType==kHelSaveMPS)
    {
       basename = "hd_actual_helicity";    //predicted actual helicity before being delayed.
       values.push_back(0.0);
       tree->Branch(basename, &(values.back()), basename+"/D");
      
      basename = "hd_delayed_helicity";   //predicted delayed helicity
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
            basename = "hd_reported_helicity";  //delayed helicity reported by the input register.
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      basename = "hd_pattern_phase";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      basename = "hd_pattern_number";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      basename = "hd_pattern_seed";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      basename = "hd_event_number";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");

      basename = "hd_event_Polarity";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");

      basename = "hd_Reported_Pattern_Hel";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      for (size_t i=0; i<fWord.size(); i++)
        {
          basename = fWord[i].fWordName;
          values.push_back(0.0);
          tree->Branch(basename, &(values.back()), basename+"/D");
        }
    }
 else if(fHistoType==kHelSavePattern)
    {
      basename = "hd_actual_helicity";    //predicted actual helicity before being delayed.
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      basename = "hd_actual_pattern_polarity";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      basename = "hd_actual_previous_pattern_polarity";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      basename = "hd_delayed_pattern_polarity";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      basename = "hd_pattern_number";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      basename = "hd_pattern_seed";
      values.push_back(0.0);
      tree->Branch(basename, &(values.back()), basename+"/D");
      //
      for (size_t i=0; i<fWord.size(); i++)
        {
          basename = fWord[i].fWordName;
          values.push_back(0.0);
          tree->Branch(basename, &(values.back()), basename+"/D");
        }
    }
  std::cout << "after construction size " << values.size() << std::endl;
  return;
}

void  QwHelicityDecoder::ConstructBranch(TTree *tree, TString &prefix)
{
TString basename;

  SetHistoTreeSave(prefix);
// print the branch and type name (arindam)
//std::cout << "QwHelicityDecoder::ConstructBranch" << std::endl;
//std::cout << tree->GetName() << " " << prefix << " " << std::endl;
//std::cout << "Histo type --> " << fHistoType << std::endl;
  if(fHistoType==kHelNoSave)
    {
      //do nothing
    }
  else if(fHistoType==kHelSaveMPS)
    {
      // basename = "hd_actual_helicity";    //predicted actual helicity before being delayed.
      // tree->Branch(basename, &fHelicityActual, basename+"/I");
      //
      basename = "hd_delayed_helicity";   //predicted delayed helicity
      tree->Branch(basename, &fHelicityDelayed, basename+"/I");
      //
      basename = "hd_reported_helicity";  //delayed helicity reported by the input register.
      tree->Branch(basename, &fHelicityReported, basename+"/I");
      //
      basename = "hd_pattern_phase";
      tree->Branch(basename, &fPatternPhaseNumber, basename+"/I");
      //
      basename = "hd_pattern_number";
      tree->Branch(basename, &fPatternNumber, basename+"/I");
      //
      basename = "hd_pattern_seed";
      tree->Branch(basename, &fPatternSeed, basename+"/I");
      //
      basename = "hd_event_number";
      tree->Branch(basename, &fEventNumber, basename+"/I");

      basename = "hd_event_Polarity";
      tree->Branch(basename, &fEventPolarity, basename+"/I");

      basename = "hd_Reported_Pattern_Hel";
      tree->Branch(basename, &fReportedPatternHel, basename+"/I");
    }
  else if(fHistoType==kHelSavePattern)
    {
      basename = "hd_actual_helicity";    //predicted actual helicity before being delayed.
      tree->Branch(basename, &fHelicityActual, basename+"/I");
      //
      basename = "hd_actual_pattern_polarity";
      tree->Branch(basename, &fActualPatternPolarity, basename+"/I");
      //
      basename = "hd_actual_previous_pattern_polarity";
      tree->Branch(basename, &fPreviousPatternPolarity, basename+"/I");
      //
      basename = "hd_delayed_pattern_polarity";
      tree->Branch(basename, &fDelayedPatternPolarity, basename+"/I");
     //
      basename = "hd_pattern_number";
      tree->Branch(basename, &fPatternNumber, basename+"/I");
      //
      basename = "hd_pattern_seed";
      tree->Branch(basename, &fPatternSeed, basename+"/I");

      for (size_t i=0; i<fWord.size(); i++)
        {
          basename = fWord[i].fWordName;
          tree->Branch(basename, &fWord[i].fValue, basename+"/I");
        }
    }

  return;
}

void  QwHelicityDecoder::ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& trim_file)
{
  TString basename;

  SetHistoTreeSave(prefix);
  if(fHistoType==kHelNoSave)
    {
      //do nothing
    }
  else if(fHistoType==kHelSaveMPS)
    {
      // basename = "hd_actual_helicity";    //predicted actual helicity before being delayed.
      // tree->Branch(basename, &fHelicityActual, basename+"/I");
      //
      basename = "hd_delayed_helicity";   //predicted delayed helicity
      tree->Branch(basename, &fHelicityDelayed, basename+"/I");
      //
      basename = "hd_reported_helicity";  //delayed helicity reported by the input register.
      tree->Branch(basename, &fHelicityReported, basename+"/I");
      //
      basename = "hd_pattern_phase";
      tree->Branch(basename, &fPatternPhaseNumber, basename+"/I");
      //
      basename = "hd_pattern_number";
      tree->Branch(basename, &fPatternNumber, basename+"/I");
      //
      basename = "hd_pattern_seed";
      tree->Branch(basename, &fPatternSeed, basename+"/I");
      //
      basename = "hd_event_number";
      tree->Branch(basename, &fEventNumber, basename+"/I");

      basename = "hd_event_Polarity";
      tree->Branch(basename, &fEventPolarity, basename+"/I");

      basename = "hd_Reported_Pattern_Hel";
      tree->Branch(basename, &fReportedPatternHel, basename+"/I");
    }
  else if(fHistoType==kHelSavePattern)
    {
      basename = "hd_actual_helicity";    //predicted actual helicity before being delayed.
      tree->Branch(basename, &fHelicityActual, basename+"/I");
      //
      basename = "hd_actual_pattern_polarity";
      tree->Branch(basename, &fActualPatternPolarity, basename+"/I");
      //
      basename = "hd_actual_previous_pattern_polarity";
      tree->Branch(basename, &fPreviousPatternPolarity, basename+"/I");
      //
      basename = "hd_delayed_pattern_polarity";
      tree->Branch(basename, &fDelayedPatternPolarity, basename+"/I");
      //
      basename = "hd_pattern_number";
      tree->Branch(basename, &fPatternNumber, basename+"/I");
      //
      basename = "hd_pattern_seed";
      tree->Branch(basename, &fPatternSeed, basename+"/I");

      for (size_t i=0; i<fWord.size(); i++)
        {
          basename = fWord[i].fWordName;
          tree->Branch(basename,&fWord[i].fValue, basename+"/I");
        }

    }

  return;
}

void  QwHelicityDecoder::FillTreeVector(std::vector<Double_t> &values) const
{
  size_t index=fTreeArrayIndex;
  if(fHistoType==kHelSaveMPS)
    {
      values[index++] = fHelicityActual;
      values[index++] = fHelicityDelayed;
      values[index++] = fHelicityReported;
      values[index++] = fPatternPhaseNumber;
      values[index++] = fPatternNumber;
      values[index++] = fPatternSeed;
      values[index++] = fEventNumber;
      values[index++] = fEventPolarity;
      values[index++] = fReportedPatternHel;
      for (size_t i=0; i<fWord.size(); i++)
        values[index++] = fWord[i].fValue;
    }
  else if(fHistoType==kHelSavePattern)
    {
      values[index++] = fHelicityActual;
      values[index++] = fActualPatternPolarity;
      values[index++] = fPreviousPatternPolarity;
      values[index++] = fDelayedPatternPolarity;
      values[index++] = fPatternNumber;
      values[index++] = fPatternSeed;
      for (size_t i=0; i<fWord.size(); i++){
        values[index++] = fWord[i].fValue;
      }
    }
  return;
}

#ifdef __USE_DATABASE__
void  QwHelicityDecoder::FillDB(QwParityDB *db, TString type)
{
  if (type=="yield" || type=="asymmetry")
    return;

  db->Connect();
  mysqlpp::Query query = db->Query();

  db->Disconnect();
}


void  QwHelicityDecoder::FillErrDB(QwParityDB *db, TString type)
{
  return;
}
#endif // __USE_DATABASE__

void QwHelicityDecoder::RunPredictor()
{
  Int_t ldebug = kFALSE;

  if(ldebug)  std::cout  << "Entering QwHelicityDecoder::RunPredictor for fEventNumber, " << fEventNumber
			 << ", fPatternNumber, " << fPatternNumber
			 <<  ", and fPatternPhaseNumber, " << fPatternPhaseNumber << std::endl;

    /**Update the random seed if the new event is from a different pattern.
       Check the difference between old pattern number and the new one and
       to see how many patterns we have missed or skipped. Then loop back
       to get the correct pattern polarities.
    */
      if((fPatternPhaseNumber==fMinPatternPhase)&& (fPatternNumber>=0)) {
        iseed_Delayed = fSeed_Reported & 0x7fffffff;

	/** set the polarity of the current pattern to be equal to the reported helicity,*/
	fDelayedPatternPolarity = fHelicityReported;
	QwDebug << "QwHelicityDecoder:: CollectRandBits30:  delayedpatternpolarity =" << fDelayedPatternPolarity << QwLog::endl;

	/** then use it as the delayed helicity, */
	fHelicityDelayed = fDelayedPatternPolarity;

	/**if the helicity is delayed by a positive number of patterns then loop the delayed ranseed backward to get the ranseed
	   for the actual helicity */
	if(fHelicityDelay >=0){
	  iseed_Actual = iseed_Delayed;
	  for(Int_t i=0; i<fHelicityDelay; i++) {
	    /**, get the pattern polarity for the actual pattern using that actual ranseed.*/
	    fPreviousPatternPolarity = fActualPatternPolarity;
	    fActualPatternPolarity = GetRandbit30(iseed_Actual);
	  }
	} 
     }
    fHelicityActual  = fActualPatternPolarity ^ fEventPolarity;
    fHelicityDelayed = fDelayedPatternPolarity ^ fEventPolarity;
//std::cout << "printing values : fHelicityDelayed = " << fHelicityDelayed << " : fDelayedPatternPolarity = " << fDelayedPatternPolarity << " : fEventPolarity = " << fEventPolarity << " : reported hel =" << fHelicityReported << " : pattern phase=" << fPatternPhaseNumber << std::endl;

  if(ldebug){
    std::cout << "Predicted Polarity ::: Delayed ="
	      << fDelayedPatternPolarity << " Actual ="
	      << fActualPatternPolarity << "\n";
    std::cout << "Predicted Helicity ::: Delayed Helicity=" << fHelicityDelayed
	      << " Actual Helicity=" << fHelicityActual << " Reported Helicity=" << fHelicityReported << "\n";
    QwError << "Exiting QwHelicityDecoder::RunPredictor " << QwLog::endl;

  }

  return;
}

void QwHelicityDecoder::PredictHelicity()
{
   Bool_t ldebug=kFALSE;

   if(ldebug)  std::cout << "Entering QwHelicityDecoder::PredictHelicity \n";
   if(ldebug)  std::cout << "QwHelicityDecoder::PredictHelicity=>Predicting the  helicity \n";
     RunPredictor();

   if(ldebug)  std::cout << "n_ranbit exiting the function = " << n_ranbits << "\n";

   return;
}



void QwHelicityDecoder::SetHelicityDelay(Int_t delay)
{
  /**Sets the number of bits the helicity reported gets delayed with.
     We predict helicity only if there is a non-zero pattern delay given. */

  if(delay>=0){
    fHelicityDelay = delay;
    if(delay == 0){
      QwWarning << "QwHelicityDecoder : SetHelicityDelay ::  helicity delay is set to 0."
		<< " Disabling helicity predictor and using reported helicity information." 
		<< QwLog::endl;
      fUsePredictor = kFALSE;
    }
    else
      fUsePredictor = kTRUE; 
  }
  else
    QwError << "QwHelicityDecoder::SetHelicityDelay We cannot handle negative delay in the prediction of delayed helicity. Exiting.." << QwLog::endl;

  return;
}


void QwHelicityDecoder::SetHelicityBitPattern(TString hex)
{
  fHelicityBitPattern.clear();
  fHelicityBitPattern = kDefaultHelicityBitPattern;
    /*  // Set the helicity pattern bits
  if (parity(bits) == 0)
    fHelicityBitPattern = bits;
  else QwError << "What, exactly, are you trying to do ?!?!?" << QwLog::endl;
    */
  // Notify the user
  QwMessage << " fPatternBits 0x" ;
  for (int i = fHelicityBitPattern.size()-1;i>=0; i--){
    QwMessage << std::hex << fHelicityBitPattern[i] << " ";
  }
  QwMessage << std::dec << QwLog::endl;
}

VQwSubsystem&  QwHelicityDecoder::operator=  (VQwSubsystem *value)
{
  Bool_t ldebug = kFALSE;
  if(Compare(value))
    {

       //QwHelicityDecoder* input= (QwHelicityDecoder*)value;
      VQwSubsystem::operator=(value);
      QwHelicityDecoder* input= dynamic_cast<QwHelicityDecoder*>(value);

      for(size_t i=0;i<input->fWord.size();i++)
	this->fWord[i].fValue=input->fWord[i].fValue;
      this->fHelicityActual = input->fHelicityActual;
      this->fPatternNumber  = input->fPatternNumber;
      this->fPatternSeed    = input->fPatternSeed;
      this->fPatternPhaseNumber=input->fPatternPhaseNumber;
      this->fEventNumber=input->fEventNumber;
      this->fActualPatternPolarity=input->fActualPatternPolarity;
      this->fDelayedPatternPolarity=input->fDelayedPatternPolarity;
      this->fPreviousPatternPolarity=input->fPreviousPatternPolarity;
      this->fHelicityReported=input->fHelicityReported;
      this->fHelicityActual=input->fHelicityActual;
      this->fHelicityDelayed=input->fHelicityDelayed;
      this->fHelicityBitPlus=input->fHelicityBitPlus;
      this->fHelicityBitMinus=input->fHelicityBitMinus;
      this->fGoodHelicity=input->fGoodHelicity;
      this->fGoodPattern=input->fGoodPattern;
      this->fIgnoreHelicity = input->fIgnoreHelicity;
      this->fEventPolarity = input->fEventPolarity;
      this->fReportedPatternHel = input->fReportedPatternHel;

      this->fErrorFlag = input->fErrorFlag;
      this->fEventNumberFirst     = input->fEventNumberFirst;
      this->fPatternNumberFirst   = input->fPatternNumberFirst;
      this->fNumMissedGates       = input->fNumMissedGates;
      this->fNumMissedEventBlocks = input->fNumMissedEventBlocks;
      this->fNumMultSyncErrors    = input->fNumMultSyncErrors;
      this->fNumHelicityErrors    = input->fNumHelicityErrors;

      if(ldebug){
	std::cout << "QwHelicityDecoder::operator = this->fPatternNumber=" << this->fPatternNumber << std::endl;
	std::cout << "input->fPatternNumber=" << input->fPatternNumber << "\n";
      }
    }

  return *this;
}

VQwSubsystem&  QwHelicityDecoder::operator+=  (VQwSubsystem *value)
{
  //  Bool_t localdebug=kFALSE;
  QwDebug << "Entering QwHelicityDecoder::operator+= adding " << value->GetName() << " to " << this->GetName() << " " << QwLog::endl;

  //this routine is most likely to be called during the computatin of assymetry
  //this call doesn't make too much sense for this class so the following lines
  //are only use to put safe gards testing for example if the two instantiation indeed
  // refers to elements in the same pattern.
  CheckPatternNum(value);
  MergeCounters(value);
  return *this;
}

void QwHelicityDecoder::CheckPatternNum(VQwSubsystem *value)
{
  //  Bool_t localdebug=kFALSE;
  if(Compare(value)) {
    QwHelicityDecoder* input= dynamic_cast<QwHelicityDecoder*>(value);
    QwDebug << "QwHelicityDecoder::MergeCounters: this->fPatternNumber=" << this->fPatternNumber 
	    << ", input->fPatternNumber=" << input->fPatternNumber << QwLog::endl;

    this->fErrorFlag |= input->fErrorFlag;
    
    //  Make sure the pattern number and poalrity agree!
    if(this->fPatternNumber!=input->fPatternNumber)
      this->fPatternNumber=-999999;
    if(this->fActualPatternPolarity!=input->fActualPatternPolarity)
      this->fPatternNumber=-999999;
    if (this->fPatternNumber==-999999){
      this->fErrorFlag |= kErrorFlag_Helicity + kGlobalCut + kEventCutMode3;
    }
  }
}

void QwHelicityDecoder::MergeCounters(VQwSubsystem *value)
{
  //  Bool_t localdebug=kFALSE;
  if(Compare(value)) {
    QwHelicityDecoder* input= dynamic_cast<QwHelicityDecoder*>(value);

    fEventNumber = (fEventNumber == 0) ? input->fEventNumber :
      std::min(fEventNumber, input->fEventNumber);
    for (size_t i=0; i<fWord.size(); i++) {
      fWord[i].fValue =  (fWord[i].fValue == 0) ? input->fWord[i].fValue :
	std::min( fWord[i].fValue, input->fWord[i].fValue);
    }
  }
}

void QwHelicityDecoder::Ratio(VQwSubsystem  *value1, VQwSubsystem  *value2)
{
  // this is stub function defined here out of completion and uniformity between each subsystem
  *this =  value1;
  CheckPatternNum(value2);
  MergeCounters(value2);
}

void  QwHelicityDecoder::AccumulateRunningSum(VQwSubsystem* value, Int_t count, Int_t ErrorMask){
  if (Compare(value)) {
    MergeCounters(value);
    QwHelicityDecoder* input = dynamic_cast<QwHelicityDecoder*>(value);
    fPatternNumber = (fPatternNumber <=0 ) ? input->fPatternNumber :
      std::min(fPatternNumber, input->fPatternNumber);
    //  Keep track of the various error quantities, so we can print 
    //  them at the end.
    fNumMissedGates       = input->fNumMissedGates;
    fNumMissedEventBlocks = input->fNumMissedEventBlocks;
    fNumMultSyncErrors    = input->fNumMultSyncErrors;
    fNumHelicityErrors    = input->fNumHelicityErrors;
  }
}

Bool_t QwHelicityDecoder::Compare(VQwSubsystem *value)
{
  Bool_t res=kTRUE;
  if(typeid(*value)!=typeid(*this)) {
    res=kFALSE;
  } else {
    QwHelicityDecoder* input= dynamic_cast<QwHelicityDecoder*>(value);
    if(input->fWord.size()!=fWord.size()) {
      res=kFALSE;
    }
  }
  return res;
}
