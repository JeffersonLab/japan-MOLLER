/**********************************************************\
* File: QwHelicity.h                                      *
*                                                         *
* Author:                                                 *
* Time-stamp:                                             *
\**********************************************************/

#ifndef __QwHELICITY__
#define __QwHELICITY__

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

class QwHelicity: public QwHelicityBase, public MQwSubsystemCloneable<QwHelicity> {

 private:
  QwHelicity();

 public:

  QwHelicity(const TString& name);
  QwHelicity(const QwHelicity& source);

  virtual ~QwHelicity() { }

  static void DefineOptions(QwOptions &options);
  void ProcessOptions(QwOptions &options);
  Int_t LoadChannelMap(TString mapfile);
  Int_t LoadInputParameters(TString pedestalfile);
  Int_t LoadEventCuts(TString  filename);//Loads event cuts applicable to QwHelicity class, derived from VQwSubsystemParity
  Bool_t ApplySingleEventCuts();//Apply event cuts in the QwHelicity class, derived from VQwSubsystemParity

  Bool_t  CheckForBurpFail(const VQwSubsystem *ev_error){
    return kFALSE;
  };

  void UpdateErrorFlag(const VQwSubsystem *ev_error){ };

  Int_t  ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id,
				   UInt_t* buffer, UInt_t num_words);
  Int_t  ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) {
    return ProcessEvBuffer(0x1,roc_id,bank_id,buffer,num_words);
  };
  Int_t  ProcessEvBuffer(UInt_t ev_type, const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words);
  void   ProcessEventUserbitMode();//ProcessEvent has two modes Userbit and Inputregister modes
  void   ProcessEventInputRegisterMode();
  void   ProcessEventInputMollerMode();

  void   EncodeEventData(std::vector<UInt_t> &buffer);


  virtual void  ClearEventData();
  virtual void  ProcessEvent();

  UInt_t GetRandomSeedActual() { return iseed_Actual; };
  UInt_t GetRandomSeedDelayed() { return iseed_Delayed; };

  void   PredictHelicity();
  void   SetHelicityDelay(Int_t delay);
  void   SetHelicityBitPattern(TString hex);

  Int_t  GetHelicityReported();
  Int_t  GetHelicityActual();
  Int_t  GetHelicityDelayed();
  Long_t GetEventNumber();
  Long_t GetPatternNumber();
  Int_t  GetPhaseNumber();
  Int_t GetMaxPatternPhase(){
    return fMaxPatternPhase;
  };
  Int_t GetMinPatternPhase(){
    return fMinPatternPhase;
  }
  void SetFirstBits(UInt_t nbits, UInt_t firstbits);
  void SetEventPatternPhase(Int_t event, Int_t pattern, Int_t phase);

  VQwSubsystem&  operator=  (VQwSubsystem *value);
  VQwSubsystem&  operator+=  (VQwSubsystem *value);

  //the following functions do nothing really : adding and subtracting helicity doesn't mean anything
  VQwSubsystem& operator-= (VQwSubsystem *value) {return *this;};
  void  Ratio(VQwSubsystem *numer, VQwSubsystem *denom);
  void  AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
  //remove one entry from the running sums for devices
  void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF){
  };

  using VQwSubsystem::ConstructHistograms;
  void  ConstructHistograms(TDirectory *folder, TString &prefix);

  using VQwSubsystem::ConstructBranchAndVector;

#ifdef __USE_DATABASE__
  void  FillDB(QwParityDB *db, TString type);
  void  FillErrDB(QwParityDB *db, TString datatype);
#endif // __USE_DATABASE__

  void  Print() const;

  Bool_t IsHelicityIgnored(){return fIgnoreHelicity;};

  virtual Bool_t IsGoodHelicity();

 protected:
  void CheckPatternNum(VQwSubsystem *value);
  void MergeCounters(VQwSubsystem *value);
  
  Bool_t CheckIORegisterMask(const UInt_t& ioregister, const UInt_t& mask) const {
    return ((mask != 0)&&((ioregister & mask) == mask));
  };

  enum HelicityEncodingType{kHelUserbitMode=0,
			    kHelInputRegisterMode,
			    kHelLocalyMadeUp,
			    kHelInputMollerMode};
  // this values allow to switch the code between different helicity encoding mode.

  enum InputRegisterBits{kDefaultInputReg_HelPlus     = 0x1,
			 kDefaultInputReg_HelMinus    = 0x2,
			 kDefaultInputReg_PatternSync = 0x4,
			 kDefaultInputReg_FakeMPS     = 0x8000};



  UInt_t fInputReg_FakeMPS;
  UInt_t fInputReg_HelPlus;
  UInt_t fInputReg_HelMinus;
  UInt_t fInputReg_PatternSync;
  UInt_t fInputReg_PairSync;

  static const std::vector<UInt_t> kDefaultHelicityBitPattern;

  std::vector<UInt_t> fHelicityBitPattern;

  std::vector <QwWord> fWord;
  std::vector < std::pair<Int_t, Int_t> > fWordsPerSubbank;  // The indices of the first & last word in each subbank

  // data taking. For example this was used during the injector data taking
  // in winter 2008-09 injector tests
  static const Int_t kUndefinedHelicity= -9999;

  Bool_t IsGoodPatternNumber();
  Bool_t IsGoodEventNumber();
  Bool_t MatchActualHelicity(Int_t actual);
  Bool_t IsGoodPhaseNumber();
  Bool_t IsContinuous();

  UInt_t GetRandbit24(UInt_t& ranseed);//for 24bit pattern
  UInt_t GetRandomSeed(UShort_t* first24randbits);
  virtual Bool_t CollectRandBits();
  Bool_t CollectRandBits24();//for 24bit pattern
  Bool_t CollectRandBits30();//for 30bit pattern


  void   ResetPredictor();

  Bool_t Compare(VQwSubsystem *source);

};

#endif


