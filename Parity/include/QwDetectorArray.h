/*!
 * \file   QwDetectorArray.h
 * \brief  Detector array for PMT analysis with integration and combination
 * \author Kevin Ward (Original Code by P. M. King)
 * \date   2007-05-08
 */

/// \ingroup QwAnalysis_ADC

#pragma once

// System headers
#include <vector>

// Qweak headers
#include "VQwSubsystemParity.h"
#include "QwIntegrationPMT.h"
#include "QwCombinedPMT.h"
#include "VQwDetectorArray.h"

// Forward declarations
class QwDetectorArrayID;

/**
 * \class QwDetectorArray
 * \ingroup QwAnalysis_ADC
 * \brief Subsystem for managing arrays of PMT detectors with integration and combination
 *
 * Manages collections of integration PMTs and combined PMT channels,
 * providing coordinated event processing, calibration, and output for
 * detector array measurements. Supports various PMT configurations
 * and combination schemes.
 */
class QwDetectorArray:
 public VQwDetectorArray,
 virtual public VQwSubsystemParity,
 public MQwSubsystemCloneable<QwDetectorArray> {

/******************************************************************
*  Class: QwDetectorArray
*
*
******************************************************************/

private:

/// Private default constructor (not implemented, will throw linker error on use)
QwDetectorArray();

public:

/// Constructor with name
QwDetectorArray(const TString& name)
 :VQwSubsystem(name), VQwSubsystemParity(name), VQwDetectorArray(name){};


/// Copy constructor
QwDetectorArray(const QwDetectorArray& source)
  :VQwSubsystem(source), VQwSubsystemParity(source), VQwDetectorArray(source){};


/// Virtual destructor
~QwDetectorArray() override;


/// Inherit assignment operator on base class
using VQwDetectorArray::operator=;

};

// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwDetectorArray);
