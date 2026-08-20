// rootScripts/ci/skim_mul.C
#include "TFile.h"
#include "TTree.h"
#include "TKey.h"
#include "TDirectory.h"
#include "TSystem.h"
#include <cstdio>

void skim_mul(const char* inFile  = "trees/isu_sample_4.root",
              const char* outFile = "trees/isu_sample_4.slim.root",
              const char* pathToMul = "mul") {
  TFile* fin = TFile::Open(inFile, "READ");
  if (!fin || fin->IsZombie()) { fprintf(stderr,"Cannot open %s\n", inFile); gSystem->Exit(1); }

  TObject* obj = fin->Get(pathToMul);
  if (!obj)  { fprintf(stderr,"Could not find \"%s\" in %s\n", pathToMul, inFile); gSystem->Exit(2); }

  TTree* tin = dynamic_cast<TTree*>(obj);
  if (!tin)  { fprintf(stderr,"Object \"%s\" is not a TTree\n", pathToMul); gSystem->Exit(3); }

  printf("Found tree \"%s\" with %lld entries\n", tin->GetName(), (long long)tin->GetEntries());

  TFile* fout = TFile::Open(outFile, "RECREATE");
  if (!fout || fout->IsZombie()) { fprintf(stderr,"Cannot create %s\n", outFile); gSystem->Exit(4); }

  fout->cd();
  TTree* tout = tin->CloneTree(-1, "fast");
  if (!tout)  { fprintf(stderr,"CloneTree failed for \"%s\"\n", pathToMul); gSystem->Exit(5); }
  tout->SetDirectory(fout);
  tout->Write();

  if (tin->GetListOfFriends() && tin->GetListOfFriends()->GetSize() > 0) {
    printf("Note: original tree has friends; consider cloning them separately.\n");
  }

  fout->Write();
  fout->Close();
  fin->Close();
  printf("Wrote %s containing only \"%s\".\n", outFile, pathToMul);
}
