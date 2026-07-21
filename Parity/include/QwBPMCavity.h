/*!
 * \file   QwBPMCavity.h
 * \brief  Cavity beam position monitor implementation
 */

#pragma once

// System headers
#include <vector>

// ROOT headres
#include <TTree.h>
#ifdef HAS_RNTUPLE_SUPPORT
#include <ROOT/RNTupleModel.hxx>
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "QwVQWK_Channel.h"
#include "VQwBPM.h"
#include "QwParameterFile.h"
#include "QwUtil.h"

// Forward declarations
class QwDBInterface;
class QwErrDBInterface;

/*****************************************************************
*  Class:
******************************************************************/
/**
 * \class QwBPMCavity
 * \ingroup QwAnalysis_BL
 * \brief Cavity-style BPM using VQWK channels
 *
 * Provides X/Y position and effective charge from cavity readouts, with
 * utilities for cuts, histograms, and tree/NTuple output.
 */
class QwBPMCavity : public VQwBPM {
  template <typename TT> friend class QwCombinedBPM;
  friend class QwEnergyCalculator;

 public:
  enum ECavElements{kXElem=0, kYElem, kQElem, kNumElements};
  static UInt_t GetSubElementIndex(TString subname);
  static Bool_t ParseChannelName(const TString &channel, TString &detname,
				 TString &subname, UInt_t &localindex);
 public:
  QwBPMCavity() { };
  QwBPMCavity(TString name):VQwBPM(name){
    InitializeChannel(name);
  };
  QwBPMCavity(TString subsystemname, TString name)
  : VQwBPM(name) {
	  SetSubsystemName(subsystemname);
	  InitializeChannel(subsystemname, name);
  };
  QwBPMCavity(const QwBPMCavity& source)
  : VQwBPM(source)
  {
    QwCopyArray(source.fElement, fElement);
    QwCopyArray(source.fRelPos, fRelPos);
    QwCopyArray(source.fAbsPos, fAbsPos);
  }
  ~QwBPMCavity() override { };

  void    InitializeChannel(TString name);
  // new routine added to update necessary information for tree trimming
  void  InitializeChannel(TString subsystem, TString name);
  void    ClearEventData() override;

  void LoadChannelParameters(QwParameterFile &paramfile) override{
    for(Short_t i=0;i<kNumElements;i++)
      fElement[i].LoadChannelParameters(paramfile);
  }

  Int_t   ProcessEvBuffer(UInt_t* buffer,
			UInt_t word_position_in_buffer,UInt_t indexnumber) override;
  void    ProcessEvent() override;
  void    PrintValue() const override;
  void    PrintInfo() const override;

  const VQwHardwareChannel* GetPosition(EBeamPositionMonitorAxis axis) const override {
    if (axis<0 || axis>2){
      TString loc="QwBPMCavity::GetPosition for "
        +this->GetElementName()+" failed for axis value "+Form("%d",axis);
      throw std::out_of_range(loc.Data());
    }
    return &fAbsPos[axis];
  }
  const VQwHardwareChannel* GetEffectiveCharge() const override {return &fElement[kQElem];}

  TString GetSubElementName(Int_t subindex) override;
  void    GetAbsolutePosition() override;

  Bool_t  ApplyHWChecks();//Check for hardware errors in the devices
  Bool_t  ApplySingleEventCuts() override;//Check for good events by setting limits on the devices readings
  //void    SetSingleEventCuts(TString ch_name, Double_t minX, Double_t maxX);
  /*! \brief Inherited from VQwDataElement to set the upper and lower limits (fULimit and fLLimit), stability % and the error flag on this channel */
  void    SetSingleEventCuts(TString ch_name, UInt_t errorflag,Double_t minX, Double_t maxX, Double_t stability, Double_t burplevel);
  void    SetEventCutMode(Int_t bcuts) override;
  void IncrementErrorCounters() override;
  void PrintErrorCounters() const override;// report number of events failed due to HW and event cut failure
  UInt_t  GetEventcutErrorFlag() override;
  UInt_t  UpdateErrorFlag() override;
  void UpdateErrorFlag(const VQwBPM *ev_error) override;

  Bool_t  CheckForBurpFail(const VQwDataElement *ev_error) override;

  void    SetDefaultSampleSize(Int_t sample_size) override;
  void    SetRandomEventParameters(Double_t meanX, Double_t sigmaX, Double_t meanY, Double_t sigmaY) override;
  void    RandomizeEventData(int helicity = 0, double time = 0.0) override;
  void    SetEventData(Double_t* block, UInt_t sequencenumber);
  void    EncodeEventData(std::vector<UInt_t> &buffer) override;
  void    SetSubElementPedestal(Int_t j, Double_t value) override;
  void    SetSubElementCalibrationFactor(Int_t j, Double_t value) override;

  void    Ratio(VQwBPM &numer, VQwBPM &denom) override;
  void    Ratio(QwBPMCavity &numer, QwBPMCavity &denom);
  void    Scale(Double_t factor) override;

  VQwBPM& operator=  (const VQwBPM &value) override;
  VQwBPM& operator+= (const VQwBPM &value) override;
  VQwBPM& operator-= (const VQwBPM &value) override;

  virtual QwBPMCavity& operator=  (const QwBPMCavity &value);
  virtual QwBPMCavity& operator+= (const QwBPMCavity &value);
  virtual QwBPMCavity& operator-= (const QwBPMCavity &value);

  void    AccumulateRunningSum(const VQwBPM &value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  void    AccumulateRunningSum(const QwBPMCavity &value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
  void    DeaccumulateRunningSum(VQwBPM &value, Int_t ErrorMask=0xFFFFFFF) override;
  void    DeaccumulateRunningSum(QwBPMCavity &value, Int_t ErrorMask=0xFFFFFFF);
  void    CalculateRunningAverage() override;

  void    ConstructHistograms(TDirectory *folder, TString &prefix) override;
  void    FillHistograms() override;

  void    ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) override;
  void    FillTreeVector(QwRootTreeBranchVector &values) const override;
  void    ConstructBranch(TTree *tree, TString &prefix) override;
  void    ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist) override;
#ifdef HAS_RNTUPLE_SUPPORT
  void    ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
  void    FillNTupleVector(std::vector<Double_t>& values) const override;
#endif

#ifdef __USE_DATABASE__
  std::vector<QwDBInterface> GetDBEntry() override;
  std::vector<QwErrDBInterface> GetErrDBEntry() override;
#endif

  protected:
  VQwHardwareChannel* GetSubelementByName(TString ch_name) override;

  /////
 private:
  /*  Position calibration factor, transform ADC counts in mm */
  static const Double_t kQwCavityCalibration;
  static const TString subelement[kNumElements];



 protected:
  std::array<QwVQWK_Channel,kNumElements> fElement;//(kNumElements);
  std::array<QwVQWK_Channel,kNumAxes> fRelPos;//(kNumAxes);

  //  These are the "real" data elements, to which the base class
  //  fAbsPos_base and fElement[kQElem]_base are pointers.
  std::array<QwVQWK_Channel,kNumAxes> fAbsPos;//(kNumAxes);


 private:
  // Functions to be removed
  void    MakeBPMCavityList();
  std::vector<QwVQWK_Channel> fBPMElementList;

};
