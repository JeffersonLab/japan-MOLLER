/*!
 * \file   VQwDataserver.h
 * \brief  Virtual base class for data server implementations
 */

#ifndef __VQwDataserver_h__
#define __VQwDataserver_h__

#include <iostream>

#include "VQwSystem.h"

/**
 * \class VQwDataserver
 * \ingroup QwAnalysis
 * \brief Abstract base for data server implementations
 *
 * Provides the interface for data server classes that handle external
 * data communication and publishing. Extends VQwSystem with server-specific
 * functionality for data distribution and client management.
 */
class VQwDataserver : public VQwSystem {

  private:
    VQwDataserver& operator= (const VQwDataserver &value) {
      if (this != &value) {
        // Private assignment operator - no implementation needed
      }
      return *this;
    };

  public:
    VQwDataserver (const char* name): VQwSystem (name) { };

    virtual void NextEvent() {
      std::cout << "Error: Not implemented!" << std::endl;
      return;
    };
    /*
    // TODO (wdc) I don't want this to depend on QwHitContainer...
    virtual QwHitContainer* GetHitList() {
      std::cout << "Error: Not implemented!" << std::endl;
      return 0;
    };
    */

};

#endif // __VQwAnalyzer_h__
