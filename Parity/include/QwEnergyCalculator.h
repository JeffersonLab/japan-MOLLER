/*!
 * \file   QwEnergyCalculator.h
 * \brief  Beam energy calculation from BPM position measurements
 *
 * \author B.Waidyawansa
 * \date   05-24-2010
 */

#pragma once

// System headers
#include <vector>

// Root headers
#include <TTree.h>

// RNTuple headers
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTupleModel.hxx"
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#include "QwVQWK_Channel.h"
#include "QwBPMStripline.h"
#include "QwCombinedBPM.h"
#include "VQwBPM.h"
#include "VQwHardwareChannel.h"
#include "QwMollerADC_Channel.h"

// Forward declarations
#ifdef __USE_DATABASE__
class QwDBInterface;
#endif // __USE_DATABASE__

/**
 * \class QwEnergyCalculator
 * \ingroup QwAnalysis_BL
 * \brief Computes beam energy change from BPM information
 *
 * Uses measured angles and dispersive positions to estimate relative
 * beam energy variations; exposes a single hardware-like channel.
 */
class QwEnergyCalculator : public VQwDataElement{
  /******************************************************************
   *  Class:QwEnergyCalculator
   *         Performs the beam energy calculation using the beam angle
   *         amnd position at the target and the beam position at the
   *         highest dispersive region on the beamline.
   *
   ******************************************************************/


 public:
  // Default constructor
  QwEnergyCalculator() { };
  QwEnergyCalculator(TString name){
    InitializeChannel(name,"derived");
  };
  QwEnergyCalculator(TString subsystem, TString name){
    InitializeChannel(subsystem, name,"derived");
  };
  QwEnergyCalculator(const QwEnergyCalculator& source)
  : VQwDataElement(source),fEnergyChange(source.fEnergyChange)
  { }
  ~QwEnergyCalculator() override { };

    void    InitializeChannel(TString name,TString datatosave);
    // new routine added to update necessary information for tree trimming
    void    InitializeChannel(TString subsystem, TString name, TString datatosave);

    void    LoadChannelParameters(QwParameterFile &paramfile) override{};

//------------------------------------------------------------------------------------
    void    SetRandomEventParameters(Double_t mean, Double_t sigma);
    void    RandomizeEventData(int helicity = 0, double time = 0.0);
    void    GetProjectedPosition(VQwBPM *device);
    size_t  GetNumberOfElements() {return fDevice.size();};
    TString GetSubElementName(Int_t index) {return fDevice.at(index)->GetElementName();};
    void    LoadMockDataParameters(QwParameterFile &paramfile) override;
//------------------------------------------------------------------------------------

    void    ClearEventData() override;
    Int_t   ProcessEvBuffer(UInt_t* buffer,
			    UInt_t word_position_in_buffer,UInt_t indexnumber) override;
    void    ProcessEvent();
    void    PrintValue() const override;
    void    PrintInfo() const override;
    void    SetRootSaveStatus(TString &prefix);

    Bool_t  ApplyHWChecks();//Check for hardware errors in the devices
    Bool_t  ApplySingleEventCuts();//Check for good events by setting limits on the devices readings
    Int_t   SetSingleEventCuts(Double_t mean, Double_t sigma);//two limits and sample size
    /*! \brief Inherited from VQwDataElement to set the upper and lower limits (fULimit and fLLimit), stability % and the error flag on this channel */
    void    SetSingleEventCuts(UInt_t errorflag,Double_t min, Double_t max, Double_t stability, Double_t burplevel);
    void    SetEventCutMode(Int_t bcuts){
      bEVENTCUTMODE=bcuts;
      fEnergyChange.SetEventCutMode(bcuts);
    }
    Bool_t CheckForBurpFail(const VQwDataElement *ev_error);

    void    IncrementErrorCounters();
    void    PrintErrorCounters() const override;// report number of events failed due to HW and event cut failure
    UInt_t   GetEventcutErrorFlag() override{//return the error flag
      return fEnergyChange.GetEventcutErrorFlag();
    }
    UInt_t   UpdateErrorFlag() override;

    void    UpdateErrorFlag(const QwEnergyCalculator *ev_error);


    void    Set(const VQwBPM* device,TString type, TString property ,Double_t tmatrix_ratio);
    void    Ratio(QwEnergyCalculator &numer,QwEnergyCalculator &denom);
    void    Scale(Double_t factor);

    QwEnergyCalculator& operator=  (const QwEnergyCalculator &value);
    QwEnergyCalculator& operator+= (const QwEnergyCalculator &value);
    QwEnergyCalculator& operator-= (const QwEnergyCalculator &value);
    void Sum(const QwEnergyCalculator &value1, const QwEnergyCalculator &value2);
    void Difference(const QwEnergyCalculator &value1, const QwEnergyCalculator &value2);

    void    AccumulateRunningSum(const QwEnergyCalculator& value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
    void    DeaccumulateRunningSum(QwEnergyCalculator& value, Int_t ErrorMask=0xFFFFFFF);
    void    CalculateRunningAverage();

    void    ConstructHistograms(TDirectory *folder, TString &prefix) override;
    void    FillHistograms() override;

    void    ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values);
    void    ConstructBranch(TTree *tree, TString &prefix);
    void    ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& trim_file);
    void    FillTreeVector(QwRootTreeBranchVector &values) const;

#ifdef HAS_RNTUPLE_SUPPORT
    void    ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs);
    void    FillNTupleVector(std::vector<Double_t>& values) const;
#endif // HAS_RNTUPLE_SUPPORT

    VQwHardwareChannel* GetEnergy(){
      return &fEnergyChange;
    };

    const VQwHardwareChannel* GetEnergy() const{
      return const_cast<QwEnergyCalculator*>(this)->GetEnergy();
    };

#ifdef __USE_DATABASE__
    std::vector<QwDBInterface> GetDBEntry();
    std::vector<QwErrDBInterface> GetErrDBEntry();
#endif // __USE_DATABASE__

 protected:
    QwMollerADC_Channel fEnergyChange;


 private:
    std::vector <const VQwBPM*> fDevice;
    std::vector <Double_t> fTMatrixRatio;
    std::vector <TString>  fProperty;
    std::vector <TString>  fType;
    Bool_t bEVENTCUTMODE;//If this set to kFALSE then Event cuts do not depend on HW checks. This is set externally through the qweak_beamline_eventcuts.map
    Bool_t   bFullSave; // used to restrict the amount of data histogramed



};
