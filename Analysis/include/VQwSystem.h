#pragma once

#include "TROOT.h"
#include "TNamed.h"

class VQwSystem : public TNamed {

  public:
    VQwSystem (const char* name): TNamed (name, "Qweak-ROOT") { };
    virtual ~VQwSystem() { };

};
