/**********************************************************\
* File: VQwDetectorArray.h                                  *
*                                                          *
* Author: Kevin Ward (Original code by P.M. King)                                      *
* Time-stamp: <2007-05-08 15:40>                           *
\**********************************************************/

///
/// \ingroup QwAnalysis_ADC

/*!
 * \file   VQwDetectorArray.h
 * \brief  Virtual base class for detector arrays (PMTs, etc.)
 */

#pragma once

// System headers
#include <vector>

// ROOT headers
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "VQwSubsystemParity.h"
#include "QwIntegrationPMT.h"
#include "QwCombinedPMT.h"


class QwDetectorArrayID {
    /**
     * \class QwDetectorArrayID
     * \ingroup QwAnalysis_ADC
     * \brief Identifier and mapping information for detector-array channels
     *
     * Encapsulates mapping metadata for channels in a detector array,
     * including subbank indexing, subelement, type, and naming.
     */

 public:

    QwDetectorArrayID():fSubbankIndex(-1),fWordInSubbank(-1),
     fTypeID(kQwUnknownPMT),fIndex(-1),
     fSubelement(kInvalidSubelementIndex),fmoduletype(""),fdetectorname("") {};

    int fSubbankIndex;
    int fWordInSubbank; //first word reported for this channel in the subbank
                        //(eg VQWK channel report 6 words for each event, scalers only report one word per event)
                        // The first word of the subbank gets fWordInSubbank=0

    EQwPMTInstrumentType fTypeID;     // type of detector
    int fIndex;      // index of this detector in the vector containing all the detector of same type
    UInt_t fSubelement; // some detectors have many subelements (eg stripline have 4 antenas)
                        // some have only one sub element(eg lumis have one channel)

    TString fmoduletype; // eg: VQWK, SCALER
    TString fdetectorname;
    TString fdetectortype; // stripline, IntegrationPMT, ... this string is encoded by fTypeID

    std::vector<TString> fCombinedChannelNames;
    std::vector<Double_t> fWeight;

    void Print() const;

};


/**
 * \class VQwDetectorArray
 * \ingroup QwAnalysis_ADC
 * \brief Abstract base for arrays of PMT-like detectors
 *
 * Provides common functionality for subsystems composed of multiple
 * integration PMTs and combined PMTs, including normalization,
 * histogram/NTuple construction, and running statistics.
 */
class VQwDetectorArray: virtual public VQwSubsystemParity {

 /******************************************************************
 *  Class: QwDetectorArray
 *
 *
 ******************************************************************/

 private:

    /// Private default constructor (not implemented, will throw linker error on use)
    VQwDetectorArray();

 public:

    /// Constructor with name
    VQwDetectorArray(const TString& name)
     :VQwSubsystem(name),VQwSubsystemParity(name),bNormalization(kFALSE) {

        fTargetCharge.InitializeChannel("q_targ","derived");
        fTargetX.InitializeChannel("x_targ","derived");
        fTargetY.InitializeChannel("y_targ","derived");
        fTargetXprime.InitializeChannel("xp_targ","derived");
        fTargetYprime.InitializeChannel("yp_targ","derived");
        fTargetEnergy.InitializeChannel("e_targ","derived");

    };

    /// Copy constructor

    VQwDetectorArray(const VQwDetectorArray& source)
     :VQwSubsystem(source),VQwSubsystemParity(source),
     fIntegrationPMT(source.fIntegrationPMT),
     fCombinedPMT(source.fCombinedPMT),
     fMainDetID(source.fMainDetID){}

    /// Virtual destructor

    ~VQwDetectorArray() override { };

    /*  Member functions derived from VQwSubsystemParity. */

    /// \brief Define options function

    static void DefineOptions(QwOptions &options);


    void ProcessOptions(QwOptions &options) override;//Handle command line options
    Int_t LoadChannelMap(TString mapfile) override;
    Int_t LoadInputParameters(TString pedestalfile) override;
    void LoadEventCuts_Init() override {};
    void LoadEventCuts_Line(QwParameterFile &mapstr, TString &varvalue, Int_t &eventcut_flag) override;
    void LoadEventCuts_Fin(Int_t &eventcut_flag) override;
    Bool_t ApplySingleEventCuts() override;//Check for good events by setting limits on the devices readings

    Bool_t  CheckForBurpFail(const VQwSubsystem *subsys) override;

    void IncrementErrorCounters() override;
    void PrintErrorCounters() const override;// report number of events failed due to HW and event cut failure
    UInt_t GetEventcutErrorFlag() override;//return the error flag

    //update the error flag in the subsystem level from the top level routines related to stability checks. This will uniquely update the errorflag at each channel based on the error flag in the corresponding channel in the ev_error subsystem
    void UpdateErrorFlag(const VQwSubsystem *ev_error) override;


    Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
    Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;

    void  ClearEventData() override;
    Bool_t IsGoodEvent();

    void  ProcessEvent() override;
    void  ExchangeProcessedData() override;
    void  ProcessEvent_2() override;

    Bool_t PublishInternalValues() const override;
    Bool_t PublishByRequest(TString device_name) override;

    void  SetRandomEventParameters(Double_t mean, Double_t sigma);
    void  SetRandomEventAsymmetry(Double_t asymmetry);
    void  RandomizeEventData(int helicity = 0, Double_t time = 0.0) override;
    void  EncodeEventData(std::vector<UInt_t> &buffer) override;
    void  RandomizeMollerEvent(int helicity/*, const QwBeamCharge& charge, const QwBeamPosition& xpos, const QwBeamPosition& ypos, const QwBeamAngle& xprime, const QwBeamAngle& yprime, const QwBeamEnergy& energy*/);

    void  ConstructHistograms(TDirectory *folder) override{

        TString tmpstr("");
        ConstructHistograms(folder,tmpstr);
    };

    using VQwSubsystem::ConstructHistograms;
    void  ConstructHistograms(TDirectory *folder, TString &prefix) override;
    void  FillHistograms() override;

    using VQwSubsystem::ConstructBranchAndVector;
    void ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) override;
    void ConstructBranch(TTree *tree, TString &prefix) override;
    void ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& trim_file ) override;

    void  FillTreeVector(QwRootTreeBranchVector &values) const override;
#ifdef HAS_RNTUPLE_SUPPORT
    void  ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
    void  FillNTupleVector(std::vector<Double_t>& values) const override;
#endif // HAS_RNTUPLE_SUPPORT

#ifdef __USE_DATABASE__
    void  FillDB(QwParityDB *db, TString datatype) override;
    void  FillErrDB(QwParityDB *db, TString datatype) override;
#endif // __USE_DATABASE__

    const QwIntegrationPMT* GetChannel(const TString name) const;

    Bool_t Compare(VQwSubsystem* source);


    VQwSubsystem&  operator=  ( VQwSubsystem *value) override;
    VQwSubsystem&  operator+= ( VQwSubsystem *value) override;
    VQwSubsystem&  operator-= ( VQwSubsystem *value) override;


    void Ratio(VQwSubsystem* numer, VQwSubsystem* denom) override;
    void Scale(Double_t factor) override;
    void Normalize(VQwDataElement* denom);

    void AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
    //remove one entry from the running sums for devices
    void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF) override;
    void CalculateRunningAverage() override;

    const QwIntegrationPMT* GetIntegrationPMT(const TString name) const;
    const QwCombinedPMT* GetCombinedPMT(const TString name) const;

    void DoNormalization(Double_t factor=1.0);

    Bool_t ApplyHWChecks(){//Check for hardware errors in the devices

        Bool_t status = kTRUE;

        for (size_t i=0; i<fIntegrationPMT.size(); i++){

            status &= fIntegrationPMT.at(i).ApplyHWChecks();

        }

        return status;

    };

    void LoadMockDataParameters(TString pedestalfile) override;

    void  PrintValue() const override;
    void  WritePromptSummary(QwPromptSummary *ps, TString type) override;
    void  PrintInfo() const override;
    void  PrintDetectorID() const;


 protected:

    Bool_t fDEBUG;

    EQwPMTInstrumentType GetDetectorTypeID(TString name);

    // when the type and the name is passed the detector index from appropriate vector
    // will be returned. For example if TypeID is IntegrationPMT  then the index of
    // the detector from fIntegrationPMT vector for given name will be returned.
    Int_t GetDetectorIndex(EQwPMTInstrumentType TypeID, TString name);

    std::vector <QwIntegrationPMT> fIntegrationPMT;
    std::vector <QwCombinedPMT> fCombinedPMT;
    std::vector <QwDetectorArrayID> fMainDetID;




 /*
    Maybe have an array of QwIntegrationPMT to describe the Sector, Ring, Slice structure?  Maybe hold Ring 5 out and have it described as one list by Sector and slice?
	Need a way to define the correlations to all beam parameters for each element.
	Need a way to define asymmetries for each element.
	Need a way to create the full event data buffers
	Start with all channels and modules in a single ROC subbank:  make a mock_moller_adc.map with 28 8-channel modules with names like we discussed
	Make a new RandomizeEventData which will take the helicity and beam current and beam params, and fill the detector elements as we discussed.
 */


 protected:

    QwBeamCharge   fTargetCharge;
    QwBeamPosition fTargetX;
    QwBeamPosition fTargetY;
    QwBeamAngle    fTargetXprime;
    QwBeamAngle    fTargetYprime;
    QwBeamEnergy   fTargetEnergy;

    Bool_t bIsExchangedDataValid;

    Bool_t bNormalization;
    Double_t fNormThreshold;

 private:

  static const Bool_t bDEBUG=kFALSE;
  Int_t fMainDetErrorCount;

};
