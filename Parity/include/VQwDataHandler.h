/*!
 * \file   VQwDataHandler.h
 * \brief  Virtual base class for data handlers accessing multiple subsystems
 * \author Michael Vallee
 * \date   2018-08-01
 */

#ifndef VQWDATAHANDLER_H_
#define VQWDATAHANDLER_H_

// Qweak headers
#include "QwHelicityPattern.h"
#include "QwSubsystemArrayParity.h"
#include "VQwHardwareChannel.h"
#include "QwFactory.h"
#include "MQwPublishable.h"

class QwParameterFile;
class QwRootFile;
class QwPromptSummary;
class QwDataHandlerArray;

/**
 * \class VQwDataHandler
 * \ingroup QwAnalysis
 * \brief Abstract base for handlers that consume multiple subsystems and produce derived outputs
 *
 * A data handler observes one or more subsystem arrays (yields, asymmetries,
 * differences) and computes derived channels or diagnostics. Typical examples
 * include linear regression, correlation studies, and alarm/quality handlers.
 *
 * Key responsibilities:
 * - Establish connections to required input channels via ConnectChannels.
 * - ProcessData once per event to update derived quantities.
 * - Maintain running sums/averages and optionally publish variables to trees,
 *   histograms, or RNTuples.
 *
 * Design notes:
 * - Uses container-delegation at the system level; underlying arithmetic is
 *   performed by the concrete channel classes.
 * - Handlers participate in the MQwPublishable pattern for on-demand output
 *   publication, and in cloneable factories for configuration-driven creation.
 */
class VQwDataHandler:  virtual public VQwDataHandlerCloneable, public MQwPublishable_child<QwDataHandlerArray,VQwDataHandler> {

  public:
  
    enum EQwHandleType {
      kHandleTypeUnknown=0, kHandleTypeMps, kHandleTypeAsym, kHandleTypeDiff, kHandleTypeYield
    };
    
    typedef std::vector< VQwHardwareChannel* >::iterator Iterator_HdwChan;
    typedef std::vector< VQwHardwareChannel* >::const_iterator ConstIterator_HdwChan;

    VQwDataHandler(const TString& name);
    VQwDataHandler(const VQwDataHandler &source);

    virtual void ParseConfigFile(QwParameterFile& file);

    void SetPointer(QwHelicityPattern *ptr){
      fHelicityPattern = ptr;
      fErrorFlagPtr = ptr->GetEventcutErrorFlagPointer();
    };
    void SetPointer(QwSubsystemArrayParity *ptr){
      fSubsystemArray = ptr;
      fErrorFlagPtr = ptr->GetEventcutErrorFlagPointer();
    };

    virtual Int_t ConnectChannels(QwSubsystemArrayParity& /*yield*/, QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff){
      return this->ConnectChannels(asym, diff);
    }

    // Subsystems with support for subsystem arrays should override this
    virtual Int_t ConnectChannels(QwSubsystemArrayParity& /*detectors*/) { return 0; }

    Int_t ConnectChannels(QwHelicityPattern& helicitypattern) {
      return this->ConnectChannels(
          helicitypattern.GetYield(),
          helicitypattern.GetAsymmetry(),
          helicitypattern.GetDifference());
    }

    virtual void ProcessData();

    virtual void UpdateBurstCounter(Short_t burstcounter){fBurstCounter=burstcounter;};

    virtual void FinishDataHandler(){
      CalculateRunningAverage();
    };

    ~VQwDataHandler() override;

    TString GetName(){return fName;}

    virtual void ClearEventData();

    void InitRunningSum();
    void AccumulateRunningSum();
    virtual void AccumulateRunningSum(VQwDataHandler &value, Int_t count = 0, Int_t ErrorMask = 0xFFFFFFF);
    void CalculateRunningAverage();
    void PrintValue() const;

#ifdef __USE_DATABASE__
    void FillDB(QwParityDB *db, TString datatype);
#endif // __USE_DATABASE__

    void WritePromptSummary(QwPromptSummary *ps, TString type);

    virtual void ConstructTreeBranches(
        QwRootFile *treerootfile,
        const std::string& treeprefix = "",
        const std::string& branchprefix = "");
    virtual void FillTreeBranches(QwRootFile *treerootfile);

    /// \brief RNTuple methods
    virtual void ConstructNTupleFields(
        QwRootFile *treerootfile,
        const std::string& treeprefix = "",
        const std::string& branchprefix = "");
    virtual void FillNTupleFields(QwRootFile *treerootfile);

    /// \brief Construct the histograms in a folder with a prefix
    virtual void  ConstructHistograms(TDirectory * /*folder*/, TString & /*prefix*/) { };
    /// \brief Fill the histograms
    virtual void  FillHistograms() { };

    // Fill the vector for this subsystem
    void FillTreeVector(std::vector<Double_t> &values) const;

    void ConstructBranchAndVector(TTree *tree, TString& prefix, std::vector<Double_t>& values);
#ifdef HAS_RNTUPLE_SUPPORT
    void ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs);
    void FillNTupleVector(std::vector<Double_t>& values) const;
#endif // HAS_RNTUPLE_SUPPORT

    void SetRunLabel(TString x) {
      run_label = x;
    }

    Int_t LoadChannelMap(){return this->LoadChannelMap(fMapFile);}
    virtual Int_t LoadChannelMap(const std::string& /*mapfile*/){return 0;};

    /// \brief Publish all variables of the subsystem
    Bool_t PublishInternalValues() const override;
    /// \brief Try to publish an internal variable matching the submitted name
    Bool_t PublishByRequest(TString device_name) override;

  protected:
    
    VQwDataHandler() { }
    
    virtual Int_t ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff);
    
    void SetEventcutErrorFlagPointer(const UInt_t* errorflagptr) {
      fErrorFlagPtr = errorflagptr;
    }
    UInt_t GetEventcutErrorFlag() const {
      if (fErrorFlagPtr)
        return *fErrorFlagPtr;
      else
        return -1;
    };

    std::pair<EQwHandleType,std::string> ParseHandledVariable(const std::string& variable);

   void CalcOneOutput(const VQwHardwareChannel* dv, VQwHardwareChannel* output,
                       std::vector< const VQwHardwareChannel* > &ivs,
                       std::vector< Double_t > &sens);

 protected:
   //
   Int_t fPriority; ///  When a datahandler array is processed, handlers with lower priority will be processed before handlers with higher priority

   Short_t fBurstCounter;

    //***************[Variables]***************
   TString fName;
   std::string fMapFile;
   std::string fTreeName;
   std::string fTreeComment;

   std::string fPrefix;

   TString run_label;

   /// Error flag pointer
   const UInt_t* fErrorFlagPtr;

   /// Single event pointer
   QwSubsystemArrayParity* fSubsystemArray;
   /// Helicity pattern pointer
   QwHelicityPattern* fHelicityPattern;

   std::vector< std::string > fDependentFull;
   std::vector< EQwHandleType > fDependentType;
   std::vector< std::string > fDependentName;

   std::vector< const VQwHardwareChannel* > fDependentVar;
   std::vector< Double_t > fDependentValues;

   std::vector< VQwHardwareChannel* > fOutputVar;
   std::vector< Double_t > fOutputValues;

   std::vector<std::vector<TString> > fPublishList;

   std::string ParseSeparator;  // Used as space between tokens in ParseHandledVariable

 protected:
   Bool_t fKeepRunningSum;
   Bool_t fRunningsumFillsTree;
   VQwDataHandler *fRunningsum;
};

#endif // VQWDATAHANDLER_H_
