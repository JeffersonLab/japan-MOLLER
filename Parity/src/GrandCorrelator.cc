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
  // Add to total count for solve step 2
  fTotalCount++;

  
  // Event error flag
  fGoodEvent |= GetEventcutErrorFlag();
  if ( GetEventcutErrorFlag() != 0) fErrCounts_EF++;
  // Dependent variable error codes
  for (size_t i = 0; i < fDependentVar.size(); ++i) {
    fGoodEvent |= fDependentVar.at(i)->GetErrorCode();
    fDependentValues.at(i) = (fDependentVar[i]->GetValue(fBlock+1));
    if (fDependentVar.at(i)->GetErrorCode() !=0)  (fErrCounts_DV.at(i))++;
  }
  // Independent variable error codes
  for (size_t i = 0; i < fIndependentVar.size(); ++i) {
    fGoodEvent |= fIndependentVar.at(i)->GetErrorCode();
    fIndependentValues.at(i) = (fIndependentVar[i]->GetValue(fBlock+1));
    if (fIndependentVar.at(i)->GetErrorCode() !=0)  (fErrCounts_IV.at(i))++;
  }

    for(size_t i = 0; i < fAllVar.size(); ++i){
    fAllGood[i] = (fAllVar[i]->GetErrorCode() == 0);
    fAllValues[i] = fAllVar[i]->GetValue(fBlock+1);
    //if(!fAllGood[i]) fErrCounts_IV[i]++;
  }

  TVectorD P(fIndependentValues.size(), fIndependentValues.data());
  TVectorD Y(fDependentValues.size(),   fDependentValues.data());
  operator+= (std::make_pair(P, Y));

  for(int i = 0; i<fAllVar.size(); ++i){
    for(int j = i; j<fAllVar.size(); ++j){
        if(!fAllGood[i] || !fAllGood[j]) continue;

        double xi = fAllValues[i];
        double xj = fAllValues[j];
        double delta_i = xi - mMij(i,j);
        double delta_j = xj - mMji(j,i);

        mNij(i,j) += 1;
        mNij(j,i) = mNij(i,j);
        mMij(i,j) += delta_i / mNij(i,j);
        mSij(i,j) += delta_i * (xi-mMij(i,j));

        mMji(j,i) += delta_j / mNij(j,i);
        mSji(j,i) += delta_j * (xj - mMji(j,i));


        mCij(i,j) += ((mNij(i,j) - 1) / mNij(i,j))  * (delta_i) * (delta_j);

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
  fGoodEventNumber = 0;
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
  // Check if any channels are active
  if (nP == 0 || nY == 0) {
    return;
  }

  QwMessage << "GrandCorrelator::CalcCorrelations(): name=" << GetName() << QwLog::endl;

  // Print entry summary
  QwVerbose << "GrandCorrelator: "
            << "total entries: " << fTotalCount << ", "
            << "good entries: " << fGoodEventNumber
            << QwLog::endl;
  // and warn if zero
  if (fTotalCount > 100 && fGoodEventNumber == 0) {
    QwWarning << "GrandCorrelator: "
              << "< 1% good events, "
              << fGoodEventNumber << " of " << fTotalCount
              << QwLog::endl;
  }

  // Event error flag
  if (fErrCounts_EF > 0) {
    QwVerbose << "   Entries failed due to error flag: "
              << fErrCounts_EF << QwLog::endl;
  }
  // Dependent variable error codes
  for (size_t i = 0; i < fDependentVar.size(); ++i) {
    if (fErrCounts_DV.at(i) > 0) {
      QwVerbose << "   Entries failed due to " << fDependentVar.at(i)->GetElementName()
                << ": " <<  fErrCounts_DV.at(i) << QwLog::endl;
    }
  }
  // Independent variable error codes
  for (size_t i = 0; i < fIndependentVar.size(); ++i) {
    if (fErrCounts_IV.at(i) > 0) {
      QwVerbose << "   Entries failed due to " << fIndependentVar.at(i)->GetElementName()
                << ": " <<  fErrCounts_IV.at(i) << QwLog::endl;
    }
  }

  QwMessage << "Before failed in CalcCorrelations" << QwLog::endl;
  if (! this->failed()) {
    QwMessage << "Inside of failed loop" << QwLog::endl;
    if (fPrintCorrelations) {
      this->printSummaryP();
      this->printSummaryY();
    }

    this->solve();

    if (kTRUE || fPrintCorrelations) {
      this->printSummaryAlphas();
      this->printSummaryMeansWithUnc();
      this->printSummaryMeansWithUncCorrected();
    }
  }

  // Fill tree
  if (fTree) fTree->Fill();
  else QwWarning << "No tree" << QwLog::endl;

  // Write alpha and alias file
  WriteAlphaFile();
  WriteAliasFile();
}

/** Load the channel map
 *
 * @param mapfile Filename of map file
 * @return Zero when success
 */
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
    // Get first token: label (dv or iv), second token is the name like "asym_blah"
    string primary_token = map.GetNextToken(" ");
    string current_token = map.GetNextToken(" ");
    // Parse current token into independent variable type and name
    type_name = ParseHandledVariable(current_token);

    if (primary_token == "iv") {
      fIndependentType.push_back(type_name.first);
      fIndependentName.push_back(type_name.second);
      fIndependentFull.push_back(current_token);
    }
    else if (primary_token == "dv") {
      fDependentType.push_back(type_name.first);
      fDependentName.push_back(type_name.second);
      fDependentFull.push_back(current_token);
    }
    else if (primary_token == "treetype") {
      QwMessage << "Tree Type read, ignoring." << QwLog::endl;
    }
    else {
      QwError << "LoadChannelMap in GrandCorrelator read invalid primary_token " << primary_token << QwLog::endl;
    }
  }

  return 0;
}

Int_t GrandCorrelator::ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff)
{
  SetEventcutErrorFlagPointer(asym.GetEventcutErrorFlagPointer());

  // Return if correlator is not enabled

  /// Fill vector of pointers to the relevant data elements
  for (size_t dv = 0; dv < fDependentName.size(); dv++) {
    // Get the dependent variables

    const VQwHardwareChannel* dv_ptr = 0;

    if (fDependentType.at(dv)==kHandleTypeMps){
      //  Quietly ignore the MPS type when we're connecting the asym & diff
      continue;
    }else{
      dv_ptr = this->RequestExternalPointer(fDependentFull.at(dv));
      if (dv_ptr==NULL){
	switch (fDependentType.at(dv)) {
        case kHandleTypeAsym:
          dv_ptr = asym.RequestExternalPointer(fDependentName.at(dv));
          break;
        case kHandleTypeDiff:
          dv_ptr = diff.RequestExternalPointer(fDependentName.at(dv));
          break;
        default:
          QwWarning << "GrandCorrelator::ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff): "
                    << "Dependent variable, " << fDependentName.at(dv)
                    << ", for asym/diff correlator does not have proper type, type=="
                    << fDependentType.at(dv) << "." << QwLog::endl;
          break;
        }
      }

      
      
      if (dv_ptr == NULL){
	QwWarning << "QwCombiner::ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff):  Dependent variable, "
		  << fDependentName.at(dv)
		  << ", was not found (fullname=="
		  << fDependentFull.at(dv)<< ")." << QwLog::endl;
	 continue;
      }
    }

    // pair creation
    if(dv_ptr != NULL){
      // fDependentVarType.push_back(fDependentType.at(dv));
      fDependentVar.push_back(dv_ptr);
    }

  }

  // Add independent variables
  for (size_t iv = 0; iv < fIndependentName.size(); iv++) {
    // Get the independent variables
    const VQwHardwareChannel* iv_ptr = 0;
    iv_ptr = this->RequestExternalPointer(fIndependentFull.at(iv));
    if (iv_ptr==NULL){
      switch (fIndependentType.at(iv)) {
      case kHandleTypeAsym:
        iv_ptr = asym.RequestExternalPointer(fIndependentName.at(iv));
        break;
      case kHandleTypeDiff:
        iv_ptr = diff.RequestExternalPointer(fIndependentName.at(iv));
        break;
      default:
        QwWarning << "Independent variable for correlator has unknown type."
                  << QwLog::endl;
        break;
      }
    }

    if (iv_ptr) {
      fIndependentVar.push_back(iv_ptr);
      
    } else {
      QwWarning << "Independent variable " << fIndependentName.at(iv) << " for correlator could not be found."
                << QwLog::endl;
    }

  }
  fIndependentValues.resize(fIndependentVar.size());
  fDependentValues.resize(fDependentVar.size());

  nP = fIndependentName.size();
  nY = fDependentName.size();

  this->setDims(nP, nY);
  this->init();

  fErrCounts_IV.resize(fIndependentVar.size(),0);
  fErrCounts_DV.resize(fDependentVar.size(),0);

  QwMessage << "Starting to make list of all variables" << QwLog::endl;
  // Create vector list for all values
  fAllVar = fIndependentVar;
  fAllVar.insert(fAllVar.end(), fDependentVar.begin(), fDependentVar.end());
  fAllValues = fIndependentValues;
  fAllValues.insert(fAllValues.end(), fDependentValues.begin(), fDependentValues.end());
  fAllGood.resize(fAllValues.size(), true);


  return 0;
}



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
  fTree->Branch(TString(branchprefix + "good_count"),  &fGoodEventNumber);

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
: nP(0),nY(0),
  fErrorFlag(-1),
  fGoodEventNumber(0)
{ }

//=================================================
//=================================================
GrandCorrelator::GrandCorrelator(const GrandCorrelator& source)
: nP(source.nP),nY(source.nY),
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
  mMP.ResizeTo(nP);
  mMY.ResizeTo(nY);
  mMYp.ResizeTo(nY);

  mVPP.ResizeTo(nP,nP);
  mVPY.ResizeTo(nP,nY);
  mVYP.ResizeTo(nY,nP);
  mVYY.ResizeTo(nY,nY);
  mVYYp.ResizeTo(nY,nY);
  mVP.ResizeTo(nP);
  mVY.ResizeTo(nY);
  mVYp.ResizeTo(nY);

  mSPP.ResizeTo(mVPP);
  mSPY.ResizeTo(mVPY);
  mSYP.ResizeTo(mVYP);
  mSYY.ResizeTo(mVYY);
  mSYYp.ResizeTo(mVYYp);

  Axy.ResizeTo(nP,nY);
  Ayx.ResizeTo(nY,nP);
  dAxy.ResizeTo(Axy);
  dAyx.ResizeTo(Ayx);

  mSP.ResizeTo(nP);
  mSY.ResizeTo(nY);
  mSYp.ResizeTo(nY);

  mRPP.ResizeTo(mVPP);
  mRPY.ResizeTo(mVPY);
  mRYP.ResizeTo(mVYP);
  mRYY.ResizeTo(mVYY);
  mRYYp.ResizeTo(mVYYp);

  mNij.ResizeTo(nP+nY,nP+nY);
  mSij.ResizeTo(nP+nY,nP+nY);
  mMij.ResizeTo(nP+nY,nP+nY);
  mSji.ResizeTo(nP+nY,nP+nY);
  mMji.ResizeTo(nP+nY,nP+nY);
  mCij.ResizeTo(nP+nY,nP+nY);
  mVij.ResizeTo(nP+nY,nP+nY);
  mRij.ResizeTo(nP+nY,nP+nY);
  sigma_ij.ResizeTo(nP+nY,nP+nY);
  sigma_ji.ResizeTo(nP+nY,nP+nY);
  mVFULL.ResizeTo(nP+nY,nP+nY);
  mRFULL.ResizeTo(nP+nY,nP+nY);
  mSFULL.ResizeTo(nP+nY,nP+nY);
  mVFULL_clean.ResizeTo(nP+nY,nP+nY);
  mSFULL_clean.ResizeTo(nP+nY,nP+nY);

  fGoodEventNumber = 0;
}

//=================================================
//=================================================
void GrandCorrelator::clear()
{
  mMP.Zero();
  mMY.Zero();
  mMYp.Zero();

  mVPP.Zero();
  mVPY.Zero();
  mVYP.Zero();
  mVYY.Zero();
  mVYYp.Zero();
  mVP.Zero();
  mVY.Zero();
  mVYp.Zero();

  mSPP.Zero();
  mSPY.Zero();
  mSYP.Zero();
  mSYY.Zero();
  mSYYp.Zero();

  Axy.Zero();
  Ayx.Zero();
  dAxy.Zero();
  dAyx.Zero();

  mSP.Zero();
  mSY.Zero();
  mSYp.Zero();

  mRPP.Zero();
  mRPY.Zero();
  mRYP.Zero();
  mRYY.Zero();
  mRYYp.Zero();

  mNij.Zero();
  mSij.Zero();
  mMij.Zero();
  mSji.Zero();
  mMji.Zero();
  mCij.Zero();
  mVij.Zero();
  mRij.Zero();
  sigma_ij.Zero();
  sigma_ji.Zero();
  mVFULL.Zero();
  mRFULL.Zero();
  mSFULL.Zero();
  mVFULL_clean.Zero();
  mSFULL_clean.Zero();

  fErrorFlag = -1;
  fGoodEventNumber = 0;
}

//=================================================
//=================================================
void GrandCorrelator::print()
{
  QwMessage << "LinReg dims:  nP=" << nP << " nY=" << nY << QwLog::endl;

  QwMessage << "MP:"; mMP.Print();
  QwMessage << "MY:"; mMY.Print();
  QwMessage << "VPP:"; mVPP.Print();
  QwMessage << "VPY:"; mVPY.Print();
  QwMessage << "VYY:"; mVYY.Print();
  QwMessage << "VYYprime:"; mVYYp.Print();
}

//==========================================================
//==========================================================
GrandCorrelator& GrandCorrelator::operator+=(const std::pair<TVectorD,TVectorD>& rhs)
{
  // Get independent and dependent components
  QwMessage << "Inside of operator += function" << QwLog::endl;
  const TVectorD& P = rhs.first;
  const TVectorD& Y = rhs.second;

  // Update number of events
  fGoodEventNumber++;

  if (fGoodEventNumber <= 1) {
    // First event, set covariances to zero and means to first value
    mVPP.Zero();
    mVPY.Zero();
    mVYY.Zero();
    mMP = P;
    mMY = Y;
  } else {
    // Deviations from mean
    TVectorD delta_y(Y - mMY);
    TVectorD delta_p(P - mMP);

    // Update covariances
    Double_t alpha = (fGoodEventNumber - 1.0) / fGoodEventNumber;
    mVPP.Rank1Update(delta_p, alpha);
    mVPY.Rank1Update(delta_p, delta_y, alpha);
    mVYY.Rank1Update(delta_y, alpha);

    // Update means
    Double_t beta = 1.0 / fGoodEventNumber;
    mMP += delta_p * beta;
    mMY += delta_y * beta;
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
  TVectorD delta_y(mMY - rhs.mMY);
  TVectorD delta_p(mMP - rhs.mMP);

  // Update covariances
  Double_t alpha = fGoodEventNumber * rhs.fGoodEventNumber
                / (fGoodEventNumber + rhs.fGoodEventNumber);
  mVYY += rhs.mVYY;
  mVYY.Rank1Update(delta_y, alpha);
  mVPY += rhs.mVPY;
  mVPY.Rank1Update(delta_p, delta_y, alpha);
  mVPP += rhs.mVPP;
  mVPP.Rank1Update(delta_p, alpha);

  // Update means
  Double_t beta = rhs.fGoodEventNumber / (fGoodEventNumber + rhs.fGoodEventNumber);
  mMY += delta_y * beta;
  mMP += delta_p * beta;

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
Int_t GrandCorrelator::getMeanY(const int i, Double_t &mean) const
{
  mean=-1e50;
  if(i<0 || i >= nY ) return -1;
  if( fGoodEventNumber<1) return -3;
  mean = mMY(i);    return 0;
}


//==========================================================
//==========================================================
Int_t GrandCorrelator::getMeanYprime(const int i, Double_t &mean) const
{
  mean=-1e50;
  if(i<0 || i >= nY ) return -1;
  if( fGoodEventNumber<1) return -3;
  mean = mMYp(i);    return 0;
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
Int_t GrandCorrelator::getSigmaY(const int i, Double_t &sigma) const
{
  sigma=-1e50;
  if(i<0 || i >= nY ) return -1;
  if( fGoodEventNumber<2) return -3;
  sigma=sqrt(mVYY(i,i)/(fGoodEventNumber-1.));
  return 0;
}

//==========================================================
//==========================================================
Int_t GrandCorrelator::getSigmaYprime(const int i, Double_t &sigma) const
{
  sigma=-1e50;
  if(i<0 || i >= nY ) return -1;
  if( fGoodEventNumber<2) return -3;
  sigma=sqrt(mVYYp(i,i)/(fGoodEventNumber-1.));
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
Int_t GrandCorrelator::getCovariancePY(  int ip, int iy, Double_t &covar) const
{
    covar=-1e50;
    //... now we need only upper right triangle
    if(ip<0 || ip >= nP ) return -11;
    if(iy<0 || iy >= nY ) return -12;
    if( fGoodEventNumber<2) return -14;
    covar=mVPY(ip,iy)/(fGoodEventNumber-1.);
    return 0;
}

//==========================================================
//==========================================================
Int_t GrandCorrelator::getCovarianceY( int i, int j, Double_t &covar) const
{
    covar=-1e50;
    if( i>j) { int k=i; i=j; j=k; }//swap i & j
    //... now we need only upper right triangle
    if(i<0 || i >= nY ) return -11;
    if( fGoodEventNumber<2) return -14;
    covar=mVYY(i,j)/(fGoodEventNumber-1.);
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
//==========================================================
//Solve step 1
QwMessage << "Starting Grand Correlator Solve Step 1" << QwLog::endl;
for(int i = 0; i < fAllVar.size(); ++i){
    for(int j = i; j < fAllVar.size(); ++j){
      if(mNij(i,j) >= 2){
        mVij(i,j) = mCij(i,j) / (mNij(i,j) - 1.);
        sigma_ij(i,j) = sqrt((mSij(i,j)) / (mNij(i,j) - 1.));
        sigma_ji(j,i) = sqrt((mSji(j,i)) / (mNij(j,i) - 1.));

        if(sigma_ij(i,j) > 0.0 && sigma_ji(j,i) > 0.0){
        mRij(i,j) = mVij(i,j) / (sigma_ij(i,j) * sigma_ji(j,i));
        } else {
            mRij(i,j) = 0.0;
        }
      }
      else {
        mVij(i,j) = 0;
        sigma_ij(i,j) = 0;
        sigma_ji(j,i) = 0;
        mRij(i,j) = 0;
      }
    }
}

//==========================================================
//Solve step 2
  QwMessage << "Starting Grand Correlator Solve Step 2" << QwLog::endl;
  mVFULL = mCij;
  mVPY = mVFULL.GetSub(0,nP-1,nP,nP+nY-1);
  mVPP = mVFULL.GetSub(0,nP-1,0,nP-1);
  mVYY = mVFULL.GetSub(nP,nP+nY-1,nP,nP+nY-1);
  mRFULL = mRij;
  mRPY = mRFULL.GetSub(0,nP-1,nP,nP+nY-1);
  mRPP = mRFULL.GetSub(0,nP-1,0,nP-1);
  mRYY = mRFULL.GetSub(nP,nP+nY-1,nP,nP+nY-1);
  mSFULL = mVij;
  mSPY = mSFULL.GetSub(0,nP-1,nP,nP+nY-1);
  mSPP = mSFULL.GetSub(0,nP-1,0,nP-1);
  mSYY = mSFULL.GetSub(nP,nP+nY-1,nP,nP+nY-1);


  // off-diagonal raw covariance
  mVYP.Transpose(mVPY);

  // diagonal variances
  TMatrixD sigmaP = sigma_ij.GetSub(0,nP-1,0,nP-1);
  sigmaP.Print();
  TMatrixD sigmaY = sigma_ij.GetSub(nP,nP+nY-1,nP,nP+nY-1);
  mVP = TMatrixDDiag(sigmaP);
  mVY = TMatrixDDiag(sigmaY);

  QwMessage << "Making Clean Matrices" << QwLog::endl;
  QwMessage << "fGoodEventNumber" << fGoodEventNumber << QwLog::endl;
  // "Clean" matrices
  for(int i = 0; i < fAllVar.size(); ++i){
    for(int j = i; j < fAllVar.size(); ++j){
      if(i < 5 && j < 5){
QwMessage << mRij(i,j) << " " << sigma_ij(i,j) << " " << sigma_ji(j,i) << QwLog::endl;
      }
        mVFULL_clean(i,j) = mRij(i,j) * sigma_ij(i,j) * sigma_ji(j,i) * (fGoodEventNumber - 1);
        mSFULL_clean(i,j) = mRij(i,j) * sigma_ij(i,j) * sigma_ji(j,i);
        
    }
}
  TMatrixD mVPY_clean = mVFULL_clean.GetSub(0,nP-1,nP,nP+nY-1);
  TMatrixD mVPP_clean = mVFULL_clean.GetSub(0,nP-1,0,nP-1);
  TMatrixD mVYY_clean = mVFULL_clean.GetSub(nP,nP+nY-1,nP,nP+nY-1);
  TMatrixD mVYP_clean(TMatrixD::kTransposed, mVPY_clean);


  TMatrixD mSPY_clean = mSFULL_clean.GetSub(0,nP-1,nP,nP+nY-1);
  TMatrixD mSPP_clean = mSFULL_clean.GetSub(0,nP-1,0,nP-1);
  TMatrixD mSYY_clean = mSFULL_clean.GetSub(nP,nP+nY-1,nP,nP+nY-1);


  TMatrixD mSYP_clean(TMatrixD::kTransposed, mSPY_clean);
  TVectorD mSP_clean = TMatrixDDiag(mSPP_clean);
  mSPP_clean.Print();
  QwMessage << "Before Sqrt of mSP_clean, number rows: " << mSP_clean.GetNrows() << QwLog::endl;
  mSP_clean.Print();
  mSP_clean.Sqrt();
  TVectorD mSY_clean = TMatrixDDiag(mSYY_clean);
  mSY_clean.Sqrt();

  // Check for goodness and then get rid of bad columns

  // Warn if correlation matrix determinant close to zero (heuristic)
  if (mRPP.Determinant() < std::pow(10,-(2*nP))) {
    QwWarning << "LRB: correlation matrix nearly singular, "
              << "determinant = " << mRPP.Determinant()
              << " (set includes highly correlated variable pairs)"
              << QwLog::endl;
    if (fGoodEventNumber > 10*nP) {
      QwMessage << fGoodEventNumber << " events" << QwLog::endl;
      QwMessage << "Covariance matrix: " << QwLog::endl; mVPP_clean.Print();
      QwMessage << "Correlation matrix: " << QwLog::endl; mRPP.Print();
    }
    QwWarning << "LRB: solving failed (this happens when only few events)."
              << QwLog::endl;
    return;
  }

  //==========================================================
  //Solve Step 3
  // slopes
  QwMessage << "Starting Grand Correlator Solve Step 3" << std::endl;
  TMatrixD invRPP(TMatrixD::kInverted, mRPP);
  Axy = TMatrixD(invRPP, TMatrixD::kMult, mRPY);
  Axy.NormByColumn(mSP_clean); // divide
  Axy.NormByRow(mSY_clean, ""); // mult
  Ayx.Transpose(Axy);

  // new means
  mMYp = mMY - Ayx * mMP;

  // new raw covariance
  mVYYp = mVYY_clean + Ayx * mVPP_clean * Axy - (Ayx * mVPY_clean + mVYP_clean * Axy);
  // new variances
  mVYp = TMatrixDDiag(mVYYp); 
  mVYp.Sqrt();

  // new normalized covariance
  mSYYp = mSYY_clean + Ayx * mSPP_clean * Axy - (Ayx * mSPY_clean + mSYP_clean * Axy);
  // uncertainties on the new means
  mSYp = TMatrixDDiag(mSYYp); 
  mSYp.Sqrt();

  // new correlation matrix
  mRYYp = mVYYp; mRYYp.NormByColumn(mVYp); mRYYp.NormByRow(mVYp);

  // slope uncertainties
  double norm = 1. / (fGoodEventNumber - nP - 1);
  dAxy.Zero();
  dAxy.Rank1Update(TMatrixDDiag(invRPP), TMatrixDDiag(mRYYp), norm); // diag mRYYp = row of ones
  dAxy.Sqrt();
  dAxy.NormByColumn(mSP_clean); // divide
  dAxy.NormByRow(mSYp, ""); // mult
  dAyx.Transpose(dAxy);

  fErrorFlag = 0;
}