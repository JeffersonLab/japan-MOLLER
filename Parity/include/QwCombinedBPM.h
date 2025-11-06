/*!
 * \file   QwCombinedBPM.h
 * \brief  Combined beam position monitor using weighted average
 * \author B. Waidyawansa
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
#include "VQwHardwareChannel.h"
#include "VQwBPM.h"
#include "QwUtil.h"

// Forward declarations
#ifdef __USE_DATABASE__
class QwDBInterface;
#endif // __USE_DATABASE__
class QwParameterFile;

/**
 * \class QwCombinedBPM
 * \ingroup QwAnalysis_BL
 * \brief Template for combined beam position monitor using multiple BPMs
 *
 * Maintains a weighted combination of individual BPMs to estimate
 * position and angle at a virtual location, supporting fits and
 * effective charge computation.
 */
template<typename T>
class QwCombinedBPM : public VQwBPM {
  friend class QwEnergyCalculator;

  /////
 public:

//-------------------------------------------------------------------------
  size_t GetNumberOfElements() override {return fElement.size();};
  TString GetSubElementName(Int_t index) override {return fElement.at(index)->GetElementName();};
//-------------------------------------------------------------------------

  QwCombinedBPM(){
  };
  QwCombinedBPM(TString name):VQwBPM(name){
    InitializeChannel(name);
  };
  QwCombinedBPM(TString subsystem, TString name): VQwBPM(name){
    InitializeChannel(subsystem, name);
  };

  QwCombinedBPM(TString subsystem, TString name, TString type): VQwBPM(name){
    InitializeChannel(subsystem, name,type);
  };
  QwCombinedBPM(const QwCombinedBPM& source)
  : VQwBPM(source),
    fEffectiveCharge(source.fEffectiveCharge)
  {
    QwCopyArray(source.fSlope, fSlope);
    QwCopyArray(source.fIntercept, fIntercept);
    QwCopyArray(source.fMinimumChiSquare, fMinimumChiSquare);
    QwCopyArray(source.fAbsPos, fAbsPos);
  }
  ~QwCombinedBPM() override { };

  using VQwBPM::EBeamPositionMonitorAxis;

  void    InitializeChannel(TString name);
  // new routine added to update necessary information for tree trimming
  void    InitializeChannel(TString subsystem, TString name);
  void    InitializeChannel(TString subsystem, TString name, TString type) {
    SetModuleType(type);
    InitializeChannel(subsystem, name);
  }

  void    LoadChannelParameters(QwParameterFile &paramfile) override{};
  void    ClearEventData() override;
  Int_t   ProcessEvBuffer(UInt_t* buffer,
			UInt_t word_position_in_buffer,UInt_t indexnumber) override;
  void    ProcessEvent() override;
  void    PrintValue() const override;
  void    PrintInfo() const override;

  const VQwHardwareChannel* GetPosition(EBeamPositionMonitorAxis axis) const override {
    if (axis<0 || axis>2){
      TString loc="QwLinearDiodeArray::GetPosition for "
        +this->GetElementName()+" failed for axis value "+Form("%d",axis);
      throw std::out_of_range(loc.Data());
    }
    return &fAbsPos[axis];
  }

  const VQwHardwareChannel* GetSlope(EBeamPositionMonitorAxis axis) const{
    if (axis<0 || axis>2){
      TString loc="QwLinearDiodeArray::GetPosition for "
        +this->GetElementName()+" failed for axis value "+Form("%d",axis);
      throw std::out_of_range(loc.Data());
    }
    return &fSlope[axis];
  }
  const VQwHardwareChannel* GetEffectiveCharge() const override {
    return &fEffectiveCharge;
  }


  Bool_t  ApplyHWChecks();//Check for hardware errors in the devices
  Bool_t  ApplySingleEventCuts() override;//Check for good events by setting limits on the devices readings
  //void    SetSingleEventCuts(TString ch_name, Double_t minX, Double_t maxX);
  /*! \brief Inherited from VQwDataElement to set the upper and lower limits (fULimit and fLLimit), stability % and the error flag on this channel */
  //void    SetSingleEventCuts(TString ch_name, UInt_t errorflag,Double_t min, Double_t max, Double_t stability);
  void    SetEventCutMode(Int_t bcuts) override;
  void IncrementErrorCounters() override;
  void PrintErrorCounters() const override;// report number of events failed due to HW and event cut failure
  UInt_t  GetEventcutErrorFlag() override;
  UInt_t  UpdateErrorFlag() override;
  void UpdateErrorFlag(const VQwBPM *ev_error) override;

  // Polymorphic burp-failure check when called via VQwBPM*
  Bool_t  CheckForBurpFail(const VQwDataElement *ev_error);


  void    SetBPMForCombo(const VQwBPM* bpm, Double_t charge_weight,  Double_t x_weight, Double_t y_weight,Double_t sumqw) override;

  void    Ratio(QwCombinedBPM &numer, QwCombinedBPM &denom);
  void    Ratio(VQwBPM &numer, VQwBPM &denom) override;
  void    Scale(Double_t factor) override;

  VQwBPM& operator=  (const VQwBPM &value) override;
  VQwBPM& operator+= (const VQwBPM &value) override;
  VQwBPM& operator-= (const VQwBPM &value) override;

  virtual QwCombinedBPM& operator=  (const QwCombinedBPM &value);
  virtual QwCombinedBPM& operator+= (const QwCombinedBPM &value);
  virtual QwCombinedBPM& operator-= (const QwCombinedBPM &value);

  void    AccumulateRunningSum(const VQwBPM& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  void    AccumulateRunningSum(const QwCombinedBPM& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
  void    DeaccumulateRunningSum(VQwBPM& value, Int_t ErrorMask=0xFFFFFFF) override;
  void    DeaccumulateRunningSum(QwCombinedBPM& value, Int_t ErrorMask=0xFFFFFFF);
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
#endif // HAS_RNTUPLE_SUPPORT

//------------------------------------------------------------------------------------------------
  void    RandomizeEventData(int helicity = 0, double time = 0.0) override;
  void    SetRandomEventParameters(Double_t meanX, Double_t sigmaX, Double_t meanY, Double_t sigmaY,
                                   Double_t meanXslope, Double_t sigmaXslope, Double_t meanYslope, Double_t sigmaYslope) override;
  void    GetProjectedPosition(VQwBPM *device) override;
  void    LoadMockDataParameters(QwParameterFile &paramfile) override;
//------------------------------------------------------------------------------------------------

  VQwHardwareChannel* GetAngleX(){ //At present this returns the slope not the angle
    return &fSlope[0];
  };

  const VQwHardwareChannel* GetAngleX() const override { //At present this returns the slope not the angle
    return const_cast<QwCombinedBPM*>(this)->GetAngleX();
  };

  VQwHardwareChannel* GetAngleY(){//At present this returns the slope not the angle
    return &fSlope[1];
  };

  const VQwHardwareChannel* GetAngleY() const override { //At present this returns the slope not the angle
    return const_cast<QwCombinedBPM*>(this)->GetAngleY();
  };


#ifdef __USE_DATABASE__
  std::vector<QwDBInterface> GetDBEntry() override;
  std::vector<QwErrDBInterface> GetErrDBEntry() override;
#endif // __USE_DATABASE__

 protected:
  VQwHardwareChannel* GetSubelementByName(TString ch_name) override;

  /* Functions for least square fit */
  void     CalculateFixedParameter(std::vector<Double_t> fWeights, Int_t pos);
  Double_t SumOver( std::vector <Double_t> weight , std::vector <T> val);
  void     LeastSquareFit(VQwBPM::EBeamPositionMonitorAxis axis, std::vector<Double_t> fWeights) ; //bbbbb



  /////
 private:
  Bool_t fixedParamCalculated;

  //used for least squares fit
  Double_t erra[2],errb[2],covab[2];
  Double_t A[2], B[2], D[2], m[2];
  Double_t chi_square[2];
  Double_t fSumQweights;

  std::vector <const VQwBPM*> fElement;
  std::vector <Double_t> fQWeights;
  std::vector <Double_t> fXWeights;
  std::vector <Double_t> fYWeights;


 protected:
  /* This channel contains the beam slope w.r.t the X & Y axis at the target */
  std::array<T,2> fSlope;//[2];

  /* This channel contains the beam intercept w.r.t the X & Y axis at the target */
  std::array<T,2> fIntercept;//[2];

  /*This channel gives the minimum chisquare value for the fit over target bpms*/
  std::array<T,2> fMinimumChiSquare;//[2];

  //  These are the "real" data elements, to which the base class
  //  fAbsPos_base and fEffectiveCharge_base are pointers.
  std::array<T,2> fAbsPos;//[2];
  T fEffectiveCharge;

private:
  // Functions to be removed
  void    MakeBPMComboList();
  std::vector<T> fBPMComboElementList;

};
