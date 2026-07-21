/*!
 * \file   QwBeamMod.h
 * \brief  Beam modulation subsystem for parity analysis
 * \author Joshua Hoskins
 * \date   2010-05-25
 */

#pragma once

// System headers
#include <vector>

// ROOT headers
#include "TTree.h"
#ifdef HAS_RNTUPLE_SUPPORT
#include <ROOT/RNTupleModel.hxx>
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "VQwSubsystemParity.h"
#include "QwVQWK_Channel.h"
#include "QwWord.h"

// Forward declarations
class QwBeamMod;

//enum EBeamInstrumentType{kBPMStripline = 0,
//			 kBCM,
//			 kCombinedBCM,
//			 kCombinedBPM};

// this emun vector needs to be coherent with the DetectorTypes declaration in the QwBeamLine constructor



/*****************************************************************
*  Class:
******************************************************************/
/**
 * \class QwModChannelID
 * \ingroup QwAnalysis_BL
 * \brief Mapping information for beam modulation channels
 */
class QwModChannelID
{
 public:

  QwModChannelID(Int_t subbankid, Int_t wordssofar,TString name,
                   TString modtype ,QwBeamMod * obj);

   QwModChannelID(Int_t subbankid, QwParameterFile &paramfile);



/*     QwModChannelID):fSubbankIndex(-1),fWordInSubbank(-1),fTypeID(-1),fIndex(-1), */
/*     fSubelement(999999),fmoduletype(""),fmodulename("") */
/*     {}; */

  Int_t fSubbankIndex;        //Generated from ROCID(readout CPU) & BankID(corresponds to internal headers to ID different types of data)
  Int_t fWordInSubbank;
  //first word reported for this channel in the subbank
  //(eg VQWK channel report 6 words for each event, scalers only report one word per event)

  // The first word of the subbank gets fWordInSubbank=0

  TString fmoduletype; // eg: VQWK, SCALER
  TString fmodulename;
  Int_t modnum;
  Int_t channum;

  // TString fdetectortype;

  Int_t  kUnknownDeviceType;
  Int_t  fTypeID;           // type of detector eg: lumi or stripline, etc..
  Int_t  fIndex;            // index of this detector in the vector containing all the detector of same type

  void Print();

};



/*****************************************************************
*  Class:
******************************************************************/
/**
 * \class QwBeamMod
 * \ingroup QwAnalysis_BL
 * \brief Subsystem for beam modulation studies and FFB handling
 *
 * Decodes modulation ramp and pattern words, maintains channels affected
 * by modulation, and computes relevant summaries for regression.
 */
class QwBeamMod: public VQwSubsystemParity, public MQwSubsystemCloneable<QwBeamMod> {

 private:
  ///
  QwBeamMod();

 public:
  /// Constructor with name
  QwBeamMod(const TString& name)
  : VQwSubsystem(name),VQwSubsystemParity(name)
    {
      fFFB_holdoff_Counter=0;
      fFFB_Flag=kTRUE;
      fRampChannelIndex = -1;
      fPatternWordIndex = -1;
      fBmwObj_Index = -1;

      // Initialize the bmwobj cuts with UL < LL to disable the cut
      fBMWObj_LL = +1;
      fBMWObj_UL = -1;
    };
  /// Copy constructor
  QwBeamMod(const QwBeamMod& source)
  : VQwSubsystem(source),VQwSubsystemParity(source),
    fWord(source.fWord)
  {
    // std::cout<< "Here in the copy constructor" << std::endl;
    this->fModChannel.reserve(source.fModChannel.size());
    for(size_t i=0;i< source.fModChannel.size();i++) {
      this->fModChannel.push_back(source.fModChannel[i]->Clone());
      *(this->fModChannel[i]) = *(source.fModChannel[i]);
      //source.fModChannel[i]->PrintValue();
    }
  }
  /// Virtual destructor
  ~QwBeamMod() override {};

  /* derived from VQwSubsystem */

  //Handle command line options
  void ProcessOptions(QwOptions &options) override;
  void AccumulateRunningSum(VQwSubsystem*, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  //remove one entry from the running sums for devices
  void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF) override{
  };

  Int_t LoadChannelMap(TString mapfile) override;
  void LoadEventCuts_Init() override;
  void LoadEventCuts_Line(QwParameterFile &mapstr, TString &varvalue, Int_t &eventcut_flag) override;
  void LoadEventCuts_Fin(Int_t &eventcut_flag) override;
  Int_t LoadGeometry(TString mapfile);
  Int_t LoadInputParameters(TString pedestalfile) override;


  Bool_t ApplySingleEventCuts() override;//derived from VQwSubsystemParity
  void IncrementErrorCounters() override;
  void PrintErrorCounters() const override;// report number of events failed due to HW and event cut failures
  UInt_t GetEventcutErrorFlag() override;//return the error flag

  Bool_t CheckForBurpFail(const VQwSubsystem *subsys) override;

  //update the error flag in the subsystem level from the top level routines related to stability checks. This will uniquely update the errorflag at each channel based on the error flag in the corresponding channel in the ev_error subsystem
  void UpdateErrorFlag(const VQwSubsystem *ev_error) override;

  Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
  Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
//  void  PrintDetectorID();

  void  ClearEventData() override;

  void  ProcessEvent() override;
  void  ProcessEvent_2() override;

  VQwSubsystem&  operator=  (VQwSubsystem *value) override;
  VQwSubsystem&  operator+= (VQwSubsystem *value) override;
  VQwSubsystem&  operator-= (VQwSubsystem *value) override;


  void Ratio(VQwSubsystem *numer, VQwSubsystem *denom) override;

  void Scale(Double_t factor) override;

  void CalculateRunningAverage() override;
  void PrintModChannelID();

  using VQwSubsystem::ConstructHistograms;
  void ConstructHistograms(TDirectory *folder, TString &prefix) override;
  void FillHistograms() override;

  using VQwSubsystem::ConstructBranchAndVector;
  void ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) override;
  void ConstructBranch(TTree *tree, TString& prefix) override { };
  void ConstructBranch(TTree *tree, TString& prefix, QwParameterFile& trim_file) override { };
  void FillTreeVector(QwRootTreeBranchVector &values) const override;
#ifdef HAS_RNTUPLE_SUPPORT
  void ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
  void FillNTupleVector(std::vector<Double_t>& values) const override;
#endif // HAS_RNTUPLE_SUPPORT

#ifdef __USE_DATABASE__
  void FillDB_MPS(QwParityDB *db, TString datatype) override;
  void FillDB(QwParityDB *db, TString datatype) override;
  void FillErrDB(QwParityDB *db, TString datatype) override;
#endif // __USE_DATABASE__
  void WritePromptSummary(QwPromptSummary *ps, TString type) override;

  Bool_t Compare(VQwSubsystem *source);

  void Print();

 protected:
 Int_t GetDetectorIndex(TString name);
 Int_t fTreeArrayIndex;

 std::vector <VQwHardwareChannel*> fModChannel;
 std::vector <QwModChannelID> fModChannelID;
 std::vector <QwWord> fWord;
 std::vector < std::pair<Int_t, Int_t> > fWordsPerSubbank;



 private:

 UInt_t fFFB_Index;
 UInt_t fFFB_holdoff;
 UInt_t fFFB_holdoff_Counter;
 UInt_t fFFB_ErrorFlag;
 Bool_t fFFB_Flag;
 static const Bool_t bDEBUG=kFALSE;

 Int_t fRampChannelIndex;
 Int_t fPatternWordIndex;
 UInt_t fBmwObj_Index;
 Int_t fBMWObj_LL;
 Int_t fBMWObj_UL;
 UInt_t fBmwObj_ErrorFlag;

};

// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwBeamMod);
