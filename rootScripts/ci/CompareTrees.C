// Fail (exit 1) if ANY branch stats differ (n, mean, or rms) between two files.
//root -l -b -q 'CompareTrees.C("./isu_sample_8c95699_4.root","./isu_sample_pr225_4.root","mul","diff_")'
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TString.h>
#include <TMath.h>
#include <cstdio>

void CompareTrees(const char* fileRef,
                        const char* filePR,
                        const char* tree   = "mul",
                        const char* prefix = "diff_")
{
  auto f1 = TFile::Open(fileRef,"READ");
  auto f2 = TFile::Open(filePR,"READ");
  if (!f1 || f1->IsZombie() || !f2 || f2->IsZombie()) { fprintf(stderr,"ERROR: open files\n"); gSystem->Exit(1); }

  auto t1 = (TTree*)f1->Get(tree);
  auto t2 = (TTree*)f2->Get(tree);
  if (!t1 || !t2) { fprintf(stderr,"ERROR: missing tree '%s'\n", tree); gSystem->Exit(1); }

  bool fail = false;

  TIter it(t1->GetListOfBranches());
  while (auto* br = (TBranch*)it()) {
    TString b = br->GetName();
    if (!b.BeginsWith(prefix)) continue;

    Long64_t n1 = t1->Draw(b, "", "goff"); const double* v1 = t1->GetV1();
    Long64_t n2 = t2->Draw(b, "", "goff"); const double* v2 = t2->GetV1();

    double m1 = (n1>0 && v1) ? TMath::Mean(n1, v1) : 0.0;
    double r1 = (n1>0 && v1) ? TMath::RMS (n1, v1) : 0.0;
    double m2 = (n2>0 && v2) ? TMath::Mean(n2, v2) : 0.0;
    double r2 = (n2>0 && v2) ? TMath::RMS (n2, v2) : 0.0;

    bool bad = (n1 != n2) || (m1 != m2) || (r1 != r2);

    printf("-- %s --\n", b.Data());
    printf("  ref: n=%lld mean=%.10g rms=%.10g\n", (long long)n1, m1, r1);
    printf("  pr: n=%lld mean=%.10g rms=%.10g%s\n",
           (long long)n2, m2, r2, bad ? "  <-- DIFF" : "");

    if (bad) fail = true;
  }

  if (fail) { fprintf(stderr,"compare: DIFFERENCES FOUND -> FAIL\n"); gSystem->Exit(1); }
  printf("compare: IDENTICAL -> OK\n");
}
