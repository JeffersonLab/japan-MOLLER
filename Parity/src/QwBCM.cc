/*!
 * \file   QwBCM.cc
 * \brief  Beam current monitor template class implementation
 */

#include "QwBCM.h"

// System headers
#include <stdexcept>

// ROOT headers for RNTuple support
#ifdef HAS_RNTUPLE_SUPPORT
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleWriter.hxx>
#endif // HAS_RNTUPLE_SUPPORT

// Qweak database headers
#ifdef __USE_DATABASE__
#include "QwDBInterface.h"
#endif // __USE_DATABASE__

//  Qweak types that we want to use in this template
#include "QwVQWK_Channel.h"
#include "QwADC18_Channel.h"
#include "QwScaler_Channel.h"
#include "QwMollerADC_Channel.h"
/********************************************************/
/**
 * \brief Set the pedestal value for the beam current monitor.
 * \param pedestal Pedestal offset to apply to the raw signal.
 */
template<typename T>
void QwBCM<T>::SetPedestal(Double_t pedestal)
{
  fBeamCurrent.SetPedestal(pedestal);
}

/**
 * \brief Set the calibration factor for the beam current monitor.
 * \param calibration Multiplicative calibration factor.
 */
template<typename T>
void QwBCM<T>::SetCalibrationFactor(Double_t calibration)
{
  fBeamCurrent.SetCalibrationFactor(calibration);
}
/********************************************************/
/**
 * \brief Initialize the BCM with a name and data-saving mode.
 * \param name Detector name used for branches and histograms.
 * \param datatosave Storage mode (e.g., "raw" or "derived").
 */
template<typename T>
void QwBCM<T>::InitializeChannel(TString name, TString datatosave)
{
  SetPedestal(0.);
  SetCalibrationFactor(1.);
  SetResolution(0.0);
  fBeamCurrent.InitializeChannel(name,datatosave);
  this->SetElementName(name);
}
/********************************************************/
/**
 * \brief Initialize the BCM with subsystem and name.
 * \param subsystem Subsystem identifier.
 * \param name Detector name used for branches and histograms.
 * \param datatosave Storage mode (e.g., "raw" or "derived").
 */
template<typename T>
void QwBCM<T>::InitializeChannel(TString subsystem, TString name, TString datatosave){
  SetPedestal(0.);
  SetCalibrationFactor(1.);
  SetResolution(0.0);
  fBeamCurrent.InitializeChannel(subsystem, "QwBCM", name, datatosave);
  SetElementName(name);
}
/********************************************************/
/**
 * \brief Initialize the BCM with subsystem, module type, and name.
 * \param subsystem Subsystem identifier.
 * \param name Detector name used for branches and histograms.
 * \param type Module type tag used in ROOT foldering.
 * \param datatosave Storage mode (e.g., "raw" or "derived").
 */
template<typename T>
void QwBCM<T>::InitializeChannel(TString subsystem, TString name, TString type, TString datatosave){
  SetPedestal(0.);
  SetCalibrationFactor(1.);
  SetResolution(0.0);
  SetModuleType(type);
  fBeamCurrent.InitializeChannel(subsystem, "QwBCM", name, datatosave);
  SetElementName(name);
}
/********************************************************/
/**
 * \brief Load mock-data configuration for the underlying channel.
 * \param paramfile Parameter file reader positioned at channel section.
 */
template<typename T>
void QwBCM<T>::LoadMockDataParameters(QwParameterFile &paramfile) {
/*
  Bool_t ldebug=kFALSE;
  Double_t asym=0.0, mean=0.0, sigma=0.0;
*/
  Double_t res=0.0;

  if (paramfile.GetLine().find("resolution")!=std::string::npos){
    paramfile.GetNextToken();
    res = paramfile.GetTypedNextToken<Double_t>();
    this->SetResolution(res);
  } else {
/*
    asym    = paramfile.GetTypedNextToken<Double_t>();
    mean    = paramfile.GetTypedNextToken<Double_t>();
    sigma   = paramfile.GetTypedNextToken<Double_t>();

    if (ldebug==1) {
      std::cout << "#################### \n";
      std::cout << "asym, mean, sigma \n" << std::endl;
      std::cout << asym                   << " / "
	        << mean                   << " / "
	        << sigma                  << " / "
                << std::endl;
    }
    this->SetRandomEventParameters(mean, sigma);
    this->SetRandomEventAsymmetry(asym);
*/
    //std::cout << "In QwBCM: ChannelName = " << GetElementName() << std::endl;
    fBeamCurrent.SetMockDataAsDiff();
    fBeamCurrent.LoadMockDataParameters(paramfile);
  }
}
/********************************************************/
/** \brief Clear event-scoped data in the underlying channel. */
template<typename T>
void QwBCM<T>::ClearEventData()
{
  fBeamCurrent.ClearEventData();
}

/********************************************************/
/** \brief Use an external random variable source for mock data. */
template<typename T>
void QwBCM<T>::UseExternalRandomVariable()
{
  fBeamCurrent.UseExternalRandomVariable();
}
/********************************************************/
/**
 * \brief Set the external random variable to drive mock data.
 * \param random_variable External random value.
 */
template<typename T>
void QwBCM<T>::SetExternalRandomVariable(double random_variable)
{
  fBeamCurrent.SetExternalRandomVariable(random_variable);
}
/********************************************************/
/**
 * \brief Configure deterministic drift parameters applied per event.
 */
template<typename T>
void QwBCM<T>::SetRandomEventDriftParameters(Double_t amplitude, Double_t phase, Double_t frequency)
{
  fBeamCurrent.SetRandomEventDriftParameters(amplitude, phase, frequency);
}
/********************************************************/
/** \brief Add additional drift parameters to the drift model. */
template<typename T>
void QwBCM<T>::AddRandomEventDriftParameters(Double_t amplitude, Double_t phase, Double_t frequency)
{
  fBeamCurrent.AddRandomEventDriftParameters(amplitude, phase, frequency);
}
/********************************************************/
/** \brief Configure Gaussian mock data parameters. */
template<typename T>
void QwBCM<T>::SetRandomEventParameters(Double_t mean, Double_t sigma)
{
  fBeamCurrent.SetRandomEventParameters(mean, sigma);
}
/********************************************************/
/** \brief Set an asymmetry parameter applied to helicity states. */
template<typename T>
void QwBCM<T>::SetRandomEventAsymmetry(Double_t asymmetry)
{
  fBeamCurrent.SetRandomEventAsymmetry(asymmetry);
}
/********************************************************/
/** \brief Smear the channel by the configured resolution. */
template<typename T>
void QwBCM<T>::ApplyResolutionSmearing(){
   fBeamCurrent.SmearByResolution(fResolution);
}
/** \brief Materialize the current event state as raw event data. */
template<typename T>
void QwBCM<T>::FillRawEventData()
{
  fBeamCurrent.SetRawEventData();
}

/**
 * \brief Generate mock event data for this BCM.
 * \param helicity Helicity state indicator.
 * \param time Event time or timestamp proxy.
 */
template<typename T>
void QwBCM<T>::RandomizeEventData(int helicity, double time)
{
  fBeamCurrent.RandomizeEventData(helicity, time);
  fBeamCurrent.SetRawEventData();

}
/********************************************************/
/** \brief Encode current event data into an output buffer. */
template<typename T>
void QwBCM<T>::EncodeEventData(std::vector<UInt_t> &buffer)
{
  fBeamCurrent.EncodeEventData(buffer);
}

/********************************************************/
/** \brief Apply hardware checks and process the event for this BCM. */
template<typename T>
void QwBCM<T>::ProcessEvent()
{
  this->ApplyHWChecks();//first apply HW checks and update HW  error flags. Calling this routine either in ApplySingleEventCuts or here do not make any difference for a BCM but do for a BPMs because they have derived devices.
  fBeamCurrent.ProcessEvent();
}
/********************************************************/
/**
 * \brief Apply hardware checks and return whether the event is valid.
 * \return kTRUE if no hardware error was detected; otherwise kFALSE.
 */
template<typename T>
Bool_t QwBCM<T>::ApplyHWChecks()
{
  Bool_t eventokay=kTRUE;

  UInt_t deviceerror=fBeamCurrent.ApplyHWChecks();//will check for HW consistency and return the error code (=0 is HW good)
  eventokay=(deviceerror & 0x0);//if no HW error return true

  return eventokay;
}
/********************************************************/

/** \brief Set basic single-event cut limits. */
template<typename T>
Int_t QwBCM<T>::SetSingleEventCuts(Double_t LL, Double_t UL){//std::vector<Double_t> & dEventCuts){//two limits and sample size
  fBeamCurrent.SetSingleEventCuts(LL,UL);
  return 1;
}

/**
 * \brief Configure detailed single-event cuts for this BCM.
 */
template<typename T>
void QwBCM<T>::SetSingleEventCuts(UInt_t errorflag, Double_t LL, Double_t UL, Double_t stability, Double_t burplevel){
  //set the unique tag to identify device type (bcm,bpm & etc)
  errorflag|=kBCMErrorFlag;
  QwMessage<<"QwBCM Error Code passing to QwVQWK_Ch "<<errorflag<<" "<<stability<<QwLog::endl;
  //QwError<<"***************************"<<typeid(fBeamCurrent).name()<<QwLog::endl;
  fBeamCurrent.SetSingleEventCuts(errorflag,LL,UL,stability,burplevel);

}

/** \brief Set the default sample size used by the channel. */
template<typename T>
void QwBCM<T>::SetDefaultSampleSize(Int_t sample_size){
  fBeamCurrent.SetDefaultSampleSize((size_t)sample_size);
}


/********************************************************/
/**
 * \brief Apply single-event cuts for this BCM and return pass/fail.
 * \return kTRUE if event passes, otherwise kFALSE.
 */
template<typename T>
Bool_t QwBCM<T>::ApplySingleEventCuts()
{
  //std::cout<<" QwBCM::SingleEventCuts() "<<std::endl;
  Bool_t status = kTRUE;

  if (fBeamCurrent.ApplySingleEventCuts()) {
    status = kTRUE;
  } else {
    status &= kFALSE;
  }
  return status;
}

/********************************************************/

/** \brief Increment error counters (number of failed events). */
template<typename T>
void QwBCM<T>::IncrementErrorCounters()
{// report number of events failed due to HW and event cut failure
  fBeamCurrent.IncrementErrorCounters();
}

/** \brief Print error counters (const overload). */
template<typename T>
void QwBCM<T>::PrintErrorCounters() const
{// report number of events failed due to HW and event cut failure
  fBeamCurrent.PrintErrorCounters();
}

/********************************************************/
/** \brief Merge error flags from a reference BCM into this instance. */
template<typename T>
void QwBCM<T>::UpdateErrorFlag(const VQwBCM *ev_error){
  try {
    if(typeid(*ev_error)==typeid(*this)) {
      // std::cout<<" Here in QwBCM::UpdateErrorFlag \n";
      if (this->GetElementName()!="") {
        const QwBCM<T>* value_bcm = dynamic_cast<const QwBCM<T>* >(ev_error);
	fBeamCurrent.UpdateErrorFlag(value_bcm->fBeamCurrent);
      }
    } else {
      TString loc="Standard exception from QwBCM::UpdateErrorFlag :"+
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
 * \brief Process the raw event buffer and decode into the channel.
 * \return The updated buffer word position.
 */
template<typename T>
Int_t QwBCM<T>::ProcessEvBuffer(UInt_t* buffer, UInt_t word_position_in_buffer, UInt_t subelement)
{
  fBeamCurrent.ProcessEvBuffer(buffer,word_position_in_buffer);

  return word_position_in_buffer;
}
/********************************************************/
/** \brief Copy-assign from another BCM (event-scoped data). */
template<typename T>
QwBCM<T>& QwBCM<T>::operator= (const QwBCM<T> &value)
{
//   std::cout<<" Here in QwBCM::operator= \n";
  if (this->GetElementName()!="")
    {
      this->fBeamCurrent=value.fBeamCurrent;
    }
//   std::cout<<" to be copied \n";
//   value.Print();
//   std::cout<<" copied \n";
//   this->Print();

  return *this;
}

/** \brief Polymorphic copy-assign from VQwBCM if types match. */
template<typename T>
VQwBCM& QwBCM<T>::operator= (const VQwBCM &value)
{
  try {
    if(typeid(value)==typeid(*this)) {
      // std::cout<<" Here in QwBCM::operator= \n";
      if (this->GetElementName()!="") {
        const QwBCM<T>* value_bcm = dynamic_cast<const QwBCM<T>* >(&value);
        QwBCM<T>* this_cast = dynamic_cast<QwBCM<T>* >(this);
        this_cast->fBeamCurrent= value_bcm->fBeamCurrent;
      }
    } else {
      TString loc="Standard exception from QwBCM::operator= :"+
        value.GetElementName()+" "+this->GetElementName()+" are not of the "
        +"same type";
      throw std::invalid_argument(loc.Data());
    }
  } catch (std::exception& e) {
    std::cerr<< e.what()<<std::endl;
  }
//   std::cout<<" to be copied \n";
//   value.Print();
//   std::cout<<" copied \n";
//   this->Print();

  return *this;
}

/** \brief Add-assign from another BCM (sum raw channels). */
template<typename T>
QwBCM<T>& QwBCM<T>::operator+= (const QwBCM<T> &value)
{
  if (this->GetElementName()!="")
    {
      this->fBeamCurrent+=value.fBeamCurrent;
    }
  return *this;
}

/** \brief Polymorphic add-assign from VQwBCM if types match. */
template<typename T>
VQwBCM& QwBCM<T>::operator+= (const VQwBCM &value)
{
  try {
    if(typeid(value)==typeid(*this)) {
      // std::cout<<" Here in QwBCM::operator+= \n";
      if (this->GetElementName()!="") {
        const QwBCM<T>* value_bcm = dynamic_cast<const QwBCM<T>* >(&value);
        QwBCM<T>* this_cast = dynamic_cast<QwBCM<T>* >(this);
        this_cast->fBeamCurrent+=value_bcm->fBeamCurrent;
      }
    } else {
      TString loc="Standard exception from QwBCM::operator+= :"+
        value.GetElementName()+" "+this->GetElementName()+" are not of the "
        +"same type";
      throw std::invalid_argument(loc.Data());
    }
  } catch (std::exception& e) {
    std::cerr<< e.what()<<std::endl;
  }

 return *this;
}

/** \brief Polymorphic subtract-assign from VQwBCM. */
template<typename T>
VQwBCM& QwBCM<T>::operator-= (const VQwBCM &value)
{
  if (this->GetElementName()!="")
    {
      const QwBCM<T>* value_bcm = dynamic_cast<const QwBCM<T>* >(&value);
      QwBCM<T>* this_cast = dynamic_cast<QwBCM<T>* >(this);
      this_cast->fBeamCurrent-=value_bcm->fBeamCurrent;
    }
  return *this;
}

/** \brief Subtract-assign from another BCM (difference raw channels). */
template<typename T>
QwBCM<T>& QwBCM<T>::operator-= (const QwBCM<T> &value)
{
  try {
    if(typeid(value)==typeid(*this)) {
      // std::cout<<" Here in QwBCM::operator-= \n";
      if (this->GetElementName()!="") {
        const QwBCM<T>* value_bcm = dynamic_cast<const QwBCM<T>* >(&value);
        QwBCM<T>* this_cast = dynamic_cast<QwBCM<T>* >(this);
        this_cast->fBeamCurrent-=value_bcm->fBeamCurrent;
      }
    } else {
      TString loc="Standard exception from QwBCM::operator-= :"+
        value.GetElementName()+" "+this->GetElementName()+" are not of the "
        +"same type";
      throw std::invalid_argument(loc.Data());
    }
  } catch (std::exception& e) {
    std::cerr<< e.what()<<std::endl;
  }
  return *this;
}

/** \brief Polymorphic ratio formation for BCM. */
template<typename T>
void QwBCM<T>::Ratio(const VQwBCM &numer, const VQwBCM &denom)
{
  Ratio(*dynamic_cast<const QwBCM<T>* >(&numer),
      *dynamic_cast<const QwBCM<T>* >(&denom));
}

/** \brief Type-specific ratio formation for BCM. */
template<typename T>
void QwBCM<T>::Ratio(const QwBCM<T> &numer, const QwBCM<T> &denom)
{
  //  std::cout<<"QwBCM::Ratio element name ="<<GetElementName()<<" \n";
  if (this->GetElementName()!="")
    {
      //  std::cout<<"here in \n";
      this->fBeamCurrent.Ratio(numer.fBeamCurrent,denom.fBeamCurrent);
    }
}

/** \brief Scale the underlying channel by a constant factor. */
template<typename T>
void QwBCM<T>::Scale(Double_t factor)
{
  fBeamCurrent.Scale(factor);
}

/** \brief Update running averages for the underlying channel. */
template<typename T>
void QwBCM<T>::CalculateRunningAverage()
{
  fBeamCurrent.CalculateRunningAverage();
}

/**
 * \brief Check for burp failures by delegating to the underlying channel.
 * \param ev_error Reference BCM to compare against.
 * \return kTRUE if a burp failure was detected; otherwise kFALSE.
 */
template<typename T>
Bool_t QwBCM<T>::CheckForBurpFail(const VQwDataElement *ev_error){
  Bool_t burpstatus = kFALSE;
  //QwError << "************* " << this->GetElementName() << "  <<<this, event>>>  " << ev_error->GetElementName() << " *****************" << QwLog::endl;
  try {
    if(typeid(*ev_error)==typeid(*this)) {
      //std::cout<<" Here in VQwBCM::CheckForBurpFail \n";
      if (this->GetElementName()!="") {
        const QwBCM<T>* value_bcm = dynamic_cast<const QwBCM<T>* >(ev_error);
        burpstatus |= fBeamCurrent.CheckForBurpFail(&(value_bcm->fBeamCurrent));
      }
    } else {
      TString loc="Standard exception from QwBCM::CheckForBurpFail :"+
        ev_error->GetElementName()+" "+this->GetElementName()+" are not of the "
        +"same type";
      throw std::invalid_argument(loc.Data());
    }
  } catch (std::exception& e) {
    std::cerr<< e.what()<<std::endl;
  }
  return burpstatus;
}

/** \brief Accumulate running sums from another BCM into this one. */
template<typename T>
void QwBCM<T>::AccumulateRunningSum(const VQwBCM& value, Int_t count, Int_t ErrorMask) {
  fBeamCurrent.AccumulateRunningSum(
      dynamic_cast<const QwBCM<T>* >(&value)->fBeamCurrent, count, ErrorMask);
}

/** \brief Remove a single entry from the running sums using a source value. */
template<typename T>
void QwBCM<T>::DeaccumulateRunningSum(VQwBCM& value, Int_t ErrorMask) {
  fBeamCurrent.DeaccumulateRunningSum(dynamic_cast<QwBCM<T>* >(&value)->fBeamCurrent, ErrorMask);
}
/** \brief Print a compact value summary for this BCM. */
template<typename T>
void QwBCM<T>::PrintValue() const
{
  fBeamCurrent.PrintValue();
}

/** \brief Print detailed information for this BCM. */
template<typename T>
void QwBCM<T>::PrintInfo() const
{
  std::cout << "QwVQWK_Channel Info " << std::endl;
  fBeamCurrent.PrintInfo();
}

/********************************************************/
/**
 * \brief Define histograms for this BCM (delegated to underlying channel).
 * \param folder ROOT folder to contain histograms.
 * \param prefix Histogram name prefix.
 */
template<typename T>
void QwBCM<T>::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  if (this->GetElementName()=="")
    {
      //  This channel is not used, so skip filling the histograms.
    }
  else
    {
      fBeamCurrent.ConstructHistograms(folder, prefix);
    }
}

/** \brief Fill histograms for this BCM if enabled. */
template<typename T>
void QwBCM<T>::FillHistograms()
{
  if (this->GetElementName()=="")
    {
      //  This channel is not used, so skip filling the histograms.
    }
  else
    {
      fBeamCurrent.FillHistograms();
    }
}

/**
 * \brief Construct ROOT branches and value vector entries.
 * \param tree Output tree.
 * \param prefix Branch name prefix.
 * \param values Output value vector to be appended.
 */
template<typename T>
void QwBCM<T>::ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values)
{
  if (this->GetElementName()==""){
    //  This channel is not used, so skip
  } else
    {
      fBeamCurrent.ConstructBranchAndVector(tree, prefix,values);
    }
}

/** \brief Construct ROOT branches for this BCM (if enabled). */
template<typename T>
void  QwBCM<T>::ConstructBranch(TTree *tree, TString &prefix)
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  } else
    {
      fBeamCurrent.ConstructBranch(tree, prefix);
      // this functions doesn't do anything yet
    }
}

/** \brief Construct ROOT branches for this BCM using a trim file filter. */
template<typename T>
void  QwBCM<T>::ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist)
{
  TString devicename;

  devicename=GetElementName();
  devicename.ToLower();
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  } else
    {

      //QwMessage <<" QwBCM "<<devicename<<QwLog::endl;
      if (modulelist.HasValue(devicename)){
	fBeamCurrent.ConstructBranch(tree, prefix);
	QwMessage <<" Tree leave added to "<<devicename<<QwLog::endl;
      }
      // this functions doesn't do anything yet
    }
}

/** \brief Fill tree vector entries for this BCM. */
template<typename T>
void QwBCM<T>::FillTreeVector(QwRootTreeBranchVector &values) const
{
  if (this->GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  } else
    {
      fBeamCurrent.FillTreeVector(values);
      // this functions doesn't do anything yet
    }
}

#ifdef HAS_RNTUPLE_SUPPORT
/** \brief Construct RNTuple fields and append values vector entries. */
template<typename T>
void QwBCM<T>::ConstructNTupleAndVector(ROOT::RNTupleModel *model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs)
{
  if (this->GetElementName()==""){
    //  This channel is not used, so skip
  } else
    {
      fBeamCurrent.ConstructNTupleAndVector(model, prefix, values, fieldPtrs);
    }
}

/** \brief Fill RNTuple values for this BCM. */
template<typename T>
void QwBCM<T>::FillNTupleVector(std::vector<Double_t>& values) const
{
  if (this->GetElementName()==""){
    //  This channel is not used, so skip filling.
  } else
    {
      fBeamCurrent.FillNTupleVector(values);
    }
}
#endif

#ifdef __USE_DATABASE__
/** \brief Collect database entries for this BCM. */
template<typename T>
std::vector<QwDBInterface> QwBCM<T>::GetDBEntry()
{
  std::vector <QwDBInterface> row_list;
  fBeamCurrent.AddEntriesToList(row_list);
  return row_list;
}

/** \brief Collect error database entries for this BCM. */
template<typename T>
std::vector<QwErrDBInterface> QwBCM<T>::GetErrDBEntry()
{
  std::vector <QwErrDBInterface> row_list;
  fBeamCurrent.AddErrEntriesToList(row_list);
  return row_list;
}
#endif // __USE_DATABASE__

/** \brief Get the current value of the beam current. */
template<typename T>
Double_t QwBCM<T>::GetValue()
{
  return fBeamCurrent.GetValue();
}


/** \brief Get the statistical error on the beam current. */
template<typename T>
Double_t QwBCM<T>::GetValueError()
{
  return fBeamCurrent.GetValueError();
}

/** \brief Get the width of the beam current distribution. */
template<typename T>
Double_t QwBCM<T>::GetValueWidth()
{
  return fBeamCurrent.GetValueWidth();
}

template class QwBCM<QwVQWK_Channel>;
template class QwBCM<QwADC18_Channel>;
template class QwBCM<QwSIS3801_Channel>;
template class QwBCM<QwSIS3801D24_Channel>;
template class QwBCM<QwMollerADC_Channel>;
