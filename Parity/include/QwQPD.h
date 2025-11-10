/*!
 * \file   QwQPD.h
 * \brief  Quadrant photodiode beam position monitor implementation
 *
 * \author B.Waidyawansa
 * \date   09-14-2010
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
#include "QwUtil.h"

// Forward declarations
#ifdef __USE_DATABASE__
class QwDBInterface;
class QwErrDBInterface;
#endif // __USE_DATABASE__

/*****************************************************************
*  Class:
******************************************************************/
/**
 * \class QwQPD
 * \ingroup QwAnalysis_BL
 * \brief Quadrant photodiode BPM computing X/Y and effective charge
 */
class QwQPD : public VQwBPM {

 public:
  static UInt_t  GetSubElementIndex(TString subname);

  QwQPD() {
  };
  QwQPD(TString name):VQwBPM(name){
    InitializeChannel(name);
  };
  QwQPD(TString subsystemname, TString name):VQwBPM(name){
    SetSubsystemName(subsystemname);
    InitializeChannel(subsystemname, name);
    fQwQPDCalibration[0] = 1.0;
    fQwQPDCalibration[1] = 1.0;
  };
  QwQPD(const QwQPD& source)
  : VQwBPM(source),
    fEffectiveCharge(source.fEffectiveCharge)
  {
    QwCopyArray(source.fPhotodiode, fPhotodiode);
    QwCopyArray(source.fRelPos, fRelPos);
    QwCopyArray(source.fAbsPos, fAbsPos);
  }
  ~QwQPD() override { };

  void    InitializeChannel(TString name);
  // new routine added to update necessary information for tree trimming
  void    InitializeChannel(TString subsystem, TString name);

  void LoadChannelParameters(QwParameterFile &paramfile) override{
    for(Short_t i=0;i<4;i++)
      fPhotodiode[i].LoadChannelParameters(paramfile);
  }

  void    GetCalibrationFactors(Double_t AlphaX, Double_t AlphaY);

  void    ClearEventData() override;
  Int_t   ProcessEvBuffer(UInt_t* buffer,
			UInt_t word_position_in_buffer,UInt_t indexnumber) override;
  void    ProcessEvent() override;

  const VQwHardwareChannel* GetPosition(EBeamPositionMonitorAxis axis) const override {
    if (axis<0 || axis>2){
      TString loc="QwQPD::GetPosition for "
        +this->GetElementName()+" failed for axis value "+Form("%d",axis);
      throw std::out_of_range(loc.Data());
    }
    return &fAbsPos[axis];
  }
  const VQwHardwareChannel* GetEffectiveCharge() const override {return &fEffectiveCharge;}

  TString GetSubElementName(Int_t subindex) override;
  void    GetAbsolutePosition() override{};

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

  void    Ratio(VQwBPM &numer, VQwBPM &denum) override;
  void    Ratio(QwQPD &numer, QwQPD &denom);
  void    Scale(Double_t factor) override;

  VQwBPM& operator=  (const VQwBPM &value) override;
  VQwBPM& operator+= (const VQwBPM &value) override;
  VQwBPM& operator-= (const VQwBPM &value) override;

  virtual QwQPD& operator=  (const QwQPD &value);
  virtual QwQPD& operator+= (const QwQPD &value);
  virtual QwQPD& operator-= (const QwQPD &value);

  void    AccumulateRunningSum(const QwQPD& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
  void    AccumulateRunningSum(const VQwBPM& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  void    DeaccumulateRunningSum(VQwBPM &value, Int_t ErrorMask=0xFFFFFFF) override;
  void    DeaccumulateRunningSum(QwQPD& value, Int_t ErrorMask=0xFFFFFFF);
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
#endif // __USE_DATABASE__

  void    MakeQPDList();


  protected:
  VQwHardwareChannel* GetSubelementByName(TString ch_name) override;


  /////
 private:


  static const TString subelement[4];
  /*  Position calibration factor, transform ADC counts in mm */
  Double_t fQwQPDCalibration[2];


 protected:
  std::array<QwVQWK_Channel,4> fPhotodiode;//[4];
  std::array<QwVQWK_Channel,2> fRelPos;//[2];

  //  These are the "real" data elements, to which the base class
  //  fAbsPos_base and fEffectiveCharge_base are pointers.
  std::array<QwVQWK_Channel,2> fAbsPos;//[2];
  QwVQWK_Channel fEffectiveCharge;

  std::vector<QwVQWK_Channel> fQPDElementList;

};
