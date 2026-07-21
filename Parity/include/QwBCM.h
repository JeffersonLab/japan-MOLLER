/*!
 * \file   QwBCM.h
 * \brief  Beam current monitor template class
 */

#pragma once

// System headers
#include <vector>

// ROOT headers
#include <TTree.h>
#include <ROOT/RNTupleModel.hxx>

#include "QwParameterFile.h"
#include "VQwDataElement.h"
#include "VQwHardwareChannel.h"
#include "VQwBCM.h"

// Forward declarations
class QwDBInterface;
class QwErrDBInterface;

template<typename T> class QwCombinedBCM;

/**
 * \class QwBCM
 * \ingroup QwAnalysis_BeamLine
 * \brief Templated concrete beam current monitor implementation
 *
 * Template class that implements a beam current monitor using a specified
 * hardware channel type T. Handles event decoding, calibration, single-event
 * cuts, mock data generation, and database output. Supports external clock
 * normalization and statistical analysis.
 */
template<typename T> class QwBCM : public VQwBCM {
/////
  friend class QwCombinedBCM<T>;
 public:
  QwBCM(): VQwBCM(fBeamCurrent) { };
  QwBCM(TString name): VQwBCM(fBeamCurrent,name) {
    InitializeChannel(name,"raw");
  };
  QwBCM(TString subsystemname, TString name)
  : VQwBCM(fBeamCurrent,name) {
    SetSubsystemName(subsystemname);
    InitializeChannel(subsystemname,name,"raw");
  };
  QwBCM(TString subsystemname, TString name, TString type, TString clock = "")
  : VQwBCM(fBeamCurrent,name) {
    fBeamCurrent.SetExternalClockName(clock.Data());
    SetSubsystemName(subsystemname);
    InitializeChannel(subsystemname,name,type,"raw");
  };
  QwBCM(const QwBCM& source)
  : VQwBCM(source),
    fBeamCurrent(source.fBeamCurrent)
  { }
  ~QwBCM() override { };

  Int_t ProcessEvBuffer(UInt_t* buffer, UInt_t word_position_in_buffer, UInt_t subelement=0) override;

  void  InitializeChannel(TString name, TString datatosave) override;
  // new routine added to update necessary information for tree trimming
  void  InitializeChannel(TString subsystem, TString name, TString datatosave) override;
  void  InitializeChannel(TString subsystem, TString name, TString type,
      TString datatosave);
  void  ClearEventData() override;

  void LoadChannelParameters(QwParameterFile &paramfile) override{
    fBeamCurrent.LoadChannelParameters(paramfile);
  };

  void  SetRandomEventDriftParameters(Double_t amplitude, Double_t phase, Double_t frequency);
  void  AddRandomEventDriftParameters(Double_t amplitude, Double_t phase, Double_t frequency) override;
  void  SetRandomEventParameters(Double_t mean, Double_t sigma) override;
  void  SetRandomEventAsymmetry(Double_t asymmetry) override;

  void  SetResolution(Double_t resolution){
    fResolution = resolution;
  }

  void  ApplyResolutionSmearing() override;
  void  FillRawEventData() override;


//-----------------------------------------------------------------------------------------------
  void  RandomizeEventData(int helicity = 0, double time = 0) override;
  void  LoadMockDataParameters(QwParameterFile &paramfile) override;
//-----------------------------------------------------------------------------------------------

  void  EncodeEventData(std::vector<UInt_t> &buffer) override;

  void  UseExternalRandomVariable();
  void  SetExternalRandomVariable(Double_t random_variable);

  void  ProcessEvent() override;
  Bool_t ApplyHWChecks();//Check for hardware errors in the devices
  Bool_t ApplySingleEventCuts() override;//Check for good events by setting limits on the devices readings
  void IncrementErrorCounters() override;
  void PrintErrorCounters() const override;// report number of events failed due to HW and event cut failure
  UInt_t GetEventcutErrorFlag() override{//return the error flag
    return fBeamCurrent.GetEventcutErrorFlag();
  }

  void UpdateErrorFlag(const VQwBCM *ev_error) override;

  UInt_t GetErrorCode() const {return (fBeamCurrent.GetErrorCode());};


  Int_t SetSingleEventCuts(Double_t mean = 0, Double_t sigma = 0);//two limits and sample size
  /*! \brief Inherited from VQwDataElement to set the upper and lower limits (fULimit and fLLimit), stability % and the error flag on this channel */
  void SetSingleEventCuts(UInt_t errorflag, Double_t min = 0, Double_t max = 0, Double_t stability = 0, Double_t burplevel = 0) override;

  void SetDefaultSampleSize(Int_t sample_size) override;
  void SetEventCutMode(Int_t bcuts) override {
    fBeamCurrent.SetEventCutMode(bcuts);
  }

  void PrintValue() const override;
  void PrintInfo() const override;

protected:
  VQwHardwareChannel* GetCharge() override {
    return &fBeamCurrent;
  };
public:

  // These are for the clocks
  std::string GetExternalClockName() override { return fBeamCurrent.GetExternalClockName(); };
  Bool_t NeedsExternalClock() override { return fBeamCurrent.NeedsExternalClock(); };
  void SetExternalClockPtr( const VQwHardwareChannel* clock) override {fBeamCurrent.SetExternalClockPtr(clock);};
  void SetExternalClockName( const std::string name) override { fBeamCurrent.SetExternalClockName(name);};
  Double_t GetNormClockValue() override { return fBeamCurrent.GetNormClockValue();}

  // Implementation of Parent class's virtual operators
  VQwBCM& operator=  (const VQwBCM &value) override;
  VQwBCM& operator+= (const VQwBCM &value) override;
  VQwBCM& operator-= (const VQwBCM &value) override;

  // This is used only by a QwComboBCM. It is placed here since in QwBeamLine we do
  // not readily have the appropriate template every time we want to use this
  // function.
  void SetBCMForCombo(VQwBCM* bcm, Double_t weight, Double_t sumqw ) override {
    std::cerr<<"SetBCMForCombo for QwCombinedBCM<T> not defined!!\n";
  };

  // This class specific operators
  QwBCM& operator=  (const QwBCM &value);
  QwBCM& operator+= (const QwBCM &value);
  QwBCM& operator-= (const QwBCM &value);
  void Ratio(const VQwBCM &numer, const VQwBCM &denom) override;
  void Ratio(const QwBCM &numer, const QwBCM &denom);
  void Scale(Double_t factor) override;

  void AccumulateRunningSum(const VQwBCM&, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  void DeaccumulateRunningSum(VQwBCM& value, Int_t ErrorMask=0xFFFFFFF) override;
  void CalculateRunningAverage() override;

  Bool_t CheckForBurpFail(const VQwDataElement *ev_error) override;

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
#endif

#ifdef __USE_DATABASE__
  std::vector<QwDBInterface> GetDBEntry() override;
  std::vector<QwErrDBInterface> GetErrDBEntry() override;
#endif

  Double_t GetValue() override;
  Double_t GetValueError() override;
  Double_t GetValueWidth() override;


/////
 protected:
  T fBeamCurrent;

/////
 private:
 Double_t fResolution;

};
