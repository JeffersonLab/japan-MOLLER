/**********************************************************\
* File: VQwHardwareChannel.h                               *
*                                                          *
* Author: P. King                                          *
* Date:   Tue Mar 29 13:08:12 EDT 2011                     *
\**********************************************************/

#pragma once

// System headers
#include <cmath>
#include <vector>
#include <stdexcept>

// Qweak headers
#include "VQwDataElement.h"

// ROOT headers
#ifdef HAS_RNTUPLE_SUPPORT
#include <ROOT/RNTupleModel.hxx>
#endif // HAS_RNTUPLE_SUPPORT

// ROOT forward declarations
class TTree;

// Qweak forward declarations
class QwDBInterface;
class QwParameterFile;
class QwErrDBInterface;
class QwRootTreeBranchVector;

/**
 * \class VQwHardwareChannel
 * \ingroup QwAnalysis
 * \brief Abstract base for concrete hardware channels implementing dual-operator pattern
 *
 * This class extends VQwDataElement to provide common services for hardware
 * channel implementations that represent single physical readouts (ADC channels,
 * scalers, etc.). It enforces the dual-operator architectural pattern at the
 * channel level and provides infrastructure for calibration, cuts, and statistics.
 *
 * \par Dual-Operator Pattern Implementation:
 * VQwHardwareChannel inherits the dual-operator requirement from VQwDataElement
 * and adds channel-specific enforcement. Derived classes must implement:
 *
 * **Required Operator Pairs:**
 * - `QwVQWK_Channel& operator+=(const QwVQWK_Channel&)` (type-specific)
 * - `VQwHardwareChannel& operator+=(const VQwHardwareChannel&)` (polymorphic)
 *
 * **Polymorphic Delegation Pattern:**
 * \code
 * VQwHardwareChannel& QwVQWK_Channel::operator+=(const VQwHardwareChannel& source) {
 *   const QwVQWK_Channel* tmpptr = dynamic_cast<const QwVQWK_Channel*>(&source);
 *   if (tmpptr != NULL) {
 *     *this += *tmpptr;  // Calls type-specific version
 *   } else {
 *     throw std::invalid_argument("Type mismatch in operator+=");
 *   }
 *   return *this;
 * }
 * \endcode
 *
 * \par Assignment + Operators Pattern:
 * Sum/Difference methods follow the canonical pattern:
 * \code
 * void Sum(const QwVQWK_Channel& value1, const QwVQWK_Channel& value2) {
 *   *this = value1;      // Uses derived class assignment operator
 *   *this += value2;     // Uses type-specific operator+=
 * }
 * \endcode
 *
 * \par Channel Infrastructure:
 * - **Calibration**: Pedestal subtraction and gain application
 * - **Event Cuts**: Single-event limits with error flag propagation
 * - **Statistics**: Running sums with error mask support
 * - **Hardware Checks**: Burp detection and error counting
 * - **Subelements**: Support for multi-element channels
 *
 * \par Representative Example:
 * QwVQWK_Channel provides the canonical implementation demonstrating:
 * - Complete dual-operator pattern with proper delegation
 * - Six-word VQWK data processing (blocks 0-5)
 * - Pedestal/calibration application
 * - Single-event cuts and error propagation
 * - Histogram and tree branch construction
 *
 * \par Error Handling Strategy:
 * - Type mismatches in operators throw std::invalid_argument
 * - Hardware errors set device-specific error codes
 * - Event cuts use configurable upper/lower limits
 * - Burp detection compares against reference channels
 */
class VQwHardwareChannel: public VQwDataElement {
/****************************************************************//**
 *  Class: VQwHardwareChannel
 *         Virtual base class to support common functions for all
 *         hardware channel data elements.
 *         Only the data element classes which contain raw data
 *         from one physical channel (such as QwVQWK_Channel,
 *         QwScaler_Channel, etc.) should inherit from this class.
 ******************************************************************/
public:
  VQwHardwareChannel();
  VQwHardwareChannel(const VQwHardwareChannel& value);
  VQwHardwareChannel(const VQwHardwareChannel& value, VQwDataElement::EDataToSave datatosave);
  ~VQwHardwareChannel() override { };

  void CopyFrom(const VQwHardwareChannel& value);

  void ProcessOptions();

  virtual VQwHardwareChannel* Clone() const{
    return Clone(this->fDataToSave);
  };
  virtual VQwHardwareChannel* Clone(VQwDataElement::EDataToSave datatosave) const = 0;

  using VQwDataElement::UpdateErrorFlag;

  /*! \brief Get the number of data words in this data element */
  size_t GetNumberOfDataWords() {return fNumberOfDataWords;}

  /*! \brief Get the number of subelements in this data element */
  size_t GetNumberOfSubelements() {return fNumberOfSubElements;};

  Int_t GetRawValue() const       {return this->GetRawValue(0);};
  Double_t GetValue() const       {return this->GetValue(0);};
  Double_t GetValueM2() const     {return this->GetValueM2(0);};
  Double_t GetValueError() const  {return this->GetValueError(0);};
  Double_t GetValueWidth() const  {return this->GetValueWidth(0);};
  virtual Int_t GetRawValue(size_t element) const = 0;
  virtual Double_t GetValue(size_t element) const = 0;
  virtual Double_t GetValueM2(size_t element) const = 0;
  virtual Double_t GetValueError(size_t element) const = 0;
  Double_t GetValueWidth(size_t element) const {
    RangeCheck(element);
    Double_t width;
    if (fGoodEventCount>0){
      width = (GetValueError(element)*std::sqrt(Double_t(fGoodEventCount)));
    } else {
      width = 0.0;
    }
    return width;
  };

  void  ClearEventData() override{
    VQwDataElement::ClearEventData();
  };

  /*   virtual void AddChannelOffset(Double_t Offset) = 0; */
  virtual void Scale(Double_t Offset) = 0;


  /// \brief Initialize the fields in this object
  void  InitializeChannel(TString name){InitializeChannel(name, "raw");};
  virtual void  InitializeChannel(TString name, TString datatosave) = 0;
  virtual void  InitializeChannel(TString subsystem, TString instrumenttype, TString name, TString datatosave) = 0;

  //Check for hardware errors in the devices. This will return the device error code.
  virtual Int_t ApplyHWChecks() = 0;

  void SetEventCutMode(Int_t bcuts){bEVENTCUTMODE=bcuts;};

  virtual Bool_t ApplySingleEventCuts() = 0;//check values read from modules are at desired level

  virtual Bool_t CheckForBurpFail(const VQwHardwareChannel *event){
    Bool_t foundburp = kFALSE;
    if (fBurpThreshold>0){
      Double_t diff = this->GetValue() - event->GetValue();
      if (fabs(diff)>fBurpThreshold){
	      foundburp = kTRUE;
	      fBurpCountdown = fBurpHoldoff;
      } else if (fBurpCountdown>0) {
	      foundburp = kTRUE;
	      fBurpCountdown--;
      }
    }
    if (foundburp){
      fErrorFlag |= kErrorFlag_BurpCut;
    }

    return foundburp;
  }

  /*! \brief Set the upper and lower limits (fULimit and fLLimit)
   *         for this channel */
  void SetSingleEventCuts(Double_t min, Double_t max);
  /*! \brief Inherited from VQwDataElement to set the upper and lower
   *         limits (fULimit and fLLimit), stability % and the
   *         error flag on this channel */
  void SetSingleEventCuts(UInt_t errorflag,Double_t min, Double_t max, Double_t stability=-1.0, Double_t BurpLevel=-1.0);

  Double_t GetEventCutUpperLimit() const { return fULimit; };
  Double_t GetEventCutLowerLimit() const { return fLLimit; };

  Double_t GetStabilityLimit() const { return fStability;};

  UInt_t UpdateErrorFlag() override {return GetEventcutErrorFlag();};
  void UpdateErrorFlag(const VQwHardwareChannel& elem){fErrorFlag |= elem.fErrorFlag;};
  virtual UInt_t GetErrorCode() const {return (fErrorFlag);};

  virtual  void IncrementErrorCounters()=0;
  virtual  void  ProcessEvent()=0;


  virtual void CalculateRunningAverage() = 0;
//   virtual void AccumulateRunningSum(const VQwHardwareChannel *value) = 0;

  /// Arithmetic assignment operator:  Should only copy event-based data
  VQwHardwareChannel& operator=(const VQwHardwareChannel& value) {
    if (this != &value) {
      VQwDataElement::operator=(value);
    }
    return *this;
  }
  void AssignScaledValue(const VQwHardwareChannel &value, Double_t scale){
     AssignValueFrom(&value);
     Scale(scale);
  };
  void Ratio(const VQwHardwareChannel* numer, const VQwHardwareChannel* denom){
    throw std::runtime_error(std::string("VQwHardwareChannel::Ratio not implemented for ") + GetElementName().Data());
  }

  void AssignValueFrom(const VQwDataElement* valueptr) override = 0;
  virtual VQwHardwareChannel& operator+=(const VQwHardwareChannel& input) = 0;
  virtual VQwHardwareChannel& operator-=(const VQwHardwareChannel& input) = 0;
  virtual VQwHardwareChannel& operator*=(const VQwHardwareChannel& input) = 0;
  virtual VQwHardwareChannel& operator/=(const VQwHardwareChannel& input) = 0;


  virtual void ScaledAdd(Double_t scale, const VQwHardwareChannel *value) = 0;

  void     SetPedestal(Double_t ped) { fPedestal = ped; kFoundPedestal = 1; };
  Double_t GetPedestal() const       { return fPedestal; };
  void     SetCalibrationFactor(Double_t factor) { fCalibrationFactor = factor; kFoundGain = 1; };
  Double_t GetCalibrationFactor() const          { return fCalibrationFactor; };

  void AddEntriesToList(std::vector<QwDBInterface> &row_list);
  virtual void AddErrEntriesToList(std::vector<QwErrDBInterface> & /*row_list*/) {};


  virtual void AccumulateRunningSum(const VQwHardwareChannel *value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF){
    if(count==0){
      count = value->fGoodEventCount;
    }
    if(ErrorMask ==  kPreserveError){QwError << "VQwHardwareChannel count=" << count << QwLog::endl;}
    AccumulateRunningSum(value, count, ErrorMask);
  };
  virtual void DeaccumulateRunningSum(const VQwHardwareChannel *value, Int_t ErrorMask=0xFFFFFFF){
    AccumulateRunningSum(value, -1, ErrorMask);
  };

  virtual void AddValueFrom(const VQwHardwareChannel* valueptr) = 0;
  virtual void SubtractValueFrom(const VQwHardwareChannel* valueptr) = 0;
  virtual void MultiplyBy(const VQwHardwareChannel* valueptr) = 0;
  virtual void DivideBy(const VQwHardwareChannel* valueptr) = 0;

  virtual void ConstructBranchAndVector(TTree *tree, TString& prefix, QwRootTreeBranchVector& values) = 0;
  virtual void ConstructBranch(TTree *tree, TString &prefix) = 0;
  void ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist);
  virtual void FillTreeVector(QwRootTreeBranchVector& values) const = 0;
#ifdef HAS_RNTUPLE_SUPPORT
  virtual void ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) = 0;
  virtual void FillNTupleVector(std::vector<Double_t>& values) const = 0;
#endif // HAS_RNTUPLE_SUPPORT

  virtual void CopyParameters(const VQwHardwareChannel* /*valueptr*/){};

 protected:
  /*! \brief Set the number of data words in this data element */
  void SetNumberOfDataWords(const UInt_t &numwords) {fNumberOfDataWords = numwords;}
  /*! \brief Set the number of data words in this data element */
  void SetNumberOfSubElements(const size_t elements) {fNumberOfSubElements = elements;};

  /*! \brief Set the flag indicating if raw or derived values are
   *         in this data element */
  void SetDataToSave(TString datatosave) {
    if      (datatosave == "raw")
      fDataToSave = kRaw;
    else if (datatosave == "derived")
      fDataToSave = kDerived;
    else
      fDataToSave = kRaw; // wdc, added default fall-through
  }
  /*! \brief Set the flag indicating if raw or derived values are
   *         in this data element */
  void SetDataToSave(VQwDataElement::EDataToSave datatosave) {
    fDataToSave = datatosave;
  }
  /*! \brief Set the flag indicating if raw or derived values are
   *         in this data element based on prefix */
  void SetDataToSaveByPrefix(const TString& prefix) {
    if (prefix.Contains("asym_")
     || prefix.Contains("diff_")
     || prefix.Contains("yield_"))
      fDataToSave = kDerived;
    if (prefix.Contains("stat"))
      fDataToSave = kMoments; // stat has priority
  }

  /*! \brief Checks that the requested element is in range, to be
   *         used in accesses to subelements similar to
   *         std::vector::at(). */
  void RangeCheck(size_t element) const {
    if (element >= fNumberOfSubElements){
      TString loc="VQwDataElement::RangeCheck for "
	+this->GetElementName()+" failed for subelement "+Form("%zu",element);
      throw std::out_of_range(loc.Data());

    }
  };

protected:
  UInt_t  fNumberOfDataWords; ///< Number of raw data words in this data element
  UInt_t  fNumberOfSubElements; ///< Number of subelements in this data element

  EDataToSave fDataToSave;

  /*  Ntuple array indices */
  size_t fTreeArrayIndex;
  size_t fTreeArrayNumEntries;

  /*! \name Channel calibration                    */
  // @{
  Double_t fPedestal; /*!< Pedestal of the hardware sum signal,
			   we assume the pedestal level is constant over time
			   and can be divided by four for use with each block,
			   units: [counts / number of samples] */
  Double_t fCalibrationFactor;
  Bool_t kFoundPedestal;
  Bool_t kFoundGain;
  //@}

  /*! \name Single event cuts and errors                    */
  // @{
  Int_t bEVENTCUTMODE;/*!<If this set to kFALSE then Event cuts are OFF*/
  Double_t fULimit, fLLimit;/*!<this sets the upper and lower limits*/
  Double_t fStability;/*!<how much deviaton from the stable reading is allowed*/

  Double_t fBurpThreshold;
  Int_t fBurpCountdown;
  //@}


public:
  /*! \name Global event cuts */
  static void SetBurpHoldoff(Int_t holdoff) {
    fBurpHoldoff = holdoff;
  }

protected:
  // @{
  static Int_t fBurpHoldoff;
  // @}

};   // class VQwHardwareChannel
