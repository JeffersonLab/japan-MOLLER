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
  // This functionality can be used for testing or generating mock data
  // Generate random values within reasonable range
  
  Double_t base_value = 1000.0; // Base ADC value
  Double_t random_component = 0.0;
  
  // Add a small helicity-dependent asymmetry (for testing)
  Double_t asym_factor = 0.0;
  if (helicity > 0) asym_factor = 1.0 + 0.0001; // 100 ppm asymmetry
  else if (helicity < 0) asym_factor = 1.0 - 0.0001;
  else asym_factor = 1.0; // No asymmetry for helicity=0
  
  // Add a time-dependent drift (for testing)
  Double_t drift_factor = 1.0 + 0.0001 * time; // 100 ppm/hour drift
  
  fHardwareBlockSum = base_value * asym_factor * drift_factor;
  
  // Add gaussian noise (about 1%)
  random_component = gRandom->Gaus(0, 0.01 * fHardwareBlockSum);
  fHardwareBlockSum += random_component;
  
  // Propagate to raw value
  fHardwareBlockSum_raw = static_cast<Int_t>(fHardwareBlockSum);
  
  // Set the individual blocks with appropriate distribution
  for (Int_t i = 0; i < fBlocksPerEvent; i++) {
    fBlock[i] = fHardwareBlockSum / fBlocksPerEvent;
    random_component = gRandom->Gaus(0, 0.02 * fBlock[i]); // Slightly more noise in individual blocks
    fBlock[i] += random_component;
    fBlock_raw[i] = static_cast<Int_t>(fBlock[i]);
  }
  
  // Set uncertainty values based on Poisson statistics
  fHardwareBlockSumError = sqrt(fabs(fHardwareBlockSum));
  fHardwareBlockSumM2 = fHardwareBlockSumError * fHardwareBlockSumError;
  
  for (Int_t i = 0; i < fBlocksPerEvent; i++) {
    fBlockError[i] = sqrt(fabs(fBlock[i]));
    fBlockM2[i] = fBlockError[i] * fBlockError[i];
  }
  
  // No errors for mock data
  fErrorFlag = 0;
}
// Enhanced SmearByResolution method is now implemented below

void QwVQWK_Channel::SetHardwareSum(Double_t hwsum, UInt_t sequencenumber)
{
  Double_t* block = new Double_t[fBlocksPerEvent];
  for (Int_t i = 0; i < fBlocksPerEvent; i++){
    block[i] = hwsum / fBlocksPerEvent;
  }
  SetEventData(block);
  delete[] block;  // Use delete[] for arrays
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

/**
 * Division by VQwHardwareChannel pointer (virtual method implementation)
 */
void QwVQWK_Channel::DivideBy(const VQwHardwareChannel* valueptr)
{
  const QwVQWK_Channel* value = dynamic_cast<const QwVQWK_Channel*>(valueptr);
  if (value != NULL) {
    DivideBy(*value);
  } else {
    TString loc = "QwVQWK_Channel::DivideBy";
    TString msg = "Cannot divide by non-QwVQWK_Channel object";
    QwError << loc << " " << msg << QwLog::endl;
  }
}

/**
 * Get average voltage value
 */
Double_t QwVQWK_Channel::GetAverageVolts() const
{
  if (fNumberOfSamples > 0) {
    return (fHardwareBlockSum / fNumberOfSamples);
  }
  return 0.0;
}

/**
 * Implementation of scaling with value
 */
void QwVQWK_Channel::ScaledAdd(Double_t scale, const VQwHardwareChannel *value)
{
  const QwVQWK_Channel* typed_value = dynamic_cast<const QwVQWK_Channel*>(value);
  if (typed_value != NULL) {
    // Make a copy and scale it
    QwVQWK_Channel scaled_value = *typed_value;
    scaled_value.Scale(scale);
    // Now add
    *this += scaled_value;
  } else {
    TString loc = "QwVQWK_Channel::ScaledAdd";
    TString msg = "Cannot add from non-QwVQWK_Channel object";
    QwError << loc << " " << msg << QwLog::endl;
  }
}

/**
 * Implementation of ArcTan function
 */
void QwVQWK_Channel::ArcTan(const QwVQWK_Channel &value)
{
  // Implementation of the ArcTan function
  // Only apply if there are no error flags
  if (fErrorFlag == 0 && value.fErrorFlag == 0) {
    // For hardware sum
    Double_t x = fHardwareBlockSum;
    Double_t y = value.fHardwareBlockSum;
    
    // Calculate error propagation for arctan(y/x)
    Double_t denom = x*x + y*y;
    Double_t dx_term = (y*y)/(denom*denom) * fHardwareBlockSumError * fHardwareBlockSumError;
    Double_t dy_term = (x*x)/(denom*denom) * value.fHardwareBlockSumError * value.fHardwareBlockSumError;
    Double_t error_squared = dx_term + dy_term;
    
    // Set new values
    fHardwareBlockSum = atan2(y, x);
    fHardwareBlockSum_raw = static_cast<Int_t>(fHardwareBlockSum);
    fHardwareBlockSumM2 = error_squared;
    fHardwareBlockSumError = sqrt(error_squared);
    
    // Do the same for each block
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      x = fBlock[i];
      y = value.fBlock[i];
      
      denom = x*x + y*y;
      dx_term = (y*y)/(denom*denom) * fBlockError[i] * fBlockError[i];
      dy_term = (x*x)/(denom*denom) * value.fBlockError[i] * value.fBlockError[i];
      error_squared = dx_term + dy_term;
      
      fBlock[i] = atan2(y, x);
      fBlock_raw[i] = static_cast<Int_t>(fBlock[i]);
      fBlockM2[i] = error_squared;
      fBlockError[i] = sqrt(error_squared);
    }
  } 
  else {
    // Propagate any error flags
    fErrorFlag |= value.fErrorFlag;
  }
}

/**
 * Addition operator for QwVQWK_Channel objects
 */
QwVQWK_Channel& QwVQWK_Channel::operator+= (const QwVQWK_Channel &value)
{
  // Only add if neither channel has an error flag set
  if (fErrorFlag == 0 && value.fErrorFlag == 0) {
    fHardwareBlockSum += value.fHardwareBlockSum;
    fHardwareBlockSum_raw += value.fHardwareBlockSum_raw;
    fHardwareBlockSumM2 += value.fHardwareBlockSumM2;
    
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlock[i] += value.fBlock[i];
      fBlock_raw[i] += value.fBlock_raw[i];
      fBlockM2[i] += value.fBlockM2[i];
    }
    
    // Update error calculations
    fHardwareBlockSumError = sqrt(fHardwareBlockSumM2);
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlockError[i] = sqrt(fBlockM2[i]);
    }
  } 
  else {
    // Propagate any error flags
    fErrorFlag |= value.fErrorFlag;
  }
  return *this;
}

/**
 * Subtraction operator for QwVQWK_Channel objects
 */
QwVQWK_Channel& QwVQWK_Channel::operator-= (const QwVQWK_Channel &value)
{
  // Only subtract if neither channel has an error flag set
  if (fErrorFlag == 0 && value.fErrorFlag == 0) {
    fHardwareBlockSum -= value.fHardwareBlockSum;
    fHardwareBlockSum_raw -= value.fHardwareBlockSum_raw;
    fHardwareBlockSumM2 += value.fHardwareBlockSumM2; // Add M2 values for error propagation
    
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlock[i] -= value.fBlock[i];
      fBlock_raw[i] -= value.fBlock_raw[i];
      fBlockM2[i] += value.fBlockM2[i]; // Add M2 values for error propagation
    }
    
    // Update error calculations
    fHardwareBlockSumError = sqrt(fHardwareBlockSumM2);
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlockError[i] = sqrt(fBlockM2[i]);
    }
  } 
  else {
    // Propagate any error flags
    fErrorFlag |= value.fErrorFlag;
  }
  return *this;
}

/**
 * Multiplication operator for QwVQWK_Channel objects
 */
QwVQWK_Channel& QwVQWK_Channel::operator*= (const QwVQWK_Channel &value)
{
  // Only multiply if neither channel has an error flag set
  if (fErrorFlag == 0 && value.fErrorFlag == 0) {
    Double_t error_squared = 0.0;
    
    // For hardware sum, need to handle error propagation properly
    error_squared = (fHardwareBlockSum * fHardwareBlockSum * value.fHardwareBlockSumError * value.fHardwareBlockSumError) +
                   (value.fHardwareBlockSum * value.fHardwareBlockSum * fHardwareBlockSumError * fHardwareBlockSumError);
    
    fHardwareBlockSum *= value.fHardwareBlockSum;
    fHardwareBlockSum_raw *= value.fHardwareBlockSum_raw;
    fHardwareBlockSumM2 = error_squared;
    fHardwareBlockSumError = sqrt(error_squared);
    
    // Do the same for each block
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      error_squared = (fBlock[i] * fBlock[i] * value.fBlockError[i] * value.fBlockError[i]) +
                     (value.fBlock[i] * value.fBlock[i] * fBlockError[i] * fBlockError[i]);
      
      fBlock[i] *= value.fBlock[i];
      fBlock_raw[i] *= value.fBlock_raw[i];
      fBlockM2[i] = error_squared;
      fBlockError[i] = sqrt(error_squared);
    }
  } 
  else {
    // Propagate any error flags
    fErrorFlag |= value.fErrorFlag;
  }
  return *this;
}

/**
 * Division operator for QwVQWK_Channel objects
 * Protected method only used internally
 */
QwVQWK_Channel& QwVQWK_Channel::operator/= (const QwVQWK_Channel &value)
{
  // Only divide if neither channel has an error flag set
  if (fErrorFlag == 0 && value.fErrorFlag == 0) {
    Double_t error_squared = 0.0;
    
    // Check for division by zero
    if (value.fHardwareBlockSum != 0) {
      // For hardware sum, need to handle error propagation properly for division
      error_squared = (fHardwareBlockSumError * fHardwareBlockSumError) / (value.fHardwareBlockSum * value.fHardwareBlockSum) +
                     (fHardwareBlockSum * fHardwareBlockSum * value.fHardwareBlockSumError * value.fHardwareBlockSumError) / 
                     (value.fHardwareBlockSum * value.fHardwareBlockSum * value.fHardwareBlockSum * value.fHardwareBlockSum);
      
      fHardwareBlockSum /= value.fHardwareBlockSum;
      fHardwareBlockSum_raw /= value.fHardwareBlockSum_raw;
      fHardwareBlockSumM2 = error_squared;
      fHardwareBlockSumError = sqrt(error_squared);
    } 
    else {
      // Division by zero
      fErrorFlag |= kErrorFlag_ZeroHW;
      return *this;
    }
    
    // Do the same for each block
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      if (value.fBlock[i] != 0) {
        error_squared = (fBlockError[i] * fBlockError[i]) / (value.fBlock[i] * value.fBlock[i]) +
                       (fBlock[i] * fBlock[i] * value.fBlockError[i] * value.fBlockError[i]) /
                       (value.fBlock[i] * value.fBlock[i] * value.fBlock[i] * value.fBlock[i]);
        
        fBlock[i] /= value.fBlock[i];
        fBlock_raw[i] /= value.fBlock_raw[i];
        fBlockM2[i] = error_squared;
        fBlockError[i] = sqrt(error_squared);
      } 
      else {
        // Division by zero
        fErrorFlag |= kErrorFlag_ZeroHW;
        return *this;
      }
    }
  } 
  else {
    // Propagate any error flags
    fErrorFlag |= value.fErrorFlag;
  }
  return *this;
}

/**
 * VQwHardwareChannel addition operator implementation
 */
VQwHardwareChannel& QwVQWK_Channel::operator+=(const VQwHardwareChannel* input)
{
  const QwVQWK_Channel* value = dynamic_cast<const QwVQWK_Channel*>(input);
  if (value != NULL) {
    *this += *value;
  }
  return *this;
}

/**
 * VQwHardwareChannel subtraction operator implementation
 */
VQwHardwareChannel& QwVQWK_Channel::operator-=(const VQwHardwareChannel* input)
{
  const QwVQWK_Channel* value = dynamic_cast<const QwVQWK_Channel*>(input);
  if (value != NULL) {
    *this -= *value;
  }
  return *this;
}

/**
 * VQwHardwareChannel multiplication operator implementation
 */
VQwHardwareChannel& QwVQWK_Channel::operator*=(const VQwHardwareChannel* input)
{
  const QwVQWK_Channel* value = dynamic_cast<const QwVQWK_Channel*>(input);
  if (value != NULL) {
    *this *= *value;
  }
  return *this;
}

/**
 * VQwHardwareChannel division operator implementation
 */
VQwHardwareChannel& QwVQWK_Channel::operator/=(const VQwHardwareChannel* input)
{
  const QwVQWK_Channel* value = dynamic_cast<const QwVQWK_Channel*>(input);
  if (value != NULL) {
    *this /= *value;
  }
  return *this;
}

/**
 * Binary addition operator
 */
const QwVQWK_Channel QwVQWK_Channel::operator+ (const QwVQWK_Channel &value) const
{
  QwVQWK_Channel result = *this;
  result += value;
  return result;
}

/**
 * Binary subtraction operator
 */
const QwVQWK_Channel QwVQWK_Channel::operator- (const QwVQWK_Channel &value) const
{
  QwVQWK_Channel result = *this;
  result -= value;
  return result;
}

/**
 * Binary multiplication operator
 */
const QwVQWK_Channel QwVQWK_Channel::operator* (const QwVQWK_Channel &value) const
{
  QwVQWK_Channel result = *this;
  result *= value;
  return result;
}

/**
 * Sum method (compute sum of two channels)
 */
void QwVQWK_Channel::Sum(const QwVQWK_Channel &value1, const QwVQWK_Channel &value2)
{
  *this = value1;
  *this += value2;
}

/**
 * Difference method (compute difference of two channels)
 */
void QwVQWK_Channel::Difference(const QwVQWK_Channel &value1, const QwVQWK_Channel &value2)
{
  *this = value1;
  *this -= value2;
}

/**
 * Product method (compute product of two channels)
 */
void QwVQWK_Channel::Product(const QwVQWK_Channel &value1, const QwVQWK_Channel &value2)
{
  *this = value1;
  *this *= value2;
}

/**
 * Ratio method (compute ratio of two channels)
 */
void QwVQWK_Channel::Ratio(const QwVQWK_Channel &numer, const QwVQWK_Channel &denom)
{
  *this = numer;
  *this /= denom;
}

/**
 * Division method (divide this channel by another)
 */
void QwVQWK_Channel::DivideBy(const QwVQWK_Channel& denom)
{
  *this /= denom;
}

/**
 * Add a constant offset to this channel
 */
void QwVQWK_Channel::AddChannelOffset(Double_t offset)
{
  if (fErrorFlag == 0) {
    fHardwareBlockSum += offset;
    fHardwareBlockSum_raw += static_cast<Int_t>(offset);
    
    // Distribute the offset across blocks
    Double_t block_offset = offset / fBlocksPerEvent;
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlock[i] += block_offset;
      fBlock_raw[i] += static_cast<Int_t>(block_offset);
    }
    // Error calculations remain unchanged as adding a constant doesn't affect error
  }
}

/**
 * Scale this channel by a factor
 */
void QwVQWK_Channel::Scale(Double_t factor)
{
  if (fErrorFlag == 0) {
    fHardwareBlockSum *= factor;
    fHardwareBlockSum_raw = static_cast<Int_t>(fHardwareBlockSum_raw * factor);
    fHardwareBlockSumError *= fabs(factor);
    fHardwareBlockSumM2 *= factor * factor;
    
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlock[i] *= factor;
      fBlock_raw[i] = static_cast<Int_t>(fBlock_raw[i] * factor);
      fBlockError[i] *= fabs(factor);
      fBlockM2[i] *= factor * factor;
    }
  }
}

/**
 * Implementation of event cut checking methods
 */
Bool_t QwVQWK_Channel::ApplySingleEventCuts(Double_t LL, Double_t UL)
{
  Bool_t eventpassed = kTRUE;
  
  // If there are already hardware errors, no need to check further
  if (fErrorFlag != 0) {
    return kFALSE;
  }
  
  // For average value check (hardware sum)
  Double_t value = fHardwareBlockSum / fNumberOfSamples;
  
  // Apply event cuts (simple threshold check)
  if (value < LL) {
    fErrorFlag |= kErrorFlag_EventCut_L;
    eventpassed = kFALSE;
  }
  if (value > UL) {
    fErrorFlag |= kErrorFlag_EventCut_U;
    eventpassed = kFALSE;
  }
  
  return eventpassed;
}

/**
 * Implementation of event cut checking methods using internal limits
 */
Bool_t QwVQWK_Channel::ApplySingleEventCuts()
{
  return ApplySingleEventCuts(fLLimit, fULimit);
}

/**
 * Print error counters
 */
void QwVQWK_Channel::PrintErrorCounters() const
{
  QwMessage << "Error counters for " << GetElementName() << QwLog::endl;
  QwMessage << "  Sample size mismatches: " << fErrorCount_sample << QwLog::endl;
  QwMessage << "  SW/HW sum mismatches: " << fErrorCount_SW_HW << QwLog::endl;
  QwMessage << "  Sequence number mismatches: " << fErrorCount_Sequence << QwLog::endl;
  QwMessage << "  Same HW value errors: " << fErrorCount_SameHW << QwLog::endl;
  QwMessage << "  Zero HW value errors: " << fErrorCount_ZeroHW << QwLog::endl;
  QwMessage << "  Hardware saturation errors: " << fErrorCount_HWSat << QwLog::endl;
  QwMessage << "  Events rejected by cuts: " << fNumEvtsWithEventCutsRejected << QwLog::endl;
}

/**
 * Static method to print error counter table header
 */
void QwVQWK_Channel::PrintErrorCounterHead()
{
  QwMessage << "Error Counter Report for QwVQWK_Channel" << QwLog::endl;
  QwMessage << "----------------------------------------" << QwLog::endl;
  QwMessage << "Channel Name          | Sample | SW/HW | Sequence | Same HW | Zero HW | HW Sat | Event Cuts" << QwLog::endl;
  QwMessage << "-------------------------------------------------------------------------------------" << QwLog::endl;
}

/**
 * Static method to print error counter table footer
 */
void QwVQWK_Channel::PrintErrorCounterTail()
{
  QwMessage << "-------------------------------------------------------------------------------------" << QwLog::endl;
}

/**
 * RNTuple field construction
 */
void QwVQWK_Channel::ConstructRNTupleFields(QwRNTuple* rntuple, const TString& prefix)
{
  // Create the branches in the ROOT tree.
  if (IsNameEmpty()) {
    //  This channel is not used, so skip filling the tree.
  } else {
    // Apply same prefix processing as other methods to ensure field name consistency
    TString prefix_tstring(prefix);
    TString basename = prefix_tstring(0, (prefix_tstring.First("|") >= 0)? prefix_tstring.First("|"): prefix_tstring.Length()) + GetElementName();
    
    // Decide what to store based on prefix (matching FillTreeVector logic)
    SetDataToSaveByPrefix(prefix);
    
    // Add fields to the RNTuple using the AddField method, which properly registers them
    rntuple->AddField<Double_t>(std::string(basename.Data()) + "_hw_sum");
    
    if (fDataToSave == kMoments) {
      rntuple->AddField<Double_t>(std::string(basename.Data()) + "_hw_sum_m2");
      rntuple->AddField<Double_t>(std::string(basename.Data()) + "_hw_sum_err");
    }
    
    rntuple->AddField<Double_t>(std::string(basename.Data()) + "_Device_Error_Code");
    
    if (fDataToSave == kRaw) {
      rntuple->AddField<Double_t>(std::string(basename.Data()) + "_hw_sum_raw");
      rntuple->AddField<Double_t>(std::string(basename.Data()) + "_sequence_number");
      rntuple->AddField<Double_t>(std::string(basename.Data()) + "_num_samples");
      
      for (Int_t i = 0; i < fBlocksPerEvent; i++) {
        rntuple->AddField<Double_t>(std::string(basename.Data()) + Form("_block%d", i));
        rntuple->AddField<Double_t>(std::string(basename.Data()) + Form("_block%d_raw", i));
      }
    }
    
    // Set up the tree array index and number of entries for vector filling
    fTreeArrayIndex = rntuple->GetVector().size();
    
    // Calculate expected number of entries based on what we added
    fTreeArrayNumEntries = 2; // hw_sum + Device_Error_Code
    if (fDataToSave == kMoments) {
      fTreeArrayNumEntries += 2; // hw_sum_m2 + hw_sum_err  
    }
    if (fDataToSave == kRaw) {
      fTreeArrayNumEntries += 3; // hw_sum_raw + sequence_number + num_samples
      fTreeArrayNumEntries += 2 * fBlocksPerEvent; // block + block_raw for each block
    }
    
    QwVerbose << "QwVQWK_Channel::ConstructRNTupleFields: " << basename 
              << " fTreeArrayIndex=" << fTreeArrayIndex 
              << " fTreeArrayNumEntries=" << fTreeArrayNumEntries 
              << " total vector size=" << rntuple->GetVector().size() 
              << QwLog::endl;
  }
}

/**
 * RNTuple vector filling
 */
void QwVQWK_Channel::FillRNTupleVector(std::vector<Double_t>& values) const
{
  // Fill the vector with the values to be stored in the ROOT tree.
  if (IsNameEmpty()) {
    // This channel is not used, so skip filling the vector.
  } else {
    // Check if RNTuple fields were properly constructed
    if (fTreeArrayNumEntries <= 0) {
      // Legacy interface detected - warn and skip to prevent segfault
      static bool warned = false;
      if (!warned) {
        QwError << "QwVQWK_Channel::FillRNTupleVector: Cannot fill RNTuple fields created with legacy interface for " 
                << GetElementName() << ". fTreeArrayNumEntries=" << fTreeArrayNumEntries << QwLog::endl;
        warned = true;
      }
      return;
    }
    
    // Proceed with standard FillTreeVector() logic since indices are properly set up
    FillTreeVector(values);
  }
}

/**
 * Print detailed channel information
 */
void QwVQWK_Channel::PrintInfo() const
{
  QwMessage << "***************************************" << QwLog::endl;
  QwMessage << "Subsystem " << GetSubsystemName() << QwLog::endl;
  QwMessage << "Beam Instrument Type: " << GetModuleType() << QwLog::endl;
  QwMessage << "QwVQWK channel: " << GetElementName() << QwLog::endl;
  QwMessage << "fPedestal= " << fPedestal << QwLog::endl;
  QwMessage << "fCalibrationFactor= " << fCalibrationFactor << QwLog::endl;
  QwMessage << "fSequenceNumber= " << fSequenceNumber << QwLog::endl;
  QwMessage << "fNumberOfSamples= " << fNumberOfSamples << QwLog::endl;
  QwMessage << "fNumberOfSamples_map= " << fNumberOfSamples_map << QwLog::endl;
  QwMessage << "fBlocksPerEvent= " << fBlocksPerEvent << QwLog::endl;
  QwMessage << "fHardwareBlockSum_raw= " << fHardwareBlockSum_raw << QwLog::endl;
  QwMessage << "fHardwareBlockSum= " << std::setprecision(8) << fHardwareBlockSum << QwLog::endl;
  QwMessage << "fSaturationABSLimit= " << fSaturationABSLimit << QwLog::endl;
  for (Int_t i = 0; i < fBlocksPerEvent; i++) {
    QwMessage << "fBlock_raw[" << i << "]= " << fBlock_raw[i] << QwLog::endl;
    QwMessage << "fBlock[" << i << "]= " << std::setprecision(8) << fBlock[i] << QwLog::endl;
  }
}

/**
 * Construct histograms for this channel
 */
void QwVQWK_Channel::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  // If we have defined a subdirectory in the ROOT file, then change into it.
  if (folder != NULL) folder->cd();

  if (IsNameEmpty()) {
    // This channel is not used, so skip filling the histograms.
  } else {
    // Now create the histograms.
    if (prefix == TString("asym_") ||
        prefix == TString("diff_") ||
        prefix == TString("yield_"))
      fDataToSave = kDerived;

    TString basename, fullname;
    basename = prefix + GetElementName();

    if (fDataToSave == kRaw) {
      fHistograms.resize(6, NULL);
      size_t index = 0;
      fHistograms[index++] = gQwHists.Construct1DHist(basename);
      fHistograms[index++] = gQwHists.Construct1DHist(basename + Form("_hw_sum_raw"));
      for (Int_t i = 0; i < fBlocksPerEvent; i++) {
        fHistograms[index++] = gQwHists.Construct1DHist(basename + Form("_block%d", i));
      }
    } else if (fDataToSave == kDerived) {
      fHistograms.resize(1, NULL);
      Int_t index = 0;
      fHistograms[index++] = gQwHists.Construct1DHist(basename);
    } else {
      // this is not recognized
    }
  }
}

/**
 * Fill histograms with current channel values
 */
void QwVQWK_Channel::FillHistograms()
{
  Int_t index = 0;

  if (IsNameEmpty()) {
    // This channel is not used, so skip creating the histograms.
  } else {
    if (fDataToSave == kRaw) {
      if (fHistograms[index] != NULL && (fErrorFlag) == 0)
        fHistograms[index++]->Fill(GetValue());
      if (fHistograms[index] != NULL && (fErrorFlag) == 0)
        fHistograms[index++]->Fill(fHardwareBlockSum_raw);
      for (Int_t i = 0; i < fBlocksPerEvent; i++) {
        if (fHistograms[index] != NULL && (fErrorFlag) == 0)
          fHistograms[index++]->Fill(fBlock[i]);
      }
    } else if (fDataToSave == kDerived) {
      if (fHistograms[index] != NULL && (fErrorFlag) == 0)
        fHistograms[index++]->Fill(GetValue());
    }
  }
}

/**
 * Construct tree branch and fill values vector
 */
void QwVQWK_Channel::ConstructBranchAndVector(TTree *tree, TString &prefix, std::vector<Double_t> &values)
{
  if (IsNameEmpty()) {
    // This channel is not used, so skip setting up the tree.
  } else {
    // Decide what to store based on prefix
    SetDataToSaveByPrefix(prefix);

    TString basename = prefix(0, (prefix.First("|") >= 0) ? prefix.First("|") : prefix.Length()) + GetElementName();
    fTreeArrayIndex = values.size();

    TString list;

    values.push_back(0.0);
    list = "hw_sum/D";
    if (fDataToSave == kMoments) {
      values.push_back(0.0);
      list += ":hw_sum_m2/D";
      values.push_back(0.0);
      list += ":hw_sum_err/D";
    }

    values.push_back(0.0);
    list += ":Device_Error_Code/D";

    if (fDataToSave == kRaw) {
      values.push_back(0.0);
      list += ":hw_sum_raw/D";
      values.push_back(0.0);
      list += ":sequence_number/D";
      values.push_back(0.0);
      list += ":num_samples/D";
      for (Int_t i = 0; i < fBlocksPerEvent; i++) {
        values.push_back(0.0);
        list += Form(":block%d/D", i);
        values.push_back(0.0);
        list += Form(":block%d_raw/D", i);
      }
    }

    fTreeArrayNumEntries = values.size() - fTreeArrayIndex;
    if (gQwHists.MatchDeviceParamsFromList(basename.Data()))
      tree->Branch(basename, &(values[fTreeArrayIndex]), list);
  }
}

/**
 * Construct simple tree branch
 */
void QwVQWK_Channel::ConstructBranch(TTree *tree, TString &prefix)
{
  if (IsNameEmpty()) {
    // This channel is not used, so skip setting up the tree.
  } else {
    TString basename = prefix + GetElementName();
    tree->Branch(basename, &fHardwareBlockSum, basename + "/D");
  }
}

/**
 * Fill values vector for tree output
 */
void QwVQWK_Channel::FillTreeVector(std::vector<Double_t> &values) const
{
  if (IsNameEmpty()) {
    // This channel is not used, so skip setting up the tree.
  } else if (fTreeArrayNumEntries < 0) {
    QwError << "QwVQWK_Channel::FillTreeVector:  fTreeArrayNumEntries=="
            << fTreeArrayNumEntries << QwLog::endl;
  } else if (fTreeArrayNumEntries == 0) {
    static bool warned = false;
    if (!warned) {
      QwError << "QwVQWK_Channel::FillTreeVector:  fTreeArrayNumEntries=="
              << fTreeArrayNumEntries << " (no branch constructed?)" << QwLog::endl;
      QwError << "Offending element is " << GetElementName() << QwLog::endl;
      warned = true;
    }
  } else if (values.size() < fTreeArrayIndex + fTreeArrayNumEntries) {
    QwError << "QwVQWK_Channel::FillTreeVector:  values.size()=="
            << values.size() << " name: " << fElementName
            << "; fTreeArrayIndex+fTreeArrayNumEntries=="
            << fTreeArrayIndex << '+' << fTreeArrayNumEntries << '='
            << fTreeArrayIndex + fTreeArrayNumEntries
            << QwLog::endl;
  } else {
    size_t index = fTreeArrayIndex;
    values[index++] = this->fHardwareBlockSum;
    if (fDataToSave == kMoments) {
      values[index++] = fHardwareBlockSumM2;
      values[index++] = fHardwareBlockSumError;
    }
    values[index++] = this->fErrorFlag;
    if (fDataToSave == kRaw) {
      values[index++] = this->fHardwareBlockSum_raw;
      values[index++] = this->fSequenceNumber;
      values[index++] = this->fNumberOfSamples;
      for (Int_t i = 0; i < fBlocksPerEvent; i++) {
        values[index++] = this->fBlock[i];
        values[index++] = this->fBlock_raw[i];
      }
    }
  }
}



/**
 * Construct RNTuple fields with shared model
 */
void QwVQWK_Channel::ConstructRNTupleFields(std::shared_ptr<ROOT::RNTupleModel> model, std::string& prefix, std::vector<Double_t>& vector, std::vector<std::shared_ptr<Double_t>>& fields)
{
  if (IsNameEmpty()) {
    // This channel is not used, so skip setting up RNTuple fields.
  } else {
    // Apply same prefix processing as legacy method to ensure field name consistency
    TString prefix_tstring(prefix.c_str());
    TString basename = prefix_tstring(0, (prefix_tstring.First("|") >= 0)? prefix_tstring.First("|"): prefix_tstring.Length()) + GetElementName();
    
    // Add the hardware sum field
    auto field = model->MakeField<Double_t>(basename.Data() + std::string("_hw_sum"));
    fields.push_back(field);
    vector.push_back(0.0);
    
    if (fDataToSave == kMoments) {
      auto field_m2 = model->MakeField<Double_t>(basename.Data() + std::string("_hw_sum_m2"));
      fields.push_back(field_m2);
      vector.push_back(0.0);
      
      auto field_err = model->MakeField<Double_t>(basename.Data() + std::string("_hw_sum_err"));
      fields.push_back(field_err);
      vector.push_back(0.0);
    }
    
    auto field_error = model->MakeField<Double_t>(basename.Data() + std::string("_Device_Error_Code"));
    fields.push_back(field_error);
    vector.push_back(0.0);
    
    if (fDataToSave == kRaw) {
      auto field_raw = model->MakeField<Double_t>(basename.Data() + std::string("_hw_sum_raw"));
      fields.push_back(field_raw);
      vector.push_back(0.0);
      
      auto field_seq = model->MakeField<Double_t>(basename.Data() + std::string("_sequence_number"));
      fields.push_back(field_seq);
      vector.push_back(0.0);
      
      auto field_samples = model->MakeField<Double_t>(basename.Data() + std::string("_num_samples"));
      fields.push_back(field_samples);
      vector.push_back(0.0);
      
      for (Int_t i = 0; i < fBlocksPerEvent; i++) {
        auto field_block = model->MakeField<Double_t>(basename.Data() + std::string(Form("_block%d", i)));
        fields.push_back(field_block);
        vector.push_back(0.0);
        
        auto field_block_raw = model->MakeField<Double_t>(basename.Data() + std::string(Form("_block%d_raw", i)));
        fields.push_back(field_block_raw);
        vector.push_back(0.0);
      }
    }
  }
}

/**
 * Assign scaled value from another QwVQWK_Channel
 */
void QwVQWK_Channel::AssignScaledValue(const QwVQWK_Channel &value, Double_t scale)
{
  if (this == &value) return;

  if (!IsNameEmpty()) {
    // Scale the hardware sum
    this->fHardwareBlockSum = value.fHardwareBlockSum * scale;
    this->fHardwareBlockSumError = value.fHardwareBlockSumError * fabs(scale);
    this->fHardwareBlockSumM2 = value.fHardwareBlockSumM2 * scale * scale;
    this->fHardwareBlockSum_raw = static_cast<Int_t>(value.fHardwareBlockSum_raw * scale);

    // Scale the individual blocks
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      this->fBlock[i] = value.fBlock[i] * scale;
      this->fBlockError[i] = value.fBlockError[i] * fabs(scale);
      this->fBlockM2[i] = value.fBlockM2[i] * scale * scale;
      this->fBlock_raw[i] = static_cast<Int_t>(value.fBlock_raw[i] * scale);
    }

    // Copy other properties without scaling
    this->fSequenceNumber = value.fSequenceNumber;
    this->fNumberOfSamples = value.fNumberOfSamples;
    this->fBlocksPerEvent = value.fBlocksPerEvent;
    this->fErrorFlag = value.fErrorFlag;
    this->fGoodEventCount = value.fGoodEventCount;
  }
}

/**
 * Method to apply resolution smearing (for simulation)
 */
void QwVQWK_Channel::SmearByResolution(double resolution)
{
  if (resolution <= 0.0) return;
  
  // Apply Gaussian smearing to hardware sum
  Double_t smearing = gRandom->Gaus(0.0, resolution);
  fHardwareBlockSum *= (1.0 + smearing);
  fHardwareBlockSum_raw = static_cast<Int_t>(fHardwareBlockSum_raw * (1.0 + smearing));
  
  // Apply to blocks as well
  for (Int_t i = 0; i < fBlocksPerEvent; i++) {
    Double_t block_smearing = gRandom->Gaus(0.0, resolution);
    fBlock[i] *= (1.0 + block_smearing);
    fBlock_raw[i] = static_cast<Int_t>(fBlock_raw[i] * (1.0 + block_smearing));
  }
  
  // Update errors
  fHardwareBlockSumError = fabs(fHardwareBlockSum * resolution);
  fHardwareBlockSumM2 = fHardwareBlockSumError * fHardwareBlockSumError;
  
  for (Int_t i = 0; i < fBlocksPerEvent; i++) {
    fBlockError[i] = fabs(fBlock[i] * resolution);
    fBlockM2[i] = fBlockError[i] * fBlockError[i];
  }
}

/**
 * Assignment operator implementation
 */
QwVQWK_Channel& QwVQWK_Channel::operator=(const QwVQWK_Channel &value)
{
  if (this == &value) return *this;
  
  if (!IsNameEmpty()) {
    VQwHardwareChannel::operator=(value);
    
    // Copy channel-specific data
    this->fHardwareBlockSum = value.fHardwareBlockSum;
    this->fHardwareBlockSumError = value.fHardwareBlockSumError;
    this->fHardwareBlockSumM2 = value.fHardwareBlockSumM2;
    this->fHardwareBlockSum_raw = value.fHardwareBlockSum_raw;
    this->fSoftwareBlockSum_raw = value.fSoftwareBlockSum_raw;
    
    // Copy block data
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      this->fBlock[i] = value.fBlock[i];
      this->fBlockError[i] = value.fBlockError[i];
      this->fBlockM2[i] = value.fBlockM2[i];
      this->fBlock_raw[i] = value.fBlock_raw[i];
    }
    
    // Copy other properties
    this->fSequenceNumber = value.fSequenceNumber;
    this->fNumberOfSamples = value.fNumberOfSamples;
    this->fBlocksPerEvent = value.fBlocksPerEvent;
    this->fErrorFlag = value.fErrorFlag;
    this->fGoodEventCount = value.fGoodEventCount;
  }
  return *this;
}

/**
 * Assign value from VQwDataElement pointer
 */
void QwVQWK_Channel::AssignValueFrom(const VQwDataElement* valueptr)
{
  const QwVQWK_Channel* tmpptr = dynamic_cast<const QwVQWK_Channel*>(valueptr);
  if (tmpptr != NULL) {
    *this = *tmpptr;
  } else {
    TString loc = "Standard exception from QwVQWK_Channel::AssignValueFrom = "
      + valueptr->GetElementName() + " is an incompatible type.";
    throw std::invalid_argument(loc.Data());
  }
}

/**
 * Add value from VQwHardwareChannel pointer
 */
void QwVQWK_Channel::AddValueFrom(const VQwHardwareChannel* valueptr)
{
  const QwVQWK_Channel* tmpptr = dynamic_cast<const QwVQWK_Channel*>(valueptr);
  if (tmpptr != NULL) {
    *this += *tmpptr;
  } else {
    TString loc = "Standard exception from QwVQWK_Channel::AddValueFrom = "
      + valueptr->GetElementName() + " is an incompatible type.";
    throw std::invalid_argument(loc.Data());
  }
}

/**
 * Subtract value from VQwHardwareChannel pointer
 */
void QwVQWK_Channel::SubtractValueFrom(const VQwHardwareChannel* valueptr)
{
  const QwVQWK_Channel* tmpptr = dynamic_cast<const QwVQWK_Channel*>(valueptr);
  if (tmpptr != NULL) {
    *this -= *tmpptr;
  } else {
    TString loc = "Standard exception from QwVQWK_Channel::SubtractValueFrom = "
      + valueptr->GetElementName() + " is an incompatible type.";
    throw std::invalid_argument(loc.Data());
  }
}

/**
 * Multiply by VQwHardwareChannel pointer
 */
void QwVQWK_Channel::MultiplyBy(const VQwHardwareChannel* valueptr)
{
  const QwVQWK_Channel* tmpptr = dynamic_cast<const QwVQWK_Channel*>(valueptr);
  if (tmpptr != NULL) {
    *this *= *tmpptr;
  } else {
    TString loc = "Standard exception from QwVQWK_Channel::MultiplyBy = "
      + valueptr->GetElementName() + " is an incompatible type.";
    throw std::invalid_argument(loc.Data());
  }
}

/**
 * Match sequence number
 */
Bool_t QwVQWK_Channel::MatchSequenceNumber(size_t seqnum)
{
  return (fSequenceNumber == seqnum);
}

/**
 * Match number of samples
 */
Bool_t QwVQWK_Channel::MatchNumberOfSamples(size_t numsamp)
{
  return (fNumberOfSamples == numsamp);
}

/**
 * Calculate running average
 */
void QwVQWK_Channel::CalculateRunningAverage()
{
  if (fGoodEventCount > 0) {
    fHardwareBlockSum /= fGoodEventCount;
    fHardwareBlockSumError = sqrt(fHardwareBlockSumM2) / fGoodEventCount;
    
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlock[i] /= fGoodEventCount;
      fBlockError[i] = sqrt(fBlockM2[i]) / fGoodEventCount;
    }
  }
}

/**
 * Accumulate running sum with QwVQWK_Channel reference and error mask
 */
void QwVQWK_Channel::AccumulateRunningSum(const QwVQWK_Channel& value, Int_t count, Int_t ErrorMask)
{
  if (count == 0) {
    count = value.fGoodEventCount;
  }
  
  if ((value.fErrorFlag & ErrorMask) == 0) {
    // Only accumulate if no relevant error flags are set
    fHardwareBlockSum += value.fHardwareBlockSum * count;
    fHardwareBlockSumM2 += value.fHardwareBlockSumM2 * count;
    
    for (Int_t i = 0; i < fBlocksPerEvent; i++) {
      fBlock[i] += value.fBlock[i] * count;
      fBlockM2[i] += value.fBlockM2[i] * count;
    }
    
    fGoodEventCount += count;
  }
  
  // Always update error flags
  fErrorFlag |= (value.fErrorFlag & ErrorMask);
}

/**
 * Print channel value
 */
void QwVQWK_Channel::PrintValue() const
{
  QwMessage << "QwVQWK_Channel " << GetElementName() 
            << ": HW Sum = " << fHardwareBlockSum 
            << " +/- " << fHardwareBlockSumError
            << " (raw: " << fHardwareBlockSum_raw << ")"
            << ", Samples = " << fNumberOfSamples
            << ", Sequence = " << fSequenceNumber;
  
  if (fErrorFlag != 0) {
    QwMessage << ", Error Flag = 0x" << std::hex << fErrorFlag << std::dec;
  }
  
  QwMessage << QwLog::endl;
}