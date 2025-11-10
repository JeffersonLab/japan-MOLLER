/*!
 * \file   QwBlindDetectorArray.h
 * \brief  Blinded detector array for PMT analysis
 * \author P. M. King
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
class QwBlinder;
class QwBlindDetectorArrayID;


/**
 * \class QwBlindDetectorArray
 * \ingroup QwAnalysis_ADC
 * \brief Detector array wrapper that applies blinding to asymmetries
 *
 * Aggregates multiple PMT-like detectors and provides methods to
 * apply blinding strategies consistently to yields, differences,
 * and asymmetries. Used for parity-violating detector analysis.
 */
class QwBlindDetectorArray:
 public VQwDetectorArray,
 virtual public VQwSubsystemParity,
 public MQwSubsystemCloneable<QwBlindDetectorArray>{

  /******************************************************************
   *  Class: QwBlindDetectorArray
   *
   *
   ******************************************************************/


 private:

  /// Private default constructor (not implemented, will throw linker error on use)
  QwBlindDetectorArray();

 public:

  /// Constructor with name
  QwBlindDetectorArray(const TString& name)
  : VQwSubsystem(name), VQwSubsystemParity(name), VQwDetectorArray(name) {};


  /// Copy constructor
  QwBlindDetectorArray(const QwBlindDetectorArray& source)
  : VQwSubsystem(source),VQwSubsystemParity(source), VQwDetectorArray(source){};


  /// Virtual destructor
  ~QwBlindDetectorArray() override;

  /// Inherit assignment operator on base class
  using VQwDetectorArray::operator=;

  public:

  /// \brief Blind the asymmetry
  void Blind(const QwBlinder *blinder) override;

  /// \brief Blind the difference using the yield
  void Blind(const QwBlinder *blinder, const VQwSubsystemParity* subsys) override;

#ifdef HAS_RNTUPLE_SUPPORT
  // Inherit RNTuple methods from VQwDetectorArray - no need to redeclare
  using VQwDetectorArray::ConstructNTupleAndVector;
  using VQwDetectorArray::FillNTupleVector;
#endif // HAS_RNTUPLE_SUPPORT

};

// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwBlindDetectorArray);
