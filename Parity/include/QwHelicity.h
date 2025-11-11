/**********************************************************\
* File: QwHelicity.h                                      *
* brief  Helicity state management and pattern recognition*
* Contributor: Arindam Sen (asen@jlab.org)                *
* Time-stamp:                                             *
\**********************************************************/

#pragma once

// System headers
#include <vector>

// ROOT headers
#include "TTree.h"
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RNTupleWriter.hxx"
#include "ROOT/RField.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "QwWord.h"
#include "QwHelicityBase.h"

/**
 * \class QwHelicity
 * \ingroup QwAnalysis_BeamLine
 * \brief Subsystem for helicity state management and pattern recognition
 *
 * Manages helicity information from the polarized electron beam, including
 * helicity state determination, pattern recognition, delayed helicity decoding,
 * and helicity-correlated systematic checks. Supports multiple helicity
 * encoding modes and provides helicity information to other subsystems.
 */
class QwHelicity: public QwHelicityBase, public MQwSubsystemCloneable<QwHelicity> {

 private:
  QwHelicity();

 public:

  QwHelicity(const TString& name);
  QwHelicity(const QwHelicity& source);

  /// destructor
  ~QwHelicity() override { }

  using QwHelicityBase::operator=;

  static void DefineOptions(QwOptions &options);
  void ProcessOptions(QwOptions &options) override;
  Int_t LoadChannelMap(TString mapfile) override;
  Int_t LoadInputParameters(TString pedestalfile) override;
  Int_t LoadEventCuts(TString  filename) override;//Loads event cuts applicable to QwHelicity class, derived from VQwSubsystemParity
  Bool_t ApplySingleEventCuts() override;//Apply event cuts in the QwHelicity class, derived from VQwSubsystemParity

  Bool_t  CheckForBurpFail(const VQwSubsystem *ev_error) override{
    return kFALSE;
  };

  Int_t  ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id,
				   UInt_t* buffer, UInt_t num_words) override;
  Int_t  ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override {
    return ProcessEvBuffer(0x1,roc_id,bank_id,buffer,num_words);
  };
  Int_t  ProcessEvBuffer(UInt_t ev_type, const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
  void   ProcessEventUserbitMode();//ProcessEvent has two modes Userbit and Inputregister modes
  void   ProcessEventInputRegisterMode();
  void   ProcessEventInputMollerMode();

  void   EncodeEventData(std::vector<UInt_t> &buffer) override;


  void  ClearEventData() override;
  void  ProcessEvent() override;

  UInt_t GetRandomSeedActual() { return iseed_Actual; };
  UInt_t GetRandomSeedDelayed() { return iseed_Delayed; };

  using VQwSubsystem::ConstructHistograms;
  void  ConstructHistograms(TDirectory *folder, TString &prefix) override;

  using VQwSubsystem::ConstructBranchAndVector;

 protected:

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

};

// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwHelicity);
