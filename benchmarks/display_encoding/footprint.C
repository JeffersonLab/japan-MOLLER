// Measure the *serialized* byte footprint of a display object, i.e. the number
// of bytes a live TMapFile must hold for that object. Uniform across encodings
// (TTree / TGraph-tree / TProfile) via TBufferFile, so the three display styles
// are compared apples-to-apples.
//
//   mode = "TREE"    -> the per-pattern "mul" asymmetry tree
//   mode = "GRAPH"   -> the "burst_graphs" tree of per-burst TGraphs
//   mode = "PROFILE" -> the four benchtrend TProfiles (mul_histo scope)
//   mode = "BASE"    -> nothing (0 bytes)
//
// Prints a single line:  BYTES=<n> OBJS=<n>

#include "TFile.h"
#include "TBufferFile.h"
#include "TTree.h"
#include "TKey.h"
#include "TString.h"

static Long64_t ser(TObject* o) {
  if (!o) return 0;
  TBufferFile b(TBuffer::kWrite, 64000);
  b.WriteObject(o);
  return b.Length();
}

void footprint(const char* fname, const char* mode) {
  Long64_t bytes = 0; int objs = 0;
  TString m(mode);
  if (m == "BASE") { printf("BYTES=0 OBJS=0\n"); return; }
  TFile* f = TFile::Open(fname);
  if (!f || f->IsZombie()) { printf("BYTES=NA OBJS=0\n"); return; }
  if (m == "TREE") {
    TTree* t = (TTree*)f->Get("mul");
    if (t) { bytes = ser(t); objs = 1; }
  } else if (m == "GRAPH") {
    TTree* t = (TTree*)f->Get("burst_graphs");
    if (t) { bytes = ser(t); objs = 1; }
  } else if (m == "PROFILE") {
    const char* nm[4] = {"mul_histo/benchtrend_bcm1h02a",
                         "mul_histo/benchtrend_bcm1h15",
                         "mul_histo/benchtrend_bcm_target",
                         "mul_histo/benchtrend_bpm1h04X"};
    for (int i = 0; i < 4; i++) {
      TObject* o = f->Get(nm[i]);
      if (o) { bytes += ser(o); objs++; }
    }
  }
  printf("BYTES=%lld OBJS=%d\n", (long long)bytes, objs);
  f->Close();
}
