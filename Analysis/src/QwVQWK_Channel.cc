#include "QwVQWK_Channel.h"
 
// System headers
#include <stdexcept>

// Qweak headers
#include "QwLog.h"
#include "QwUnits.h"
#include "QwRNTupleFile.h"
#include "QwBlinder.h"
#include "QwHistogramHelper.h"
#ifdef __USE_DATABASE__
#include "QwDBInterface.h"
#endif

const Bool_t QwVQWK_Channel::kDEBUG = kFALSE;

const Int_t  QwVQWK_Channel::kWordsPerChannel = 6;
const Int_t  QwVQWK_Channel::kMaxChannels     = 8;

const Double_t QwVQWK_Channel::kTimePerSample = (2.0/30.0) * Qw::us; //2.0 originally

/*!  Conversion factor to translate the average bit count in an ADC
 *   channel into average voltage.
 *   The base factor is roughly 76 uV per count, and zero counts corresponds
 *   to zero voltage.
 *   Store as the exact value for 20 V range, 18 bit ADC.
 */
const Double_t QwVQWK_Channel::kVQWK_VoltsPerBit = (20./(1<<18));

/*!  Static member function to return the word offset within a data buffer
 *   given the module number index and the channel number index.
 *   @param moduleindex   Module index within this buffer; counts from zero
 *   @param channelindex  Channel index within this module; counts from zero
 *   @return   The number of words offset to the beginning of this
 *             channel's data from the beginning of the VQWK buffer.
 */
Int_t QwVQWK_Channel::GetBufferOffset(Int_t moduleindex, Int_t channelindex){
    Int_t offset = -1;
    if (moduleindex<0 ){
      QwError << "QwVQWK_Channel::GetBufferOffset:  Invalid module index,"
              << moduleindex
              << ".  Must be zero or greater."
              << QwLog::endl;
    } else if (channelindex<0 || channelindex>kMaxChannels){
      QwError << "QwVQWK_Channel::GetBufferOffset:  Invalid channel index,"
              << channelindex
              << ".  Must be in range [0," << kMaxChannels << "]."
              << QwLog::endl;
    } else {
      offset = ( (moduleindex * kMaxChannels) + channelindex )
        * kWordsPerChannel;
    }
    return offset;
  }


/********************************************************/
Int_t QwVQWK_Channel::ApplyHWChecks()
{
  Bool_t fEventIsGood=kTRUE;
  Bool_t bStatus;
  if (bEVENTCUTMODE>0){//Global switch to ON/OFF event cuts set at the event cut file

    if (bDEBUG)
      QwWarning<<" QwQWVK_Channel "<<GetElementName()<<"  "<<GetNumberOfSamples()<<QwLog::endl;

    // Sample size check
    bStatus = MatchNumberOfSamples(fNumberOfSamples_map);//compare the default sample size with no.of samples read by the module
    if (!bStatus) {
      fErrorFlag |= kErrorFlag_sample;
    }

    // Check SW and HW return the same sum
    bStatus = (GetRawHardwareSum() == GetRawSoftwareSum());
    //fEventIsGood = bStatus;
    if (!bStatus) {
      fErrorFlag |= kErrorFlag_SW_HW;
    }



    //check sequence number
    fSequenceNo_Prev++;
    if (fSequenceNo_Counter==0 || GetSequenceNumber()==0){//starting the data run
      fSequenceNo_Prev=GetSequenceNumber();
    }

    if (!MatchSequenceNumber(fSequenceNo_Prev)){//we have a sequence number error
      fEventIsGood=kFALSE;
      fErrorFlag|=kErrorFlag_Sequence;
      if (bDEBUG)       QwWarning<<" QwQWVK_Channel "<<GetElementName()<<" Sequence number previous value = "<<fSequenceNo_Prev<<" Current value= "<< GetSequenceNumber()<<QwLog::endl;
    }

    fSequenceNo_Counter++;

    //Checking for HW_sum is returning same value.
    if (fPrev_HardwareBlockSum != GetRawHardwareSum()){
      //std::cout<<" BCM hardware sum is different  "<<std::endl;
      fPrev_HardwareBlockSum = GetRawHardwareSum();
      fADC_Same_NumEvt=0;
    }else
      fADC_Same_NumEvt++;//hw_sum is same increment the counter

    //check for the hw_sum is giving the same value
    if (fADC_Same_NumEvt>0){//we have ADC stuck with same value
      if (bDEBUG) QwWarning<<" BCM hardware sum is same for more than  "<<fADC_Same_NumEvt<<" time consecutively  "<<QwLog::endl;
      fErrorFlag|=kErrorFlag_SameHW;
    }

    //check for the hw_sum is zero
    if (GetRawHardwareSum()==0){
      fErrorFlag|=kErrorFlag_ZeroHW;
    }
    if (!fEventIsGood)    
      fSequenceNo_Counter=0;//resetting the counter after ApplyHWChecks() a failure

    if ((TMath::Abs(GetRawHardwareSum())*kVQWK_VoltsPerBit/fNumberOfSamples) > GetVQWKSaturationLimt()){
      if (bDEBUG) 
        QwWarning << this->GetElementName()<<" "<<GetRawHardwareSum() << "Saturating VQWK invoked! " <<TMath::Abs(GetRawHardwareSum())*kVQWK_VoltsPerBit/fNumberOfSamples<<" Limit "<<GetVQWKSaturationLimt() << QwLog::endl;
      fErrorFlag|=kErrorFlag_VQWK_Sat; 
    }

  }
  else {
    fGoodEventCount = 1;
    fErrorFlag = 0;
  }

  return fErrorFlag;
}


/********************************************************/
void QwVQWK_Channel::IncrementErrorCounters(){
  if ( (kErrorFlag_sample &  fErrorFlag)==kErrorFlag_sample)
    fErrorCount_sample++; //increment the hw error counter
  if ( (kErrorFlag_SW_HW &  fErrorFlag)==kErrorFlag_SW_HW)
    fErrorCount_SW_HW++; //increment the hw error counter
  if ( (kErrorFlag_Sequence &  fErrorFlag)==kErrorFlag_Sequence)
    fErrorCount_Sequence++; //increment the hw error counter
  if ( (kErrorFlag_SameHW &  fErrorFlag)==kErrorFlag_SameHW)
    fErrorCount_SameHW++; //increment the hw error counter
  if ( (kErrorFlag_ZeroHW &  fErrorFlag)==kErrorFlag_ZeroHW)
    fErrorCount_ZeroHW++; //increment the hw error counter
  if ( (kErrorFlag_VQWK_Sat &  fErrorFlag)==kErrorFlag_VQWK_Sat)
    fErrorCount_HWSat++; //increment the hw saturation error counter
  if ( ((kErrorFlag_EventCut_L &  fErrorFlag)==kErrorFlag_EventCut_L) 
       || ((kErrorFlag_EventCut_U &  fErrorFlag)==kErrorFlag_EventCut_U)){
    fNumEvtsWithEventCutsRejected++; //increment the event cut error counter
  }
}

/********************************************************/

void QwVQWK_Channel::InitializeChannel(TString name, TString datatosave)
{
  SetElementName(name);
  SetDataToSave(datatosave);
  SetNumberOfDataWords(6);
  SetNumberOfSubElements(5);

  kFoundPedestal = 0;
  kFoundGain = 0;

  fPedestal            = 0.0;
  fCalibrationFactor   = 1.0;

  fBlocksPerEvent      = 4;

  fTreeArrayIndex      = 0;
  fTreeArrayNumEntries = 0;

  ClearEventData();

  fPreviousSequenceNumber = 0;
  fNumberOfSamples_map    = 0;
  fNumberOfSamples        = 0;

  // Use internal random variable by default
  fUseExternalRandomVariable = false;

  // Mock drifts
  fMockDriftAmplitude.clear();
  fMockDriftFrequency.clear();
  fMockDriftPhase.clear();

  // Mock asymmetries
  fMockAsymmetry     = 0.0;
  fMockGaussianMean  = 0.0;
  fMockGaussianSigma = 0.0;

  // Event cuts
  fULimit=-1;
  fLLimit=1;
  fNumEvtsWithEventCutsRejected = 0;

  fErrorFlag=0;               //Initialize the error flag
  fErrorConfigFlag=0;         //Initialize the error config. flag

  //init error counters//
  fErrorCount_sample     = 0;
  fErrorCount_SW_HW      = 0;
  fErrorCount_Sequence   = 0;
  fErrorCount_SameHW     = 0;
  fErrorCount_ZeroHW     = 0;
  fErrorCount_HWSat      = 0;

  fADC_Same_NumEvt       = 0;
  fSequenceNo_Prev       = 0;
  fSequenceNo_Counter    = 0;
  fPrev_HardwareBlockSum = 0.0;

  fGoodEventCount        = 0;

  bEVENTCUTMODE          = 0;

  //std::cout<< "name = "<<name<<" error count same _HW = "<<fErrorCount_SameHW <<std::endl;
  return;
}

/********************************************************/

void QwVQWK_Channel::InitializeChannel(TString subsystem, TString instrumenttype, TString name, TString datatosave){
  InitializeChannel(name,datatosave);
  SetSubsystemName(subsystem);
  SetModuleType(instrumenttype);
  //PrintInfo();
}

void QwVQWK_Channel::LoadChannelParameters(QwParameterFile &paramfile){
  UInt_t value = 0;
  if (paramfile.ReturnValue("sample_size",value)){
    SetDefaultSampleSize(value);
  } else {
    QwWarning << "VQWK Channel "
              << GetElementName()
              << " cannot set the default sample size."
              << QwLog::endl;
  }
};


void QwVQWK_Channel::ClearEventData()
{
  for (Int_t i = 0; i < fBlocksPerEvent; i++) {
    fBlock_raw[i] = 0;
    fBlock[i] = 0.0;
    fBlockM2[i] = 0.0;
    fBlockError[i] = 0.0;
  }
  fHardwareBlockSum_raw = 0;
  fSoftwareBlockSum_raw = 0;
  fHardwareBlockSum   = 0.0;
  fHardwareBlockSumM2 = 0.0;
  fHardwareBlockSumError = 0.0;
  fSequenceNumber   = 0;
  fNumberOfSamples  = 0;
  fGoodEventCount   = 0;
  fErrorFlag=0;
  return;
}

void QwVQWK_Channel::RandomizeEventData(int helicity, double time)
{
  //std::cout << "In channel: " << GetElementName() << std::endl;
  // The blocks are assumed to be independent measurements
  // Double_t* block = new Double_t[fBlocksPerEvent];
  //  Double_t sqrt_fBlocksPerEvent = 0.0;
  // sqrt_fBlocksPerEvent = sqrt(fBlocksPerEvent);

  // Calculate drift (if time is not specified, it stays constant at zero)
  //Double_t drift = 0.0;
  //for (UInt_t i = 0; i < fMockDriftFrequency.size(); i++) {
  //  drift += fMockDriftAmplitude[i] * sin(2.0 * Qw::pi * fMockDriftFrequency[i] * time + fMockDriftPhase[i]);
  //}

  // updated to calculate the drift for each block individually
  Double_t drift = 0.0;
  for (Int_t i = 0; i < fBlocksPerEvent; i++){
    drift = 0.0;
    if (i >= 1){
      time += (fNumberOfSamples_map/4.0)*kTimePerSample;
    }
    for (UInt_t i = 0; i < fMockDriftFrequency.size(); i++) {
      drift += fMockDriftAmplitude[i] * sin(2.0 * Qw::pi * fMockDriftFrequency[i] * time + fMockDriftPhase[i]);
      //std::cout << "Drift: " << drift << std::endl;
    }
  }

  // Calculate signal
  fHardwareBlockSum = 0.0;
  fHardwareBlockSumM2 = 0.0; // second moment is zero for single events

  for (Int_t i = 0; i < fBlocksPerEvent; i++) {
    double tmpvar = GetRandomValue();
    //std::cout << "tmpvar: " << tmpvar << std::endl;
    //std::cout << "->fMockSigma: " << fMockGaussianSigma << std::endl;

    fBlock[i] = fMockGaussianMean + drift;
    //std::cout << "(Start of loop)  " << this->GetElementName() << "-> "<< "fBlock[" << i << "]: " << fBlock[i] << ", Drift: " << drift <<", Mean: " << fMockGaussianMean<<  std::endl;
    if (fCalcMockDataAsDiff) {
      fBlock[i] += helicity*fMockAsymmetry;
    } else {
      fBlock[i] *= 1.0 + helicity*fMockAsymmetry;
    }
    fBlock[i] += fMockGaussianSigma*tmpvar*sqrt(fBlocksPerEvent);
    //std::cout << "(End of loop)    " << this->GetElementName() << "-> "<< "fBlock[" << i << "]: " << fBlock[i] << ", Drift: " << drift <<", Mean: " << fMockGaussianMean<<  std::endl;

    
/*    
    fBlock[i] = //GetRandomValue();
     fMockGaussianMean * (1 + helicity * fMockAsymmetry) 
      + fMockGaussianSigma*sqrt(fBlocksPerEvent) * tmpvar
      + drift; */

    fBlockM2[i] = 0.0; // second moment is zero for single events
    fHardwareBlockSum += fBlock[i];

/*        std::cout << "In RandomizeEventData: " << tmpvar << " " << fMockGaussianMean << " "<<  (1 + helicity * fMockAsymmetry) << " "
                  << fMockGaussianSigma << " " << fMockGaussianSigma*tmpvar << " "
                  << drift << " " << block[i] << std::endl; */
  }
  fHardwareBlockSum /= fBlocksPerEvent;
  fSequenceNumber = 0;
  fNumberOfSamples = fNumberOfSamples_map;
  //  SetEventData(block);
  //  delete block;
  return;
}

void QwVQWK_Channel::SmearByResolution(double resolution){

  fHardwareBlockSum   = 0.0;
  fHardwareBlockSumM2 = 0.0; // second moment is zero for single events
  for (Int_t i = 0; i < fBlocksPerEvent; i++) {
    // std::cout << i << " " << fBlock[i] << "->";
    //std::cout << "resolution = " << resolution << "\t for channel \t" << GetElementName() << std::endl;
    fBlock[i] += resolution*sqrt(fBlocksPerEvent) * GetRandomValue();
    // std::cout << fBlock[i] << ": ";
    fBlockM2[i] = 0.0; // second moment is zero for single events
    fHardwareBlockSum += fBlock[i];
  }
  // std::cout << std::endl;
  fHardwareBlockSum /= fBlocksPerEvent;

  fNumberOfSamples = fNumberOfSamples_map;
  // SetRawEventData();
  return;
}

void QwVQWK_Channel::SetHardwareSum(Double_t hwsum, UInt_t sequencenumber)
{
  Double_t* block = new Double_t[fBlocksPerEvent];
  for (Int_t i = 0; i < fBlocksPerEvent; i++){
    block[i] = hwsum / fBlocksPerEvent;
  }
  SetEventData(block);
  delete block;
  return;
}


// SetEventData() is used by the mock data generator to turn "model"
// data values into their equivalent raw data.  It should be used
// nowhere else.  -- pking, 2010-09-16

void QwVQWK_Channel::SetEventData(Double_t* block, UInt_t sequencenumber)
{
  fHardwareBlockSum = 0.0;
  fHardwareBlockSumM2 = 0.0; // second moment is zero for single events
  for (Int_t i = 0; i < fBlocksPerEvent; i++) {
    fBlock[i] = block[i];
    fBlockM2[i] = 0.0; // second moment is zero for single events
    fHardwareBlockSum += block[i];
  }
  fHardwareBlockSum /= fBlocksPerEvent;

  fSequenceNumber = sequencenumber;
  fNumberOfSamples = fNumberOfSamples_map;

//  Double_t thispedestal = 0.0;
//  thispedestal = fPedestal * fNumberOfSamples;

  SetRawEventData();
  return;
}

void QwVQWK_Channel::SetRawEventData(){
  fNumberOfSamples = fNumberOfSamples_map;
  fHardwareBlockSum_raw = 0;
//  Double_t hwsum_test = 0.0;
//  std::cout <<  "*******In QwVQWK_Channel::SetRawEventData for channel:\t" << this->GetElementName() << std::endl;
  for (Int_t i = 0; i < fBlocksPerEvent; i++) 
    {
     //  The raw data is decoded ino calibrated values with the following (in ProcessEvent()):
     //      fBlock[i] = fCalibrationFactor * ( (1.0 * fBlock_raw[i] * fBlocksPerEvent / fNumberOfSamples) - fPedestal );
     //  We should invert that function here:
/*     if (fBlock[i]<-10.0 || fBlock[i]>+10.0)
        QwError << "In QwVQWK_Channel::SetRawEventData for channel:\t" << this->GetElementName() << ", Block " << i << " is out of range (-10 V,+10V):" << fBlock[i] << QwLog::endl;
*/
     fBlock_raw[i] = Int_t((fBlock[i] / fCalibrationFactor + fPedestal) * fNumberOfSamples / (fBlocksPerEvent * 1.0));
     fHardwareBlockSum_raw += fBlock_raw[i];
     //hwsum_test +=fBlock[i] /(fBlocksPerEvent * 1.0);

     
  //   fBlock[i] = fCalibrationFactor * ((1.0 * fBlock_raw[i] * fBlocksPerEvent / fNumberOfSamples) - fPedestal);
  //   fHardwareBlockSum += fBlock[i];

  /*    std::cout << "\t fBlock[i] = "                                        << std::setprecision(6) << fBlock[i]                                                                                 << "\n"
               << "\t fCalibrationFactor = "                               << fCalibrationFactor                                                                        << "\n"
               << "\t fPedestal = "                                        << fPedestal                                                                                 << "\n"
               << "\t fNumberOfSamples = "                                 << fNumberOfSamples                                                                          << "\n"
               << "\t fBlocksPerEvent = "                                  << fBlocksPerEvent                                                                           << "\n"
               << "\t fBlock[i] / fCalibrationFactor + fPedestal = "       << fBlock[i] / fCalibrationFactor + fPedestal                                                << "\n"
               << "\t That * fNumberOfSamples / (fBlocksPerEvent * 1) = "  << (fBlock[i] / fCalibrationFactor + fPedestal) * fNumberOfSamples / (fBlocksPerEvent * 1.0) << "\n"
               << "\t fBlock_raw[i] = "                                    << fBlock_raw[i]                                                                             << "\n"
               << "\t fHardwareBlockSum_raw = "                            << fHardwareBlockSum_raw                                                                     << "\n"
               << std::endl;
   */   
    }

/*  std::cout << "fBlock[0] = " << std::setprecision(16) << fBlock[0] << std::endl
            << "fBlockraw[0] after calib: " << fCalibrationFactor * ((1.0 * fBlock_raw[0] * fBlocksPerEvent / fNumberOfSamples) - fPedestal) << std::endl;

  std::cout << "fHardwareBlockSum = " << std::setprecision(8) << fHardwareBlockSum << std::endl;
  std::cout << "hwsum_test = " << std::setprecision(8) << hwsum_test << std::endl;
  std::cout << "fHardwareBlockSum_raw = " << std::setprecision(8) << fHardwareBlockSum_raw << std::endl;
  std::cout << "fHardwareBlockSum_Raw after calibration = " << fCalibrationFactor * ((1.0 * fHardwareBlockSum_raw / fNumberOfSamples) - fPedestal) << std::endl;
*/

  fSoftwareBlockSum_raw = fHardwareBlockSum_raw;

  return;
}

void QwVQWK_Channel::EncodeEventData(std::vector<UInt_t> &buffer)
{
  Long_t localbuf[6] = {0};

  if (IsNameEmpty()) {
    //  This channel is not used, but is present in the data stream.
    //  Skip over this data.
  } else {
    //    localbuf[4] = 0;
    for (Int_t i = 0; i < 4; i++) {
      localbuf[i] = fBlock_raw[i];
      //        localbuf[4] += localbuf[i]; // fHardwareBlockSum_raw
    }
    // The following causes many rounding errors and skips due to the check
    // that fHardwareBlockSum_raw == fSoftwareBlockSum_raw in IsGoodEvent().
    localbuf[4] = fHardwareBlockSum_raw;
    localbuf[5] = (fNumberOfSamples << 16 & 0xFFFF0000)
                | (fSequenceNumber  << 8  & 0x0000FF00);

    for (Int_t i = 0; i < 6; i++){
        buffer.push_back(localbuf[i]);
    }
  }
}



Int_t QwVQWK_Channel::ProcessEvBuffer(UInt_t* buffer, UInt_t num_words_left, UInt_t index)
{
  UInt_t words_read = 0;
  UInt_t localbuf[kWordsPerChannel] = {0};
  // The conversion from UInt_t to Double_t discards the sign, so we need an intermediate
  // static_cast from UInt_t to Int_t.
  Int_t localbuf_signed[kWordsPerChannel] = {0};

  if (IsNameEmpty()){
    //  This channel is not used, but is present in the data stream.
    //  Skip over this data.
    words_read = fNumberOfDataWords;
  } else if (num_words_left >= fNumberOfDataWords)
    {
      for (Int_t i=0; i<kWordsPerChannel; i++){
        localbuf[i] = buffer[i];
        localbuf_signed[i] = static_cast<Int_t>(localbuf[i]);
      }

      fSoftwareBlockSum_raw = 0;
      for (Int_t i=0; i<fBlocksPerEvent; i++){
        fBlock_raw[i] = localbuf_signed[i];
        fSoftwareBlockSum_raw += fBlock_raw[i];
      }
      fHardwareBlockSum_raw = localbuf_signed[4];

      /*  Permanent change in the structure of the 6th word of the ADC readout.
       *  The upper 16 bits are the number of samples, and the upper 8 of the
       *  lower 16 are the sequence number.  This matches the structure of
       *  the ADC readout in block read mode, and now also in register read mode.
       *  P.King, 2007sep04.
       */
      fSequenceNumber   = (localbuf[5]>>8)  & 0xFF;
      fNumberOfSamples  = (localbuf[5]>>16) & 0xFFFF;

      words_read = fNumberOfDataWords;

    } else
      {
        std::cerr << "QwVQWK_Channel::ProcessEvBuffer: Not enough words!"
                  << std::endl;
      }
  return words_read;
}



void QwVQWK_Channel::ProcessEvent()
{
  if (fNumberOfSamples == 0 && fHardwareBlockSum_raw == 0) {
    //  There isn't valid data for this channel.  Just flag it and
    //  move on.
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlock[i] = 0.0;
      fBlockM2[i] = 0.0;
    }
    fHardwareBlockSum = 0.0;
    fHardwareBlockSumM2 = 0.0;
    fErrorFlag |= kErrorFlag_sample;
  } else if (fNumberOfSamples == 0) {
    //  This is probably a more serious problem.
    QwWarning << "QwVQWK_Channel::ProcessEvent:  Channel "
              << this->GetElementName().Data()
              << " has fNumberOfSamples==0 but has valid data in the hardware sum.  "
              << "Flag this as an error."
              << QwLog::endl;
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlock[i] = 0.0;
      fBlockM2[i] = 0.0;
    }
    fHardwareBlockSum = 0.0;
    fHardwareBlockSumM2 = 0.0;
    fErrorFlag|=kErrorFlag_sample;
  } else {
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlock[i] = fCalibrationFactor * ( (1.0 * fBlock_raw[i] * fBlocksPerEvent / fNumberOfSamples) - fPedestal );
      fBlockM2[i] = 0.0; // second moment is zero for single events
    }
    fHardwareBlockSum = fCalibrationFactor * ( (1.0 * fHardwareBlockSum_raw / fNumberOfSamples) - fPedestal );
    fHardwareBlockSumM2 = 0.0; // second moment is zero for single events
  }
  return;
}

Double_t QwVQWK_Channel::GetAverageVolts() const
{
  //Double_t avgVolts = (fBlock[0]+fBlock[1]+fBlock[2]+fBlock[3])*kVQWK_VoltsPerBit/fNumberOfSamples;
  Double_t avgVolts = fHardwareBlockSum * kVQWK_VoltsPerBit / fNumberOfSamples;
  //std::cout<<"QwVQWK_Channel::GetAverageVolts() = "<<avgVolts<<std::endl;
  return avgVolts;

}

void QwVQWK_Channel::PrintInfo() const
{
  std::cout<<"***************************************"<<"\n";
  std::cout<<"Subsystem "<<GetSubsystemName()<<"\n"<<"\n";
  std::cout<<"Beam Instrument Type: "<<GetModuleType()<<"\n"<<"\n";
  std::cout<<"QwVQWK channel: "<<GetElementName()<<"\n"<<"\n";
  std::cout<<"fPedestal= "<< fPedestal<<"\n";
  std::cout<<"fCalibrationFactor= "<<fCalibrationFactor<<"\n";
  std::cout<<"fBlocksPerEvent= "<<fBlocksPerEvent<<"\n"<<"\n";
  std::cout<<"fSequenceNumber= "<<fSequenceNumber<<"\n";
  std::cout<<"fNumberOfSamples= "<<fNumberOfSamples<<"\n";
  std::cout<<"fBlock_raw ";

  for (Int_t i = 0; i < fBlocksPerEvent; i++)
    std::cout << " : " << fBlock_raw[i];
  std::cout<<"\n";
  std::cout<<"fHardwareBlockSum_raw= "<<fHardwareBlockSum_raw<<"\n";
  std::cout<<"fSoftwareBlockSum_raw= "<<fSoftwareBlockSum_raw<<"\n";
  std::cout<<"fBlock ";
  for (Int_t i = 0; i < fBlocksPerEvent; i++)
    std::cout << " : " <<std::setprecision(8) << fBlock[i];
  std::cout << std::endl;

  std::cout << "fHardwareBlockSum = "<<std::setprecision(8) <<fHardwareBlockSum << std::endl;
  std::cout << "fHardwareBlockSumM2 = "<<fHardwareBlockSumM2 << std::endl;
  std::cout << "fHardwareBlockSumError = "<<fHardwareBlockSumError << std::endl;

  return;
}

void  QwVQWK_Channel::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  //  If we have defined a subdirectory in the ROOT file, then change into it.
  if (folder != NULL) folder->cd();

  if (IsNameEmpty()){
    //  This channel is not used, so skip filling the histograms.
  } else {
    //  Now create the histograms.
    SetDataToSaveByPrefix(prefix);

    TString basename = prefix + GetElementName();

    if(fDataToSave==kRaw)
      {
        fHistograms.resize(8+2+1, NULL);
        size_t index=0;
        for (Int_t i=0; i<fBlocksPerEvent; i++){
          fHistograms[index]   = gQwHists.Construct1DHist(basename+Form("_block%d_raw",i));
          fHistograms[index+1] = gQwHists.Construct1DHist(basename+Form("_block%d",i));
          index += 2;
        }
        fHistograms[index]   = gQwHists.Construct1DHist(basename+Form("_hw_raw"));
        fHistograms[index+1] = gQwHists.Construct1DHist(basename+Form("_hw"));
        index += 2;
        fHistograms[index]   = gQwHists.Construct1DHist(basename+Form("_sw-hw_raw"));
      }
    else if(fDataToSave==kDerived)
      {
        fHistograms.resize(4+1+1, NULL);
        Int_t index=0;
        for (Int_t i=0; i<fBlocksPerEvent; i++){
          fHistograms[index] = gQwHists.Construct1DHist(basename+Form("_block%d",i));
          index += 1;
        }
        fHistograms[index] = gQwHists.Construct1DHist(basename+Form("_hw"));
        index += 1;
        fHistograms[index] = gQwHists.Construct1DHist(basename+Form("_dev_err"));
        index += 1;
      }
    else
      {
        // this is not recognized
      }

  }
}

void  QwVQWK_Channel::FillHistograms()
{
  Int_t index=0;
  
  if (IsNameEmpty())
    {
      //  This channel is not used, so skip creating the histograms.
    } else
      {
        if(fDataToSave==kRaw)
          {
            for (Int_t i=0; i<fBlocksPerEvent; i++)
              {
                if (fHistograms[index] != NULL && (fErrorFlag)==0)
                  fHistograms[index]->Fill(this->GetRawBlockValue(i));
                if (fHistograms[index+1] != NULL && (fErrorFlag)==0)
                  fHistograms[index+1]->Fill(this->GetBlockValue(i));
                index+=2;
              }
            if (fHistograms[index] != NULL && (fErrorFlag)==0)
              fHistograms[index]->Fill(this->GetRawHardwareSum());
            if (fHistograms[index+1] != NULL && (fErrorFlag)==0)
              fHistograms[index+1]->Fill(this->GetHardwareSum());
            index+=2;
            if (fHistograms[index] != NULL && (fErrorFlag)==0)
              fHistograms[index]->Fill(this->GetRawSoftwareSum()-this->GetRawHardwareSum());
          }
        else if(fDataToSave==kDerived)
          {
            for (Int_t i=0; i<fBlocksPerEvent; i++)
              {
                if (fHistograms[index] != NULL && (fErrorFlag)==0)
                  fHistograms[index]->Fill(this->GetBlockValue(i));
                index+=1;
              }
            if (fHistograms[index] != NULL && (fErrorFlag)==0)
              fHistograms[index]->Fill(this->GetHardwareSum());
            index+=1; 
            if (fHistograms[index] != NULL){
              if ( (kErrorFlag_sample &  fErrorFlag)==kErrorFlag_sample)
                fHistograms[index]->Fill(kErrorFlag_sample);
              if ( (kErrorFlag_SW_HW &  fErrorFlag)==kErrorFlag_SW_HW)
                fHistograms[index]->Fill(kErrorFlag_SW_HW);
              if ( (kErrorFlag_Sequence &  fErrorFlag)==kErrorFlag_Sequence)
                fHistograms[index]->Fill(kErrorFlag_Sequence);
              if ( (kErrorFlag_ZeroHW &  fErrorFlag)==kErrorFlag_ZeroHW)
                fHistograms[index]->Fill(kErrorFlag_ZeroHW);
              if ( (kErrorFlag_VQWK_Sat &  fErrorFlag)==kErrorFlag_VQWK_Sat)
                fHistograms[index]->Fill(kErrorFlag_VQWK_Sat);
              if ( (kErrorFlag_SameHW &  fErrorFlag)==kErrorFlag_SameHW)
                fHistograms[index]->Fill(kErrorFlag_SameHW);
            }
            
          }
 
    }
}

void  QwVQWK_Channel::ConstructBranchAndVector(TTree *tree, TString &prefix, std::vector<Double_t> &values)
{
  //  This channel is not used, so skip setting up the tree.
  if (IsNameEmpty()) return;

  //  Decide what to store based on prefix
  SetDataToSaveByPrefix(prefix);

  TString basename = prefix(0, (prefix.First("|") >= 0)? prefix.First("|"): prefix.Length()) + GetElementName();
  fTreeArrayIndex  = values.size();

  TString list = "";

  bHw_sum =     gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "hw_sum");
  bHw_sum_raw = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "hw_sum_raw");
  bBlock =     gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "block");
  bBlock_raw = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "block_raw");
  bNum_samples = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "num_samples");
  bDevice_Error_Code = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "Device_Error_Code");
  bSequence_number = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "sequence_number");

  if (bHw_sum) {
    values.push_back(0.0);
    list += "hw_sum/D";
    if (fDataToSave == kMoments) {
      values.push_back(0.0);
      list += ":hw_sum_m2/D";
      values.push_back(0.0);
      list += ":hw_sum_err/D";
    }
  }

  if (bBlock) {
    values.push_back(0.0);
    list += ":block0/D";
    values.push_back(0.0);
    list += ":block1/D";
    values.push_back(0.0);
    list += ":block2/D";
    values.push_back(0.0);
    list += ":block3/D";
  }

  if (bNum_samples) {
    values.push_back(0.0);
    list += ":num_samples/D";
  }

  if (bDevice_Error_Code) {
    values.push_back(0.0);
    list += ":Device_Error_Code/D";
  }

  if (fDataToSave == kRaw) {
    if (bHw_sum_raw) {
      values.push_back(0.0);
      list += ":hw_sum_raw/D";
    }
    if (bBlock_raw) {
      values.push_back(0.0);
      list += ":block0_raw/D";
      values.push_back(0.0);
      list += ":block1_raw/D";
      values.push_back(0.0);
      list += ":block2_raw/D";
      values.push_back(0.0);
      list += ":block3_raw/D";
    }
    if (bSequence_number) {
      values.push_back(0.0);
      list += ":sequence_number/D";
    }
  }

  fTreeArrayNumEntries = values.size() - fTreeArrayIndex;

  if (gQwHists.MatchDeviceParamsFromList(basename.Data())
    && (bHw_sum || bBlock || bNum_samples || bDevice_Error_Code ||
        bHw_sum_raw || bBlock_raw || bSequence_number)) {

    // This is for the RT mode
    if (list == "hw_sum/D")
      list = basename+"/D";

    if (kDEBUG)
      QwMessage << "base name " << basename << " List " << list << QwLog::endl;

    tree->Branch(basename, &(values[fTreeArrayIndex]), list);
  }

  if (kDEBUG) {
    std::cerr << "QwVQWK_Channel::ConstructBranchAndVector: fTreeArrayIndex==" << fTreeArrayIndex
              << "; fTreeArrayNumEntries==" << fTreeArrayNumEntries
              << "; values.size()==" << values.size()
              << "; list==" << list
              << std::endl;
  }
}

void  QwVQWK_Channel::ConstructBranch(TTree *tree, TString &prefix)
{
  //  This channel is not used, so skip setting up the tree.
  if (IsNameEmpty()) return;

  TString basename = prefix + GetElementName();
  tree->Branch(basename,&fHardwareBlockSum,basename+"/D");
  if (kDEBUG){
    std::cerr << "QwVQWK_Channel::ConstructBranchAndVector: fTreeArrayIndex==" << fTreeArrayIndex
              << "; fTreeArrayNumEntries==" << fTreeArrayNumEntries
              << std::endl;
  }
}


void  QwVQWK_Channel::ConstructRNTupleFields(class QwRNTuple *rntuple, const TString &prefix)
{
  //  This channel is not used, so skip setting up the RNTuple fields.
  if (IsNameEmpty()) return;

  //  Decide what to store based on prefix
  SetDataToSaveByPrefix(prefix);

  TString basename = prefix(0, (prefix.First("|") >= 0)? prefix.First("|"): prefix.Length()) + GetElementName();

  bHw_sum =     gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "hw_sum");
  bHw_sum_raw = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "hw_sum_raw");
  bBlock =     gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "block");
  bBlock_raw = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "block_raw");
  bNum_samples = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "num_samples");
  bDevice_Error_Code = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "Device_Error_Code");
  bSequence_number = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "sequence_number");

  if (gQwHists.MatchDeviceParamsFromList(basename.Data())
    && (bHw_sum || bBlock || bNum_samples || bDevice_Error_Code ||
        bHw_sum_raw || bBlock_raw || bSequence_number)) {
    
    // Convert TString to std::string for RNTuple field names
    std::string base_str = basename.Data();

    if (bHw_sum) {
      rntuple->AddField<Double_t>(base_str + "_hw_sum");
      if (fDataToSave == kMoments) {
        rntuple->AddField<Double_t>(base_str + "_hw_sum_m2");
        rntuple->AddField<Double_t>(base_str + "_hw_sum_err");
      }
    }

    if (bBlock) {
      rntuple->AddField<Double_t>(base_str + "_block0");
      rntuple->AddField<Double_t>(base_str + "_block1");
      rntuple->AddField<Double_t>(base_str + "_block2");
      rntuple->AddField<Double_t>(base_str + "_block3");
    }

    if (bNum_samples) {
      rntuple->AddField<Double_t>(base_str + "_num_samples");
    }

    if (bDevice_Error_Code) {
      rntuple->AddField<Double_t>(base_str + "_Device_Error_Code");
    }

    if (fDataToSave == kRaw) {
      if (bHw_sum_raw) {
        rntuple->AddField<Double_t>(base_str + "_hw_sum_raw");
      }
      if (bBlock_raw) {
        rntuple->AddField<Double_t>(base_str + "_block0_raw");
        rntuple->AddField<Double_t>(base_str + "_block1_raw");
        rntuple->AddField<Double_t>(base_str + "_block2_raw");
        rntuple->AddField<Double_t>(base_str + "_block3_raw");
      }
      if (bSequence_number) {
        rntuple->AddField<Double_t>(base_str + "_sequence_number");
      }
    }
  }

  if (kDEBUG) {
    QwMessage << "QwVQWK_Channel::ConstructRNTupleFields: " << basename << QwLog::endl;
  }
}

void  QwVQWK_Channel::FillRNTupleVector(std::vector<Double_t> &values) const
{
  if (IsNameEmpty()) {
    //  This channel is not used, so skip filling the RNTuple vector.
  } else if (fTreeArrayNumEntries <= 0) {
    if (bDEBUG) std::cerr << "QwVQWK_Channel::FillRNTupleVector:  fTreeArrayNumEntries=="
              << fTreeArrayNumEntries << std::endl;
  } else if (values.size() < fTreeArrayIndex+fTreeArrayNumEntries){
    if (bDEBUG) std::cerr << "QwVQWK_Channel::FillRNTupleVector:  values.size()=="
              << values.size()
              << "; fTreeArrayIndex+fTreeArrayNumEntries=="
              << fTreeArrayIndex+fTreeArrayNumEntries
              << std::endl;
  } else {

    UInt_t index = fTreeArrayIndex;

    // hw_sum
    if (bHw_sum) {
      values[index++] = this->GetHardwareSum();
      if (fDataToSave == kMoments) {
        values[index++] = this->GetHardwareSumM2();
        values[index++] = this->GetHardwareSumError();
      }
    }

    if (bBlock) {
      for (Int_t i = 0; i < fBlocksPerEvent; i++) {
        // blocki
        values[index++] = this->GetBlockValue(i);
      }
    }

    // num_samples
    if (bNum_samples)
      values[index++] =
          (fDataToSave == kMoments)? this->fGoodEventCount: this->fNumberOfSamples;

    // Device_Error_Code
    if (bDevice_Error_Code)
      values[index++] = this->fErrorFlag;

    if (fDataToSave == kRaw)
      {
        // hw_sum_raw
        if (bHw_sum_raw)
          values[index++] = this->GetRawHardwareSum();

        if (bBlock_raw) {
          for (Int_t i = 0; i < fBlocksPerEvent; i++) {
            // blocki_raw
            values[index++] = this->GetRawBlockValue(i);
          }
        }

        // sequence_number
        if (bSequence_number)
          values[index++] = this->fSequenceNumber;
      }
  }
}

void  QwVQWK_Channel::ConstructRNTupleFields(std::shared_ptr<ROOT::RNTupleModel> model, std::string& prefix, std::vector<Double_t>& vector, std::vector<std::shared_ptr<Double_t>>& fields)
{
  //  This channel is not used, so skip setting up the RNTuple fields.
  if (IsNameEmpty()) return;

  //  Decide what to store based on prefix
  SetDataToSaveByPrefix(TString(prefix.c_str()));

  std::string basename = prefix + GetElementName().Data();

  bHw_sum =     gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "hw_sum");
  bHw_sum_raw = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "hw_sum_raw");
  bBlock =     gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "block");
  bBlock_raw = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "block_raw");
  bNum_samples = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "num_samples");
  bDevice_Error_Code = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "Device_Error_Code");
  bSequence_number = gQwHists.MatchVQWKElementFromList(GetSubsystemName().Data(), GetModuleType().Data(), "sequence_number");

  fTreeArrayIndex = vector.size();

  if (gQwHists.MatchDeviceParamsFromList(basename.c_str())
    && (bHw_sum || bBlock || bNum_samples || bDevice_Error_Code ||
        bHw_sum_raw || bBlock_raw || bSequence_number)) {

    if (bHw_sum) {
      auto field_hw_sum = model->MakeField<Double_t>(basename + "_hw_sum");
      fields.push_back(field_hw_sum);
      vector.push_back(0.0);
      if (fDataToSave == kMoments) {
        auto field_hw_sum_m2 = model->MakeField<Double_t>(basename + "_hw_sum_m2");
        fields.push_back(field_hw_sum_m2);
        vector.push_back(0.0);
        auto field_hw_sum_err = model->MakeField<Double_t>(basename + "_hw_sum_err");
        fields.push_back(field_hw_sum_err);
        vector.push_back(0.0);
      }
    }

    if (bBlock) {
      auto field_block0 = model->MakeField<Double_t>(basename + "_block0");
      fields.push_back(field_block0);
      vector.push_back(0.0);
      auto field_block1 = model->MakeField<Double_t>(basename + "_block1");
      fields.push_back(field_block1);
      vector.push_back(0.0);
      auto field_block2 = model->MakeField<Double_t>(basename + "_block2");
      fields.push_back(field_block2);
      vector.push_back(0.0);
      auto field_block3 = model->MakeField<Double_t>(basename + "_block3");
      fields.push_back(field_block3);
      vector.push_back(0.0);
    }

    if (bNum_samples) {
      auto field_num_samples = model->MakeField<Double_t>(basename + "_num_samples");
      fields.push_back(field_num_samples);
      vector.push_back(0.0);
    }

    if (bDevice_Error_Code) {
      auto field_error_code = model->MakeField<Double_t>(basename + "_Device_Error_Code");
      fields.push_back(field_error_code);
      vector.push_back(0.0);
    }

    if (fDataToSave == kRaw) {
      if (bHw_sum_raw) {
        auto field_hw_sum_raw = model->MakeField<Double_t>(basename + "_hw_sum_raw");
        fields.push_back(field_hw_sum_raw);
        vector.push_back(0.0);
      }
      if (bBlock_raw) {
        auto field_block0_raw = model->MakeField<Double_t>(basename + "_block0_raw");
        fields.push_back(field_block0_raw);
        vector.push_back(0.0);
        auto field_block1_raw = model->MakeField<Double_t>(basename + "_block1_raw");
        fields.push_back(field_block1_raw);
        vector.push_back(0.0);
        auto field_block2_raw = model->MakeField<Double_t>(basename + "_block2_raw");
        fields.push_back(field_block2_raw);
        vector.push_back(0.0);
        auto field_block3_raw = model->MakeField<Double_t>(basename + "_block3_raw");
        fields.push_back(field_block3_raw);
        vector.push_back(0.0);
      }
      if (bSequence_number) {
        auto field_sequence_number = model->MakeField<Double_t>(basename + "_sequence_number");
        fields.push_back(field_sequence_number);
        vector.push_back(0.0);
      }
    }
  }

  fTreeArrayNumEntries = vector.size() - fTreeArrayIndex;

  if (kDEBUG) {
    QwMessage << "QwVQWK_Channel::ConstructRNTupleFields(model): " << basename 
              << " fTreeArrayIndex=" << fTreeArrayIndex 
              << " fTreeArrayNumEntries=" << fTreeArrayNumEntries << QwLog::endl;
  }
}
