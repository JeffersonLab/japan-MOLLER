/*!
 * \file   VQwSystem.h
 * \brief  Virtual base class for all Qweak system objects
 */

#pragma once

#include "TROOT.h"
#include "TNamed.h"

/**
 * \class VQwSystem
 * \ingroup QwAnalysis
 * \brief Base class for all named Qweak system objects
 *
 * Provides ROOT TNamed functionality for all framework objects
 * that need naming and identification within the analysis system.
 */
class VQwSystem : public TNamed {

  public:
    VQwSystem (const char* name): TNamed (name, "Qweak-ROOT") { };
    virtual ~VQwSystem() { };

};
