#include "TCanvas.h"
// #include "TCut.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"
// #include "TLeaf.h"
#include "TTree.h"

#include <cstring>
#include <iostream>
// #import <vector>

void unimplemented() {
  std::cout << "This option is not yet implemented" << std::endl;
}

void tq_asym_compare(const char *rootfile = "isu_sample_4.root",
                     const char *elem1 = "asym_tq02_r5c",
                     const char *elem2 = "asym_bcm_target") {
  // Load ROOT file
  TFile *f = TFile::Open(rootfile, "READ");
  if ((f == nullptr) || f->IsZombie()) {
    std::cerr << "Error: unable to open input file" << std::endl;
    if ((bool)f) {
      f->Close();
      delete f;
    }
    return;
  }

  auto *mul = dynamic_cast<TTree *>(f->Get("mul"));
  if (!(bool)mul) {
    std::cerr << "Error: 'mul' tree not found in file" << std::endl;
    f->Close();
    delete f;
    return;
  }
  mul->AddFriend("avgs");

  auto *c1 = new TCanvas;
  c1->Divide(2, 2);
  gStyle->SetOptFit(1111);

  c1->cd(1);
  std::string draw_expr1 = "1e6*" + std::string(elem1) + ">>h1";
  mul->Draw(draw_expr1.c_str());
  auto *h1 = dynamic_cast<TH1F *>(f->Get("h1"));
  h1->Fit("gaus");

  c1->cd(2);
  std::string draw_expr2 = "1e6*" + std::string(elem2) + ">>h2";
  mul->Draw(draw_expr2.c_str());
  auto *h2 = dynamic_cast<TH1F *>(f->Get("h2"));
  h2->Fit("gaus");

  c1->cd(3);
  std::string draw_expr3 =
      "1e6*(" + std::string(elem1) + "+" + std::string(elem2) + ")>>sum";
  mul->Draw(draw_expr3.c_str());
  auto *sum = dynamic_cast<TH1F *>(f->Get("sum"));
  sum->Fit("gaus");

  c1->cd(4);
  std::string draw_expr4 =
      "1e6*(" + std::string(elem1) + "-" + std::string(elem2) + ")>>diff";
  mul->Draw(draw_expr4.c_str());
  auto *diff = dynamic_cast<TH1F *>(f->Get("diff"));
  diff->Fit("gaus");

  // double width = sqrt(h1->GetStdDev() * h1->GetStdDev() +
  // h2->GetStdDev() * h2->GetStdDev());

  // std::cout << "Width of sum (calculated - given):  " << width -
  // h3->GetStdDev()
  //           << std::endl;
  // std::cout << "Width of diff (calculated - given): " << width -
  // h4->GetStdDev()
  //           << std::endl;

  c1->Update();

  std::cout << "  h1: " << h1->GetRMS() << " ± " << h1->GetRMSError()
            << std::endl;
  std::cout << "  h2: " << h2->GetRMS() << " ± " << h2->GetRMSError()
            << std::endl;
  std::cout << "diff: " << h1->GetRMS() - h2->GetRMS() << std::endl;
  std::cout << "√dsq: "
            << (h1->GetRMS() > h2->GetRMS() ? sqrt(h1->GetRMS() * h1->GetRMS() -
                                                   h2->GetRMS() * h2->GetRMS())
                                            : sqrt(h2->GetRMS() * h2->GetRMS() -
                                                   h1->GetRMS() * h1->GetRMS()))
            << std::endl;
  // std::cout << " sum: " << sum->GetRMS() << " ± " << sum->GetRMSError()
  //           << std::endl;
  // std::cout << "diff: " << diff->GetRMS() << " ± " << diff->GetRMSError()
  //           << std::endl;
}
