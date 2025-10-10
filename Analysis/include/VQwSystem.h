/*!
 * \file   VQwSystem.h
 * \brief  Virtual base class for all Qweak system objects
 */

#ifndef __VQwSystem_h__
#define __VQwSystem_h__

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

#endif // __VQwSystem_h__
