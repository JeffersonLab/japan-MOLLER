/**********************************************************\
* File: QwHelicityDecoder.h                               *
* Implemented the helicity data decoder                   *
* Contributor: Arindam Sen (asen@jlab.org)                *
* Time-stamp:                                             *
\**********************************************************/

#pragma once

// System headers
#include <vector>

// ROOT headers
#include "TTree.h"

// Qweak headers
#include "QwWord.h"
#include "QwHelicityBase.h"

// Forward declarations
#ifdef __USE_DATABASE__
class QwParityDB;
#endif

/*****************************************************************
*  Class:
******************************************************************/

class QwHelicityDecoder: public QwHelicityBase, public MQwSubsystemCloneable<QwHelicityDecoder> {

 private:
  QwHelicityDecoder();

 public:

  QwHelicityDecoder(const TString& name);
  QwHelicityDecoder(const QwHelicityDecoder& source);

  virtual ~QwHelicityDecoder() { }



  /* derived from VQwSubsystem */
  /// \brief Define options function

  static void DefineOptions(QwOptions &options);
  void ProcessOptions(QwOptions &options);
  Int_t LoadChannelMap(TString mapfile);
  Int_t LoadInputParameters(TString pedestalfile);
  Int_t LoadEventCuts(TString  filename);//Loads event cuts applicable to QwHelicityDecoder class, derived from VQwSubsystemParity
  Bool_t ApplySingleEventCuts();//Apply event cuts in the QwHelicityDecoder class, derived from VQwSubsystemParity

  Bool_t  CheckForBurpFail(const VQwSubsystem *ev_error){
    return kFALSE;
  };

  void UpdateErrorFlag(const VQwSubsystem *ev_error){
  };

  Int_t  ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id,
				   UInt_t* buffer, UInt_t num_words);
  Int_t  ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) {
    return ProcessEvBuffer(0x1,roc_id,bank_id,buffer,num_words);
  };
  Int_t  ProcessEvBuffer(UInt_t ev_type, const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words);

  void   EncodeEventData(std::vector<UInt_t> &buffer);


  virtual void  ClearEventData();
  virtual void  ProcessEvent();

  UInt_t GetRandomSeedActual() { return iseed_Actual; };
  UInt_t GetRandomSeedDelayed() { return iseed_Delayed; };

  void   PredictHelicity();
  void   RunPredictor();
  void   SetHelicityDelay(Int_t delay);
  void   SetHelicityBitPattern(TString hex);

  void SetFirstBits(UInt_t nbits, UInt_t firstbits);

  VQwSubsystem&  operator=  (VQwSubsystem *value);
  VQwSubsystem&  operator+=  (VQwSubsystem *value);

  VQwSubsystem& operator-= (VQwSubsystem *value) {return *this;};
  void  Ratio(VQwSubsystem *numer, VQwSubsystem *denom);

  void  AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
  //remove one entry from the running sums for devices
  void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF){
  };

  using VQwSubsystem::ConstructHistograms;
  void  ConstructHistograms(TDirectory *folder, TString &prefix);
  void  FillHistograms();

  using VQwSubsystem::ConstructBranchAndVector;
  void  ConstructBranchAndVector(TTree *tree, TString &prefix, std::vector<Double_t> &values);
  void  ConstructBranch(TTree *tree, TString &prefix);
  void  ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& trim_file);
  void  FillTreeVector(std::vector<Double_t> &values) const;

#ifdef __USE_DATABASE__
  void  FillDB(QwParityDB *db, TString type);
  void  FillErrDB(QwParityDB *db, TString datatype);
#endif // __USE_DATABASE__

  void  Print() const;

// protected:
  void CheckPatternNum(VQwSubsystem *value);
  void MergeCounters(VQwSubsystem *value);
  
  Bool_t CheckIORegisterMask(const UInt_t& ioregister, const UInt_t& mask) const {
    return ((mask != 0)&&((ioregister & mask) == mask));
  };

  static const std::vector<UInt_t> kDefaultHelicityBitPattern;

//  std::vector<UInt_t> fHelicityBitPattern;

//  std::vector <QwWord> fWord;
//  std::vector < std::pair<Int_t, Int_t> > fWordsPerSubbank;  // The indices of the first & last word in each subbank

  void SetHistoTreeSave(const TString &prefix);



  static const Bool_t kDEBUG=kFALSE;
  // local helicity is a special mode for encoding helicity info
  // it is not the fullblown helicity encoding we want to use for the main
  // data taking. For example this was used during the injector data taking
  // in winter 2008-09 injector tests
  static const Int_t kUndefinedHelicity= -9999;


  Bool_t Compare(VQwSubsystem *source);


//----------------------------------

  UInt_t  fSeed_Reported;
  Int_t   fNum_TStable_Fall;
  Int_t   fNum_Pair_Sync;
  Int_t   fTime_since_TStable;
  Int_t   fTime_since_TSettle;
  Int_t   fLast_Duration_TStable;
  Int_t   fLast_Duration_TSettle;
  Int_t   fEventPolarity;
  Int_t   fReportedPatternHel;
  Int_t   fBit_Helicity;
  Int_t   fBit_PairSync;
  Int_t   fBit_PatSync;
  Int_t   fBit_TStable;
  UInt_t  fEvtHistory_PatSync;
  UInt_t  fEvtHistory_PairSync;
  UInt_t  fEvtHistory_ReportedHelicity;
  UInt_t  fPatHistory_ReportedHelicity;

//----------------------------------

static const Int_t  fNumDecoderWords;
void   FillHDVariables(uint32_t data, uint32_t index);

};
//
// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwHelicityDecoder);
