/**********************************************************\
* File: QwIntegratedRaster.h                              *
*                                                         *
* Author:                                                 *
* Time-stamp:                                             *
\**********************************************************/

#ifndef __QwIntegratedRaster__
#define __QwIntegratedRaster__

// System headers
#include <vector>

// ROOT headers
#include "TTree.h"
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RField.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "VQwSubsystemParity.h"

#include "QwIntegratedRasterChannel.h"
#include "QwTypes.h"


class QwIntegratedRasterDetectorID{

 public:

  QwIntegratedRasterDetectorID():fSubbankIndex(-1),fWordInSubbank(-1),
    fIndex(-1),
    fSubelement(999999),fmoduletype(""),fdetectorname("")
    {};

  int fSubbankIndex;
  int fWordInSubbank; //first word reported for this channel in the subbank
                      //(eg VQWK channel report 6 words for each event, scalers oly report one word per event)
  // The first word of the subbank gets fWordInSubbank=0

  //  EQwInstrumentType fTypeID;     // type of detector
  int fIndex;      // index of this detector in the vector containing all the detector of same type
  UInt_t fSubelement; // some detectors have many subelements (eg stripline have 4 antenas) some have only one sub element(eg lumis have one channel)

  TString fmoduletype; // eg: VQWK, SCALER
  TString fdetectorname;
  TString fdetectortype; // IntegratedRasterChannel,fLumiCounter .. this string is encoded by fTypeID
  //  std::vector<TString> fCombinedChannelNames;
  //  std::vector<Double_t> fWeight;
  void Print() const;

};


/*****************************************************************
*  Class:
******************************************************************/
class QwIntegratedRaster : public VQwSubsystemParity, public MQwSubsystemCloneable<QwIntegratedRaster> {
  /////  

 private:
  /// Private default constructor (not implemented, will throw linker error on use)
  QwIntegratedRaster();

 public:
  /// Constructor with name
  QwIntegratedRaster(const TString& name)
  : VQwSubsystem(name),VQwSubsystemParity(name)
  { };
  /// Copy constructor
  QwIntegratedRaster(const QwIntegratedRaster& source)
  : VQwSubsystem(source),VQwSubsystemParity(source),
    fIntegratedRasterChannel(source.fIntegratedRasterChannel)
  { }
  /// Virtual destructor
  ~QwIntegratedRaster() override { };


  /* derived from VQwSubsystem */

  /// \brief Define options function
  static void DefineOptions(QwOptions &options);


  void ProcessOptions(QwOptions &options) override;//Handle command line options
  Int_t LoadChannelMap(TString mapfile) override;
  Int_t LoadInputParameters(TString pedestalfile) override;
  Int_t LoadEventCuts(TString filename) override;//derived from VQwSubsystemParity
  void IncrementErrorCounters() override;
  Bool_t ApplySingleEventCuts() override;//derived from VQwSubsystemParity
  void PrintErrorCounters() const override;// report number of events failed due to HW and event cut faliures
  UInt_t GetEventcutErrorFlag() override;//return the error flag

  Bool_t CheckForBurpFail(const VQwSubsystem *subsys) override{
    return kFALSE;
  };

  //update the error flag in the subsystem level from the top level routines related to stability checks. This will uniquely update the errorflag at each channel based on the error flag in the corresponding channel in the ev_error subsystem
  void UpdateErrorFlag(const VQwSubsystem *ev_error) override{
  };

  void AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  //remove one entry from the running sums for devices
  void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF) override{
  };
  void CalculateRunningAverage() override;

  Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
  Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
  void  PrintDetectorID() const;

  void  ClearEventData() override;
  Bool_t IsGoodEvent();

  void  ProcessEvent() override;
  void  ExchangeProcessedData() override{};
  void  ProcessEvent_2() override{};

  void  SetRandomEventParameters(Double_t mean, Double_t sigma);
  void  SetRandomEventAsymmetry(Double_t asymmetry);
  void RandomizeEventData(int helicity = 0, double time = 0.0) override;
  void EncodeEventData(std::vector<UInt_t> &buffer) override;

  VQwSubsystem&  operator=  (VQwSubsystem *value) override;
  VQwSubsystem&  operator+= (VQwSubsystem *value) override;
  VQwSubsystem&  operator-= (VQwSubsystem *value) override;
  void Sum(VQwSubsystem  *value1, VQwSubsystem  *value2) override;
  void Difference(VQwSubsystem  *value1, VQwSubsystem  *value2) override;
  void Ratio(VQwSubsystem *numer, VQwSubsystem *denom) override;
  void Scale(Double_t factor) override;

  using VQwSubsystem::ConstructHistograms;
  void  ConstructHistograms(TDirectory *folder, TString &prefix) override;
  void  FillHistograms() override;

  void  ConstructBranchAndVector(TTree *tree, TString &prefix, std::vector<Double_t> &values) override;
  void  ConstructBranch(TTree *tree, TString &prefix) override;
  void  ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& trim_file) override;
  void  FillTreeVector(std::vector<Double_t> &values) const override;

#ifdef HAS_RNTUPLE_SUPPORT
  // RNTuple methods  
  void  ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
  void  FillNTupleVector(std::vector<Double_t>& values) const override;
#endif
#ifdef __USE_DATABASE__
  void  FillDB(QwParityDB *db, TString datatype);
  void  FillErrDB(QwParityDB *db, TString datatype);
#endif // __USE_DATABASE__
  void  WritePromptSummary(QwPromptSummary *ps, TString type) override;

  const VQwDataElement* GetChannel(const TString name) const;

  Bool_t Compare(VQwSubsystem *source);

  void PrintValue() const override;
  void PrintInfo() const override;

/////
protected: 
 
  // EQwPMTInstrumentType GetDetectorTypeID(TString name);
 
  Int_t GetDetectorIndex(TString name);

  std::vector <QwIntegratedRasterChannel<QwVQWK_Channel> >      fIntegratedRasterChannel;  
  std::vector <QwIntegratedRasterDetectorID>   fDetectorIDs;

protected:

/////
private:
  Int_t fQwIntegratedRasterErrorCount;

  static const Bool_t bDEBUG=kFALSE;

};




#endif
