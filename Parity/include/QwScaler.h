/*!
 * \file   QwScaler.h
 * \brief  Scaler subsystem for counting and rate measurements
 */

#pragma once

// System headers
#include <vector>

// ROOT headers
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RField.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "VQwSubsystemParity.h"
#include "QwScaler_Channel.h"


/**
 * \class QwScaler
 * \ingroup QwAnalysis_BL
 * \brief Subsystem managing scaler modules and derived rates
 *
 * Wraps hardware scaler channels, provides per-MPS processing, histogram
 * and tree output, and utilities for normalization and cuts.
 */
class QwScaler: public VQwSubsystemParity, public MQwSubsystemCloneable<QwScaler>
{
  private:
    /// Private default constructor (not implemented, will throw linker error on use)
    QwScaler();

  public:

    /// Constructor with name
    QwScaler(const TString& name);
    /// Copy constructor
    QwScaler(const QwScaler& source)
    : VQwSubsystem(source),VQwSubsystemParity(source)
    {
      fScaler.resize(source.fScaler.size());
      for (size_t i = 0; i < fScaler.size(); i++) {
        VQwScaler_Channel* scaler_tmp = 0;
        if ((scaler_tmp = dynamic_cast<QwSIS3801D24_Channel*>(source.fScaler.at(i)))) {
          QwSIS3801D24_Channel* scaler = dynamic_cast<QwSIS3801D24_Channel*>(source.fScaler.at(i));
          fScaler.at(i) = new QwSIS3801D24_Channel(*scaler);
        } else if ((scaler_tmp = dynamic_cast<QwSIS3801D32_Channel*>(source.fScaler.at(i)))) {
          QwSIS3801D32_Channel* scaler = dynamic_cast<QwSIS3801D32_Channel*>(source.fScaler.at(i));
          fScaler.at(i) = new QwSIS3801D32_Channel(*scaler);
        }
      }
    }
    /// Destructor
    ~QwScaler() override;

    // Handle command line options
    static void DefineOptions(QwOptions &options);
    void ProcessOptions(QwOptions &options) override;

    Int_t LoadChannelMap(TString mapfile) override;
    Int_t LoadInputParameters(TString pedestalfile) override;

    void  ClearEventData() override;

    Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
    Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t *buffer, UInt_t num_words) override;
    void  ProcessEvent() override;

    using VQwSubsystem::ConstructHistograms;
    void  ConstructHistograms(TDirectory *folder, TString &prefix) override;
    void  FillHistograms() override;

    using VQwSubsystem::ConstructBranchAndVector;
    void  ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) override;
    void  ConstructBranch(TTree *tree, TString& prefix) override { };
    void  ConstructBranch(TTree *tree, TString& prefix, QwParameterFile& trim_file) override { };
    void  FillTreeVector(QwRootTreeBranchVector &values) const override;

    // RNTuple methods
#ifdef HAS_RNTUPLE_SUPPORT
    void ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
    void FillNTupleVector(std::vector<Double_t>& values) const override;
#endif // HAS_RNTUPLE_SUPPORT

    Bool_t Compare(VQwSubsystem *source);

    VQwSubsystem& operator=(VQwSubsystem *value) override;
    VQwSubsystem& operator+=(VQwSubsystem *value) override;
    VQwSubsystem& operator-=(VQwSubsystem *value) override;
    void Ratio(VQwSubsystem *value1, VQwSubsystem  *value2) override;
    void Scale(Double_t factor) override;

    void AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
    //remove one entry from the running sums for devices
    void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF) override;
    void CalculateRunningAverage() override;

    Int_t LoadEventCuts(TString filename) override;
    Bool_t SingleEventCuts();
    Bool_t ApplySingleEventCuts() override;

    Bool_t CheckForBurpFail(const VQwSubsystem *subsys) override{
        //QwError << "************* test inside Scaler *****************" << QwLog::endl;
        return kFALSE;
    };

    void IncrementErrorCounters() override;

    void PrintErrorCounters() const override;
    UInt_t GetEventcutErrorFlag() override;
    //update the error flag in the subsystem level from the top level routines related to stability checks. This will uniquely update the errorflag at each channel based on the error flag in the corresponding channel in the ev_error subsystem
    void UpdateErrorFlag(const VQwSubsystem *ev_error) override{
    };

    void PrintValue() const override;
    void PrintInfo() const override;

    Double_t* GetRawChannelArray();

    Double_t GetDataForChannelInModule(Int_t modnum, Int_t channum) {
      Int_t index = fModuleChannel_Map[std::pair<Int_t,Int_t>(modnum,channum)];
      return fScaler.at(index)->GetValue();
    }

    Int_t GetChannelIndex(TString channelName, UInt_t module_number);

  private:

    // Number of good events
    Int_t fGoodEventCount;

    // Mapping from subbank to scaler channels
    typedef std::map< Int_t, std::vector< std::vector<Int_t> > > Subbank_to_Scaler_Map_t;
    Subbank_to_Scaler_Map_t fSubbank_Map;

    // Mapping from module and channel number to scaler channel
    typedef std::map< std::pair<Int_t,Int_t>, Int_t > Module_Channel_to_Scaler_Map_t;
    Module_Channel_to_Scaler_Map_t fModuleChannel_Map;

    // Mapping from name to scaler channel
    typedef std::map< TString, Int_t> Name_to_Scaler_Map_t;
    Name_to_Scaler_Map_t fName_Map;

    // Vector of scaler channels
    std::vector< VQwScaler_Channel* > fScaler; // Raw channels
    std::vector< UInt_t > fBufferOffset; // Offset in scaler buffer
    std::vector< std::pair< VQwScaler_Channel*, double > > fNorm;
};

// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwScaler);
