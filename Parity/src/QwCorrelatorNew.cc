#include "QwCorrelatorNew.h"

// System includes
#include <utility>

// ROOT headers
#include "TFile.h"
#include "TH2D.h"

// Qweak headers
#include "QwOptions.h"
#include "QwHelicityPattern.h"
#include "VQwDataElement.h"
#include "QwVQWK_Channel.h"
#include "QwParameterFile.h"
#include "QwRootFile.h"

// Static members
bool QwCorrelatorNew::fPrintCorrelations = false;

QwCorrelatorNew::QwCorrelatorNew(const TString& name)
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

QwCorrelatorNew::QwCorrelatorNew(const QwCorrelatorNew& source)
: VQwDataHandler(source),
  fBlock(source.fBlock),
  fDisableHistos(source.fDisableHistos),
  fAlphaOutputFileBase(source.fAlphaOutputFileBase),
  fAlphaOutputFileSuff(source.fAlphaOutputFileSuff),
  fAlphaOutputPath(source.fAlphaOutputPath),
  fAlphaOutputFile(0),
  fTree(0),
  fAliasOutputFileBase(source.fAliasOutputFileBase),
  fAliasOutputFileSuff(source.fAliasOutputFileSuff),
  fAliasOutputPath(source.fAliasOutputPath),
  nP(source.nP),nY(source.nY),
  fCycleCounter(source.fCycleCounter)
{
  QwWarning << "QwCorrelatorNew copy constructor required but untested" << QwLog::endl;

  // Clear all data
  ClearEventData();
}

QwCorrelatorNew::~QwCorrelatorNew()
{
  // Close alpha and alias file
  CloseAlphaFile();
  CloseAliasFile();
}

void QwCorrelatorNew::DefineOptions(QwOptions &options)
{
  options.AddOptions()("print-correlations",
      po::value<bool>(&fPrintCorrelations)->default_bool_value(false),
      "print correlations after determining them");
}

void QwCorrelatorNew::ProcessOptions(QwOptions &options)
{
}

void QwCorrelatorNew::ParseConfigFile(QwParameterFile& file)
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
    QwWarning << "QwCorrelatorNew: expect 0 <= block <= 3 but block = "
              << fBlock << QwLog::endl;
}

void QwCorrelatorNew::ProcessData()
{
  // Add to total count
  fTotalCount++;

  // Start as good event
  fGoodEvent = 0;

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

  // If good, process event
  if (fGoodEvent == 0) {
    fGoodCount++;

    TVectorD P(fIndependentValues.size(), fIndependentValues.data());
    TVectorD Y(fDependentValues.size(),   fDependentValues.data());
    linReg += std::make_pair(P, Y);
  }
}

void QwCorrelatorNew::ClearEventData()
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
  linReg.clear();
}

void QwCorrelatorNew::AccumulateRunningSum(VQwDataHandler &value, Int_t count, Int_t ErrorMask)
{
  QwCorrelatorNew* correlator = dynamic_cast<QwCorrelatorNew*>(&value);
  if (correlator) {
    linReg += correlator->linReg;
  } else {
    QwWarning << "QwCorrelatorNew::AccumulateRunningSum "
              << "can only accept other QwCorrelatorNew objects."
              << QwLog::endl;
  }
}

void QwCorrelatorNew::CalcCorrelations()
{
  // Check if any channels are active
  if (nP == 0 || nY == 0) {
    return;
  }

  QwMessage << "QwCorrelatorNew::CalcCorrelations(): name=" << GetName() << QwLog::endl;

  // Print entry summary
  QwVerbose << "QwCorrelatorNew: "
            << "total entries: " << fTotalCount << ", "
            << "good entries: " << fGoodCount
            << QwLog::endl;
  // and warn if zero
  if (fTotalCount > 100 && fGoodCount == 0) {
    QwWarning << "QwCorrelatorNew: "
              << "< 1% good events, "
              << fGoodCount << " of " << fTotalCount
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

  if (! linReg.failed()) {

    if (fPrintCorrelations) {
      linReg.printSummaryP();
      linReg.printSummaryY();
    }

    linReg.solve();

    if (fPrintCorrelations) {
      linReg.printSummaryAlphas();
      linReg.printSummaryMeansWithUnc();
      linReg.printSummaryMeansWithUncCorrected();
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
Int_t QwCorrelatorNew::LoadChannelMap(const std::string& mapfile)
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
      QwError << "LoadChannelMap in QwCorrelatorNew read invalid primary_token " << primary_token << QwLog::endl;
    }
  }

  return 0;
}


Int_t QwCorrelatorNew::ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff)
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
          QwWarning << "QwCorrelatorNew::ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff): "
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

  linReg.setDims(nP, nY);
  linReg.init();

  fErrCounts_IV.resize(fIndependentVar.size(),0);
  fErrCounts_DV.resize(fDependentVar.size(),0);

  return 0;
}


void QwCorrelatorNew::ConstructTreeBranches(
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
    QwWarning << "QwCorrelatorNew: no tree name specified, use 'tree-name = value'" << QwLog::endl;
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

  fTree->Branch(TString(branchprefix + "n"), &(linReg.fGoodEventNumber));
  fTree->Branch(TString(branchprefix + "ErrorFlag"), &(linReg.fErrorFlag));

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

  branchm(fTree,linReg.Axy,  "A");
  branchm(fTree,linReg.dAxy, "dA");

  branchm(fTree,linReg.mVPP,  "VPP");
  branchm(fTree,linReg.mVPY,  "VPY");
  branchm(fTree,linReg.mVYP,  "VYP");
  branchm(fTree,linReg.mVYY,  "VYY");
  branchm(fTree,linReg.mVYYp, "VYYp");

  branchm(fTree,linReg.mSPP,  "SPP");
  branchm(fTree,linReg.mSPY,  "SPY");
  branchm(fTree,linReg.mSYP,  "SYP");
  branchm(fTree,linReg.mSYY,  "SYY");
  branchm(fTree,linReg.mSYYp, "SYYp");

  branchm(fTree,linReg.mRPP,  "RPP");
  branchm(fTree,linReg.mRPY,  "RPY");
  branchm(fTree,linReg.mRYP,  "RYP");
  branchm(fTree,linReg.mRYY,  "RYY");
  branchm(fTree,linReg.mRYYp, "RYYp");

  branchv(fTree,linReg.mMP,  "MP");   // Parameter mean
  branchv(fTree,linReg.mMY,  "MY");   // Uncorrected mean
  branchv(fTree,linReg.mMYp, "MYp");  // Corrected mean

  branchv(fTree,linReg.mSP,  "dMP");  // Parameter mean error
  branchv(fTree,linReg.mSY,  "dMY");  // Uncorrected mean error
  branchv(fTree,linReg.mSYp, "dMYp"); // Corrected mean error

}

/// \brief Construct the histograms in a folder with a prefix
void QwCorrelatorNew::ConstructHistograms(TDirectory *folder, TString &prefix)
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
void QwCorrelatorNew::FillHistograms()
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

void QwCorrelatorNew::WriteAlphaFile()
{
  // Ensure in output file
  if (fAlphaOutputFile) fAlphaOutputFile->cd();

  // Write objects
  linReg.Axy.Write("slopes");
  linReg.dAxy.Write("sigSlopes");

  linReg.mRPP.Write("IV_IV_correlation");
  linReg.mRPY.Write("IV_DV_correlation");
  linReg.mRYY.Write("DV_DV_correlation");
  linReg.mRYYp.Write("DV_DV_correlation_prime");

  linReg.mMP.Write("IV_mean");
  linReg.mMY.Write("DV_mean");
  linReg.mMYp.Write("DV_mean_prime");

  // number of events
  TMatrixD Mstat(1,1);
  Mstat(0,0)=linReg.getUsedEve();
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
  linReg.mSP.Write("IV_sigma");
  linReg.mSY.Write("DV_sigma");
  linReg.mSYp.Write("DV_sigma_prime");

  // raw covariances
  linReg.mVPP.Write("IV_IV_rawVariance");
  linReg.mVPY.Write("IV_DV_rawVariance");
  linReg.mVYY.Write("DV_DV_rawVariance");
  linReg.mVYYp.Write("DV_DV_rawVariance_prime");
  TVectorD mVY2(TMatrixDDiag(linReg.mVYY));
  mVY2.Write("DV_rawVariance");
  TVectorD mVP2(TMatrixDDiag(linReg.mVPP));
  mVP2.Write("IV_rawVariance");
  TVectorD mVY2prime(TMatrixDDiag(linReg.mVYYp));
  mVY2prime.Write("DV_rawVariance_prime");

  // normalized covariances
  linReg.mSPP.Write("IV_IV_normVariance");
  linReg.mSPY.Write("IV_DV_normVariance");
  linReg.mSYY.Write("DV_DV_normVariance");
  linReg.mSYYp.Write("DV_DV_normVariance_prime");
  TVectorD sigY2(TMatrixDDiag(linReg.mSYY));
  sigY2.Write("DV_normVariance");
  TVectorD sigX2(TMatrixDDiag(linReg.mSPP));
  sigX2.Write("IV_normVariance");
  TVectorD sigY2prime(TMatrixDDiag(linReg.mSYYp));
  sigY2prime.Write("DV_normVariance_prime");

  linReg.Axy.Write("A_xy");
  linReg.Ayx.Write("A_yx");
}

void QwCorrelatorNew::OpenAlphaFile(const std::string& prefix)
{
  // Create old-style blueR ROOT file
  std::string name = prefix + fAlphaOutputFileBase + run_label.Data() + fAlphaOutputFileSuff;
  std::string path = fAlphaOutputPath + "/";
  std::string file = path + name;
  fAlphaOutputFile = new TFile(TString(file), "RECREATE", "correlation coefficients");
  if (! fAlphaOutputFile->IsWritable()) {
    QwError << "QwCorrelatorNew could not create output file " << file << QwLog::endl;
    delete fAlphaOutputFile;
    fAlphaOutputFile = 0;
  }
}

void QwCorrelatorNew::OpenAliasFile(const std::string& prefix)
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
    QwWarning << "QwCorrelatorNew: Could not write to alias output file " << QwLog::endl;
  }
}

void QwCorrelatorNew::CloseAlphaFile()
{
  // Close slopes output file
  if (fAlphaOutputFile) {
    fAlphaOutputFile->Write();
    fAlphaOutputFile->Close();
  }
}

void QwCorrelatorNew::CloseAliasFile()
{
  // Close alias output file
  if (fAliasOutputFile.good()) {
    fAliasOutputFile << "}" << std::endl << std::endl;
    fAliasOutputFile.close();
  } else {
    QwWarning << "QwCorrelatorNew: Unable to close alias output file." << QwLog::endl;
  }
}

void QwCorrelatorNew::WriteAliasFile()
{
  // Ensure output file is open
  if (fAliasOutputFile.bad()) {
    QwWarning << "QwCorrelatorNew: Could not write to alias output file " << QwLog::endl;
    return;
  }

  fAliasOutputFile << " if (i == " << fCycleCounter << ") {" << std::endl;
  fAliasOutputFile << Form("  TTree* tree = (TTree*) gDirectory->Get(\"mul\");") << std::endl;
  for (int i = 0; i < nY; i++) {
    fAliasOutputFile << Form("  tree->SetAlias(\"reg_%s\",",fDependentFull[i].c_str()) << std::endl;
    fAliasOutputFile << Form("         \"%s",fDependentFull[i].c_str());
    for (int j = 0; j < nP; j++) {
      fAliasOutputFile << Form("%+.4e*%s", -linReg.Axy(j,i), fIndependentFull[j].c_str());
    }
    fAliasOutputFile << "\");" << std::endl;
  }
  fAliasOutputFile << " }" << std::endl;

  // Increment call counter
  fCycleCounter++;
}

#include <assert.h>
#include <math.h>

#include "TString.h"

#include "LinReg_Bevington_Pebay.h"

#include "QwLog.h"

//=================================================
//=================================================
QwCorrelatorNew::QwCorrelatorNew()
: nP(0),nY(0),
  fErrorFlag(-1),
  fGoodEventNumber(0)
{ }

//=================================================
//=================================================
QwCorrelatorNew::QwCorrelatorNew(const QwCorrelatorNew& source)
: nP(source.nP),nY(source.nY),
  fErrorFlag(-1),
  fGoodEventNumber(0)
{
  QwMessage << fGoodEventNumber << QwLog::endl;
}

//=================================================
//=================================================
void QwCorrelatorNew::init()
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

  fGoodEventNumber = 0;
}

void QwCorrelatorNew::clear()
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

  fErrorFlag = -1;
  fGoodEventNumber = 0;
}

//=================================================
//=================================================
void QwCorrelatorNew::print()
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
QwCorrelatorNew& QwCorrelatorNew::operator+=(const std::pair<TVectorD,TVectorD>& rhs)
{
  // Get independent and dependent components
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
QwCorrelatorNew& QwCorrelatorNew::operator+=(const QwCorrelatorNew& rhs)
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
Int_t QwCorrelatorNew::getMeanP(const int i, Double_t &mean) const
{
   mean=-1e50;
   if(i<0 || i >= nP ) return -1;
   if( fGoodEventNumber<1) return -3;
   mean = mMP(i);    return 0;
}


//==========================================================
//==========================================================
Int_t QwCorrelatorNew::getMeanY(const int i, Double_t &mean) const
{
  mean=-1e50;
  if(i<0 || i >= nY ) return -1;
  if( fGoodEventNumber<1) return -3;
  mean = mMY(i);    return 0;
}


//==========================================================
//==========================================================
Int_t QwCorrelatorNew::getMeanYprime(const int i, Double_t &mean) const
{
  mean=-1e50;
  if(i<0 || i >= nY ) return -1;
  if( fGoodEventNumber<1) return -3;
  mean = mMYp(i);    return 0;
}


//==========================================================
//==========================================================
Int_t QwCorrelatorNew::getSigmaP(const int i, Double_t &sigma) const
{
  sigma=-1e50;
  if(i<0 || i >= nP ) return -1;
  if( fGoodEventNumber<2) return -3;
  sigma=sqrt(mVPP(i,i)/(fGoodEventNumber-1.));
  return 0;
}


//==========================================================
//==========================================================
Int_t QwCorrelatorNew::getSigmaY(const int i, Double_t &sigma) const
{
  sigma=-1e50;
  if(i<0 || i >= nY ) return -1;
  if( fGoodEventNumber<2) return -3;
  sigma=sqrt(mVYY(i,i)/(fGoodEventNumber-1.));
  return 0;
}

//==========================================================
//==========================================================
Int_t QwCorrelatorNew::getSigmaYprime(const int i, Double_t &sigma) const
{
  sigma=-1e50;
  if(i<0 || i >= nY ) return -1;
  if( fGoodEventNumber<2) return -3;
  sigma=sqrt(mVYYp(i,i)/(fGoodEventNumber-1.));
  return 0;
}

//==========================================================
//==========================================================
Int_t QwCorrelatorNew::getCovarianceP( int i, int j, Double_t &covar) const
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
Int_t QwCorrelatorNew::getCovariancePY(  int ip, int iy, Double_t &covar) const
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
Int_t QwCorrelatorNew::getCovarianceY( int i, int j, Double_t &covar) const
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
void QwCorrelatorNew::printSummaryP() const
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
void QwCorrelatorNew::printSummaryY() const
{
  QwMessage << Form("\nLinRegBevPeb::printSummaryY seen good eve=%lld  (CSV-format)",fGoodEventNumber)<<QwLog::endl;
  QwMessage << Form("  j,       mean,     sig(mean),   nSig(mean),  sig(distribution)    \n");

  for (int i = 0; i <nY; i++) {
    double meanI,sigI;
    if (getMeanY(i,meanI) < 0) QwWarning << "LRB::getMeanY failed" << QwLog::endl;
    if (getSigmaY(i,sigI) < 0) QwWarning << "LRB::getSigmaY failed" << QwLog::endl;
    double err = sigI / sqrt(fGoodEventNumber);
    double nSigErr = meanI / err;
    QwMessage << Form("Y%02d, %+11.4g, %12.4g, %8.1f, %12.4g "" ",i,meanI,err,nSigErr,sigI)<<QwLog::endl;

  }
}



//==========================================================
//==========================================================
void QwCorrelatorNew::printSummaryAlphas() const
{
  QwMessage << Form("\nLinRegBevPeb::printSummaryAlphas seen good eve=%lld",fGoodEventNumber)<<QwLog::endl;
  QwMessage << Form("\n  j                slope         sigma     mean/sigma\n");
  for (int iy = 0; iy <nY; iy++) {
    QwMessage << Form("dv=Y%d: ",iy)<<QwLog::endl;
    for (int j = 0; j < nP; j++) {
      double val=Axy(j,iy);
      double err=dAxy(j,iy);
      double nSig=val/err;
      char x=' ';
      if(fabs(nSig)>3.) x='*';
      QwMessage << Form("  slope_%d = %11.3g  +/-%11.3g  (nSig=%.2f) %c\n",j,val, err,nSig,x);
    }
  }
}


//==========================================================
//==========================================================
void QwCorrelatorNew::printSummaryYP() const
{
  QwMessage << Form("\nLinRegBevPeb::printSummaryYP seen good eve=%lld",fGoodEventNumber)<<QwLog::endl;

  if(fGoodEventNumber<2) { QwMessage<<"  too few events, skip"<<QwLog::endl; return;}
  QwMessage << Form("\n         name:             ");
  for (int i = 0; i <nP; i++) {
    QwMessage << Form(" %10sP%d "," ",i);
  }
  QwMessage << Form("\n  j                   meanY         sigY      correlation with Ps ....\n");
  for (int iy = 0; iy <nY; iy++) {
    double meanI,sigI;
    if (getMeanY(iy,meanI) < 0) QwWarning << "LRB::getMeanY failed" << QwLog::endl;
    if (getSigmaY(iy,sigI) < 0) QwWarning << "LRB::getSigmaY failed" << QwLog::endl;

    QwMessage << Form(" %3d %6sY%d:  %+12.4g  %12.4g ",iy," ",iy,meanI,sigI);
    for (int ip = 0; ip <nP; ip++) {
      double sigJ,cov;
      if (getSigmaP(ip,sigJ) < 0) QwWarning << "LRB::getSigmaP failed" << QwLog::endl;
      if (getCovariancePY(ip,iy,cov) < 0) QwWarning << "LRB::getCovariancePY failed" << QwLog::endl;
      double corel = cov / sigI / sigJ;
      QwMessage << Form("  %12.3g",corel);
    }
    QwMessage << Form("\n");
  }
}


//==========================================================
//==========================================================
void QwCorrelatorNew::printSummaryMeansWithUnc() const
{
  QwMessage << "Uncorrected Y values:" << QwLog::endl;
  QwMessage << "     mean          sig" << QwLog::endl;
  for (int i = 0; i < nY; i++){
    QwMessage << "Y" << i << ":  " << mMY(i) << " +- " << mSY(i) << QwLog::endl;
  }
  QwMessage << QwLog::endl;
}


//==========================================================
//==========================================================
void QwCorrelatorNew::printSummaryMeansWithUncCorrected() const
{
  QwMessage << "Corrected Y values:" << QwLog::endl;
  QwMessage << "     mean          sig" << QwLog::endl;
  for (int i = 0; i < nY; i++){
    QwMessage << "Y" << i << ":  " << mMYp(i) << " +- " << mSYp(i) << QwLog::endl;
  }
  QwMessage << QwLog::endl;
}


//==========================================================
//==========================================================
void QwCorrelatorNew::solve()
{
  // off-diagonal raw covariance
  mVYP.Transpose(mVPY);

  // diagonal variances
  mVP = TMatrixDDiag(mVPP); mVP.Sqrt();
  mVY = TMatrixDDiag(mVYY); mVY.Sqrt();

  // correlation matrices
  mRPP = mVPP; mRPP.NormByColumn(mVP); mRPP.NormByRow(mVP);
  mRYY = mVYY; mRYY.NormByColumn(mVY); mRYY.NormByRow(mVY);
  mRPY = mVPY; mRPY.NormByColumn(mVP); mRPY.NormByRow(mVY);

  /// normalized covariances
  mSYY = mVYY * (1.0 / (fGoodEventNumber - 1.));
  mSPP = mVPP * (1.0 / (fGoodEventNumber - 1.));
  mSPY = mVPY * (1.0 / (fGoodEventNumber - 1.));
  mSYP.Transpose(mSPY);

  // uncertainties on the means
  mSP = TMatrixDDiag(mSPP); mSP.Sqrt();
  mSY = TMatrixDDiag(mSYY); mSY.Sqrt();

  // Warn if correlation matrix determinant close to zero (heuristic)
  if (mRPP.Determinant() < std::pow(10,-(2*nP))) {
    QwWarning << "LRB: correlation matrix nearly singular, "
              << "determinant = " << mRPP.Determinant()
              << " (set includes highly correlated variable pairs)"
              << QwLog::endl;
    if (fGoodEventNumber > 10*nP) {
      QwMessage << fGoodEventNumber << " events" << QwLog::endl;
      QwMessage << "Covariance matrix: " << QwLog::endl; mVPP.Print();
      QwMessage << "Correlation matrix: " << QwLog::endl; mRPP.Print();
    }
    QwWarning << "LRB: solving failed (this happens when only few events)."
              << QwLog::endl;
    return;
  }
  // slopes
  TMatrixD invRPP(TMatrixD::kInverted, mRPP);
  Axy = TMatrixD(invRPP, TMatrixD::kMult, mRPY);
  Axy.NormByColumn(mSP); // divide
  Axy.NormByRow(mSY, ""); // mult
  Ayx.Transpose(Axy);

  // new means
  mMYp = mMY - Ayx * mMP;

  // new raw covariance
  mVYYp = mVYY + Ayx * mVPP * Axy - (Ayx * mVPY + mVYP * Axy);
  // new variances
  mVYp = TMatrixDDiag(mVYYp); mVYp.Sqrt();

  // new normalized covariance
  mSYYp = mSYY + Ayx * mSPP * Axy - (Ayx * mSPY + mSYP * Axy);
  // uncertainties on the new means
  mSYp = TMatrixDDiag(mSYYp); mSYp.Sqrt();

  // new correlation matrix
  mRYYp = mVYYp; mRYYp.NormByColumn(mVYp); mRYYp.NormByRow(mVYp);

  // slope uncertainties
  double norm = 1. / (fGoodEventNumber - nP - 1);
  dAxy.Zero();
  dAxy.Rank1Update(TMatrixDDiag(invRPP), TMatrixDDiag(mRYYp), norm); // diag mRYYp = row of ones
  dAxy.Sqrt();
  dAxy.NormByColumn(mSP); // divide
  dAxy.NormByRow(mSYp, ""); // mult
  dAyx.Transpose(dAxy);

  fErrorFlag = 0;
}

