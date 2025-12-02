/*!
 * \file   QwLinearDiodeArray.h
 * \brief  Linear diode array beam position monitor implementation
 * \author B.Waidyawansa
 * \date   2010-09-14
 */

#pragma once

// System headers
#include <vector>

// ROOT headers
#include <TTree.h>
#include <ROOT/RNTupleModel.hxx>

// Qweak headers
#include "QwVQWK_Channel.h"
#include "VQwBPM.h"
#include "QwParameterFile.h"

// Forward declarations
class QwDBInterface;
class QwErrDBInterface;

/**
 * \class QwLinearDiodeArray
 * \ingroup QwAnalysis_BL
 * \brief Linear diode array beam position monitor implementation
 *
 * Implements beam position monitoring using a linear array of photodiodes.
 * Provides position calculation from diode array readouts with calibration
 * and error handling for linear array detectors.
 */
class QwLinearDiodeArray : public VQwBPM {

 public:
  static UInt_t  GetSubElementIndex(TString subname);

  QwLinearDiodeArray() {
  };
  QwLinearDiodeArray(TString name):VQwBPM(name){
  };
  QwLinearDiodeArray(TString subsystemname, TString name):VQwBPM(name){
    SetSubsystemName(subsystemname);
    InitializeChannel(subsystemname, name);
  };
  QwLinearDiodeArray(const QwLinearDiodeArray& source)
  : VQwBPM(source),
    fEffectiveCharge(source.fEffectiveCharge)
  {
    for (size_t i = 0; i < 2; i++) {
      fRelPos[i] = source.fRelPos[i];
      fAbsPos[i] = source.fAbsPos[i];
    }
    for (size_t i = 0; i < 8; i++) {
      fPhotodiode[i] = source.fPhotodiode[i];
    }
  }
  ~QwLinearDiodeArray() override { };

  void    InitializeChannel(TString name);
  // new routine added to update necessary information for tree trimming
  void    InitializeChannel(TString subsystem, TString name);
  void    ClearEventData() override;

  void LoadChannelParameters(QwParameterFile &paramfile) override{
    for(size_t i=0;i<kMaxElements;i++)
      fPhotodiode[i].LoadChannelParameters(paramfile);
  }

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
  const VQwHardwareChannel* GetEffectiveCharge() const override {return &fEffectiveCharge;}

  TString GetSubElementName(Int_t subindex) override;
  UInt_t  SetSubElementName(TString subname);
  void    GetAbsolutePosition() override;

  Bool_t  ApplyHWChecks();//Check for hardware errors in the devices
  Bool_t  ApplySingleEventCuts() override;//Check for good events by setting limits on the devices readings
  //void    SetSingleEventCuts(TString ch_name, Double_t minX, Double_t maxX);
  /*! \brief Inherited from VQwDataElement to set the upper and lower limits (fULimit and fLLimit), stability % and the error flag on this channel */
  void    SetSingleEventCuts(TString ch_name, UInt_t errorflag,Double_t minX, Double_t maxX, Double_t stability, Double_t burplevel);
  void    SetEventCutMode(Int_t bcuts) override;
  void IncrementErrorCounters() override;
  void PrintErrorCounters() const override;// report number of events failed due to HW and event cut failure
  UInt_t GetEventcutErrorFlag() override;
  UInt_t UpdateErrorFlag() override;
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
  void    Ratio(QwLinearDiodeArray &numer, QwLinearDiodeArray &denom);
  void    Scale(Double_t factor) override;


  VQwBPM& operator=  (const VQwBPM &value) override;
  VQwBPM& operator+= (const VQwBPM &value) override;
  VQwBPM& operator-= (const VQwBPM &value) override;

  virtual QwLinearDiodeArray& operator=  (const QwLinearDiodeArray &value);
  virtual QwLinearDiodeArray& operator+= (const QwLinearDiodeArray &value);
  virtual QwLinearDiodeArray& operator-= (const QwLinearDiodeArray &value);

  void    AccumulateRunningSum(const QwLinearDiodeArray& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
  void    AccumulateRunningSum(const VQwBPM& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  void    DeaccumulateRunningSum(QwLinearDiodeArray& value, Int_t ErrorMask=0xFFFFFFF);
  void    DeaccumulateRunningSum(VQwBPM& value, Int_t ErrorMask=0xFFFFFFF) override;
  void    CalculateRunningAverage() override;

  void    ConstructHistograms(TDirectory *folder, TString &prefix) override;
  void    FillHistograms() override;

  void    ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) override;
  void    ConstructBranch(TTree *tree, TString &prefix) override;
  void    ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist) override;
  void    FillTreeVector(QwRootTreeBranchVector &values) const override;
#ifdef HAS_RNTUPLE_SUPPORT
  void    ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
  void    FillNTupleVector(std::vector<Double_t>& values) const override;
#endif


#ifdef __USE_DATABASE__
  std::vector<QwDBInterface> GetDBEntry() override;
  std::vector<QwErrDBInterface> GetErrDBEntry() override;
#endif

  void    MakeLinearArrayList();

  protected:
  VQwHardwareChannel* GetSubelementByName(TString ch_name) override;

  /////
 private:
  static const size_t kMaxElements;
  static const TString subelement[8];

  /*  Position calibration factor, transform ADC counts in mm */
  static const Double_t kQwLinearDiodeArrayPadSize;



 protected:
  // std::vector<QwVQWK_Channel> fPhotodiode;
  QwVQWK_Channel fPhotodiode[8];
  QwVQWK_Channel fRelPos[2];

  //  These are the "real" data elements, to which the base class
  //  fAbsPos_base and fEffectiveCharge_base are pointers.
  QwVQWK_Channel fAbsPos[2];
  QwVQWK_Channel fEffectiveCharge;

  std::vector<QwVQWK_Channel> fLinearArrayElementList;

};
