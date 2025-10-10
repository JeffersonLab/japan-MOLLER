/**********************************************************\
* File: QwScaler_Channel.h                                 *
*                                                          *
* Author: J. Pan                                           *
* Date:   Thu Sep 16 18:08:33 CDT 2009                     *
\**********************************************************/

#ifndef __QWSCALER_CHANNEL__
#define __QWSCALER_CHANNEL__

// System headers
#include <vector>

// ROOT headers
#include "TTree.h"

// Qweak headers
#include "VQwHardwareChannel.h"
#include "MQwMockable.h"

// Forward declarations
class QwParameterFile;

///
/// \ingroup QwAnalysis_ADC
///
/// \ingroup QwAnalysis_BL
class VQwScaler_Channel: public VQwHardwareChannel, public MQwMockable {

public:
  static Int_t GetBufferOffset(Int_t scalerindex, Int_t wordindex, UInt_t header = 1);
  static void  PrintErrorCounterHead();
  static void  PrintErrorCounterTail();

  using MQwMockable::LoadMockDataParameters;

  using VQwHardwareChannel::GetRawValue;
  using VQwHardwareChannel::GetValue;
  using VQwHardwareChannel::GetValueM2;
  using VQwHardwareChannel::GetValueError;
  using VQwHardwareChannel::GetValueWidth;

  using VQwHardwareChannel::AccumulateRunningSum;
  using VQwHardwareChannel::DeaccumulateRunningSum;

public:
  VQwScaler_Channel(): MQwMockable() {
    InitializeChannel("","");
  }
    
  VQwScaler_Channel(TString name, TString datatosave = "raw"): MQwMockable() {
    InitializeChannel(name,datatosave);
  };
  VQwScaler_Channel(const VQwScaler_Channel& source)
    : VQwHardwareChannel(source),MQwMockable(source),
      fValue_Raw_Old(source.fValue_Raw_Old),
      fValue_Raw(source.fValue_Raw),
      fValue(source.fValue),
      fValueM2(source.fValueM2),
      fValueError(source.fValueError),
      // TODO:  Don't copy the pointer; we need to regenerate it somehow.
      //fNormChannelPtr(source.fNormChannelPtr);
      fClockNormalization(source.fClockNormalization),
      fNormChannelName(source.fNormChannelName),
      fNeedsExternalClock(source.fNeedsExternalClock),
      fIsDifferentialScaler(source.fIsDifferentialScaler)
  { }
  VQwScaler_Channel(const VQwScaler_Channel& source, VQwDataElement::EDataToSave datatosave)
    : VQwHardwareChannel(source,datatosave),MQwMockable(source),
      fValue_Raw_Old(source.fValue_Raw_Old),
      fValue_Raw(source.fValue_Raw),
      fValue(source.fValue),
      fValueM2(source.fValueM2),
      fValueError(source.fValueError),
      // TODO:  Don't copy the pointer; we need to regenerate it somehow.
      //fNormChannelPtr(source.fNormChannelPtr);
      fClockNormalization(source.fClockNormalization),
      fNormChannelName(source.fNormChannelName),
      fNeedsExternalClock(source.fNeedsExternalClock),
      fIsDifferentialScaler(source.fIsDifferentialScaler)
  { }
  ~VQwScaler_Channel() override { };

  void CopyFrom(const VQwScaler_Channel& value){
    VQwHardwareChannel::CopyFrom(value);
    fValue_Raw_Old = value.fValue_Raw_Old;
    fValue_Raw = value.fValue_Raw;
    fValue = value.fValue;
    fValueM2 = value.fValueM2;
    fValueError = value.fValueError;
    fClockNormalization = value.fClockNormalization;
    fNormChannelName = value.fNormChannelName;
    fNeedsExternalClock = value.fNeedsExternalClock;
    fIsDifferentialScaler = value.fIsDifferentialScaler;
  };


  /// \brief Initialize the fields in this object
  void  InitializeChannel(TString name, TString datatosave = "raw") override;

  /// \brief Initialize the fields in this object
  void  InitializeChannel(TString subsystem, TString instrumenttype, TString name, TString datatosave) override;

  void SetDefaultSampleSize(size_t /*NumberOfSamples_map*/) {
    //std::cerr << "QwScaler_Channel SetDefaultSampleSize does nothing!"
    //    << std::endl;
  }

  void LoadChannelParameters(QwParameterFile &paramfile) override;

  void  ClearEventData() override;

  void  RandomizeEventData(int helicity = 0, double time = 0.0) override;
  void  SetEventData(Double_t value);
  void  SetRawEventData() override{
     //fValue     = fCalibrationFactor * (Double_t(fValue_Raw) - Double_t(fValue_Raw_Old) - fPedestal);

     fValue_Raw = Int_t(fValue/fCalibrationFactor + fPedestal) + fValue_Raw_Old;
     if (IsDifferentialScaler())
       fValue_Raw_Old = fValue_Raw;
     else
      fValue_Raw_Old = 0;

};


  void  EncodeEventData(std::vector<UInt_t> &buffer) override = 0;
  Int_t ProcessEvBuffer(UInt_t* buffer, UInt_t num_words_left, UInt_t index = 0) override = 0;

//-----------------------------------------------------------------------------------------------
  void SmearByResolution(double resolution) override;
//-----------------------------------------------------------------------------------------------

  void  ProcessEvent() override;

  Int_t GetRawValue(size_t /*element*/) const override      { return fValue_Raw; };
  Double_t GetValue(size_t /*element*/) const override      { return fValue; };
  Double_t GetValueM2(size_t /*element*/) const override    { return fValueM2; };
  Double_t GetValueError(size_t /*element*/) const override { return fValueError; };

  VQwScaler_Channel& operator=  (const VQwScaler_Channel &value);
  void AssignScaledValue(const VQwScaler_Channel &value, Double_t scale);
  void AssignValueFrom(const VQwDataElement* valueptr) override;
  void AddValueFrom(const VQwHardwareChannel* valueptr) override;
  void SubtractValueFrom(const VQwHardwareChannel* valueptr) override;
  void MultiplyBy(const VQwHardwareChannel* valueptr) override;
  void DivideBy(const VQwHardwareChannel* valueptr) override;

  VQwScaler_Channel& operator+= (const VQwScaler_Channel &value);
  VQwScaler_Channel& operator-= (const VQwScaler_Channel &value);
  VQwScaler_Channel& operator*= (const VQwScaler_Channel &value);

  VQwHardwareChannel& operator+=(const VQwHardwareChannel& input) override;
  VQwHardwareChannel& operator-=(const VQwHardwareChannel& input) override;
  VQwHardwareChannel& operator*=(const VQwHardwareChannel& input) override;
  VQwHardwareChannel& operator/=(const VQwHardwareChannel& input) override;

  void Sum(VQwScaler_Channel &value1, VQwScaler_Channel &value2);
  void Difference(VQwScaler_Channel &value1, VQwScaler_Channel &value2);
  void Ratio(const VQwScaler_Channel &numer, const VQwScaler_Channel &denom);
  void Product(VQwScaler_Channel &numer, VQwScaler_Channel &denom);
  void AddChannelOffset(Double_t Offset);
  void Scale(Double_t Offset) override;
  void DivideBy(const VQwScaler_Channel &denom);
  

  Int_t ApplyHWChecks() override; //Check for harware errors in the devices. This will return the device error code.

  Bool_t ApplySingleEventCuts() override;//check values read from modules are at desired level

  Bool_t CheckForBurpFail(const VQwDataElement * /*ev_error*/) {return kFALSE;};

  void IncrementErrorCounters() override;

  /// report number of events failed due to HW and event cut failure
  void PrintErrorCounters() const override;

//   UInt_t GetDeviceErrorCode(){//return the device error code
//     return fDeviceErrorCode;
//   };

  void  ConstructHistograms(TDirectory *folder, TString &prefix) override;
  void  FillHistograms() override;

  void  ConstructBranchAndVector(TTree *tree, TString &prefix, std::vector<Double_t> &values) override = 0;
  void  FillTreeVector(std::vector<Double_t> &values) const override = 0;
  void  ConstructBranch(TTree *tree, TString &prefix) override;
#ifdef HAS_RNTUPLE_SUPPORT
  void  ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override = 0;
  void  FillNTupleVector(std::vector<Double_t>& values) const override = 0;
#endif // HAS_RNTUPLE_SUPPORT


  void AccumulateRunningSum(const VQwScaler_Channel &value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
  void AccumulateRunningSum(const VQwHardwareChannel *value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override{
    const VQwScaler_Channel *tmp_ptr = dynamic_cast<const VQwScaler_Channel*>(value);
    if (tmp_ptr != NULL) AccumulateRunningSum(*tmp_ptr, count, ErrorMask);
  };
  inline void DeaccumulateRunningSum(const VQwScaler_Channel& value, Int_t ErrorMask){
    AccumulateRunningSum(value, -1, ErrorMask);
  };
  
  void PrintValue() const override;
  void PrintInfo() const override;
  void CalculateRunningAverage() override;

  // These are related to those hardware channels that need to normalize
  // to an external clock
  Bool_t NeedsExternalClock() override { return fNeedsExternalClock; };
  void SetNeedsExternalClock(Bool_t needed) override { fNeedsExternalClock = needed; };
  std::string GetExternalClockName() override {  return fNormChannelName; };
  void SetExternalClockPtr( const VQwHardwareChannel* clock) override { fNormChannelPtr = clock; };
  void SetExternalClockName( const std::string name) override { fNormChannelName = name; };

  // Differential scalers automatically subtract the previous value
  virtual Bool_t IsDifferentialScaler() { return fIsDifferentialScaler; };
  virtual void SetDifferentialScaler(Bool_t diff) { fIsDifferentialScaler = diff; };

  void ScaledAdd(Double_t scale, const VQwHardwareChannel *value) override;

protected:
  VQwScaler_Channel& operator/=(const VQwScaler_Channel&);
  
protected:
  static const Bool_t kDEBUG;

  UInt_t   fHeader;
  UInt_t   fValue_Raw_Old;
  UInt_t   fValue_Raw;
  Double_t fValue;
  Double_t fValueM2;
  Double_t fValueError;
  const VQwHardwareChannel *fNormChannelPtr;
  Double_t fClockNormalization;
  std::string fNormChannelName;

  Bool_t fNeedsExternalClock;
  Bool_t fIsDifferentialScaler;


  Int_t fNumEvtsWithHWErrors;//counts the HW failed events
  Int_t fNumEvtsWithEventCutsRejected;////counts the Event cut rejected events
};


//  Derived templated class
template <UInt_t data_mask=0xffffffff, UInt_t data_shift=0 >
class QwScaler_Channel: public VQwScaler_Channel
{
  public:

  // Define the constructors (cascade)
  QwScaler_Channel(): VQwScaler_Channel() { };
  QwScaler_Channel(const QwScaler_Channel& source)
    : VQwScaler_Channel(source) { };
  QwScaler_Channel(TString name, TString datatosave = "raw")
    : VQwScaler_Channel(name,datatosave) { };
  QwScaler_Channel(const QwScaler_Channel& source, VQwDataElement::EDataToSave datatosave)
    : VQwScaler_Channel(source,datatosave) { };

  using VQwScaler_Channel::CopyFrom;
  using VQwHardwareChannel::Clone;
  VQwHardwareChannel* Clone(VQwDataElement::EDataToSave datatosave) const override;

  public:

  // Implement the templated methods
  void  EncodeEventData(std::vector<UInt_t> &buffer) override;
  Int_t ProcessEvBuffer(UInt_t* buffer, UInt_t num_words_left, UInt_t index = 0) override;

  void  ConstructBranchAndVector(TTree *tree, TString &prefix, std::vector<Double_t> &values) override;
  void  FillTreeVector(std::vector<Double_t> &values) const override;
#ifdef HAS_RNTUPLE_SUPPORT
  void  ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
  void  FillNTupleVector(std::vector<Double_t>& values) const override;
#endif // HAS_RNTUPLE_SUPPORT


};

//  These typedef's should be the last things in the file.
//  Class template instationation must be made in the source
//  file for anything defined here.
typedef class QwScaler_Channel<0x00ffffff,0> QwSIS3801D24_Channel;
typedef class QwScaler_Channel<0xffffffff,0> QwSIS3801D32_Channel;
typedef class QwScaler_Channel<0xffffffff,0>    QwSIS3801_Channel;
typedef class QwScaler_Channel<0xffffffff,0>    QwSTR7200_Channel;


#endif
