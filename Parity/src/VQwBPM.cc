/*!
 * \file   VQwBPM.cc
 * \brief  Virtual base class implementation for beam position monitors
 *
 * Base helper implementations for BPMs (beam position monitors). Contains
 * small utility methods shared by concrete BPM types plus factory helpers.
 * Documentation-only edits; runtime behavior unchanged.
 */

#include "VQwBPM.h"

// System headers
#include <stdexcept>

// Qweak headers
#include "QwLog.h"
#include "QwBPMStripline.h"
#include "QwCombinedBPM.h"
#include "QwVQWK_Channel.h"
#include "QwScaler_Channel.h"
#include "QwMollerADC_Channel.h"


/* With X being vertical up and Z being the beam direction toward the beamdump */
const TString  VQwBPM::kAxisLabel[2]={"X","Y"};


/*!
 * \brief Initialize common BPM state and set the element name.
 * \param name Element name to assign to this BPM.
 * 
 * Initializes position center array to zero and sets the element name.
 * This is the base initialization common to all BPM types.
 */
void  VQwBPM::InitializeChannel(TString name)
{ 
  Short_t i = 0; 

  for(i=0;i<3;i++) fPositionCenter[i] = 0.0;

  SetElementName(name);
  
  return;
}

/*!
 * \brief Store geometry/survey offsets for absolute position calibration.
 * \param Xoffset X-axis survey offset in mm.
 * \param Yoffset Y-axis survey offset in mm.
 * \param Zoffset Z-axis survey offset in mm.
 * 
 * Reads position offsets from geometry map file for absolute position
 * calibration. These offsets correct for known mechanical installation
 * differences from ideal positions.
 */
void VQwBPM::GetSurveyOffsets(Double_t Xoffset, Double_t Yoffset, Double_t Zoffset)
{
  // Read in the position offsets from the geometry map file
  for(Short_t i=0;i<3;i++) fPositionCenter[i]=0.0;
  fPositionCenter[0]=Xoffset;
  fPositionCenter[1]=Yoffset;
  fPositionCenter[2]=Zoffset;
  return;
}


/*!
 * \brief Apply per-detector electronic calibration and relative gains.
 * \param BSENfactor Beam sensitivity factor for position calibration.
 * \param AlphaX Relative gain correction factor for X position.
 * \param AlphaY Relative gain correction factor for Y position.
 * 
 * Reads electronic factors from calibration file and applies stripline-specific
 * calibrations. BSENfactor is scaled by 18.81 to convert to mm/V, and a
 * correction factor of 0.250014 is applied for stripline geometry.
 */
void VQwBPM::GetElectronicFactors(Double_t BSENfactor, Double_t AlphaX, Double_t AlphaY)
{
  // Read in the electronic factors from the file
  Bool_t ldebug = kFALSE;

  fQwStriplineCalibration = BSENfactor*18.81;
  fQwStriplineCorrection = 0.250014;

  fRelativeGains[0]=AlphaX;
  fRelativeGains[1]=AlphaY;

  if(ldebug){
    std::cout<<"\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
    std::cout<<this->GetElementName();
    std::cout<<"\nfQwStriplineCalibration = "<<fQwStriplineCalibration<<std::endl;
    std::cout<<"\nfQwStriplineCorrection = "<<fQwStriplineCorrection<<std::endl;
    std::cout<<"AlphaX = "<<fRelativeGains[0]<<std::endl;
    std::cout<<"AlphaY = "<<fRelativeGains[1]<<std::endl;
    
  }
  return;
}

/*!
 * \brief Set detector rotation angle and update cached trigonometric values.
 * \param rotation_angle Rotation angle in degrees (positive = clockwise from beam's perspective).
 * 
 * Sets the BPM rotation angle and pre-computes sin/cos values for efficient
 * coordinate transformations. Rotation is applied to correct for mechanical
 * installation angles that differ from ideal orientation.
 */
void VQwBPM::SetRotation(Double_t rotation_angle){
  // Read the rotation angle in degrees (to beam right)
  Bool_t ldebug = kFALSE;
  fSinRotation = 0;
  fCosRotation = 0;
  fRotationAngle = rotation_angle;
  fSinRotation = TMath::Sin(fRotationAngle*(TMath::DegToRad()));
  fCosRotation = TMath::Cos(fRotationAngle*(TMath::DegToRad()));

  if(ldebug){
    std::cout<<"\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n";
    std::cout<<this->GetElementName();
    std::cout<<" is rotated by angle = "<<rotation_angle<<std::endl;
    
  }
}

/** Disable rotation, restoring accelerator coordinates (0 degrees). */
void VQwBPM::SetRotationOff(){
  // Turn off rotation. This object is already in accelerator coordinates.
  fRotationAngle = 0.0;
  SetRotation(fRotationAngle);
  bRotated=kFALSE;
}

/** Configure position-dependent gains (X or Y). */
void VQwBPM::SetGains(TString pos, Double_t value){
  if(pos.Contains("X")) fGains[0] = value;
  if(pos.Contains("Y")) fGains[1] = value;
}

void VQwBPM::SetSingleEventCuts(TString ch_name, Double_t minX, Double_t maxX)
{
  VQwHardwareChannel* tmpptr = GetSubelementByName(ch_name);
  QwMessage << GetElementName() << " " << ch_name 
	    << " LL " <<  minX <<" UL " << maxX <<QwLog::endl;
  tmpptr->SetSingleEventCuts(minX,maxX);
}

void VQwBPM::SetSingleEventCuts(TString ch_name, UInt_t errorflag,Double_t minX, Double_t maxX, Double_t stability, Double_t burplevel)
{
  VQwHardwareChannel* tmpptr = GetSubelementByName(ch_name);
  errorflag|=kBPMErrorFlag;//update the device flag
  QwMessage << GetElementName() << " " << ch_name 
	    << " LL " <<  minX <<" UL " << maxX <<QwLog::endl;
  tmpptr->SetSingleEventCuts(errorflag,minX,maxX,stability,burplevel);
}

VQwBPM& VQwBPM::operator= (const VQwBPM &value)
{
  if (GetElementName()!=""){
    fQwStriplineCalibration = value.fQwStriplineCalibration;
    fQwStriplineCorrection = value.fQwStriplineCorrection;
    bRotated = value.bRotated;
    fRotationAngle = value.fRotationAngle;
    fCosRotation = value.fCosRotation;
    fSinRotation = value.fSinRotation;
    fGoodEvent = value.fGoodEvent;
    //    fDeviceErrorCode = value.fDeviceErrorCode;
    //    bFullSave = value.bFullSave;
    for(size_t axis=kXAxis;axis<kNumAxes;axis++){
      fRelativeGains[axis]=value.fRelativeGains[axis];
      this->fPositionCenter[axis]=value.fPositionCenter[axis];
    }
    // Copy Z center position
    this->fPositionCenter[2]=value.fPositionCenter[2];
  }
  return *this;
}

// VQwBPM& VQwBPM::operator+= (const VQwBPM &value)
// {
//   if (GetElementName()!=""){
//     this->fEffectiveCharge+=value.fEffectiveCharge;
//     for(Short_t i=kXAxis;i<kNumAxes;i++) this->fAbsPos[i]+=value.fAbsPos[i];
//   }
//   return *this;
// }

// VQwBPM& VQwBPM::operator-= (const VQwBPM &value)
// {
//   if (GetElementName()!=""){
//     this->fEffectiveCharge-=value.fEffectiveCharge;
//     for(Short_t i=kXAxis;i<kNumAxes;i++) this->fAbsPos[i]-=value.fAbsPos[i];
//   }
//   return *this;
// }


// void VQwBPM::Sum(VQwBPM &value1, VQwBPM &value2)
// {
//   *this =  value1;
//   *this += value2;
// }

// void VQwBPM::Difference(VQwBPM &value1, VQwBPM &value2)
// {
//   *this =  value1;
//   *this -= value2;
// }


// void VQwBPM::Scale(Double_t factor)
// {
//   fEffectiveCharge_base->Scale(factor);
//   for(Short_t i = 0;i<2;i++)fAbsPos_base[i]->Scale(factor);
//   return;
// }




void VQwBPM::SetRootSaveStatus(TString &prefix)
{
  if(prefix.Contains("diff_")||prefix.Contains("yield_")|| prefix.Contains("asym_"))
    bFullSave=kFALSE;
  
  return;
}

// void VQwBPM::PrintValue() const
// {
//   Short_t i;
//   for (i = 0; i < 2; i++) fAbsPos_base[i]->PrintValue();
//   fEffectiveCharge_base->PrintValue();
    
//   return;
// }

// void VQwBPM::PrintInfo() const
// {
//   Short_t i = 0;
//   for (i = 0; i < 4; i++)  fAbsPos_base[i]->PrintInfo();
//   fEffectiveCharge_base->PrintInfo();
// }


// void VQwBPM::CalculateRunningAverage()
// {
//   Short_t i = 0;
//   for (i = 0; i < 2; i++) fAbsPos_base[i]->CalculateRunningAverage();
//   fEffectiveCharge_base->CalculateRunningAverage();

//   return;
// }


// void VQwBPM::AccumulateRunningSum(const VQwBPM& value)
// {
//   // TODO This is unsafe, see QwBeamline::AccumulateRunningSum
//   Short_t i = 0;
//   for (i = 0; i < 2; i++) fAbsPos_base[i]->AccumulateRunningSum(value.fAbsPos_base[i]);
//   fEffectiveCharge_base->AccumulateRunningSum(value.fEffectiveCharge_base);
//   return;
// }

/**
 * \brief A fast way of creating a BPM stripline of specified type
 */
VQwBPM* VQwBPM::CreateStripline(TString subsystemname, TString name, TString type)
{
  Bool_t localDebug = kFALSE;
  type.ToUpper();
  if( localDebug ) QwMessage<<"Creating BPM of type: "<<type<<" with name: "<<
    name<<". Subsystem Name: " <<subsystemname<<"\n";
  // (jc2) As a first try, let's do this the ugly way (but rather very
  // simple), just list out the types of BPM's supported by this code!!!
  if( type == "VQWK") {
    return new QwBPMStripline<QwVQWK_Channel>(subsystemname,name,type);
  } else if ( type == "SIS3801" ) {
    return new QwBPMStripline<QwSIS3801_Channel>(subsystemname,name,type);
  } else if ( type == "SCALER" || type == "SIS3801D24" ) {
    return new QwBPMStripline<QwSIS3801D24_Channel>(subsystemname,name,type);
  } else if ( type == "MOLLERADC" ) {
    return new QwBPMStripline<QwMollerADC_Channel>(subsystemname,name,type);
  } else { // Unsupported one!
    QwWarning << "BPM of type="<<type<<" is UNSUPPORTED!!\n";
    exit(-1);
  }
}

VQwBPM* VQwBPM::CreateStripline(const VQwBPM& source)
{
  Bool_t localDebug = kFALSE;
  TString type = source.GetModuleType();
  type.ToUpper();
  if( localDebug ) QwMessage<<"Creating BPM of type: "<<type << QwLog::endl;
  // (jc2) As a first try, let's do this the ugly way (but rather very
  // simple), just list out the types of BPM's supported by this code!!!
  if( type == "VQWK") {
    return new QwBPMStripline<QwVQWK_Channel>(dynamic_cast<const QwBPMStripline<QwVQWK_Channel>&>(source));
  } else if ( type == "SIS3801" ) {
    return new QwBPMStripline<QwSIS3801_Channel>(dynamic_cast<const QwBPMStripline<QwSIS3801_Channel>&>(source));
  } else if ( type == "SCALER" || type == "SIS3801D24" ) {
    return new QwBPMStripline<QwSIS3801D24_Channel>(dynamic_cast<const QwBPMStripline<QwSIS3801D24_Channel>&>(source));
  } else if ( type == "MOLLERADC" ) {
    return new QwBPMStripline<QwMollerADC_Channel>(dynamic_cast<const QwBPMStripline<QwMollerADC_Channel>&>(source));
  } else { // Unsupported one!
    QwWarning << "BPM of type="<<type<<" is UNSUPPORTED!!\n";
    exit(-1);
  }
}

/**
 * \brief A fast way of creating a BPM stripline of specified type
 */
VQwBPM* VQwBPM::CreateCombo(TString subsystemname, TString name,
    TString type)
{
  Bool_t localDebug = kFALSE;
  type.ToUpper();
  if( localDebug ) QwMessage<<"Creating CombinedBPM of type: "<<type<<" with name: "<<
    name<<". Subsystem Name: " <<subsystemname<<"\n";
  // (jc2) As a first try, let's do this the ugly way (but rather very
  // simple), just list out the types of BPM's supported by this code!!!
  if( type == "VQWK") {
    return new QwCombinedBPM<QwVQWK_Channel>(subsystemname,name,type);
  } else if (type == "SIS3801" ) { // Default SCALER channel
    return new QwCombinedBPM<QwSIS3801_Channel>(subsystemname,name,type);
  } else if ( type == "SCALER" || type == "SIS3801D24" ) {
    return new QwCombinedBPM<QwSIS3801D24_Channel>(subsystemname,name,type);
  } else if ( type == "MOLLERADC" ) {
    return new QwCombinedBPM<QwMollerADC_Channel>(subsystemname,name,type);
  } else { // Unsupported one!
    QwWarning << "BPM of type="<<type<<" is UNSUPPORTED!!\n";
    exit(-1);
  }
}

VQwBPM* VQwBPM::CreateCombo(const VQwBPM& source)
{
  Bool_t localDebug = kFALSE;
  TString type = source.GetModuleType();
  type.ToUpper();
  if( localDebug ) QwMessage<<"Creating CombinedBCM of type: "<<type<< QwLog::endl;
  // (jc2) As a first try, let's do this the ugly way (but rather very
  // simple), just list out the types of BCM's supported by this code!!!
  if( type == "VQWK") {
    return new QwCombinedBPM<QwVQWK_Channel>(dynamic_cast<const QwCombinedBPM<QwVQWK_Channel>&>(source));
  } else if ( type == "SIS3801" ) { // Default SCALER channel
    return new QwCombinedBPM<QwSIS3801_Channel>(dynamic_cast<const QwCombinedBPM<QwSIS3801_Channel>&>(source));
  } else if ( type == "SCALER" || type == "SIS3801D24" ) {
    return new QwCombinedBPM<QwSIS3801D24_Channel>(dynamic_cast<const QwCombinedBPM<QwSIS3801D24_Channel>&>(source));
  } else if ( type == "MOLLERADC" ) {
    return new QwCombinedBPM<QwMollerADC_Channel>(dynamic_cast<const QwCombinedBPM<QwMollerADC_Channel>&>(source));
  } else { // Unsupported one!
    QwWarning << "BPM of type="<<type<<" is UNSUPPORTED!!\n";
    exit(-1);
  }
}

