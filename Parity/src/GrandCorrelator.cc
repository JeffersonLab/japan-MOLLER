#include "GrandCorrelator.h"

// System includes
#include <algorithm>
#include <fstream>
#include <utility>
#include <assert.h>
#include <math.h>

// ROOT headers
#include "TFile.h"
#include "TH2D.h"
#include "TString.h"


// Qweak headers
#include "QwOptions.h"
#include "QwHelicityPattern.h"
#include "VQwDataElement.h"
#include "QwVQWK_Channel.h"
#include "QwParameterFile.h"
#include "QwRootFile.h"
#include "QwLog.h"

// Static members
bool GrandCorrelator::fPrintCorrelations = false;

GrandCorrelator::GrandCorrelator(const TString& name)
: VQwDataHandler(name),
  fBlock(-1),
  fDisableHistos(true),
  fAlphaOutputFileBase("blueR"),
  fAlphaOutputFileSuff("new.slope.root"),
  fAlphaOutputPath("."),
  fAlphaOutputFile(0),
  fTree(0),
  fAliasOutputFileBase("regalias_"),
  fAliasOutputFileSuff(""),
  fAliasOutputPath("."),
  fNameNoSpaces(name),
  nP(0),nY(0),
  fCycleCounter(0)
{
  fNameNoSpaces.ReplaceAll(" ","_");
  // Set default tree name and descriptions (in VQwDataHandler)
  fTreeName = "lrb";
  fTreeComment = "Correlations";
  // Parsing separator
  ParseSeparator = "_";

  // Clear all data
  ClearEventData();
}

GrandCorrelator::~GrandCorrelator()
{
  // Close alpha and alias file
  CloseAlphaFile();
  CloseAliasFile();
}

void GrandCorrelator::DefineOptions(QwOptions &options)
{
  options.AddOptions()("print-correlations",
      po::value<bool>(&fPrintCorrelations)->default_bool_value(false),
      "print correlations after determining them");
}
void GrandCorrelator::ProcessOptions(QwOptions &options)
{
}

void GrandCorrelator::ParseConfigFile(QwParameterFile& file)
{
  VQwDataHandler::ParseConfigFile(file);
  file.PopValue("slope-file-base", fAlphaOutputFileBase);
  file.PopValue("slope-file-suff", fAlphaOutputFileSuff);
  file.PopValue("slope-path", fAlphaOutputPath);
  file.PopValue("alias-file-base", fAliasOutputFileBase);
  file.PopValue("alias-file-suff", fAliasOutputFileSuff);
  file.PopValue("alias-path", fAliasOutputPath);
  file.PopValue("disable-histos", fDisableHistos);
  file.PopValue("block", fBlock);
  if (fBlock >= 4)
    QwWarning << "GrandCorrelator: expect 0 <= block <= 3 but block = "
              << fBlock << QwLog::endl;
}


void GrandCorrelator::ProcessData()
{
  // Add to total count
  fTotalCount++;

  for(size_t i = 0; i < fAllVar.size(); ++i){
    fAllGood[i] = (fAllVar[i]->GetErrorCode() == 0);
    fAllValues[i] = fAllVar[i]->GetValue(fBlock+1);
    if(!fAllGood[i]) fErrCounts_IV[i]++;
  }

  fGoodCount++;
  for(int i = 0; i<nP; ++i){
    if(!fAllGood[i]) continue;
    for(int j = i; j<nP; ++j){
        if(!fAllGood[j]) continue;

        double xi = fAllValues[i];
        double xj = fAllValues[j];

        mNij(i,j) += 1.0;
        double delta = xi - mMij(i,j);
        mMij(i,j) += delta / mNij(i,j);
        mSij(i,j) += delta * (xi-mMij(i,j));
    }
  }
}

void GrandCorrelator::ClearEventData()
{
  // Clear error counters
  fErrCounts_EF = 0;
  std::fill(fErrCounts_DV.begin(), fErrCounts_DV.end(), 0);
  std::fill(fErrCounts_IV.begin(), fErrCounts_IV.end(), 0);

  // Clear event counts
  fTotalCount = 0;
  fGoodCount = 0;
  fGoodEvent = -1;

  // Clear regression
  this->clear();
}

void GrandCorrelator::AccumulateRunningSum(VQwDataHandler &value, Int_t count, Int_t ErrorMask)
{
  GrandCorrelator* correlator = dynamic_cast<GrandCorrelator*>(&value);
  if (correlator) {
    operator+=(*correlator);
  } else {
    QwWarning << "GrandCorrelator::AccumulateRunningSum "
              << "can only accept other GrandCorrelator objects."
              << QwLog::endl;
  }
}

void GrandCorrelator::CalcCorrelations()
{
  if (nP == 0) return;
  
  for(int i = 0; i<nP; ++i){
    for(int j = i; j<nP; ++j){
        if(mNij(i,j) > 1){
            double var = mSij(i,j) / (mNij(i,j) - 1.0);
            mSY[i] = sqrt(var);
            mRPY(i,j) = var;
        }
    }
  }

  if(fPrintCorrelations){
    printSummaryAlphas();
    printSummaryMeansWithUnc();
  }

  if(fTree) fTree->Fill();
  WriteAlphaFile();
  WriteAliasFile();
}

Int_t GrandCorrelator::LoadChannelMap(const std::string& mapfile)
{
  // Open the file
  QwParameterFile map(mapfile);

  // Read the sections of dependent variables
  std::pair<EQwHandleType,std::string> type_name;

  // Add independent variables and sensitivities
  while (map.ReadNextLine()) {
    // Throw away comments, whitespace, empty lines
    map.TrimComment();
    map.TrimWhitespace();
    if (map.LineIsEmpty()) continue;
    std::string token = map.GetNextToken(" ");
    fAllName.push_back(token);
    fAllVar.push_back(nullptr);
}
return 0;
}

Int_t GrandCorrelator::ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff)
{
  SetEventcutErrorFlagPointer(asym.GetEventcutErrorFlagPointer());

  fAllValues.resize(fAllName.size());
  fAllGood.resize(fAllName.size(), true);

  for(size_t i = 0; i<fAllName.size(); ++i){
    const VQwHardwareChannel* ptr = RequestExternalPointer(fAllName[i]);
    if(!ptr){
        ptr = asym.RequestExternalPointer(fAllName[i]);
        if(!ptr) ptr = diff.RequestExternalPointer(fAllName[i]);
    }
    if(!ptr){
        QwWarning << "GrandCorrelator: variable " << fAllName[i] << " not found." << QwLog::endl;
        continue;
    }
    fAllVar[i] = ptr;
  }

  nP = fAllName.size();
  setDims(nP,nP);
  init();
  fErrCounts_IV.resize(nP,0);
  fErrCounts_DV.resize(nP,0);

  return 0;
}



//NEED TO LOOK AT THESE LATER
void GrandCorrelator::ConstructTreeBranches(
    QwRootFile *treerootfile,
    const std::string& treeprefix,
    const std::string& branchprefix)
{
  // Check if any channels are active
  if (nP == 0 || nY == 0) {
    return;
  }

  // Check if tree name is specified
  if (fTreeName == "") {
    QwWarning << "GrandCorrelator: no tree name specified, use 'tree-name = value'" << QwLog::endl;
    return;
  }

  // Create alpha and alias files before trying to create the tree
  OpenAlphaFile(treeprefix);
  OpenAliasFile(treeprefix);

  // Construct tree name and create new tree
  const std::string name = treeprefix + fTreeName;
  treerootfile->NewTree(name, fTreeComment.c_str());
  fTree = treerootfile->GetTree(name);
  // Check to make sure the tree was created successfully
  if (fTree == NULL) return;

  // Set up branches
  fTree->Branch(TString(branchprefix + "total_count"), &fTotalCount);
  fTree->Branch(TString(branchprefix + "good_count"),  &fGoodCount);

  fTree->Branch(TString(branchprefix + "n"), &(this->fGoodEventNumber));
  fTree->Branch(TString(branchprefix + "ErrorFlag"), &(this->fErrorFlag));

  auto bn = [&](const TString& n) {
    return TString(branchprefix + n);
  };
  auto pm = [](TMatrixD& m) {
    return m.GetMatrixArray();
  };
  auto lm = [](TMatrixD& m, const TString& n) {
    return Form("%s[%d][%d]/D", n.Data(), m.GetNrows(), m.GetNcols());
  };
  auto branchm = [&](TTree* tree, TMatrixD& m, const TString& n) {
    tree->Branch(bn(n),pm(m),lm(m,n));
  };
  auto pv = [](TVectorD& v) {
    return v.GetMatrixArray();
  };
  auto lv = [](TVectorD& v, const TString& n) {
    return Form("%s[%d]/D", n.Data(), v.GetNrows());
  };
  auto branchv = [&](TTree* tree, TVectorD& v, const TString& n) {
    tree->Branch(bn(n),pv(v),lv(v,n));
  };

  branchm(fTree,this->Axy,  "A");
  branchm(fTree,this->dAxy, "dA");

  branchm(fTree,this->mVPP,  "VPP");
  branchm(fTree,this->mVPY,  "VPY");
  branchm(fTree,this->mVYP,  "VYP");
  branchm(fTree,this->mVYY,  "VYY");
  branchm(fTree,this->mVYYp, "VYYp");

  branchm(fTree,this->mSPP,  "SPP");
  branchm(fTree,this->mSPY,  "SPY");
  branchm(fTree,this->mSYP,  "SYP");
  branchm(fTree,this->mSYY,  "SYY");
  branchm(fTree,this->mSYYp, "SYYp");

  branchm(fTree,this->mRPP,  "RPP");
  branchm(fTree,this->mRPY,  "RPY");
  branchm(fTree,this->mRYP,  "RYP");
  branchm(fTree,this->mRYY,  "RYY");
  branchm(fTree,this->mRYYp, "RYYp");

  branchv(fTree,this->mMP,  "MP");   // Parameter mean
  branchv(fTree,this->mMY,  "MY");   // Uncorrected mean
  branchv(fTree,this->mMYp, "MYp");  // Corrected mean

  branchv(fTree,this->mSP,  "dMP");  // Parameter mean error
  branchv(fTree,this->mSY,  "dMY");  // Uncorrected mean error
  branchv(fTree,this->mSYp, "dMYp"); // Corrected mean error

}

/// \brief Construct the histograms in a folder with a prefix
void GrandCorrelator::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  // Skip if disabled
  if (fDisableHistos) return;

  // Check if any channels are active
  if (nP == 0 || nY == 0) {
    return;
  }

  // Go to directory
  TString name(fName);
  name.ReplaceAll(" ","_");
  folder->mkdir(name)->cd();

  //..... 1D,  iv
  fH1iv.resize(nP);
  for (int i = 0; i < nP; i++) {
    fH1iv[i] = TH1D(
        Form("P%d",i),
        Form("iv P%d=%s, pass=%s ;iv=%s (ppm)",i,fIndependentName[i].c_str(),fName.Data(),fIndependentName[i].c_str()),
        128,0.,0.);
    fH1iv[i].GetXaxis()->SetNdivisions(4);
  }

  //..... 2D,  iv correlations
  Double_t x1 = 0;
  fH2iv.resize(nP);
  for (int i = 0; i < nP; i++) {
    fH2iv[i].resize(nP);
    for (int j = i+1; j < nP; j++) { // not all are used
      fH2iv[i][j] = TH2D(
          Form("P%d_P%d",i,j),
          Form("iv correlation  P%d_P%d, pass=%s ;P%d=%s (ppm);P%d=%s   (ppm)  ",
              i,j,fName.Data(),i,fIndependentName[i].c_str(),j,fIndependentName[j].c_str()),
          64,-x1,x1,
          64,-x1,x1);
      fH2iv[i][j].GetXaxis()->SetTitleColor(kBlue);
      fH2iv[i][j].GetYaxis()->SetTitleColor(kBlue);
      fH2iv[i][j].GetXaxis()->SetNdivisions(4);
      fH2iv[i][j].GetYaxis()->SetNdivisions(4);
    }
  }

  //..... 1D,  dv
  fH1dv.resize(nY);
  for (int i = 0; i < nY; i++) {
    fH1dv[i] = TH1D(
        Form("Y%d",i),
        Form("dv Y%d=%s, pass=%s ;dv=%s (ppm)",i,fDependentName[i].c_str(),fName.Data(),fDependentName[i].c_str()),
        128,0.,0.);
    fH1dv[i].GetXaxis()->SetNdivisions(4);
  }

  //..... 2D,  dv-iv correlations
  Double_t y1 = 0;
  fH2dv.resize(nP);
  for (int i = 0; i < nP; i++) {
    fH2dv[i].resize(nY);
    for (int j = 0; j < nY; j++) {
      fH2dv[i][j] = TH2D(
          Form("P%d_Y%d",i,j),
          Form("iv-dv correlation  P%d_Y%d, pass=%s ;P%d=%s (ppm);Y%d=%s   (ppm)  ",
              i,j,fName.Data(),i,fIndependentName[i].c_str(),j,fDependentName[j].c_str()),
          64,-x1,x1,
          64,-y1,y1);
      fH2dv[i][j].GetXaxis()->SetTitleColor(kBlue);
      fH2dv[i][j].GetYaxis()->SetTitleColor(kBlue);
      fH2dv[i][j].GetXaxis()->SetNdivisions(4);
      fH2dv[i][j].GetYaxis()->SetNdivisions(4);
    }
  }

  // store list of names to be archived
  fHnames.resize(2);
  fHnames[0] = TH1D("NamesIV",Form("IV name list nIV=%d",nP),nP,0,1);
  for (int i = 0; i < nP; i++)
    fHnames[0].Fill(fIndependentName[i].c_str(),1.*i);
  fHnames[1] = TH1D("NamesDV",Form("DV name list nIV=%d",nY),nY,0,1);
  for (int i = 0; i < nY; i++)
    fHnames[1].Fill(fDependentName[i].c_str(),i*1.);
}

/// \brief Fill the histograms
void GrandCorrelator::FillHistograms()
{
  // Skip if disabled
  if (fDisableHistos) return;

  // Check if any channels are active
  if (nP == 0 || nY == 0) {
    return;
  }

  // Skip if bad event
  if (fGoodEvent != 0) return;

  // Fill histograms
  for (size_t i = 0; i < fIndependentValues.size(); i++) {
    fH1iv[i].Fill(fIndependentValues[i]);
    for (size_t j = i+1; j < fIndependentValues.size(); j++)
      fH2iv[i][j].Fill(fIndependentValues[i], fIndependentValues[j]);
  }
  for (size_t j = 0; j < fDependentValues.size(); j++) {
    fH1dv[j].Fill(fDependentValues[j]);
    for (size_t i = 0; i < fIndependentValues.size(); i++)
      fH2dv[i][j].Fill(fIndependentValues[i], fDependentValues[j]);
  }
}

void GrandCorrelator::WriteAlphaFile()
{
  // Ensure in output file
  if (fAlphaOutputFile) fAlphaOutputFile->cd();

  // Write objects
  this->Axy.Write("slopes");
  this->dAxy.Write("sigSlopes");

  this->mRPP.Write("IV_IV_correlation");
  this->mRPY.Write("IV_DV_correlation");
  this->mRYY.Write("DV_DV_correlation");
  this->mRYYp.Write("DV_DV_correlation_prime");

  this->mMP.Write("IV_mean");
  this->mMY.Write("DV_mean");
  this->mMYp.Write("DV_mean_prime");

  // number of events
  TMatrixD Mstat(1,1);
  Mstat(0,0)=this->getUsedEve();
  Mstat.Write("MyStat");

  //... IVs
  TH1D hiv("IVname","names of IVs",nP,-0.5,nP-0.5);
  for (int i=0;i<nP;i++) hiv.Fill(fIndependentFull[i].c_str(),i);
  hiv.Write();

  //... DVs
  TH1D hdv("DVname","names of IVs",nY,-0.5,nY-0.5);
  for (int i=0;i<nY;i++) hdv.Fill(fDependentFull[i].c_str(),i);
  hdv.Write();

  // sigmas
  this->mSP.Write("IV_sigma");
  this->mSY.Write("DV_sigma");
  this->mSYp.Write("DV_sigma_prime");

  // raw covariances
  this->mVPP.Write("IV_IV_rawVariance");
  this->mVPY.Write("IV_DV_rawVariance");
  this->mVYY.Write("DV_DV_rawVariance");
  this->mVYYp.Write("DV_DV_rawVariance_prime");
  TVectorD mVY2(TMatrixDDiag(this->mVYY));
  mVY2.Write("DV_rawVariance");
  TVectorD mVP2(TMatrixDDiag(this->mVPP));
  mVP2.Write("IV_rawVariance");
  TVectorD mVY2prime(TMatrixDDiag(this->mVYYp));
  mVY2prime.Write("DV_rawVariance_prime");

  // normalized covariances
  this->mSPP.Write("IV_IV_normVariance");
  this->mSPY.Write("IV_DV_normVariance");
  this->mSYY.Write("DV_DV_normVariance");
  this->mSYYp.Write("DV_DV_normVariance_prime");
  TVectorD sigY2(TMatrixDDiag(this->mSYY));
  sigY2.Write("DV_normVariance");
  TVectorD sigX2(TMatrixDDiag(this->mSPP));
  sigX2.Write("IV_normVariance");
  TVectorD sigY2prime(TMatrixDDiag(this->mSYYp));
  sigY2prime.Write("DV_normVariance_prime");

  this->Axy.Write("A_xy");
  this->Ayx.Write("A_yx");
}

void GrandCorrelator::OpenAlphaFile(const std::string& prefix)
{
  // Create old-style blueR ROOT file
  std::string name = prefix + fAlphaOutputFileBase + run_label.Data() + fAlphaOutputFileSuff;
  std::string path = fAlphaOutputPath + "/";
  std::string file = path + name;
  fAlphaOutputFile = new TFile(TString(file), "RECREATE", "correlation coefficients");
  if (! fAlphaOutputFile->IsWritable()) {
    QwError << "GrandCorrelator could not create output file " << file << QwLog::endl;
    delete fAlphaOutputFile;
    fAlphaOutputFile = 0;
  }
}

void GrandCorrelator::OpenAliasFile(const std::string& prefix)
{
  // Turn "." into "_" in run_label (no "." allowed in function name, and must
  // agree with the filename)
  std::string label(run_label);
  std::replace(label.begin(), label.end(), '.', '_');
  // Create old-style regalias script
  std::string name = prefix + fAliasOutputFileBase + label + fAliasOutputFileSuff;
  std::string path = fAliasOutputPath + "/";
  std::string file = path + name + ".C"; // add extension outside of file suffix
  fAliasOutputFile.open(file, std::ofstream::out);
  if (fAliasOutputFile.good()) {
    fAliasOutputFile << Form("void %s(int i = 0) {", name.c_str()) << std::endl;
  } else {
    QwWarning << "GrandCorrelator: Could not write to alias output file " << QwLog::endl;
  }
}

void GrandCorrelator::CloseAlphaFile()
{
  // Close slopes output file
  if (fAlphaOutputFile) {
    fAlphaOutputFile->Write();
    fAlphaOutputFile->Close();
  }
}

void GrandCorrelator::CloseAliasFile()
{
  // Close alias output file
  if (fAliasOutputFile.good()) {
    fAliasOutputFile << "}" << std::endl << std::endl;
    fAliasOutputFile.close();
  } else {
    QwWarning << "GrandCorrelator: Unable to close alias output file." << QwLog::endl;
  }
}

void GrandCorrelator::WriteAliasFile()
{
  // Ensure output file is open
  if (fAliasOutputFile.bad()) {
    QwWarning << "GrandCorrelator: Could not write to alias output file " << QwLog::endl;
    return;
  }

  fAliasOutputFile << " if (i == " << fCycleCounter << ") {" << std::endl;
  fAliasOutputFile << Form("  TTree* tree = (TTree*) gDirectory->Get(\"mul\");") << std::endl;
  for (int i = 0; i < nY; i++) {
    fAliasOutputFile << Form("  tree->SetAlias(\"reg_%s\",",fDependentFull[i].c_str()) << std::endl;
    fAliasOutputFile << Form("         \"%s",fDependentFull[i].c_str());
    for (int j = 0; j < nP; j++) {
      fAliasOutputFile << Form("%+.4e*%s", -this->Axy(j,i), fIndependentFull[j].c_str());
    }
    fAliasOutputFile << "\");" << std::endl;
  }
  fAliasOutputFile << " }" << std::endl;

  // Increment call counter
  fCycleCounter++;
}




GrandCorrelator::GrandCorrelator()
: nP(0),
  fErrorFlag(-1),
  fGoodEventNumber(0)
{ }

//=================================================
//=================================================
GrandCorrelator::GrandCorrelator(const GrandCorrelator& source)
: nP(source.nP),
  fErrorFlag(-1),
  fGoodEventNumber(0),
  VQwDataHandler(source),
  fBlock(source.fBlock),
  fDisableHistos(source.fDisableHistos),
  fAlphaOutputFileBase(source.fAlphaOutputFileBase),
  fAlphaOutputFileSuff(source.fAlphaOutputFileSuff),
  fAlphaOutputPath(source.fAlphaOutputPath),
  fAlphaOutputFile(nullptr),
  fTree(nullptr),
  fAliasOutputFileBase(source.fAliasOutputFileBase),
  fAliasOutputFileSuff(source.fAliasOutputFileSuff),
  fAliasOutputPath(source.fAliasOutputPath),
  fCycleCounter(source.fCycleCounter)
{
  QwMessage << fGoodEventNumber << QwLog::endl;

  QwWarning << "GrandCorrelator copy constructor required but untested" << QwLog::endl;

  // Clear all data
  ClearEventData();
}

//=================================================
//=================================================
void GrandCorrelator::init()
{
  mVPP.ResizeTo(nP,nP); //pairwise covariance
  mVP.ResizeTo(nP);     //variances
  mMP.ResizeTo(nP);     //means
  mSY.ResizeTo(nP);     //standard deviations

  mRPP.ResizeTo(nP,nP); //correlation matrix

  fGoodEventNumber = 0;
}

//=================================================
//=================================================
void GrandCorrelator::clear()
{
  mVPP.Zero();
  mVP.Zero();
  mMP.Zero();
  mSY.Zero();
  mRPP.Zero();

  fErrorFlag = -1;
  fGoodEventNumber = 0;
}

//=================================================
//=================================================
void GrandCorrelator::print()
{
  QwMessage << "LinReg dims:  nP=" << nP << QwLog::endl;

  QwMessage << "VPP:"; mVPP.Print();
  QwMessage << "VP:"; mVP.Print();
  QwMessage << "MP:"; mMP.Print();
  QwMessage << "SY:"; mSY.Print();
  QwMessage << "RPP:"; mRPP.Print();
}

//==========================================================
//==========================================================
GrandCorrelator& GrandCorrelator::operator+=(const std::pair<TVectorD,TVectorD>& rhs)
{
  const TVectorD& vals = rhs.first;
  
  fGoodEventNumber++;
  if(fGoodEventNumber == 1){
    mMP = vals;
    mVPP.Zero();
  } else {
    TVectorD delta(vals - mMP);

    Double_t alpha = (fGoodEventNumber - 1.0)/fGoodEventNumber;
    mVPP.Rank1Update(delta,alpha);

    Double_t beta = 1.0/fGoodEventNumber;
    mMP += delta*beta;
  }

  return *this;
}

//==========================================================
//==========================================================
GrandCorrelator& GrandCorrelator::operator+=(const GrandCorrelator& rhs)
{
  // If set X = A + B, then
  //   Cov[X] = Cov[A] + Cov[B]
  //          + (E[x_A] - E[x_B]) * (E[y_A] - E[y_B]) * n_A * n_B / n_X
  // Ref: E. Schubert, M. Gertz (9 July 2018).
  // "Numerically stable parallel computation of (co-)variance".
  // SSDBM '18 Proceedings of the 30th International Conference
  // on Scientific and Statistical Database Management.
  // https://doi.org/10.1145/3221269.3223036

  if(fGoodEventNumber + rhs.fGoodEventNumber == 0)
    return *this;

  // Deviations from mean
  TVectorD delta(mMP - rhs.mMP);

  // Update covariances
  Double_t alpha = fGoodEventNumber * rhs.fGoodEventNumber
                / (fGoodEventNumber + rhs.fGoodEventNumber);
  mVPP += rhs.mVPP;
  mVPP.Rank1Update(delta, alpha);

  // Update means
  Double_t beta = rhs.fGoodEventNumber / (fGoodEventNumber + rhs.fGoodEventNumber);
  mMP += delta * beta;

  fGoodEventNumber += rhs.fGoodEventNumber;

  return *this;
}

//==========================================================
//==========================================================
Int_t GrandCorrelator::getMeanP(const int i, Double_t &mean) const
{
   mean=-1e50;
   if(i<0 || i >= nP ) return -1;
   if( fGoodEventNumber<1) return -3;
   mean = mMP(i);    return 0;
}

//==========================================================
//==========================================================
Int_t GrandCorrelator::getSigmaP(const int i, Double_t &sigma) const
{
  sigma=-1e50;
  if(i<0 || i >= nP ) return -1;
  if( fGoodEventNumber<2) return -3;
  sigma=sqrt(mVPP(i,i)/(fGoodEventNumber-1.));
  return 0;
}

//==========================================================
//==========================================================
Int_t GrandCorrelator::getCovarianceP( int i, int j, Double_t &covar) const
{
    covar=-1e50;
    if( i>j) { int k=i; i=j; j=k; }//swap i & j
    //... now we need only upper right triangle
    if(i<0 || i >= nP ) return -11;
    if( fGoodEventNumber<2) return -14;
    covar=mVPP(i,j)/(fGoodEventNumber-1.);
    return 0;
}

//==========================================================
//==========================================================
void GrandCorrelator::printSummaryP() const
{
  QwMessage << Form("\nLinRegBevPeb::printSummaryP seen good eve=%lld",fGoodEventNumber)<<QwLog::endl;

  size_t dim=nP;
  if(fGoodEventNumber>2) { // print full matrix
    QwMessage << Form("\nname:                                               ");
    for (size_t i = 1; i <dim; i++) {
      QwMessage << Form("P%d%11s",(int)i," ");
    }
    QwMessage << Form("\n           mean     sig(distrib)   nSig(mean)       correlation-matrix ....\n");
    for (size_t i = 0; i <dim; i++) {
      double meanI,sigI;
      if (getMeanP(i,meanI) < 0) QwWarning << "LRB::getMeanP failed" << QwLog::endl;
      if (getSigmaP(i,sigI) < 0) QwWarning << "LRB::getSigmaP failed" << QwLog::endl;
      double nSig=-1;
      double err=sigI/sqrt(fGoodEventNumber);
      if(sigI>0.) nSig=meanI/err;

      QwMessage << Form("P%d:  %+12.4g  %12.3g      %.1f    ",(int)i,meanI,sigI,nSig);
      for (size_t j = 1; j <dim; j++) {
        if( j<=i) { QwMessage << Form("  %12s","._._._."); continue;}
        double sigJ,cov;
        if (getSigmaP(j,sigJ) < 0) QwWarning << "LRB::getSigmaP failed" << QwLog::endl;
        if (getCovarianceP(i,j,cov) < 0) QwWarning << "LRB::getCovarianceP failed" << QwLog::endl;
        double corel=cov / sigI / sigJ;

        QwMessage << Form("  %12.3g",corel);
      }
      QwMessage << Form("\n");
    }
  }
}

//==========================================================
//==========================================================
void GrandCorrelator::solve()
{
  // diagonal variances
  for(int i=0; i<nP; i++)
    mVP(i) = sqrt(mVPP(i,i));

  mRPP = mVPP;
  for(int i=0; i<nP; i++){
    for(int j=0; j<nP; j++){
        if(mVP(i)*mVP(j) != 0){
            mRPP(i,j) /=(mVP(i)*mVP(j));
        }else{
            mRPP(i,j) = 0.0;
        }
    }
}

  fErrorFlag = 0;
}