/*!
 * \file   QwBPMStripline.h
 * \brief  Stripline beam position monitor implementation
 */

#pragma once

// System headers
#include <vector>

// ROOT headers
#include <TTree.h>
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RField.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "VQwBPM.h"
#include "QwParameterFile.h"
#include "QwUtil.h"

// Forward declarations
#ifdef __USE_DATABASE__
class QwDBInterface;
class QwErrDBInterface;
#endif // __USE_DATABASE__
class QwPromptSummary;

/**
 * \class QwBPMStripline
 * \ingroup QwAnalysis_BeamLine
 * \brief Templated concrete stripline beam position monitor implementation
 *
 * Template class for stripline BPMs using hardware channel type T.
 * Implements position calculation from four stripline signals (XP, XM, YP, YM),
 * coordinate transformations, effective charge calculation, and calibration.
 * Supports rotation corrections and geometry-based position calculations.
 */
template<typename T>
class QwBPMStripline : public VQwBPM {
  template <typename TT> friend class QwCombinedBPM;
  friend class QwEnergyCalculator;

 public:
  static UInt_t  GetSubElementIndex(TString subname);

 public:
  QwBPMStripline() { };
  QwBPMStripline(TString name) {
    InitializeChannel(name);
    fRotationAngle = 45.0;
    SetRotation(fRotationAngle);
    bRotated=kTRUE;
  };
  QwBPMStripline(TString subsystemname, TString name) {
    SetSubsystemName(subsystemname);
    InitializeChannel(subsystemname, name);
    fRotationAngle = 45.0;
    SetRotation(fRotationAngle);
    bRotated=kTRUE;
  };
  QwBPMStripline(TString subsystemname, TString name, TString type) {
    SetSubsystemName(subsystemname);
    InitializeChannel(subsystemname, name, type);
    fRotationAngle = 45.0;
    SetRotation(fRotationAngle);
    bRotated=kTRUE;
  };
  QwBPMStripline(const QwBPMStripline& source)
  : VQwBPM(source),
    fEffectiveCharge(source.fEffectiveCharge),fEllipticity(source.fEllipticity)
  {
    QwCopyArray(source.fWire, fWire);
    QwCopyArray(source.fRelPos, fRelPos);
    QwCopyArray(source.fAbsPos, fAbsPos);
  }
  ~QwBPMStripline() override { };

  void    InitializeChannel(TString name);
  // new routine added to update necessary information for tree trimming
  void    InitializeChannel(TString subsystem, TString name);
  void    InitializeChannel(TString subsystem, TString name, TString type);
  void    ClearEventData() override;

  void LoadChannelParameters(QwParameterFile &paramfile) override{
    for(Short_t i=0;i<4;i++){
      fWire[i].LoadChannelParameters(paramfile);
    }
    fAbsPos[0].LoadChannelParameters(paramfile);
    fAbsPos[1].LoadChannelParameters(paramfile);
  }

  Int_t   ProcessEvBuffer(UInt_t* buffer,
			UInt_t word_position_in_buffer,UInt_t indexnumber) override;
  void    ProcessEvent() override;
  void    PrintValue() const override;
  void    PrintInfo() const override;
  void    WritePromptSummary(QwPromptSummary *ps, TString type);


  const VQwHardwareChannel* GetPosition(EBeamPositionMonitorAxis axis) const override {
    if (axis<0 || axis>2){
      TString loc="QwBPMStripline::GetPosition for "
        +this->GetElementName()+" failed for axis value "+Form("%d",axis);
      throw std::out_of_range(loc.Data());
    }
    return &fAbsPos[axis];
  }
  const VQwHardwareChannel* GetEffectiveCharge() const override {return &fEffectiveCharge;}
  const VQwHardwareChannel* GetEllipticity() const {return &fEllipticity;}

  TString GetSubElementName(Int_t subindex) override;
  void    GetAbsolutePosition() override;

  Bool_t  ApplyHWChecks();//Check for hardware errors in the devices
  Bool_t  ApplySingleEventCuts() override;//Check for good events by setting limits on the devices readings
  //void    SetSingleEventCuts(TString ch_name, Double_t minX, Double_t maxX);
  /*   /\*! \brief Inherited from VQwDataElement to set the upper and lower limits (fULimit and fLLimit), stability % and the error flag on this channel *\/ */
  void    SetSingleEventCuts(TString ch_name, UInt_t errorflag,Double_t minX, Double_t maxX, Double_t stability, Double_t burplevel);
  //void    SetSingleEventCuts(TString ch_name, UInt_t errorflag,Double_t min, Double_t max, Double_t stability, Double_t burplevel){return;};
  void    SetEventCutMode(Int_t bcuts) override;
  void    IncrementErrorCounters() override;
  void    PrintErrorCounters() const override;   // report number of events failed due to HW and event cut failure
  UInt_t  GetEventcutErrorFlag() override;
  UInt_t  UpdateErrorFlag() override;

  Bool_t  CheckForBurpFail(const VQwDataElement *ev_error) override;
  void    UpdateErrorFlag(const VQwBPM *ev_error) override;


  void    SetDefaultSampleSize(Int_t sample_size) override;
  void    SetRandomEventParameters(Double_t meanX, Double_t sigmaX, Double_t meanY, Double_t sigmaY) override;
  void    RandomizeEventData(int helicity = 0, double time = 0.0) override;
  void    LoadMockDataParameters(QwParameterFile &paramfile) override;
  void    ApplyResolutionSmearing() override;
  void    ApplyResolutionSmearing(EBeamPositionMonitorAxis iaxis) override;
  void    FillRawEventData() override;
  void    EncodeEventData(std::vector<UInt_t> &buffer) override;
  void    SetSubElementPedestal(Int_t j, Double_t value) override;
  void    SetSubElementCalibrationFactor(Int_t j, Double_t value) override;

  void    Ratio(VQwBPM &numer, VQwBPM &denom) override;
  void    Ratio(QwBPMStripline &numer, QwBPMStripline &denom);
  void    Scale(Double_t factor) override;

  VQwBPM& operator=  (const VQwBPM &value) override;
  VQwBPM& operator+= (const VQwBPM &value) override;
  VQwBPM& operator-= (const VQwBPM &value) override;

  virtual QwBPMStripline& operator=  (const QwBPMStripline &value);
  virtual QwBPMStripline& operator+= (const QwBPMStripline &value);
  virtual QwBPMStripline& operator-= (const QwBPMStripline &value);

  void    AccumulateRunningSum(const QwBPMStripline& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
  void    AccumulateRunningSum(const VQwBPM& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  void    DeaccumulateRunningSum(VQwBPM& value, Int_t ErrorMask=0xFFFFFFF) override;
  void    DeaccumulateRunningSum(QwBPMStripline& value, Int_t ErrorMask=0xFFFFFFF);

  void    CalculateRunningAverage() override;

  void    ConstructHistograms(TDirectory *folder, TString &prefix) override;
  void    FillHistograms() override;

  void    ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) override;
  void    ConstructBranch(TTree *tree, TString &prefix) override;
  void    ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist) override;
  void    FillTreeVector(QwRootTreeBranchVector &values) const override;

#ifdef HAS_RNTUPLE_SUPPORT
  // RNTuple methods
  void    ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
  void    FillNTupleVector(std::vector<Double_t>& values) const override;
#endif


#ifdef __USE_DATABASE__
  std::vector<QwDBInterface> GetDBEntry() override;
  std::vector<QwErrDBInterface> GetErrDBEntry() override;
#endif // __USE_DATABASE__

 protected:
  ///  Should be used inside the QwCombinedBPM::GetProjectedPosition function to assign the
  ///  BPM's X and Y values based on the slope and intercept of the combinedBPM.
  VQwHardwareChannel* GetPosition(EBeamPositionMonitorAxis axis) override {
    if (axis<0 || axis>2){
      TString loc="QwBPMStripline::GetPosition for "
        +this->GetElementName()+" failed for axis value "+Form("%d",axis);
      throw std::out_of_range(loc.Data());
    }
    return &fAbsPos[axis];
  }

  VQwHardwareChannel* GetSubelementByName(TString ch_name) override;


  /////
 private:
  /* /\*  Position calibration factor, transform ADC counts in mm *\/ */
  /* static Double_t kQwStriplineCalibration; */
  /* static Double_t fRelativeGains[2]; */
  /* Rotation factor for the BPM which antenna are at 45 deg */
  static const Double_t kRotationCorrection;
  static const TString subelement[4];



 protected:
  std::array<T,4> fWire;//[4];
  std::array<T,2> fRelPos;//[2];

  //  These are the "real" data elements, to which the base class
  //  fAbsPos_base and fEffectiveCharge_base are pointers.
  std::array<T,2> fAbsPos;//[2];
  T fEffectiveCharge;
  T fEllipticity;

private:
  // Functions to be removed
  void    SetEventData(Double_t* block, UInt_t sequencenumber);
  std::vector<T> fBPMElementList;
  void    MakeBPMList();




};
