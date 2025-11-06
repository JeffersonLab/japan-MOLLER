/**********************************************************\
* File: VQwBCM.h                                           *
*                                                          *
* Author: (??) & J.C Cornejo                               *
* Time-stamp: <2011-05-26>                                 *
\**********************************************************/

/*!
 * \file   VQwBCM.h
 * \brief  Virtual base class for beam current monitors
 */

#pragma once

// System headers
#include <vector>

// ROOT headers
#include <TTree.h>

// RNTuple headers
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#endif // HAS_RNTUPLE_SUPPORT

#include "QwParameterFile.h"
#include "VQwDataElement.h"
#include "VQwHardwareChannel.h"

// Forward declarations
#ifdef __USE_DATABASE__
class QwDBInterface;
class QwErrDBInterface;
#endif // __USE_DATABASE__

template<typename T> class QwCombinedBCM;
template<typename T> class QwBCM;

/**
 * \ingroup QwAnalysis_BeamLine
 */
/**
 * \class VQwBCM
 * \ingroup QwAnalysis_BeamLine
 * \brief Abstract base for beam current monitors (BCMs)
 *
 * Provides the interface for current-like data elements used for normalization
 * and beam quality monitoring. Concrete implementations (e.g., QwBCM<T>,
 * QwCombinedBCM<T>) implement hardware decoding, event processing, and error
 * handling, while this base exposes common hooks for the analysis framework.
 */
class VQwBCM : public VQwDataElement {
  /***************************************************************
   *  Class:  VQwBCM
   *          Pure Virtual base class for the BCMs in the beamline.
   *          Through use of the Create factory function, one can
   *          get a concrete instance of a templated QwBCM.
   *
   ***************************************************************/
  template <typename T> friend class QwBCM;
  template <typename T> friend class QwCombinedBCM;

protected:
  VQwBCM(VQwDataElement& beamcurrent): fBeamCurrent_ref(beamcurrent) { };
  VQwBCM(VQwDataElement& beamcurrent, TString /*name*/): fBeamCurrent_ref(beamcurrent) { };

public:
  ~VQwBCM() override { };

  // VQwDataElement virtual functions
  Int_t ProcessEvBuffer(UInt_t* buffer, UInt_t word_position_in_buffer, UInt_t subelement=0) override = 0;
  void  ConstructHistograms(TDirectory *folder, TString &prefix) override = 0;
  void  FillHistograms() override = 0;
  /*! \brief Inherited from VQwDataElement to set the upper and lower limits (fULimit and fLLimit), stability % and the error flag on this channel */
  virtual void SetSingleEventCuts(UInt_t errorflag,Double_t min, Double_t max, Double_t stability, Double_t burplevel) = 0;
  virtual void Ratio( const VQwBCM &/*numer*/, const VQwBCM &/*denom*/)
    { throw std::runtime_error(std::string("Ratio() is not defined for BCM named ") + GetElementName().Data()); }
  void ClearEventData() override = 0;

  // Virtual functions delegated to sub classes
  virtual void  InitializeChannel(TString name, TString datatosave) = 0;
  // new routine added to update necessary information for tree trimming
  virtual void  InitializeChannel(TString subsystem, TString name, TString datatosave) = 0;

  void LoadChannelParameters(QwParameterFile &paramfile) override = 0;
  Bool_t NeedsExternalClock() override = 0;
  void SetExternalClockPtr( const VQwHardwareChannel* clock) override = 0;
  void SetExternalClockName( const std::string name) override = 0;
  Double_t GetNormClockValue() override = 0;

  virtual void SetDefaultSampleSize(Int_t sample_size) = 0;
  virtual void SetEventCutMode(Int_t bcuts) = 0;
  UInt_t UpdateErrorFlag() override{return this->GetEventcutErrorFlag();};
  virtual void UpdateErrorFlag(const VQwBCM *ev_error) = 0;
  virtual void SetPedestal(Double_t ped) = 0;
  virtual void SetCalibrationFactor(Double_t calib) = 0;
  virtual void RandomizeEventData(int helicity, double time) = 0;
  virtual void EncodeEventData(std::vector<UInt_t> &buffer) = 0;
  virtual Bool_t ApplySingleEventCuts() = 0;//Check for good events by setting limits on the devices readings
  virtual void IncrementErrorCounters() = 0;
  virtual void  ProcessEvent() = 0;
  virtual void Scale(Double_t factor) = 0;
  virtual void CalculateRunningAverage() = 0;
  virtual void AccumulateRunningSum(const VQwBCM& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) = 0;
  virtual void DeaccumulateRunningSum(VQwBCM& value, Int_t ErrorMask=0xFFFFFFF) = 0;
  virtual void ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) = 0;
  virtual void ConstructBranch(TTree *tree, TString &prefix) = 0;
  virtual void ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist) = 0;
  virtual void FillTreeVector(QwRootTreeBranchVector &values) const = 0;

#ifdef HAS_RNTUPLE_SUPPORT
  virtual void ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) = 0;
  virtual void FillNTupleVector(std::vector<Double_t>& values) const = 0;
#endif // HAS_RNTUPLE_SUPPORT

//----------------------------------------------------------------------------------------------------------------
  virtual void ApplyResolutionSmearing()
    {std::cerr << "ApplyResolutionSmearing is undefined!!!\n";}
  virtual void    FillRawEventData()
    {std::cerr << "FillRawEventData for VQwBPM not implemented!\n";};
  virtual void GetProjectedCharge(VQwBCM */*device*/){};
  virtual size_t GetNumberOfElements(){return size_t(1);}
  virtual TString GetSubElementName(Int_t /*subindex*/)
  {
    std::cerr << "GetSubElementName()  is not implemented!! for device: " << GetElementName() << "\n";
    return TString("OBJECT_UNDEFINED"); // Return an erroneous TString
  }
//----------------------------------------------------------------------------------------------------------------

#ifdef __USE_DATABASE__
  virtual std::vector<QwDBInterface> GetDBEntry() = 0;
  virtual std::vector<QwErrDBInterface> GetErrDBEntry() = 0;
#endif // __USE_DATABASE__

  virtual Double_t GetValue() = 0;
  virtual Double_t GetValueError() = 0;
  virtual Double_t GetValueWidth() = 0;

//------------------------------------------------------------------------------------------------------------------------------------------

  //  virtual const VQwHardwareChannel* GetCharge() const = 0; //Replace this with a const_cast use of the protected function; define a pure virtual protected function that is non-const; In the QwBCM class, change the defined function to be the protected non-const version.

  virtual const VQwHardwareChannel* GetCharge() const{
    return const_cast<VQwBCM*>(this)->GetCharge();
  }

  // Ensure polymorphic dispatch for burp-failure checks via VQwBCM*
  virtual Bool_t CheckForBurpFail(const VQwDataElement* ev_error) {
    return VQwDataElement::CheckForBurpFail(ev_error);
  }

protected:
  virtual VQwHardwareChannel* GetCharge() = 0;

//------------------------------------------------------------------------------------------------------------------------------------------

public:

  virtual void  SetRandomEventParameters(Double_t mean, Double_t sigma) = 0;
  virtual void  SetRandomEventAsymmetry(Double_t asymmetry) = 0 ;
  virtual void  AddRandomEventDriftParameters(Double_t amplitude, Double_t phase, Double_t frequency) =0;

  // Operators
  virtual VQwBCM& operator=  (const VQwBCM &value) =0;
  virtual VQwBCM& operator+= (const VQwBCM &value) =0;
  virtual VQwBCM& operator-= (const VQwBCM &value) =0;

  // This one is for QwCombinedBCM (could be done differently)
  virtual void SetBCMForCombo(VQwBCM* bcm, Double_t weight, Double_t sumqw ) = 0;


  // Factory function to produce appropriate BCM
  static VQwBCM* Create(TString subsystemname, TString type, TString name, TString clock = "");
  static VQwBCM* Create(const VQwBCM& source); // Create a generic BCM (define properties later)
  static VQwBCM* CreateCombo(TString subsystemname, TString type, TString name);
  static VQwBCM* CreateCombo(const VQwBCM& source); // Create a generic BCM (define properties later)


protected:
  VQwDataElement& fBeamCurrent_ref;

};

typedef std::shared_ptr<VQwBCM> VQwBCM_ptr;
