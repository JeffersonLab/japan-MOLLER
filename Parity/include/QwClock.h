/*!
 * \file   QwClock.h
 * \brief  Clock channel implementation for normalization and timing
 *
 * \author Juan Carlos Cornejo <cornejo@jlab.org>
 * \date   2011-06-16
 */

#pragma once

// System headers
#include <vector>

// ROOT headers
#include <TTree.h>
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#endif // HAS_RNTUPLE_SUPPORT

#include "QwParameterFile.h"
#include <stdexcept>
#include "VQwDataElement.h"
#include "VQwHardwareChannel.h"
#include "VQwClock.h"

// Forward declarations
#ifdef __USE_DATABASE__
class QwDBInterface;
#endif // __USE_DATABASE__

/**
 * \class QwClock
 * \ingroup QwAnalysis_BL
 * \brief Standard clock channel with calibration representing frequency
 *
 * Provides timing and normalization support for subsystems that need an
 * external clock. The calibration factor encodes the clock frequency.
 * Implements specialized polymorphic dispatch for burp-failure checks
 * via the VQwClock base as per the dual-operator pattern.
 */
template<typename T>
class QwClock : public VQwClock {
/////
 public:
  QwClock() { };
  QwClock(TString subsystemname, TString name, TString type = ""){
    SetSubsystemName(subsystemname);
    InitializeChannel(subsystemname, name, "raw", type);
  };
  QwClock(const QwClock& source)
  : VQwClock(source),
    fPedestal(source.fPedestal),fCalibration(source.fCalibration),
    fULimit(source.fULimit),fLLimit(source.fLLimit),
    fClock(source.fClock),
    fNormalizationValue(source.fNormalizationValue)
  { }
  ~QwClock() override { };

  void LoadChannelParameters(QwParameterFile &paramfile) override{
    fClock.LoadChannelParameters(paramfile);
  };

  Int_t ProcessEvBuffer(UInt_t* buffer, UInt_t word_position_in_buffer, UInt_t subelement=0) override;

  void  InitializeChannel(TString subsystem, TString name, TString datatosave, TString type = "") override;
  void  ClearEventData() override;


  void  EncodeEventData(std::vector<UInt_t> &buffer);

  void  ProcessEvent() override;
  Bool_t ApplyHWChecks();//Check for hardware errors in the devices
  Bool_t ApplySingleEventCuts() override;//Check for good events by setting limits on the devices readings
  void IncrementErrorCounters() override{fClock.IncrementErrorCounters();}

  Bool_t CheckForBurpFail(const QwClock *ev_error){
    return fClock.CheckForBurpFail(&(ev_error->fClock));
  }

  // Override the pure virtual from VQwClock to enable polymorphic dispatch via VQwClock*
  Bool_t CheckForBurpFail(const VQwClock* ev_error) override {
    auto rhs = dynamic_cast<const QwClock*>(ev_error);
    if (!rhs) {
      throw std::invalid_argument("Type mismatch in QwClock::CheckForBurpFail(VQwClock*)");
    }
    return CheckForBurpFail(rhs);
  }

  // Polymorphic delegator: match VQwDataElement signature so calls via VQwClock* hit this path
  Bool_t CheckForBurpFail(const VQwDataElement* ev_error){
    auto rhs = dynamic_cast<const QwClock*>(ev_error);
    if (!rhs) {
      throw std::invalid_argument("Type mismatch in QwClock::CheckForBurpFail");
    }
    return CheckForBurpFail(rhs);
  }


  void PrintErrorCounters() const override;// report number of events failed due to HW and event cut failure
  UInt_t GetEventcutErrorFlag() override{//return the error flag
    return fClock.GetEventcutErrorFlag();
  }
  UInt_t UpdateErrorFlag() override {return GetEventcutErrorFlag();};
  void UpdateErrorFlag(const QwClock *ev_error){
    fClock.UpdateErrorFlag(ev_error->fClock);
  }

  /*! \brief Inherited from VQwDataElement to set the upper and lower limits (fULimit and fLLimit), stability % and the error flag on this channel */
  void SetSingleEventCuts(UInt_t errorflag, Double_t min = 0, Double_t max = 0, Double_t stability = 0, Double_t burplevel = 0) override;

  void SetDefaultSampleSize(Int_t sample_size);
  void SetEventCutMode(Int_t bcuts) override{
    bEVENTCUTMODE=bcuts;
    fClock.SetEventCutMode(bcuts);
  }

  void PrintValue() const override;
  void PrintInfo() const override;

  // Implementation of Parent class's virtual operators
  VQwClock& operator=  (const VQwClock &value) override;
  VQwClock& operator+= (const VQwClock &value) override;
  VQwClock& operator-= (const VQwClock &value) override;

  // This class specific operators
  QwClock& operator=  (const QwClock &value);
  QwClock& operator+= (const QwClock &value);
  QwClock& operator-= (const QwClock &value);
  void Ratio(const VQwClock &numer, const VQwClock &denom) override;
  void Ratio(const QwClock &numer, const QwClock &denom);
  void Scale(Double_t factor) override;

  void AccumulateRunningSum(const VQwClock& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  void DeaccumulateRunningSum(VQwClock& value, Int_t ErrorMask=0xFFFFFFF) override;
  void CalculateRunningAverage() override;

  void SetPedestal(Double_t ped) override;
  void SetCalibrationFactor(Double_t calib) override;

  void  ConstructHistograms(TDirectory *folder, TString &prefix) override;
  void  FillHistograms() override;

  void  ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) override;
  void  ConstructBranch(TTree *tree, TString &prefix) override;
  void  ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist) override;
  void  FillTreeVector(QwRootTreeBranchVector &values) const override;
#ifdef HAS_RNTUPLE_SUPPORT
  void  ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
  void  FillNTupleVector(std::vector<Double_t>& values) const override;
#endif // HAS_RNTUPLE_SUPPORT

#ifdef __USE_DATABASE__
  std::vector<QwDBInterface> GetDBEntry() override;
#endif // __USE_DATABASE__

  // These are related to those hardware channels that need to normalize
  // to an external clock
  Double_t GetNormClockValue() override { return fNormalizationValue;};
  Double_t GetStandardClockValue() override { return fCalibration; };

  const VQwHardwareChannel* GetTime() const override {
    return &fClock;
  };


/////
 protected:

/////
 private:
  Double_t fPedestal;
  Double_t fCalibration;
  Double_t fULimit, fLLimit;

  T fClock;

  Int_t fDeviceErrorCode;//keep the device HW status using a unique code from the QwVQWK_Channel::fDeviceErrorCode

  const static  Bool_t bDEBUG=kFALSE;//debugging display purposes
  Bool_t bEVENTCUTMODE;//If this set to kFALSE then Event cuts do not depend on HW checks. This is set externally through the qweak_beamline_eventcuts.map

  Double_t fNormalizationValue;

};
