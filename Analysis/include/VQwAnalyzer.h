#pragma once

#include <iostream>

#include "VQwSystem.h"

class VQwAnalyzer : public VQwSystem {

  private:
    VQwAnalyzer& operator= (const VQwAnalyzer &value) {
      if (this != &value) {
        // Private assignment operator - no implementation needed
      }
      return *this;
    };

  public:
    VQwAnalyzer (const char* name): VQwSystem (name) { };

    /*
    virtual void SetHitList(QwHitContainer* hitlist) {
      std::cout << "Error: Not implemented!" << std::endl;
      return;
    };
    */
    virtual void Process() {
      std::cout << "Error: Not implemented!" << std::endl;
      return;
    };

};
