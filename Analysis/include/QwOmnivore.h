/*!
 * \file   QwOmnivore.h
 * \brief  An omnivorous subsystem template class
 */

#pragma once

#include "VQwSubsystemParity.h"

/**
 *  \class QwOmnivore
 *  \ingroup QwAnalysis
 *  \brief An omnivorous subsystem.
 */
template<class VQwSubsystem_t>
class QwOmnivore: public VQwSubsystem_t {

  private:
    /// Private default constructor (not implemented, will throw linker error on use)
    QwOmnivore();

  public:
    /// Constructor with name
    QwOmnivore(const TString& name): VQwSubsystem(name),VQwSubsystem_t(name) { };
    /// Copy constructor
    QwOmnivore(const QwOmnivore& source)
    : VQwSubsystem(source),VQwSubsystem_t(source)
    { }
    /// Virtual destructor
    ~QwOmnivore() override;

    /// Map file definition
    Int_t LoadChannelMap(TString mapfile) override { return 0; };
    /// Parameter file definition
    Int_t LoadInputParameters(TString mapfile) override { return 0; };
    /// Geometry definition for tracking subsystems
    Int_t LoadGeometryDefinition(TString mapfile) override { return 0; };

    /// Load the event cuts file
    Int_t LoadEventCuts(TString filename) override { return 0; };
    /// Apply the single event cuts
    Bool_t ApplySingleEventCuts() override { return kTRUE; };

    Bool_t CheckForBurpFail(const VQwSubsystem *subsys) override{return kFALSE;};

    /// Report the number of events failed due to HW and event cut failures
    void PrintErrorCounters() const override { };
    /// Return the error flag to the main routine
    UInt_t GetEventcutErrorFlag() override { return 0; };

    /// Increment error counters
    void IncrementErrorCounters() override { };
    /// Update error flag
    void UpdateErrorFlag(const VQwSubsystem*) override { };

    /// Get the hit list
    //void  GetHitList(QwHitContainer& grandHitContainer) { };
    /// Get the detector geometry information
    //Int_t GetDetectorInfo(std::vector< std::vector< QwDetectorInfo > > & detect_info) { return 0; };


    /// Clear event data
    void  ClearEventData() override { };

    /// Process the configuration events
    Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override {
      /// Om nom nom nom
      // TODO (wdc) configuration events seem to have num_words = 0xffffffff
      //UInt_t cheeseburger;
      //for (UInt_t word = 0; word < num_words; word++)
      //  cheeseburger += buffer[word]; // (addition to prevent compiler from optimizing local away)
      return 0; // my plate is empty
    };

    /// Process the event buffer
    Int_t ProcessEvBuffer(const UInt_t event_type, const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override {
      /// TODO:  Subsystems should be changing their ProcessEvBuffer routines to take the event_type as the first
      ///  argument.  But in the meantime, default to just calling the non-event-type-aware ProcessEvBuffer routine.
      return this->ProcessEvBuffer(roc_id, bank_id, buffer, num_words);
    };
    /// TODO:  The non-event-type-aware ProcessEvBuffer routine should be replaced with the event-type-aware version.
    Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override {
      /// Om nom nom nom
      UInt_t cheeseburger;
      for (UInt_t word = 0; word < num_words; word++)
        cheeseburger += buffer[word]; // (addition to prevent compiler from optimizing local away)
      return 0; // my plate is empty
    };

    /// Process the event
    void  ProcessEvent() override { };

    /// Assignment/addition/subtraction operators
    VQwSubsystem& operator=  (VQwSubsystem *value) override { return *this; };
    VQwSubsystem& operator+= (VQwSubsystem *value) override { return *this; };
    VQwSubsystem& operator-= (VQwSubsystem *value) override { return *this; };
    /// Sum/difference/ratio/scale operations
    void Sum(VQwSubsystem *value1, VQwSubsystem *value2) override { };
    void Difference(VQwSubsystem *value1, VQwSubsystem *value2) override { };
    void Ratio(VQwSubsystem *numer, VQwSubsystem *denom) override { };
    void Scale(Double_t factor) override { };

    /// Construct the histograms for this subsystem in a folder with a prefix
    void  ConstructHistograms(TDirectory *folder, TString &prefix) override { };
    /// Fill the histograms for this subsystem
    void  FillHistograms() override { };

    /// Construct the branch and tree vector
    void ConstructBranchAndVector(TTree *tree, TString & prefix, QwRootTreeBranchVector &values) override { };
    /// Fill the tree vector
    void FillTreeVector(QwRootTreeBranchVector &values) const override { };
#ifdef HAS_RNTUPLE_SUPPORT
    /// Construct the RNTuple fields and vector
    void ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override { };
    /// Fill the RNTuple vector
    void FillNTupleVector(std::vector<Double_t>& values) const override { };
#endif // HAS_RNTUPLE_SUPPORT
    /// Construct branch
    void ConstructBranch(TTree*, TString&) override { };
    /// Construct branch
    void ConstructBranch(TTree*, TString&, QwParameterFile&) override { };

    /// \brief Update the running sums for devices
    void AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override { };
    void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF) override { };
    /// \brief Calculate the average for all good events
    void CalculateRunningAverage() override { };
};

// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwOmnivore<VQwSubsystemParity>);
