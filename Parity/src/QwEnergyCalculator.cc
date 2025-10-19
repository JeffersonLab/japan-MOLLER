/*!
 * \file   QwEnergyCalculator.cc
 * \brief  Implementation of energy calculator for beam energy determination
 * \author B.Waidyawansa
 * \date   2010-05-24
 */

#include "QwEnergyCalculator.h"

// System headers
#include <stdexcept>

// ROOT headers for RNTuple support
#ifdef HAS_RNTUPLE_SUPPORT
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleWriter.hxx>
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#ifdef __USE_DATABASE__
#include "QwDBInterface.h"
#endif // __USE_DATABASE__


// static QwVQWK_Channel  targetbeamangle;
// static QwVQWK_Channel  targetbeamx;
// static QwVQWK_Channel  beamx;

/**
 * \brief Initialize this energy calculator with a name and data-saving mode.
 * \param name        Element name used for histogram and branch prefixes.
 * \param datatosave  Storage mode (e.g., "raw" or "derived").
 */
void QwEnergyCalculator::InitializeChannel(TString name,TString datatosave )
{
  SetElementName(name);
  fEnergyChange.InitializeChannel(name,datatosave);
  //  beamx.InitializeChannel("beamx","derived");
  return;
}

/**
 * \brief Initialize this energy calculator with subsystem and name.
 * \param subsystem   Subsystem identifier for tree/hist foldering.
 * \param name        Element name used for histogram and branch prefixes.
 * \param datatosave  Storage mode (e.g., "raw" or "derived").
 */
void QwEnergyCalculator::InitializeChannel(TString subsystem, TString name,TString datatosave )
{
  SetElementName(name);
  fEnergyChange.InitializeChannel(subsystem, "QwEnergyCalculator", name,datatosave);
  //  beamx.InitializeChannel("beamx","derived");
  return;
}

/**
 * \brief Register a BPM-based device contributing to the energy calculation.
 * \param device         Pointer to contributing BPM object.
 * \param type           Device type string (e.g., BPM, ComboBPM).
 * \param property       Property descriptor (e.g., targetbeamangle).
 * \param tmatrix_ratio  Transport matrix ratio coefficient for contribution.
 */
void QwEnergyCalculator::Set(const VQwBPM* device, TString type, TString property,Double_t tmatrix_ratio)
{
  Bool_t ldebug = kFALSE;

  fDevice.push_back(device);
  fProperty.push_back(property);
  fType.push_back(type);
  fTMatrixRatio.push_back(tmatrix_ratio);

  if(ldebug)
    std::cout<<"QwEnergyCalculator:: Using "<<device->GetElementName()<<" with ratio "<< tmatrix_ratio <<" for "<<property<<std::endl;
 
  return;
}

/**
 * \brief Determine whether to save full ROOT output based on the prefix.
 * \param prefix  Branch/histogram prefix (may toggle full save for asym/diff).
 */
void QwEnergyCalculator::SetRootSaveStatus(TString &prefix)
{
  if(prefix.Contains("diff_")||prefix.Contains("yield_")|| prefix.Contains("asym_"))
    bFullSave=kFALSE;
  
  return;
}

/**
 * \brief Clear event-scoped data of this calculator and underlying channel.
 */
void QwEnergyCalculator::ClearEventData(){
  fEnergyChange.ClearEventData();
  return;
}



/**
 * \brief Compute per-event energy change by summing configured device terms.
 */
void QwEnergyCalculator::ProcessEvent()
{
  //Bool_t ldebug = kFALSE;
  //Double_t targetbeamangle = 0.0;
  static QwMollerADC_Channel tmp;
  tmp.InitializeChannel("tmp","derived");
  tmp.ClearEventData();

  this->ClearEventData();

  for(UInt_t i = 0; i<fProperty.size(); i++){
    if(fProperty[i].Contains("targetbeamangle")){
      tmp.ArcTan((((QwCombinedBPM<QwMollerADC_Channel>*)fDevice[i])->fSlope[VQwBPM::kXAxis]));
     } else {
      tmp.AssignValueFrom(fDevice[i]->GetPosition(VQwBPM::kXAxis));
     }
    tmp.Scale(fTMatrixRatio[i]);
    fEnergyChange += tmp;
   }
/*
    if(fProperty[i].Contains("targetbeamangle")){
      tmp.ArcTan((((QwCombinedBPM<QwVQWK_Channel>*)fDevice[i])->fSlope[VQwBPM::kXAxis]));
      tmp.Scale(fTMatrixRatio[i]);
     // targetbeamangle = atan((((QwCombinedBPM<QwVQWK_Channel>*)fDevice[i])->fSlope[VQwBPM::kXAxis]).GetValue());
     // targetbeamangle *= fTMatrixRatio[i];
      //if(ldebug) std::cout<<"QwEnegyCalculator::ProcessEvent() :: Beam angle in X at target = "<<targetbeamangle<<std::endl;
    //  fEnergyChange.AddChannelOffset(targetbeamangle);
      fEnergyChange += tmp;
      //if(ldebug) std::cout<<"QwEnegyCalculator::ProcessEvent() :: dp/p += (M12/M16)*X angle = "<<fEnergyChange.GetValue()<<std::endl;
    } else {
      tmp.AssignValueFrom(fDevice[i]->GetPosition(VQwBPM::kXAxis));
      if(ldebug) std::cout<<"QwEnegyCalculator::ProcessEvent() :: X position from "<<fDevice[i]->GetElementName()<<" = "<<tmp.GetValue()<<std::endl;
      tmp.Scale(fTMatrixRatio[i]);
      if(ldebug) std::cout<<"QwEnegyCalculator::ProcessEvent() :: (M11/M16)*X position = "<<tmp.GetValue()<<std::endl;
      fEnergyChange += tmp;
    }
*/
  //if(ldebug) std::cout<<"QwEnegyCalculator::ProcessEvent() :: dp/p = "<<fEnergyChange.GetValue()<<std::endl;
  return;
}

//------------------------------------------------------------------------------------------------------------------------------------

/**
 * \brief Generate mock event data for testing.
 * \param helicity  Helicity state indicator.
 * \param time      Event time or timestamp proxy.
 */
void QwEnergyCalculator::RandomizeEventData(int helicity, double time)
{
  fEnergyChange.RandomizeEventData(helicity, time);
  return;
}


/**
 * \brief Configure Gaussian mock data parameters for the underlying channel.
 * \param mean   Mean value for the distribution.
 * \param sigma  Standard deviation for the distribution.
 */
void QwEnergyCalculator::SetRandomEventParameters(Double_t mean, Double_t sigma)
{
  fEnergyChange.SetRandomEventParameters(mean, sigma);
  return;
}


/**
 * \brief Back-project a BPM position from the current dp/p estimate.
 * \param device  BPM device whose X position is solved from energy and other terms.
 */
void QwEnergyCalculator::GetProjectedPosition(VQwBPM *device)
{
  UInt_t idevice = fProperty.size()+1; /** fProperty.size()=3, idevice=4 **/

  for(UInt_t i=0; i<fProperty.size(); i++) {
   if (fDevice[i]->GetElementName()==device->GetElementName()) { /** fDevice[i] = qwk_1c12 = device **/
     idevice = i; /** i=0=idevice **/
     break;
    }
  } /** idevice=0, fProperty.size()=3 **/

  if (idevice>fProperty.size()) return;  // Return without trying to find a new position if "device" doesn't contribute to the energy calculator

  static QwMollerADC_Channel tmp;
  tmp.InitializeChannel("tmp","derived");
  tmp.ClearEventData();
  //  Set the device position value to be equal to the energy change 
  (device->GetPosition(VQwBPM::kXAxis))->AssignValueFrom(&fEnergyChange);
  /** qwk_1c12X changes only **/

  /** idevice=0 **/
  for(UInt_t i = 0; i<fProperty.size(); i++) {
   if (i==idevice) {
     // Do nothing for this device!
     continue;
   } else {
      //  Calculate contribution to fEnergyChange from the other devices
      if(fProperty[i].Contains("targetbeamangle")) {
       tmp.ArcTan((((QwCombinedBPM<QwMollerADC_Channel>*)fDevice[i])->fSlope[VQwBPM::kXAxis]));
      } else {
       tmp.AssignValueFrom(fDevice[i]->GetPosition(VQwBPM::kXAxis));
      }
     tmp.Scale(fTMatrixRatio[i]);
     //  And subtract it from the device we are trying to get the position of.
     (device->GetPosition(VQwBPM::kXAxis))->operator-=(tmp);
   }
  } // end of for(UInt_t i = 0; i<fProperty.size(); i++)

  //  At this point, device position is (Energy - ( M_x*x + M_xp*arctan(xp) ) )
  //  Now divide by matrixratio for 1c12 to get: (Energy - ( M_x*x + M_xp*arctan(xp) ) )/M_1c12 === x_1c12

  (device->GetPosition(VQwBPM::kXAxis))->Scale(1.0/fTMatrixRatio[idevice]);
  device->ApplyResolutionSmearing(VQwBPM::kXAxis);
  device->FillRawEventData();

/*
///  Reclaculate energy and see what we get
  static QwVQWK_Channel tmp_e;
  tmp_e.InitializeChannel("tmp_e","derived");
  tmp_e.ClearEventData();

  for(UInt_t i = 0; i<fProperty.size(); i++){
    if(fProperty[i].Contains("targetbeamangle")){
      tmp.ArcTan((((QwCombinedBPM<QwVQWK_Channel>*)fDevice[i])->fSlope[VQwBPM::kXAxis]));
    } else {
      tmp.AssignValueFrom(fDevice[i]->GetPosition(VQwBPM::kXAxis));
    }
    tmp.Scale(fTMatrixRatio[i]);
    tmp_e += tmp;
   }
  //  std::cout << "GetProjectedPosition()::fEnergyChange = " << fEnergyChange.GetValue(0) << "\t ProcessEvent()::fEnergyChange = " << tmp_e.GetValue(0) << std::endl;
*/

  return;
}


/**
 * \brief Load mock-data configuration for the underlying channel from a file.
 * \param paramfile  Parameter file reader positioned at channel section.
 */
void QwEnergyCalculator::LoadMockDataParameters(QwParameterFile &paramfile){

  //std::cout << "In QwEnergyCalculator: ChannelName = " << GetElementName() << std::endl;
  fEnergyChange.SetMockDataAsDiff();  // Sets this channel to use the difference mode in the RandomizeMockData
  fEnergyChange.LoadMockDataParameters(paramfile);

/*  Bool_t   ldebug=kFALSE;
  Double_t mean=0.0, sigma=0.0;

  mean  = paramfile.GetTypedNextToken<Double_t>(); 
  sigma = paramfile.GetTypedNextToken<Double_t>();      

   if (ldebug==1) {
     std::cout << "#################### \n";
     std::cout << "! mean, sigma \n" << std::endl;
     std::cout << mean                             << " / "
	       << sigma                            << " / "
	       << std::endl;
   }
  this->SetRandomEventParameters(mean, sigma);
*/
}
//------------------------------------------------------------------------------------------------------------------------------------

/**
 * \brief Apply single-event cuts to the energy channel and contributing devices.
 * \return kTRUE if the event passes cuts; otherwise kFALSE.
 */
Bool_t QwEnergyCalculator::ApplySingleEventCuts(){
  Bool_t status=kTRUE;

  UInt_t error_code = 0;
  for(UInt_t i = 0; i<fProperty.size(); i++){
    if(fProperty[i].Contains("targetbeamangle")){
      error_code |= ((QwCombinedBPM<QwMollerADC_Channel>*)fDevice[i])->fSlope[0].GetErrorCode();
    } else {
      error_code |= fDevice[i]->GetPosition(VQwBPM::kXAxis)->GetErrorCode();
    }
  }
  //fEnergyChange.UpdateErrorFlag(error_code);//No need to do this. error codes are ORed when energy is calculated

  if (fEnergyChange.ApplySingleEventCuts()){
    status=kTRUE;
  }
  else{
    status&=kFALSE;
  }
  return status;
}

/**
 * \brief Increment error counters in the underlying energy channel.
 */
void QwEnergyCalculator::IncrementErrorCounters()
{
  fEnergyChange.IncrementErrorCounters();
}


/**
 * \brief Print accumulated error counters for diagnostic purposes.
 */
void QwEnergyCalculator::PrintErrorCounters() const{
  // report number of events failed due to HW and event cut failure
  fEnergyChange.PrintErrorCounters();
}
/*
void QwEnergyCalculator::PrintRandomEventParameters(){
  
}
*/
/**
 * \brief Update and return the composite event-cut error flag.
 * \return The updated event-cut error flag for this calculator.
 */
UInt_t QwEnergyCalculator::UpdateErrorFlag()
{
  UInt_t error_code = 0;
  for(UInt_t i = 0; i<fProperty.size(); i++){
    if(fProperty[i].Contains("targetbeamangle")){
      error_code |= ((QwCombinedBPM<QwMollerADC_Channel>*)fDevice[i])->fSlope[0].GetErrorCode();
    } else {
      error_code |= fDevice[i]->GetPosition(VQwBPM::kXAxis)->GetErrorCode();
    }
  }
  fEnergyChange.UpdateErrorFlag(error_code);
  return fEnergyChange.GetEventcutErrorFlag();
}


/**
 * \brief Propagate error flags from a reference calculator into this one.
 * \param ev_error  Reference energy calculator containing error flags to merge.
 */
void QwEnergyCalculator::UpdateErrorFlag(const QwEnergyCalculator *ev_error){
  fEnergyChange.UpdateErrorFlag(ev_error->fEnergyChange);
};


/**
 * \brief Update running averages for the underlying energy channel.
 */
void QwEnergyCalculator::CalculateRunningAverage(){
  fEnergyChange.CalculateRunningAverage();
}



/**
 * \brief Accumulate running sums from another calculator into this one.
 * \param value      Source calculator to accumulate from.
 * \param count      Weight/count for accumulation; 0 implies use source count.
 * \param ErrorMask  Mask to control error propagation behavior.
 */
void QwEnergyCalculator::AccumulateRunningSum(const QwEnergyCalculator& value, Int_t count, Int_t ErrorMask){
  fEnergyChange.AccumulateRunningSum(value.fEnergyChange, count, ErrorMask);
}

/**
 * \brief Remove a single entry from the running sums using a source value.
 * \param value      Source calculator to subtract.
 * \param ErrorMask  Mask to control error propagation behavior.
 */
void QwEnergyCalculator::DeaccumulateRunningSum(QwEnergyCalculator& value, Int_t ErrorMask){
  fEnergyChange.DeaccumulateRunningSum(value.fEnergyChange, ErrorMask);
}



/**
 * \brief Process a configuration/event buffer (no-op for this calculator).
 * \return The updated buffer word position (unchanged here).
 */
Int_t QwEnergyCalculator::ProcessEvBuffer(UInt_t* buffer, UInt_t word_position_in_buffer,UInt_t subelement){
  return 0;
}


/** \brief Copy-assign from another calculator (event-scoped data). */
QwEnergyCalculator& QwEnergyCalculator::operator= (const QwEnergyCalculator &value){
  if (this != &value) {
    if (GetElementName()!="")
      this->fEnergyChange=value.fEnergyChange;
  }
  return *this;
}

/** \brief Add-assign from another calculator (sum energy change). */
QwEnergyCalculator& QwEnergyCalculator::operator+= (const QwEnergyCalculator &value){

  if (GetElementName()!="")
    this->fEnergyChange+=value.fEnergyChange;

  return *this;
}

/** \brief Subtract-assign from another calculator (difference energy change). */
QwEnergyCalculator& QwEnergyCalculator::operator-= (const QwEnergyCalculator &value){
  if (GetElementName()!="")
    this->fEnergyChange-=value.fEnergyChange;

  return *this;
}

void QwEnergyCalculator::Sum(const QwEnergyCalculator &value1, const QwEnergyCalculator &value2) {
  *this = value1;
  *this += value2;
}

void QwEnergyCalculator::Difference(const QwEnergyCalculator &value1, const QwEnergyCalculator &value2) {
  *this = value1;
  *this -= value2;
}


/**
 * \brief Define the ratio for asymmetry formation (here acts as pass-through).
 * \param numer  Numerator calculator.
 * \param denom  Denominator calculator.
 */
void QwEnergyCalculator::Ratio(QwEnergyCalculator &numer, QwEnergyCalculator &denom){
  // this function is called when forming asymmetries. In this case what we actually want for the
  // qwk_energy/(dp/p) is the difference only not the asymmetries

  *this=numer;
  return;
}


/**
 * \brief Scale the underlying energy channel by a constant factor.
 * \param factor  Multiplicative scale factor.
 */
void QwEnergyCalculator::Scale(Double_t factor){
  fEnergyChange.Scale(factor);
  return;
}


/** \brief Print detailed information for this calculator. */
void QwEnergyCalculator::PrintInfo() const{
  fEnergyChange.PrintInfo();
  return;
}


/** \brief Print a compact value summary for this calculator. */
void QwEnergyCalculator::PrintValue() const{
  fEnergyChange.PrintValue();
  return;
}

/**
 * \brief Apply hardware checks (delegated to contributing channels if any).
 * \return Always kTRUE; no direct hardware channels in this calculator.
 */
Bool_t QwEnergyCalculator::ApplyHWChecks(){
  // For the energy calculator there are no physical channels that we can relate to because it is being
  // derived from combinations of physical channels. Therefore, this is not exactly a "HW Check"
  // but just a check of the HW checks of the used channels.

  Bool_t eventokay=kTRUE;
  return eventokay;
}


/**
 * \brief Set single-event cut limits on the underlying energy channel.
 * \param minX  Lower limit.
 * \param maxX  Upper limit.
 * \return 1 on success.
 */
Int_t QwEnergyCalculator::SetSingleEventCuts(Double_t minX, Double_t maxX){
  fEnergyChange.SetSingleEventCuts(minX,maxX);
  return 1;
}

/**
 * \brief Configure detailed single-event cuts for the underlying channel.
 * \param errorflag  Device-specific error flag mask to set.
 * \param LL         Lower limit.
 * \param UL         Upper limit.
 * \param stability  Stability threshold.
 * \param burplevel  Burp detection threshold.
 */
void QwEnergyCalculator::SetSingleEventCuts(UInt_t errorflag, Double_t LL=0, Double_t UL=0, Double_t stability=0, Double_t burplevel=0){
  //set the unique tag to identify device type (bcm,bpm & etc)
  errorflag|=kBCMErrorFlag;//currently I use the same flag for bcm
  QwMessage<<"QwEnergyCalculator Error Code passing to QwMollerADC_Ch "<<errorflag<<QwLog::endl;
  fEnergyChange.SetSingleEventCuts(errorflag,LL,UL,stability,burplevel);
}

/**
 * \brief Check for burp failures by delegating to the energy channel.
 * \param ev_error  Reference energy calculator to compare against.
 * \return kTRUE if a burp failure was detected; otherwise kFALSE.
 */
Bool_t QwEnergyCalculator::CheckForBurpFail(const VQwDataElement *ev_error){
  Bool_t burpstatus = kFALSE;
  try {
    if(typeid(*ev_error)==typeid(*this)) {
      //std::cout<<" Here in QwEnergyCalculator::CheckForBurpFail \n";
      if (this->GetElementName()!="") {
        const QwEnergyCalculator* value_halo = dynamic_cast<const QwEnergyCalculator* >(ev_error);
        burpstatus |= fEnergyChange.CheckForBurpFail(&(value_halo->fEnergyChange)); 
      }
    } else {
      TString loc="Standard exception from QwEnergyCalculator::CheckForBurpFail :"+
        ev_error->GetElementName()+" "+this->GetElementName()+" are not of the "
        +"same type";
      throw std::invalid_argument(loc.Data());
    }
  } catch (std::exception& e) {
    std::cerr<< e.what()<<std::endl;
  }
  return burpstatus;
}

/**
 * \brief Define histograms for this calculator (delegated to energy channel).
 * \param folder  ROOT folder to contain histograms.
 * \param prefix  Histogram name prefix.
 */
void  QwEnergyCalculator::ConstructHistograms(TDirectory *folder, TString &prefix){
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else{
    TString thisprefix=prefix;
    if(prefix.Contains("asym_"))
      thisprefix.ReplaceAll("asym_","diff_");
    SetRootSaveStatus(thisprefix);
    fEnergyChange.ConstructHistograms(folder, thisprefix);
  }
  return;
}

/** \brief Fill histograms for this calculator if enabled. */
void  QwEnergyCalculator::FillHistograms(){
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else
    fEnergyChange.FillHistograms();
  
  return;
}

/**
 * \brief Construct ROOT branches and value vector entries.
 * \param tree    Output tree.
 * \param prefix  Branch name prefix.
 * \param values  Output value vector to be appended.
 */
void  QwEnergyCalculator::ConstructBranchAndVector(TTree *tree, TString &prefix,
						   std::vector<Double_t> &values){
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else{
    TString thisprefix=prefix;
    if(prefix.Contains("asym_"))
      thisprefix.ReplaceAll("asym_","diff_");
    
    SetRootSaveStatus(thisprefix);
    
    fEnergyChange.ConstructBranchAndVector(tree,thisprefix,values);
  }
    return;
}



/**
 * \brief Construct ROOT branches for this calculator (if enabled).
 * \param tree    Output tree.
 * \param prefix  Branch name prefix.
 */
void  QwEnergyCalculator::ConstructBranch(TTree *tree, TString &prefix){
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else{
    TString thisprefix=prefix;
    if(prefix.Contains("asym_"))
      thisprefix.ReplaceAll("asym_","diff_");

    SetRootSaveStatus(thisprefix);
    fEnergyChange.ConstructBranch(tree,thisprefix);
  }
  return;
}

void  QwEnergyCalculator::ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist){

  TString devicename;
  devicename=GetElementName();
  devicename.ToLower();
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else
    if (modulelist.HasValue(devicename)){
      TString thisprefix=prefix;
      if(prefix.Contains("asym_"))
	thisprefix.ReplaceAll("asym_","diff_");
      SetRootSaveStatus(thisprefix);   
      fEnergyChange.ConstructBranch(tree,thisprefix);
      QwMessage <<" Tree leave added to "<<devicename<<QwLog::endl;
      }
  return;
}

void  QwEnergyCalculator::FillTreeVector(std::vector<Double_t> &values) const
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else
    fEnergyChange.FillTreeVector(values);
  return;
}

#ifdef HAS_RNTUPLE_SUPPORT
void  QwEnergyCalculator::ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs)
{
  if (GetElementName()==""){
    //  This channel is not used, so skip construction.
  }
  else{
    TString thisprefix=prefix;
    if(prefix.Contains("asym_"))
      thisprefix.ReplaceAll("asym_","diff_");
    
    fEnergyChange.ConstructNTupleAndVector(model, thisprefix, values, fieldPtrs);
  }
  return;
}

void  QwEnergyCalculator::FillNTupleVector(std::vector<Double_t>& values) const
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling.
  }
  else
    fEnergyChange.FillNTupleVector(values);
  return;
}
#endif // HAS_RNTUPLE_SUPPORT

#ifdef __USE_DATABASE__
std::vector<QwDBInterface> QwEnergyCalculator::GetDBEntry()
{
  std::vector <QwDBInterface> row_list;
  row_list.clear();
  fEnergyChange.AddEntriesToList(row_list);
  return row_list;

}

std::vector<QwErrDBInterface> QwEnergyCalculator::GetErrDBEntry()
{
  std::vector <QwErrDBInterface> row_list;
  row_list.clear();
  fEnergyChange.AddErrEntriesToList(row_list);
  return row_list;
}
#endif // __USE_DATABASE__
