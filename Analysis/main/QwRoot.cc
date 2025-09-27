/*------------------------------------------------------------------------*//*!

 \file QwRoot.cc

 \ingroup QwAnalysis

 \brief ROOT wrapper with Qweak functionality

*//*-------------------------------------------------------------------------*/

// ROOT headers
#include <TSystem.h>
#include <TROOT.h>
#include <TString.h>

// Qweak headers
#include "QwRint.h"
#include "QwOptions.h"

int main(int argc, char** argv)
{
  // Start Qw-Root command prompt
  auto* qwrint = new QwRint("Qweak-Root Analyzer", &argc, argv);
  // Set some paths
  TString path = getenv_safe_TString("QWANALYSIS");
  gROOT->ProcessLine(".include " + path + "/Analysis/include");
  gROOT->ProcessLine(".include " + path + "/Parity/include");
  gROOT->ProcessLine(".include " + path + "/Tracking/include");
  #if ROOT_VERSION_CODE < ROOT_VERSION(6,0,0)
    gROOT->ProcessLine("gSystem->Load(\"libCint.so\");");
  #endif
  // Run the interface
  qwrint->Run();
  // Delete object
  delete qwrint;
}
