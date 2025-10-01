/**********************************************************\
* File: QwIntegrationPMT.cc                               *
*                                                         *
* Author:                                                 *
* Time-stamp:                                             *
\**********************************************************/

#include "QwIntegrationPMT.h"

// System headers
#include <stdexcept>

// ROOT headers
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RField.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#ifdef __USE_DATABASE__
#include "QwDBInterface.h"
#endif

/********************************************************/
/**
 * \brief Set the pedestal value and propagate to the underlying ADC channel.
 * \param pedestal Pedestal offset to apply to the raw signal.
 */
void QwIntegrationPMT::SetPedestal(Double_t pedestal)
{
	fPedestal=pedestal;
	fTriumf_ADC.SetPedestal(fPedestal);
	return;
}

/**
 * \brief Set the calibration factor and propagate to the underlying ADC channel.
 * \param calib Multiplicative calibration factor.
 */
void QwIntegrationPMT::SetCalibrationFactor(Double_t calib)
{
	fCalibration=calib;
	fTriumf_ADC.SetCalibrationFactor(fCalibration);
	return;
}
/********************************************************/
/**
 * \brief Initialize the PMT channel with a name and data-saving mode.
 * \param name Detector name used for branches and histograms.
 * \param datatosave Storage mode (e.g., "raw" or "derived").
 */
void  QwIntegrationPMT::InitializeChannel(TString name, TString datatosave)
{
  SetPedestal(0.);
  SetCalibrationFactor(1.);
  fTriumf_ADC.InitializeChannel(name,datatosave);
  SetElementName(name);
  SetBlindability(kTRUE);
  SetNormalizability(kTRUE);
  return;
}
/********************************************************/
/**
 * \brief Initialize the PMT channel with subsystem and name.
 * \param subsystem Subsystem identifier for foldering.
 * \param name Detector name used for branches and histograms.
 * \param datatosave Storage mode (e.g., "raw" or "derived").
 */
void  QwIntegrationPMT::InitializeChannel(TString subsystem, TString name, TString datatosave)
{
  SetPedestal(0.);
  SetCalibrationFactor(1.);
  fTriumf_ADC.InitializeChannel(subsystem,"QwIntegrationPMT", name, datatosave);
  SetElementName(name);
  SetBlindability(kTRUE);
  SetNormalizability(kTRUE);
  return;
}
/********************************************************/
/**
 * \brief Initialize the PMT channel including an intermediate module tag.
 * \param subsystem Subsystem identifier.
 * \param module Module type tag used in ROOT foldering.
 * \param name Detector name used for branches and histograms.
 * \param datatosave Storage mode (e.g., "raw" or "derived").
 */
void  QwIntegrationPMT::InitializeChannel(TString subsystem, TString module, TString name, TString datatosave)
{
  SetPedestal(0.);
  SetCalibrationFactor(1.);
  fTriumf_ADC.InitializeChannel(subsystem,module, name, datatosave);
  SetElementName(name);
  SetBlindability(kTRUE);
  SetNormalizability(kTRUE);
  return;
}
/********************************************************/
/** \brief Clear event-scoped data in the underlying ADC channel. */
void QwIntegrationPMT::ClearEventData()
{
  fTriumf_ADC.ClearEventData();
  return;
}
/********************************************************/
/** \brief Print accumulated error counters for this PMT. */
void QwIntegrationPMT::PrintErrorCounters(){
  fTriumf_ADC.PrintErrorCounters();
}
/********************************************************/
/** \brief Use an external random variable source for mock data. */
void QwIntegrationPMT::UseExternalRandomVariable()
{
  fTriumf_ADC.UseExternalRandomVariable();
  return;
}
/********************************************************/
/**
 * \brief Set the external random variable to drive mock data.
 * \param random_variable External random value.
 */
void QwIntegrationPMT::SetExternalRandomVariable(double random_variable)
{
  fTriumf_ADC.SetExternalRandomVariable(random_variable);
  return;
}
/********************************************************/
/**
 * \brief Configure deterministic drift parameters applied per event.
 * \param amplitude Drift amplitude.
 * \param phase Drift phase in radians.
 * \param frequency Drift frequency.
 */
void QwIntegrationPMT::SetRandomEventDriftParameters(Double_t amplitude, Double_t phase, Double_t frequency)
{
  fTriumf_ADC.SetRandomEventDriftParameters(amplitude, phase, frequency);
  return;
}
/********************************************************/
/**
 * \brief Add additional drift parameters to the existing drift model.
 * \param amplitude Drift amplitude.
 * \param phase Drift phase in radians.
 * \param frequency Drift frequency.
 */
void QwIntegrationPMT::AddRandomEventDriftParameters(Double_t amplitude, Double_t phase, Double_t frequency)
{
  fTriumf_ADC.AddRandomEventDriftParameters(amplitude, phase, frequency);
  return;
}
/********************************************************/
/**
 * \brief Configure Gaussian mock data parameters.
 * \param mean Mean of the generated distribution.
 * \param sigma Standard deviation of the distribution.
 */
void QwIntegrationPMT::SetRandomEventParameters(Double_t mean, Double_t sigma)
{
  fTriumf_ADC.SetRandomEventParameters(mean, sigma);
  return;
}
/********************************************************/
/**
 * \brief Set an asymmetry parameter applied to helicity states.
 * \param asymmetry Fractional asymmetry to apply.
 */
void QwIntegrationPMT::SetRandomEventAsymmetry(Double_t asymmetry)
{
  fTriumf_ADC.SetRandomEventAsymmetry(asymmetry);
  return;
}
/********************************************************/
/**
 * \brief Generate mock data for a single event.
 * \param helicity Helicity state indicator.
 * \param time Event time or timestamp proxy.
 */
void QwIntegrationPMT::RandomizeEventData(int helicity, double time)
{
  fTriumf_ADC.RandomizeEventData(helicity, time);
  return;
}

/********************************************************/
/**
 * \brief Generate a mock MOLLER detector event using beam parameters.
 * \param helicity Helicity state indicator.
 * \param charge Beam charge reference.
 * \param xpos Beam X position.
 * \param ypos Beam Y position.
 * \param xprime Beam X angle.
 * \param yprime Beam Y angle.
 * \param energy Beam energy reference.
 */
void QwIntegrationPMT::RandomizeMollerEvent(int helicity, const QwBeamCharge& charge, const QwBeamPosition& xpos, const QwBeamPosition& ypos, const QwBeamAngle& xprime, const QwBeamAngle& yprime, const QwBeamEnergy& energy)
{
  QwMollerADC_Channel temp(this->fTriumf_ADC);
  fTriumf_ADC.ClearEventData();

  temp.AssignScaledValue(xpos, fCoeff_x);
  fTriumf_ADC += temp;

  temp.AssignScaledValue(ypos, fCoeff_y);
  fTriumf_ADC += temp;

  temp.AssignScaledValue(xprime, fCoeff_xp);
  fTriumf_ADC += temp;

  temp.AssignScaledValue(yprime, fCoeff_yp);
  fTriumf_ADC += temp;

  temp.AssignScaledValue(energy, fCoeff_e);
  fTriumf_ADC += temp;

//fTriumf_ADC.AddChannelOffset(fBaseRate * (1+helicity*fAsym));
  fTriumf_ADC.AddChannelOffset(1.0+helicity*fAsym);

  fTriumf_ADC *= charge;
  fTriumf_ADC.Scale(fNormRate*fVoltPerHz);  //  After this Scale function, fTriumf_ADC should be the detector signal in volts.
  fTriumf_ADC.ForceMapfileSampleSize();
  //  Double_t voltage_width = sqrt(fTriumf_ADC.GetValue()*window_length/fVoltPerHz)/(window_length/fVoltPerHz);
  Double_t voltage_width = sqrt( fTriumf_ADC.GetValue() / (fTriumf_ADC.GetNumberOfSamples()*QwMollerADC_Channel::kTimePerSample/Qw::sec/fVoltPerHz) );
  //std::cout << "Voltage Width: " << voltage_width << std::endl;
  fTriumf_ADC.SmearByResolution(voltage_width);
  fTriumf_ADC.SetRawEventData();

  return;

}


/********************************************************/
/**
 * \brief Set the hardware-level sum measurement for a sequence.
 * \param hwsum Hardware sum value.
 * \param sequencenumber Sequence identifier.
 */
void QwIntegrationPMT::SetHardwareSum(Double_t hwsum, UInt_t sequencenumber)
{
  fTriumf_ADC.SetHardwareSum(hwsum, sequencenumber);
  return;
}

/** \brief Get the integrated value over the current event window. */
Double_t QwIntegrationPMT::GetValue()
{
  return fTriumf_ADC.GetValue();
}

/**
 * \brief Get the integrated value for a specific block.
 * \param blocknum Block index within the event.
 */
Double_t QwIntegrationPMT::GetValue(Int_t blocknum)
{
  return fTriumf_ADC.GetValue(blocknum);
}

/********************************************************/
/**
 * \brief Set the block data for the current event sequence.
 * \param block Pointer to block data.
 * \param sequencenumber Sequence identifier.
 */
void QwIntegrationPMT::SetEventData(Double_t* block, UInt_t sequencenumber)
{
  fTriumf_ADC.SetEventData(block, sequencenumber);
  return;
}
/********************************************************/
/** \brief Encode current event data into an output buffer. */
void QwIntegrationPMT::EncodeEventData(std::vector<UInt_t> &buffer)
{
  fTriumf_ADC.EncodeEventData(buffer);
}
/********************************************************/
/** \brief Apply hardware checks and process the event for this PMT. */
void  QwIntegrationPMT::ProcessEvent()
{
  ApplyHWChecks();//first apply HW checks and update HW  error flags.
  fTriumf_ADC.ProcessEvent();

  return;
}
/********************************************************/
/**
 * \brief Apply hardware checks and return whether the event is valid.
 * \return kTRUE if no hardware error was detected; otherwise kFALSE.
 */
Bool_t QwIntegrationPMT::ApplyHWChecks()
{
  Bool_t eventokay=kTRUE;

  UInt_t deviceerror=fTriumf_ADC.ApplyHWChecks();//will check for consistancy between HWSUM and SWSUM also check for sample size
  eventokay=(deviceerror & 0x0);//if no HW error return true


  return eventokay;
}
/********************************************************/

/**
 * \brief Set basic single-event cut limits.
 * \param LL Lower limit.
 * \param UL Upper limit.
 * \return 1 on success.
 */
Int_t QwIntegrationPMT::SetSingleEventCuts(Double_t LL=0, Double_t UL=0){//std::vector<Double_t> & dEventCuts){//two limts and sample size
  fTriumf_ADC.SetSingleEventCuts(LL,UL);
  return 1;
}

/********************************************************/
/**
 * \brief Configure detailed single-event cuts for this PMT.
 * \param errorflag Device-specific error flag mask to set.
 * \param LL Lower limit.
 * \param UL Upper limit.
 * \param stability Stability threshold.
 * \param burplevel Burp detection threshold.
 */
void QwIntegrationPMT::SetSingleEventCuts(UInt_t errorflag, Double_t LL=0, Double_t UL=0, Double_t stability=0, Double_t burplevel=0){
  //set the unique tag to identify device type (bcm,bpm & etc)
  errorflag|=kPMTErrorFlag;
  QwMessage<<"QwIntegrationPMT Error Code passing to QwMollerADC_Ch "<<errorflag<<QwLog::endl;
  fTriumf_ADC.SetSingleEventCuts(errorflag,LL,UL,stability,burplevel);

}

/********************************************************/

/** \brief Set the default sample size used by the ADC channel. */
void QwIntegrationPMT::SetDefaultSampleSize(Int_t sample_size){
 fTriumf_ADC.SetDefaultSampleSize((size_t)sample_size);
}

/********************************************************/
/** \brief Set the saturation voltage limit for the ADC front-end. */
void QwIntegrationPMT::SetSaturationLimit(Double_t saturation_volt){
  fTriumf_ADC.SetMollerADCSaturationLimt(saturation_volt);
}
//*/

/********************************************************/
/**
 * \brief Apply single-event cuts for this PMT and return pass/fail.
 * \return kTRUE if event passes, otherwise kFALSE.
 */
Bool_t QwIntegrationPMT::ApplySingleEventCuts(){


//std::cout<<" QwBCM::SingleEventCuts() "<<std::endl;
  Bool_t status=kTRUE;

  if (fTriumf_ADC.ApplySingleEventCuts()){
    status=kTRUE;
    //std::cout<<" BCM Sample size "<<fTriumf_ADC.GetNumberOfSamples()<<std::endl;
  }
  else{
    status&=kFALSE;//kTRUE;//kFALSE;
  }

  return status;

}

/********************************************************/

/** \brief Print error counters (const overload). */
void QwIntegrationPMT::PrintErrorCounters() const{// report number of events failed due to HW and event cut faliure
  fTriumf_ADC.PrintErrorCounters();
}

/*********************************************************/

/**
 * \brief Check for burp failures by comparing against a reference PMT.
 * \param ev_error Reference PMT to compare error conditions.
 * \return kTRUE if a burp failure was detected; otherwise kFALSE.
 */
Bool_t QwIntegrationPMT::CheckForBurpFail(const VQwDataElement *ev_error){
  Bool_t burpstatus = kFALSE;
  try {
    if(typeid(*ev_error)==typeid(*this)) {
      //std::cout<<" Here in QwIntegrationPMT::CheckForBurpFail \n";
      if (this->GetElementName()!="") {
        const QwIntegrationPMT* value_pmt = dynamic_cast<const QwIntegrationPMT* >(ev_error);
        burpstatus |= fTriumf_ADC.CheckForBurpFail(&(value_pmt->fTriumf_ADC)); 
      }
    } else {
      TString loc="Standard exception from QwIntegrationPMT::CheckForBurpFail :"+
        ev_error->GetElementName()+" "+this->GetElementName()+" are not of the "
        +"same type";
      throw std::invalid_argument(loc.Data());
    }
  } catch (std::exception& e) {
    std::cerr<< e.what()<<std::endl;
  }
  return burpstatus;
};

/********************************************************/
/**
 * \brief Merge error flags from a reference PMT into this instance.
 * \param ev_error Reference PMT whose flags are merged.
 */
void QwIntegrationPMT::UpdateErrorFlag(const QwIntegrationPMT* ev_error){
  try {
    if(typeid(*ev_error)==typeid(*this)) {
      // std::cout<<" Here in QwIntegrationPMT::UpdateErrorFlag \n";
      if (this->GetElementName()!="") {
	fTriumf_ADC.UpdateErrorFlag(ev_error->fTriumf_ADC);
      }
    } else {
      TString loc="Standard exception from QwIntegrationPMT::UpdateErrorFlag :"+
        ev_error->GetElementName()+" "+this->GetElementName()+" are not of the "
        +"same type";
      throw std::invalid_argument(loc.Data());
    }
  } catch (std::exception& e) {
    std::cerr<< e.what()<<std::endl;
  }  
};

/********************************************************/


/**
 * \brief Process the raw event buffer and decode into the ADC channel.
 * \return The updated buffer word position.
 */
Int_t QwIntegrationPMT::ProcessEvBuffer(UInt_t* buffer, UInt_t word_position_in_buffer, UInt_t subelement)
{
  fTriumf_ADC.ProcessEvBuffer(buffer,word_position_in_buffer);

  return word_position_in_buffer;
}  Double_t fULimit, fLLimit;

/********************************************************/
/** \brief Copy-assign from another PMT (event-scoped data). */
QwIntegrationPMT& QwIntegrationPMT::operator= (const QwIntegrationPMT &value)
{
//   std::cout<<" Here in QwIntegrationPMT::operator= \n";
  if (GetElementName()!="")
    {
      this->fTriumf_ADC=value.fTriumf_ADC;
      this->fPedestal=value.fPedestal;
      this->fCalibration=value.fCalibration;
    }
//   std::cout<<" to be copied \n";
//   value.Print();
//   std::cout<<" copied \n";
//   this->Print();

  return *this;
}

/** \brief Add-assign from another PMT (sum raw channels). */
QwIntegrationPMT& QwIntegrationPMT::operator+= (const QwIntegrationPMT &value)
{
  if (GetElementName()!="")
    {
      this->fTriumf_ADC+=value.fTriumf_ADC;
      this->fPedestal+=value.fPedestal;
      this->fCalibration=0;
    }
  return *this;
}

QwIntegrationPMT& QwIntegrationPMT::operator-= (const QwIntegrationPMT &value)
{
  if (GetElementName()!="")
    {
      this->fTriumf_ADC-=value.fTriumf_ADC;
      this->fPedestal-=value.fPedestal;
      this->fCalibration=0;
    }
  return *this;
}

void QwIntegrationPMT::Sum(const QwIntegrationPMT &value1, const QwIntegrationPMT &value2) {
  *this = value1;
  *this += value2;
}

void QwIntegrationPMT::Difference(const QwIntegrationPMT &value1, const QwIntegrationPMT &value2) {
  *this = value1;
  *this -= value2;
}

void QwIntegrationPMT::Ratio(QwIntegrationPMT &numer, QwIntegrationPMT &denom)
{
  //  std::cout<<"QwIntegrationPMT::Ratio element name ="<<GetElementName()<<" \n";
  if (GetElementName()!="")
    {
      //  std::cout<<"here in \n";
      this->fTriumf_ADC.Ratio(numer.fTriumf_ADC,denom.fTriumf_ADC);
      this->fPedestal=0;
      this->fCalibration=0;
    }
  return;
}

void QwIntegrationPMT::Scale(Double_t factor)
{
  fTriumf_ADC.Scale(factor);
  return;
}

void QwIntegrationPMT::Normalize(VQwDataElement* denom)
{
  if (fIsNormalizable) {
    QwMollerADC_Channel* denom_ptr = dynamic_cast<QwMollerADC_Channel*>(denom);
    QwMollerADC_Channel vqwk_denom(*denom_ptr);
    fTriumf_ADC.DivideBy(vqwk_denom);
  }
}

void QwIntegrationPMT::PrintValue() const
{
  fTriumf_ADC.PrintValue();
}

void QwIntegrationPMT::PrintInfo() const
{
  //std::cout<<"QwMollerADC_Channel Info " <<std::endl;
  //std::cout<<" Running AVG "<<GetElementName()<<" current running AVG "<<IntegrationPMT_Running_AVG<<std::endl;
  std::cout<<"QwMollerADC_Channel Info " <<std::endl;
  fTriumf_ADC.PrintInfo();
  std::cout<< "Blindability is "    << (fIsBlindable?"TRUE":"FALSE") 
	   <<std::endl;
  std::cout<< "Normalizability is " << (fIsNormalizable?"TRUE":"FALSE")
	   <<std::endl;
  std::cout << "fNormRate=" << fNormRate << "fVoltPerHz=" << fVoltPerHz 
            << " Asym=" << fAsym << " C_x=" << fCoeff_x << " C_y=" << fCoeff_y 
            << " C_xp=" << fCoeff_xp << " C_yp=" << fCoeff_yp 
            << " C_e=" << fCoeff_e << std::endl;
  return;
}

/********************************************************/
void  QwIntegrationPMT::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  if (GetElementName()=="")
    {
      //  This channel is not used, so skip filling the histograms.
    }
  else
    {
      fTriumf_ADC.ConstructHistograms(folder, prefix);
    }
  return;
}

void  QwIntegrationPMT::FillHistograms()
{
  if (GetElementName()=="")
    {
      //  This channel is not used, so skip filling the histograms.
    }
  else
    {
      fTriumf_ADC.FillHistograms();
    }


  return;
}

void  QwIntegrationPMT::ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values)
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  } else
    {
      fTriumf_ADC.ConstructBranchAndVector(tree, prefix,values);
    }
  return;
}

void  QwIntegrationPMT::ConstructBranch(TTree *tree, TString &prefix)
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  } else
    {
      fTriumf_ADC.ConstructBranch(tree, prefix);
    }
  return;
}

void  QwIntegrationPMT::ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist)
{
   TString devicename;
   devicename=GetElementName();
   devicename.ToLower();
   if (GetElementName()==""){
     //  This channel is not used, so skip filling the histograms.
   } else {
     if (modulelist.HasValue(devicename)){
       fTriumf_ADC.ConstructBranch(tree, prefix);
       QwMessage <<"QwIntegrationPMT Tree leave added to "<<devicename<<QwLog::endl;
       }

   }
  return;
}


void  QwIntegrationPMT::FillTreeVector(QwRootTreeBranchVector &values) const
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  } else {
    fTriumf_ADC.FillTreeVector(values);
  }
}

#ifdef HAS_RNTUPLE_SUPPORT
void QwIntegrationPMT::ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, QwRootTreeBranchVector &values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs)
{
  if (GetElementName()==""){
    //  This channel is not used, so skip RNTuple construction.
  } else
    {
      fTriumf_ADC.ConstructNTupleAndVector(model, prefix, values, fieldPtrs);
    }
}

void QwIntegrationPMT::FillNTupleVector(QwRootTreeBranchVector &values) const
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the RNTuple.
  } else {
    fTriumf_ADC.FillNTupleVector(values);
  }
}
#endif // HAS_RNTUPLE_SUPPORT

void QwIntegrationPMT::CalculateRunningAverage()
{
  fTriumf_ADC.CalculateRunningAverage();
}

void QwIntegrationPMT::AccumulateRunningSum(const QwIntegrationPMT& value, Int_t count, Int_t ErrorMask)
{
  fTriumf_ADC.AccumulateRunningSum(value.fTriumf_ADC, count, ErrorMask);
}

void QwIntegrationPMT::DeaccumulateRunningSum(QwIntegrationPMT& value, Int_t ErrorMask)
{
  fTriumf_ADC.DeaccumulateRunningSum(value.fTriumf_ADC, ErrorMask);
}


void QwIntegrationPMT::Blind(const QwBlinder *blinder)
{
  if (fIsBlindable)  fTriumf_ADC.Blind(blinder);
}

void QwIntegrationPMT::Blind(const QwBlinder *blinder, const QwIntegrationPMT& yield)
{
  if (fIsBlindable)  fTriumf_ADC.Blind(blinder, yield.fTriumf_ADC);
}

#ifdef __USE_DATABASE__
std::vector<QwDBInterface> QwIntegrationPMT::GetDBEntry()
{
  std::vector <QwDBInterface> row_list;
  row_list.clear();
  fTriumf_ADC.AddEntriesToList(row_list);
  return row_list;

}

std::vector<QwErrDBInterface> QwIntegrationPMT::GetErrDBEntry()
{
  std::vector <QwErrDBInterface> row_list;
  row_list.clear();
  fTriumf_ADC.AddErrEntriesToList(row_list);
  return row_list;
};
#endif
