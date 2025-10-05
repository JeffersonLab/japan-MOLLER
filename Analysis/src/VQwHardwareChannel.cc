/**
 * VQwHardwareChannel.cc
 *
 * Base implementation for hardware channels providing common functionality:
 * constructors, option processing, single-event cuts, database interfaces,
 * and tree branch construction with module list filtering. Used by all
 * concrete channel types. Documentation-only edits; runtime behavior unchanged.
 */

// Qweak database headers
#include "QwLog.h"
#ifdef __USE_DATABASE__
#include "QwDBInterface.h"
#endif
#include "QwParameterFile.h"
#include "QwOptions.h"

Int_t VQwHardwareChannel::fBurpHoldoff = 10;

/** Default constructor: initialize limits, error flags, and process options. */
VQwHardwareChannel::VQwHardwareChannel():
  fNumberOfDataWords(0),
  fNumberOfSubElements(0),
  fDataToSave(kRaw)
{
  fULimit = -1;
  fLLimit = 1;
  fErrorFlag = 0;
  fErrorConfigFlag = 0;
  fBurpThreshold = -1.0;
}

/** Copy constructor: duplicate all channel state and configuration. */
VQwHardwareChannel::VQwHardwareChannel(const VQwHardwareChannel& value)
  :VQwDataElement(value),
   fNumberOfDataWords(value.fNumberOfDataWords),
   fNumberOfSubElements(value.fNumberOfSubElements),
   fDataToSave(value.fDataToSave),
   fTreeArrayIndex(value.fTreeArrayIndex),
   fTreeArrayNumEntries(value.fTreeArrayNumEntries),
   fPedestal(value.fPedestal),
   fCalibrationFactor(value.fCalibrationFactor),
   kFoundPedestal(value.kFoundPedestal),
   kFoundGain(value.kFoundGain),
   bEVENTCUTMODE(value.bEVENTCUTMODE),
   fULimit(value.fULimit),
   fLLimit(value.fLLimit),
   fStability(value.fStability),
   fBurpThreshold(value.fBurpThreshold),
   fBurpCountdown(value.fBurpCountdown)
{
}

/** Copy constructor with data-to-save override. */
VQwHardwareChannel::VQwHardwareChannel(const VQwHardwareChannel& value, VQwDataElement::EDataToSave datatosave)
  :VQwDataElement(value),
   fNumberOfDataWords(value.fNumberOfDataWords),
   fNumberOfSubElements(value.fNumberOfSubElements),
   fDataToSave(datatosave),
   fTreeArrayIndex(value.fTreeArrayIndex),
   fTreeArrayNumEntries(value.fTreeArrayNumEntries),
   fPedestal(value.fPedestal),
   fCalibrationFactor(value.fCalibrationFactor),
   kFoundPedestal(value.kFoundPedestal),
   kFoundGain(value.kFoundGain),
   bEVENTCUTMODE(value.bEVENTCUTMODE),
   fULimit(value.fULimit),
   fLLimit(value.fLLimit),
   fStability(value.fStability),
   fBurpThreshold(value.fBurpThreshold),
   fBurpCountdown(value.fBurpCountdown)
{
}

/** Copy all state from another hardware channel instance. */
void VQwHardwareChannel::CopyFrom(const VQwHardwareChannel& value)
{
  VQwDataElement::CopyFrom(value);
  fNumberOfDataWords = value.fNumberOfDataWords;
  fNumberOfSubElements = value.fNumberOfSubElements;
  fDataToSave = value.fDataToSave;
  fTreeArrayIndex = value.fTreeArrayIndex;
  fTreeArrayNumEntries = value.fTreeArrayNumEntries;
  fPedestal = value.fPedestal;
  fCalibrationFactor = value.fCalibrationFactor;
  kFoundPedestal = value.kFoundPedestal;
  kFoundGain = value.kFoundGain;
  bEVENTCUTMODE = value.bEVENTCUTMODE;
  fULimit = value.fULimit;
  fLLimit = value.fLLimit;
  fStability = value.fStability;
  fBurpThreshold = value.fBurpThreshold;
  fBurpCountdown = value.fBurpCountdown;
}

/** Configure upper and lower limits for single-event cuts. */
void VQwHardwareChannel::SetSingleEventCuts(Double_t min, Double_t max)
{
  fULimit=max;
  fLLimit=min;
}

/**
 * Configure comprehensive single-event cuts with error flags, stability, and
 * burp detection thresholds.
 */
void VQwHardwareChannel::SetSingleEventCuts(UInt_t errorflag,Double_t min, Double_t max, Double_t stability, Double_t BurpLevel)
{
  //QwError<<"***************************inside VQwHardwareChannel, BurpLevel = "<<BurpLevel<<QwLog::endl;
  fErrorConfigFlag=errorflag;
  fStability=stability;
  fBurpThreshold=BurpLevel;
  SetSingleEventCuts(min,max);
  QwMessage << "Set single event cuts for " << GetElementName() << ": "
      << "Config-error-flag == 0x" << std::hex << errorflag << std::dec
      << ", global? " << ((fErrorConfigFlag & kGlobalCut)==kGlobalCut) << ", stability? " << ((fErrorConfigFlag & kStabilityCut)==kStabilityCut)<<" cut "<<fStability << ", burpcut  " << fBurpThreshold << QwLog::endl;
}

/**
 * Build database interface rows for all subelements of this channel.
 */
#ifdef __USE_DATABASE__
void VQwHardwareChannel::AddEntriesToList(std::vector<QwDBInterface> &row_list)
{
  QwDBInterface row;
  TString name    = GetElementName();
  UInt_t  entries = GetGoodEventCount();
  //  Loop over subelements and build the list.
  for(UInt_t subelement=0; 
      subelement<GetNumberOfSubelements();
      subelement++) {
    row.Reset();
    row.SetDetectorName(name);
    row.SetSubblock(subelement);
    row.SetN(entries);
    row.SetValue(GetValue(subelement));
    row.SetError(GetValueError(subelement));
    row_list.push_back(row);
  }
}
#endif // __USE_DATABASE__

/**
 * Conditionally construct tree branch if this channel name appears in the
 * module list filter.
 */
void VQwHardwareChannel::ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist){
  if (GetElementName()!=""){
    TString devicename;
    devicename=GetElementName();
    devicename.ToLower();
    if (modulelist.HasValue(devicename)){
      ConstructBranch(tree,prefix);
    }
  }
}
