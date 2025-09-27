/**********************************************************\
* File: QwCombinedBCM.h                                  *
* File: QwBCM.h                                           *
*                                                         *
* Author:                                                 *
* Time-stamp:                                             *
\**********************************************************/

#ifndef __Qw_COMBINEDBCM__
#define __Qw_COMBINEDBCM__

// System headers
#include <vector>

// Boost math library for random number generation
#include "boost/random.hpp"

// ROOT headers
#include <TTree.h>

// Qweak headers
#include "VQwDataElement.h"
#include "VQwHardwareChannel.h"
#include "QwBCM.h"

// Forward declarations
#ifdef __USE_DATABASE__
class QwDBInterface;
#endif // __USE_DATABASE__

/*****************************************************************
*  Class:
******************************************************************/

template<typename T>
class QwCombinedBCM : public QwBCM<T> {
/////
 public:
  QwCombinedBCM() { };
  QwCombinedBCM(TString name){
    InitializeChannel(name, "derived");
  };
  QwCombinedBCM(TString subsystem, TString name){
    InitializeChannel(subsystem, name, "derived");
  };
  QwCombinedBCM(TString subsystemname, TString name, TString type){
    this->SetSubsystemName(subsystemname);
    InitializeChannel(subsystemname, name,type,"raw");
  };
  QwCombinedBCM(const QwCombinedBCM& source)
  : QwBCM<T>(source)
  { }
  ~QwCombinedBCM() override { };

  // This is to setup one of the used BCM's in this combo
  void SetBCMForCombo(VQwBCM* bcm, Double_t weight, Double_t sumqw ) override;

  // No processing of event buffer
  Int_t ProcessEvBuffer(UInt_t* buffer, UInt_t word_position_in_buffer, UInt_t subelement = 0) override { return 0; };

  void  InitializeChannel(TString name, TString datatosave) override;
  // new routine added to update necessary information for tree trimming
  void  InitializeChannel(TString subsystem, TString name, TString datatosave) override;
  void  InitializeChannel(TString subsystem, TString name, TString type,
      TString datatosave);

  void  ProcessEvent() override;

//---------------------------------------------------------------------------------------------
  void    GetProjectedCharge(VQwBCM *device) override;
  void    RandomizeEventData(int helicity = 0, double time = 0.0) override;
  size_t  GetNumberOfElements() override {return fElement.size();};
  TString GetSubElementName(Int_t index) override {return fElement.at(index)->GetElementName();};
  void    LoadMockDataParameters(QwParameterFile &paramfile) override;
//---------------------------------------------------------------------------------------------

  Bool_t ApplyHWChecks(){
    return kTRUE;
  };

  Bool_t ApplySingleEventCuts() override;//Check for good events by stting limits on the devices readings

  UInt_t UpdateErrorFlag() override;


  // Implementation of Parent class's virtual operators
  VQwBCM& operator=  (const VQwBCM &value) override;
  QwCombinedBCM& operator=  (const QwCombinedBCM &value);

  /*
  void AccumulateRunningSum(const VQwBCM &value);
  void DeaccumulateRunningSum(VQwBCM &value);
  void CalculateRunningAverage();
  */
  void SetPedestal(Double_t ped) override {
    QwBCM<T>::SetPedestal(0.0);
  }
  void SetCalibrationFactor(Double_t calib) override {
    QwBCM<T>::SetCalibrationFactor(1.0);
  }

  VQwHardwareChannel* GetCharge() override{
    return &(this->fBeamCurrent);
  };

  const VQwHardwareChannel* GetCharge() const override {
    return const_cast<QwCombinedBCM*>(this)->GetCharge();
  };

/////
 private:

  std::vector <QwBCM<T>*> fElement;
  std::vector <Double_t>  fWeights;
  Double_t fSumQweights;

  Double_t fLastTripTime;
  Double_t fTripPeriod;
  Double_t fTripLength;
  Double_t fTripRamp;
  Double_t fProbabilityOfTrip;

 protected:
  /// \name Parity mock data generation
  // @{
  /// Internal randomness generator
  static boost::mt19937 fRandomnessGenerator;
  /// Internal normal probability distribution
  static boost::random::uniform_real_distribution<double> fDistribution;
  /// Internal normal random variable
  static boost::variate_generator
    < boost::mt19937, boost::random::uniform_real_distribution<double> > fRandomVariable;
public: 
  static void SetTripSeed(uint seedval);
  // @}
};

#endif
