/*!
 * \file   QwBeamLine.h
 * \brief  Beamline subsystem containing BPMs, BCMs, and other beam monitoring devices
 */

#pragma once

// System headers
#include <vector>

// ROOT headers
#include "TTree.h"
#include "TString.h"

// Qweak headers
#include "VQwSubsystemParity.h"
#include "QwTypes.h"
#include "QwBPMStripline.h"
#include "VQwBCM.h"
#include "QwBCM.h"
#include "QwBPMCavity.h"
#include "QwCombinedBCM.h"
#include "QwCombinedBPM.h"
#include "QwEnergyCalculator.h"
#include "QwHaloMonitor.h"
#include "QwQPD.h"
#include "QwLinearDiodeArray.h"
#include "VQwClock.h"
#include "QwBeamDetectorID.h"


/**
 * \class QwBeamLine
 * \ingroup QwAnalysis_BeamLine
 * \brief Subsystem aggregating beamline instruments (BPMs, BCMs, clocks, etc.)
 *
 * QwBeamLine owns and orchestrates multiple beam monitoring devices and
 * provides a unified subsystem interface for map loading, event decoding,
 * event processing, cuts, error propagation, histogram/tree output, and
 * publishing. It supports combinations (e.g., combined BPM/BCM), mock-data
 * generation, and stability/burp checks at the subsystem level.
 */
class QwBeamLine : public VQwSubsystemParity, public MQwSubsystemCloneable<QwBeamLine> {

 private:
  /// Private default constructor (not implemented, will throw linker error on use)
  QwBeamLine();

 public:
  /// Constructor with name
  QwBeamLine(const TString& name)
  : VQwSubsystem(name),VQwSubsystemParity(name)
  { };
  /// Copy constructor
  QwBeamLine(const QwBeamLine& source)
  : VQwSubsystem(source),VQwSubsystemParity(source),
    fQPD(source.fQPD),
    fLinearArray(source.fLinearArray),
    fCavity(source.fCavity),
    fHaloMonitor(source.fHaloMonitor),
    fECalculator(source.fECalculator),
    fBeamDetectorID(source.fBeamDetectorID)
  { this->CopyTemplatedDataElements(&source); }
  /// Virtual destructor
  ~QwBeamLine() override { };

  void   CopyTemplatedDataElements(const VQwSubsystem *source);

  /* derived from VQwSubsystem */

  void   ProcessOptions(QwOptions &options) override;//Handle command line options
  Int_t  LoadChannelMap(TString mapfile) override;
  Int_t  LoadInputParameters(TString pedestalfile) override;
  void LoadEventCuts_Init() override {};
  void LoadEventCuts_Line(QwParameterFile &mapstr, TString &varvalue, Int_t &eventcut_flag) override;
  void LoadEventCuts_Fin(Int_t &eventcut_flag) override;
  Int_t  LoadGeometryDefinition(TString mapfile) override;
  void  LoadMockDataParameters(TString mapfile) override;
  void   AssignGeometry(QwParameterFile* mapstr, VQwBPM * bpm);

  Bool_t ApplySingleEventCuts() override;//derived from VQwSubsystemParity
  void   IncrementErrorCounters() override;

  Bool_t CheckForBurpFail(const VQwSubsystem *subsys) override;

  void   PrintErrorCounters() const override;// report number of events failed due to HW and event cut failures
  UInt_t GetEventcutErrorFlag() override;//return the error flag

  UInt_t UpdateErrorFlag() override;//Update and return the error flags

  //update the error flag in the subsystem level from the top level routines related to stability checks. This will uniquely update the errorflag at each channel based on the error flag in the corresponding channel in the ev_error subsystem
  void UpdateErrorFlag(const VQwSubsystem *ev_error) override;

  Int_t  ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
  Int_t  ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
  void   PrintDetectorID() const;

  void   ClearEventData() override;
  void   ProcessEvent() override;

  Bool_t PublishInternalValues() const override;
  Bool_t PublishByRequest(TString device_name) override;


 public:
  size_t GetNumberOfElements();
  void   RandomizeEventData(int helicity = 0, double time = 0.0) override;
  void   SetRandomEventAsymmetry(Double_t asymmetry);
  void   EncodeEventData(std::vector<UInt_t> &buffer) override;

  VQwSubsystem&  operator=  (VQwSubsystem *value) override;
  VQwSubsystem&  operator+= (VQwSubsystem *value) override;
  VQwSubsystem&  operator-= (VQwSubsystem *value) override;
  void   Ratio(VQwSubsystem *numer, VQwSubsystem *denom) override;

  void   Scale(Double_t factor) override;

  void   AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
  //remove one entry from the running sums for devices
  void   DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF) override;


  void   CalculateRunningAverage() override;

  using  VQwSubsystem::ConstructHistograms;
  void   ConstructHistograms(TDirectory *folder, TString &prefix) override;
  void   FillHistograms() override;

  using  VQwSubsystem::ConstructBranchAndVector;
  void   ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) override;
  void   ConstructBranch(TTree *tree, TString &prefix) override;
  void   ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& trim_file ) override;
  void   FillTreeVector(QwRootTreeBranchVector &values) const override;

#ifdef HAS_RNTUPLE_SUPPORT
  using  VQwSubsystem::ConstructNTupleAndVector;
  void   ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
  void   FillNTupleVector(std::vector<Double_t>& values) const override;
#endif

#ifdef __USE_DATABASE__
  void   FillDB(QwParityDB *db, TString datatype) override;
  void   FillErrDB(QwParityDB *db, TString datatype) override;
#endif // __USE_DATABASE__

  Bool_t Compare(VQwSubsystem *source);

  void   PrintValue() const override;
  void   PrintInfo() const override;
  void   WritePromptSummary(QwPromptSummary *ps, TString type) override;

  VQwDataElement* GetElement(QwBeamDetectorID det_id);
  VQwDataElement* GetElement(EQwBeamInstrumentType TypeID, TString name);
  VQwDataElement* GetElement(EQwBeamInstrumentType TypeID, Int_t index);
  const  VQwDataElement* GetElement(EQwBeamInstrumentType TypeID, Int_t index) const;

  const  VQwHardwareChannel* GetChannel(EQwBeamInstrumentType TypeID, Int_t index, TString device_prop) const;

  VQwBPM* GetBPMStripline(const TString name);
  VQwBCM* GetBCM(const TString name);
  VQwClock* GetClock(const TString name);
  QwBPMCavity* GetBPMCavity(const TString name);
  VQwBCM* GetCombinedBCM(const TString name);
  VQwBPM* GetCombinedBPM(const TString name);
  QwEnergyCalculator* GetEnergyCalculator(const TString name);
  QwHaloMonitor* GetScalerChannel(const TString name);
  const QwBPMCavity* GetBPMCavity(const TString name) const;
  const VQwBPM* GetBPMStripline(const TString name) const;
  const VQwBCM* GetBCM(const TString name) const;
  const VQwClock* GetClock(const TString name) const;
  const VQwBCM* GetCombinedBCM(const TString name) const;
  const VQwBPM* GetCombinedBPM(const TString name) const;
  const QwEnergyCalculator* GetEnergyCalculator(const TString name) const;
  const QwHaloMonitor* GetScalerChannel(const TString name) const;


/////
protected:

  ///  \brief Adds a new element to a vector of data elements, and returns
  ///  the index of that element within the array.
  template <typename TT>
  Int_t AddToElementList(std::vector<TT> &elementlist, QwBeamDetectorID &detector_id);

  Int_t GetDetectorIndex(EQwBeamInstrumentType TypeID, TString name) const;
  //when the type and the name is passed the detector index from appropriate vector will be returned
  //for example if TypeID is bcm  then the index of the detector from fBCM vector for given name will be returned.

  std::vector <VQwBPM_ptr> fStripline;
  std::vector <VQwBPM_ptr> fBPMCombo;

  std::vector <VQwBCM_ptr> fBCM;
  std::vector <VQwBCM_ptr> fBCMCombo;

  std::vector <VQwClock_ptr> fClock;

  std::vector <QwQPD> fQPD;
  std::vector <QwLinearDiodeArray> fLinearArray;
  std::vector <QwBPMCavity> fCavity;
  std::vector <QwHaloMonitor> fHaloMonitor;


  std::vector <QwEnergyCalculator> fECalculator;
  std::vector <QwBeamDetectorID> fBeamDetectorID;



/////
private:
  // std::vector<TString> DetectorTypes;// for example could be BCM, LUMI,BPMSTRIPLINE, etc..
  Int_t fQwBeamLineErrorCount;


  static const Bool_t bDEBUG=kFALSE;



};

// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwBeamLine);
