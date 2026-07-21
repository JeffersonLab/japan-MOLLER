/*!
 * \file   VQwBCM.cc
 * \brief  Virtual base class implementation for beam current monitors
 *
 * Factory helpers to create concrete BCMs and combined BCMs by module type.
 * Documentation-only edits; runtime behavior unchanged.
 */

#include "VQwBCM.h"

#include "QwBCM.h"
#include "QwCombinedBCM.h"

// System headers
#include <stdexcept>

// Qweak database headers
#ifdef __USE_DATABASE__
#include "QwDBInterface.h"
#endif // __USE_DATABASE__

//  Qweak types that we want to use in this template
#include "QwVQWK_Channel.h"
#include "QwADC18_Channel.h"
#include "QwScaler_Channel.h"
#include "QwMollerADC_Channel.h"

/*!
 * \brief Factory method to create a concrete BCM instance for the requested module type.
 * \param subsystemname Name of the parent subsystem.
 * \param name BCM channel name.
 * \param type Module type string (VQWK, ADC18, SIS3801, SIS3801D24/SCALER, MOLLERADC).
 * \param clock Clock reference name for timing-based modules.
 * \return Pointer to newly created BCM instance.
 *
 * Creates appropriate concrete BCM template instantiation based on module type.
 * Supported types include integrating ADCs (VQWK, ADC18, MOLLERADC) and
 * scalers (SIS3801, SIS3801D24). Each type uses the corresponding channel class
 * for data handling and calibration.
 */
VQwBCM* VQwBCM::Create(TString subsystemname, TString name, TString type, TString clock)
{
  Bool_t localDebug = kFALSE;
  type.ToUpper();
  if( localDebug ) QwMessage<<"Creating BCM of type: "<<type<<" with name: "<<
    name<<". Subsystem Name: " <<subsystemname<<" and clock name="<<clock<<"\n";
  // (jc2) As a first try, let's do this the ugly way (but rather very
  // simple), just list out the types of BCM's supported by this code!!!
  if( type == "VQWK") {
    return new QwBCM<QwVQWK_Channel>(subsystemname,name,type);
  } else if ( type == "ADC18" ) {
    return new QwBCM<QwADC18_Channel>(subsystemname,name,type,clock);
  } else if ( type == "SIS3801" ) {
    return new QwBCM<QwSIS3801_Channel>(subsystemname,name,type,clock);
  } else if ( type == "SCALER" || type == "SIS3801D24" ) {
    return new QwBCM<QwSIS3801D24_Channel>(subsystemname,name,type,clock);
  } else if ( type == "MOLLERADC" ) {
    return new QwBCM<QwMollerADC_Channel>(subsystemname,name,type,clock);
  } else { // Unsupported one!
    QwWarning << "BCM of type="<<type<<" is UNSUPPORTED!!\n";
    exit(-1);
  }
}

/*!
 * \brief Copy constructor factory method to clone a BCM from an existing instance.
 * \param source Reference BCM to copy from.
 * \return Pointer to newly created BCM copy.
 *
 * Creates a deep copy of the source BCM by determining its concrete type
 * and calling the appropriate template constructor. Preserves all calibration
 * parameters and configuration from the source.
 */
VQwBCM* VQwBCM::Create(const VQwBCM& source)
{
  Bool_t localDebug = kFALSE;
  TString type = source.GetModuleType();
  type.ToUpper();
  if( localDebug ) QwMessage<<"Creating BCM of type: "<<type<<QwLog::endl;
  // (jc2) As a first try, let's do this the ugly way (but rather very
  // simple), just list out the types of BCM's supported by this code!!!
  if( type == "VQWK") {
    return new QwBCM<QwVQWK_Channel>(dynamic_cast<const QwBCM<QwVQWK_Channel>&>(source));
  } else if ( type == "ADC18" ) {
    return new QwBCM<QwADC18_Channel>(dynamic_cast<const QwBCM<QwADC18_Channel>&>(source));
  } else if ( type == "SIS3801" ) {
    return new QwBCM<QwSIS3801_Channel>(dynamic_cast<const QwBCM<QwSIS3801_Channel>&>(source));
  } else if ( type == "SCALER" || type == "SIS3801D24" ) {
    return new QwBCM<QwSIS3801D24_Channel>(dynamic_cast<const QwBCM<QwSIS3801D24_Channel>&>(source));
  } else if ( type == "MOLLERADC" ) {
    return new QwBCM<QwMollerADC_Channel>(dynamic_cast<const QwBCM<QwMollerADC_Channel>&>(source));
  } else { // Unsupported one!
    QwWarning << "BCM of type="<<type<<" is UNSUPPORTED!!\n";
    exit(-1);
  }
}

/*!
 * \brief Factory method to create a concrete Combined BCM for the requested module type.
 */
VQwBCM* VQwBCM::CreateCombo(TString subsystemname, TString name, TString type)
{
  Bool_t localDebug = kFALSE;
  type.ToUpper();
  if( localDebug ) QwMessage<<"Creating CombinedBCM of type: "<<type<<" with name: "<<
    name<<". Subsystem Name: " <<subsystemname<<"\n";
  // (jc2) As a first try, let's do this the ugly way (but rather very
  // simple), just list out the types of BCM's supported by this code!!!
  if( type == "VQWK") {
    return new QwCombinedBCM<QwVQWK_Channel>(subsystemname,name,type);
  } else if ( type == "ADC18" ) {
    return new QwCombinedBCM<QwADC18_Channel>(subsystemname,name,type);
  } else if ( type == "SIS3801" ) {
    return new QwCombinedBCM<QwSIS3801_Channel>(subsystemname,name,type);
  } else if ( type == "SCALER" || type == "SIS3801D24" ) {
    return new QwCombinedBCM<QwSIS3801D24_Channel>(subsystemname,name,type);
  } else if ( type == "MOLLERADC" ) {
    return new QwCombinedBCM<QwMollerADC_Channel>(subsystemname,name,type);
  } else { // Unsupported one!
    QwWarning << "BCM of type="<<type<<" is UNSUPPORTED!!\n";
    exit(-1);
  }
}

VQwBCM* VQwBCM::CreateCombo(const VQwBCM& source)
{
  Bool_t localDebug = kFALSE;
  TString type = source.GetModuleType();
  type.ToUpper();
  if( localDebug ) QwMessage<<"Creating CombinedBCM of type: "<<type<< QwLog::endl;
  // (jc2) As a first try, let's do this the ugly way (but rather very
  // simple), just list out the types of BCM's supported by this code!!!
  if( type == "VQWK") {
    return new QwCombinedBCM<QwVQWK_Channel>(dynamic_cast<const QwCombinedBCM<QwVQWK_Channel>&>(source));
  } else if ( type == "ADC18" ) {
    return new QwCombinedBCM<QwADC18_Channel>(dynamic_cast<const QwCombinedBCM<QwADC18_Channel>&>(source));
  } else if ( type == "SIS3801" ) {
    return new QwCombinedBCM<QwSIS3801_Channel>(dynamic_cast<const QwCombinedBCM<QwSIS3801_Channel>&>(source));
  } else if ( type == "SCALER" || type == "SIS3801D24" ) {
    return new QwCombinedBCM<QwSIS3801D24_Channel>(dynamic_cast<const QwCombinedBCM<QwSIS3801D24_Channel>&>(source));
  } else if ( type == "MOLLERADC" ) {
    return new QwCombinedBCM<QwMollerADC_Channel>(dynamic_cast<const QwCombinedBCM<QwMollerADC_Channel>&>(source));
  } else { // Unsupported one!
    QwWarning << "BCM of type="<<type<<" is UNSUPPORTED!!\n";
    exit(-1);
  }
}
