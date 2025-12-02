/*!
 * \file   LRBCorrector.h
 * \brief  Linear regression blue corrector data handler class
 * \author Michael Vallee
 * \date   2018-08-01
 */

#pragma once

// Parent Class
#include "VQwDataHandler.h"


/**
 * \class LRBCorrector
 * \ingroup QwAnalysis_BL
 * \brief Linear-regression corrector applying per-burst slopes to data
 *
 * Loads cycle-dependent sensitivities and applies linear regression
 * corrections to monitored channels, selecting the appropriate set
 * based on the current burst counter.
 */
class LRBCorrector : public VQwDataHandler, public MQwDataHandlerCloneable<LRBCorrector>{
 public:
    /// \brief Constructor with name
    LRBCorrector(const TString& name);

    void ParseConfigFile(QwParameterFile& file) override;

    Int_t LoadChannelMap(const std::string& mapfile) override;

    Int_t ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override;

    void ProcessData() override;

    void UpdateBurstCounter(Short_t burstcounter) override{
      if (burstcounter<fLastCycle){
	fBurstCounter=burstcounter;
      } else if (fLastCycle==1){
	//  If we have a single cycle of slopes, don't throw a warning.
	fBurstCounter = 0;
      } else {
	fBurstCounter = fLastCycle-1;
	QwWarning << "LRBCorrector, " << GetName()
		  << ": Burst counter, " << burstcounter
		  << ", is greater than the stored number of sets of slopes.  "
		  << "Using the last set of slopes (cycle=" << fLastCycle
		  << ")" << QwLog::endl;
      }
    };

  protected:
    LRBCorrector() { }

    std::string fAlphaFileBase;
    std::string fAlphaFileSuff;
    std::string fAlphaFilePath;

    std::vector< EQwHandleType > fIndependentType;
    std::vector< std::string > fIndependentName;
    std::vector< std::string > fIndependentFull;

    std::vector< const VQwHardwareChannel* > fIndependentVar;
    std::vector< Double_t > fIndependentValues;

    Short_t fLastCycle;
    std::map<Short_t,std::vector<std::vector<Double_t>>> fSensitivity;

};

// Register this handler with the factory
REGISTER_DATA_HANDLER_FACTORY(LRBCorrector);
