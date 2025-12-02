/*!
 * \file   QwFakeHelicity.h
 * \brief  Fake helicity generator using pregenerated random seeds
 * \author B.Waidyawansa
 * \date   2010-03-06
 */
/**
  The QwFakeHelicity class uses a pregenerated random seed to generate
  the fake helicity signal that then can be used to perform helicity
  related calculations.
*/

#pragma once

#include "QwHelicity.h"

/**
 * \class QwFakeHelicity
 * \ingroup QwAnalysis_BL
 * \brief Helicity source that generates a reproducible sequence from seeds
 *
 * Used for testing and simulations when real helicity decoding is not
 * available. Inherits the helicity interface and overrides event handling.
 */
class QwFakeHelicity: public QwHelicity {
 public:
  QwFakeHelicity(TString region_tmp):VQwSubsystem(region_tmp),QwHelicity(region_tmp),fMinPatternPhase(1)

    {
      // using the constructor of the base class
    };

    ~QwFakeHelicity() override { };

    /// Inherit assignment operator on base class
    using QwHelicity::operator=;

    void    ClearEventData() override;
    Bool_t  IsGoodHelicity() override;
    void    ProcessEvent() override;

    Bool_t CheckForBurpFail(const VQwSubsystem *subsys) override{
      return kFALSE;
    };

 protected:
    Int_t fMinPatternPhase;

    Bool_t CollectRandBits() override;
    UInt_t GetRandbit(UInt_t& ranseed) override;

};

// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwFakeHelicity);
