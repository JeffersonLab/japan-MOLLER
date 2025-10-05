/*!
 * \file   QwFakeHelicity.h
 * \brief  Fake helicity generator using pregenerated random seeds
 * \author B.Waidyawansa
 * \date   2010-03-06
 */
/**
  The QwFakeHelicity class uses a pregenerated random seed to generate
  the fake helicity signal that then can be used to perfrom helicity
  related calculations.
*/


#ifndef __QwFAKEHELICITY__
#define __QwFAKEHELICITY__

#include "QwHelicity.h"

class QwFakeHelicity: public QwHelicity {
 public:
  QwFakeHelicity(TString region_tmp):VQwSubsystem(region_tmp),QwHelicity(region_tmp),fMinPatternPhase(1)

    {
      // using the constructor of the base class
    };

    ~QwFakeHelicity() override { };

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

#endif
