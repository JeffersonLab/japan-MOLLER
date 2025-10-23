/*!
 * \file   VQwSubsystemParity.h
 * \brief  Virtual base class for parity analysis subsystems
 * \author P. M. King, Rakitha Beminiwattha
 * \date   2007-05-08
 */

#pragma once

// ROOT headers
#include <TTree.h>

// Qweak headers
#include "VQwSubsystem.h"
#include "QwParameterFile.h"

// Forward declarations
class QwBlinder;
class QwParityDB;
class QwPromptSummary;


/**
 * \class VQwSubsystemParity
 * \ingroup QwAnalysis
 * \brief Abstract base class for subsystems participating in parity analysis
 *
 * Extends VQwSubsystem with parity-specific capabilities including asymmetry
 * formation, blinding support, database output, running sum accumulation,
 * and event cuts. Provides the contract for subsystems that contribute to
 * parity violation measurements and helicity-based analyses.
 */
class VQwSubsystemParity: virtual public VQwSubsystem {

  private:
    /// Private default constructor (not implemented, will throw linker error on use)
    VQwSubsystemParity();

  public:
    /// Constructor with name
    VQwSubsystemParity(const TString& name): VQwSubsystem(name) {
      SetEventTypeMask(0x1); // only accept 0x1
    };
    /// Copy constructor
    VQwSubsystemParity(const VQwSubsystemParity& source)
    : VQwSubsystem(source)
    { }
    /// Default destructor
    ~VQwSubsystemParity() override { };

    /// \brief Fill the database with MPS-based variables
    ///        Note that most subsystems don't need to do this.
    virtual void FillDB_MPS(QwParityDB * /*db*/, TString /*type*/) {};
    /// \brief Fill the database
    virtual void FillDB(QwParityDB * /*db*/, TString /*type*/) { };
    virtual void FillErrDB(QwParityDB * /*db*/, TString /*type*/) { };

    // VQwSubsystem routine is overridden. Call it at the beginning by VQwSubsystem::operator=(value)
    VQwSubsystem& operator=  (VQwSubsystem *value) override = 0;
    virtual VQwSubsystem& operator+= (VQwSubsystem *value) = 0;
    virtual VQwSubsystem& operator-= (VQwSubsystem *value) = 0;
    virtual void Sum(VQwSubsystem *value1, VQwSubsystem *value2)
    {
      if(Compare(value1)&&Compare(value2)){
        *this = value1;
        *this += value2;
      }
    };
    virtual void Difference(VQwSubsystem *value1, VQwSubsystem *value2)
    {
      if(Compare(value1)&&Compare(value2)){
        *this = value1;
        *this -= value2;
      }
    };
    virtual void Ratio(VQwSubsystem *numer, VQwSubsystem *denom) = 0;
    virtual void Scale(Double_t factor) = 0;

    /// \brief Update the running sums for devices
    virtual void AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) = 0;
    /// \brief remove one entry from the running sums for devices
    virtual void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF) = 0;

    /// \brief Calculate the average for all good events
    virtual void CalculateRunningAverage() = 0;

    /// \brief Load the event cuts file
    Int_t LoadEventCuts(TString filename) override{
      Int_t eventcut_flag = 1;

      // Open the file
      QwParameterFile mapstr(filename.Data());
      fDetectorMaps.insert(mapstr.GetParamFileNameContents());
      this->LoadEventCuts_Init();

      while (mapstr.ReadNextLine()){
        mapstr.TrimComment('!');   // Remove everything after a '!' character.
        mapstr.TrimWhitespace();   // Get rid of leading and trailing spaces.
        if (mapstr.LineIsEmpty())
          continue;
        TString varname, varvalue;
        if (mapstr.HasVariablePair("=",varname,varvalue)){
          if (varname == "EVENTCUTS"){
	          eventcut_flag = QwParameterFile::GetUInt(varvalue);
          }
        } else {
          this->LoadEventCuts_Line(mapstr, varvalue, eventcut_flag);
        }
      }

      this->LoadEventCuts_Fin(eventcut_flag);
      mapstr.Close();
      return 0;
    };
    virtual void LoadEventCuts_Init() {};
    virtual void LoadEventCuts_Line(QwParameterFile & /*mapstr*/, TString & /*varvalue*/, Int_t & /*eventcut_flag*/) {};
    virtual void LoadEventCuts_Fin(Int_t & /*eventcut_flag*/) {};
    /// \brief Apply the single event cuts
    virtual Bool_t ApplySingleEventCuts() = 0;
    /// \brief Report the number of events failed due to HW and event cut failures

    virtual Bool_t CheckForBurpFail(const VQwSubsystem *subsys)=0;

    virtual void PrintErrorCounters() const = 0;
    /// \brief Increment the error counters
    virtual void IncrementErrorCounters() = 0;

    /// \brief Return the error flag to the top level routines related to stability checks and ErrorFlag updates
    virtual UInt_t GetEventcutErrorFlag() = 0;
    /// \brief Uses the error flags of contained data elements to update
    ///        Returns the error flag to the top level routines related
    ///        to stability checks and ErrorFlag updates
    virtual UInt_t UpdateErrorFlag(){return GetEventcutErrorFlag();};

    /// \brief update the error flag in the subsystem level from the top level routines related to stability checks. This will uniquely update the errorflag at each channel based on the error flag in the corresponding channel in the ev_error subsystem
    virtual void UpdateErrorFlag(const VQwSubsystem *ev_error) = 0;


    /// \brief Blind the asymmetry of this subsystem
    virtual void Blind(const QwBlinder * /*blinder*/) { return; };
    /// \brief Blind the difference of this subsystem
    virtual void Blind(const QwBlinder * /*blinder*/, const VQwSubsystemParity* /*subsys*/) { return; };

    /// \brief Print values of all channels
    virtual void PrintValue() const { };

    virtual void WritePromptSummary(QwPromptSummary * /*ps*/, TString /*type*/) {};


    virtual Bool_t CheckForEndOfBurst() const {return kFALSE;};

    virtual void LoadMockDataParameters(TString /*mapfile*/) {};

}; // class VQwSubsystemParity
