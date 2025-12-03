#pragma once

// Parent Class
#include "VQwDataHandler.h"

// System headers
#include <fstream>
#include <utility>
// Forward declarations
class TH1D;
class TH2D;
class QwRootFile;

// ROOT headers
#include <TVectorD.h>
#include <TMatrixD.h>
/**
 * \class QwCorrelatorNew
 * \ingroup QwAnalysis
 * \brief Data handler computing correlations and linear-regression coefficients
 *
 * Uses Bevington/Pebay algorithms to estimate correlations between independent
 * and dependent variables selected from subsystem arrays. Produces summary
 * histograms and optional output trees/files for further analysis.
 */
class QwCorrelatorNew : public VQwDataHandler, public MQwDataHandlerCloneable<QwCorrelatorNew>
{
 public:
  /// \brief Constructor with name
  QwCorrelatorNew(const TString& name);
  QwCorrelatorNew(const QwCorrelatorNew& name);
  ~QwCorrelatorNew() override;

 private:
  static bool fPrintCorrelations;
 public:
  static void DefineOptions(QwOptions &options);
  void ProcessOptions(QwOptions &options);

  void ParseConfigFile(QwParameterFile& file) override;

  Int_t LoadChannelMap(const std::string& mapfile) override;

  /// \brief Connect to Channels (asymmetry/difference only)
  /// \param asym Subsystem array with asymmetries
  /// \param diff Subsystem array with differences
  /// \return 0 on success, non-zero on failure
  Int_t ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override;

  void ProcessData() override;
  void FinishDataHandler() override{
    CalcCorrelations();
  }
  void CalcCorrelations();

  /// \brief Construct the tree branches
  void ConstructTreeBranches(
      QwRootFile *treerootfile,
      const std::string& treeprefix = "",
      const std::string& branchprefix = "") override;
  /// \brief Fill the tree branches
  void FillTreeBranches(QwRootFile *treerootfile) override { };

  /// \brief Construct the histograms in a folder with a prefix
  void  ConstructHistograms(TDirectory *folder, TString &prefix) override;
  /// \brief Fill the histograms
  void  FillHistograms() override;

  void ClearEventData() override;
  void AccumulateRunningSum(VQwDataHandler &value, Int_t count = 0, Int_t ErrorMask = 0xFFFFFFF) override;

 protected:

  Int_t fBlock;

  bool fDisableHistos;

  std::vector< std::string > fIndependentFull;

  //  Using the fDependentType and fDependentName from base class, but override the IV arrays
  std::vector< EQwHandleType > fIndependentType;
  std::vector< std::string > fIndependentName;

  std::vector< const VQwHardwareChannel* > fIndependentVar;
  std::vector< Double_t > fIndependentValues;

  std::string fAlphaOutputFileBase;
  std::string fAlphaOutputFileSuff;
  std::string fAlphaOutputPath;
  TFile* fAlphaOutputFile;
  void OpenAlphaFile(const std::string& prefix);
  void WriteAlphaFile();
  void CloseAlphaFile();

  TTree* fTree;

  std::string fAliasOutputFileBase;
  std::string fAliasOutputFileSuff;
  std::string fAliasOutputPath;
  std::ofstream fAliasOutputFile;
  void OpenAliasFile(const std::string& prefix);
  void WriteAliasFile();
  void CloseAliasFile();

  int fTotalCount;
  int fGoodCount;

  int fErrCounts_EF;
  std::vector<int> fErrCounts_IV;
  std::vector<int> fErrCounts_DV;

  unsigned int fGoodEvent;

 private:

  TString fNameNoSpaces;
  int nP, nY;

  // monitoring histos for iv & dv
  std::vector<TH1D> fHnames;
  std::vector<TH1D> fH1iv;
  std::vector<TH1D> fH1dv;
  std::vector<std::vector<TH2D>> fH2iv;
  std::vector<std::vector<TH2D>> fH2dv;

  QwCorrelatorNew linReg;

  Int_t fCycleCounter;

  QwCorrelatorNew();

  
 private:
  Int_t fErrorFlag;             ///< is information valid
  Long64_t fGoodEventNumber;    ///< accumulated so far

  /// correlations
  TMatrixD mRPY, mRYP;
  TMatrixD mRPP, mRYY;
  TMatrixD mRYYp;

  /// unnormalized covariances
  TMatrixD mVPY, mVYP;
  TMatrixD mVPP, mVYY;
  TMatrixD mVYYp;
  /// variances
  TVectorD mVP, mVY;
  TVectorD mVYp;

  /// normalized covariances
  TMatrixD mSPY, mSYP;
  TMatrixD mSPP, mSYY;
  TMatrixD mSYYp;
  /// sigmas
  TVectorD mSP, mSY;
  TVectorD mSYp;

  /// mean values
  TVectorD mMP, mMY, mMYp;


  /// slopes
  TMatrixD Axy, Ayx, dAxy, dAyx; // found slopes and their standard errors

 public:

  QwCorrelatorNew();
  QwCorrelatorNew(const QwCorrelatorNew& source);
  virtual ~QwCorrelatorNew() { };

  void solve();
  bool failed() { return fGoodEventNumber < nP + 1; }

  // after last event
  void printSummaryP() const;
  void printSummaryY() const;
  void printSummaryYP() const;
  void printSummaryAlphas() const;
  void printSummaryMeansWithUnc() const;
  void printSummaryMeansWithUncCorrected() const;

  void print();
  void init();
  void clear();
  void setDims(int a, int b){ nP = a; nY = b;}

  /// Get mean value of a variable, returns error code
  Int_t getMeanP(const int i, Double_t &mean) const;
  Int_t getMeanY(const int i, Double_t &mean) const;
  Int_t getMeanYprime(const int i, Double_t &mean) const;

  /// Get mean value of a variable, returns error code
  Int_t getSigmaP(const int i, Double_t &sigma) const;
  Int_t getSigmaY(const int i, Double_t &sigma) const;
  Int_t getSigmaYprime(const int i, Double_t &sigma) const;

  /// Get mean value of a variable, returns error code
  Int_t getCovarianceP (int i,  int j,  Double_t &covar) const;
  Int_t getCovariancePY(int ip, int iy, Double_t &covar) const;
  Int_t getCovarianceY (int i,  int j,  Double_t &covar) const;

  double  getUsedEve() const { return fGoodEventNumber; };

  // Addition-assignment
  QwCorrelatorNew& operator+=(const std::pair<TVectorD,TVectorD>& rhs);
  QwCorrelatorNew& operator+=(const QwCorrelatorNew& rhs);
  // Addition using addition-assignment
  friend // friends defined inside class body are inline and are hidden from non-ADL lookup
  QwCorrelatorNew operator+(QwCorrelatorNew lhs,  // passing lhs by value helps optimize chained a+b+c
                   const QwCorrelatorNew& rhs) // otherwise, both parameters may be const references
  {
    lhs += rhs; // reuse compound assignment
    return lhs; // return the result by value (uses move constructor)
  }

  /// \brief Output stream operator
  friend std::ostream& operator<< (std::ostream& stream, const QwCorrelatorNew& h);

  /// Friend class with correlator for ROOT tree output
  friend class QwCorrelator;
};

/// Output stream operator
inline std::ostream& operator<< (std::ostream& stream, const QwCorrelatorNew& h)
{
  stream << "LRB: " << h.fGoodEventNumber << " events";
  return stream;
}


// Register this handler with the factory
REGISTER_DATA_HANDLER_FACTORY(QwCorrelatorNew);


