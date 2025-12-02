/*!
 * \file   QwCorrelator.h
 * \brief  Correlator data handler using LinRegBlue algorithms
 * \author Michael Vallee
 * \date   2018-08-01
 */

#pragma once

// Parent Class
#include "VQwDataHandler.h"

// LinRegBlue Correlator Class
#include "LinReg_Bevington_Pebay.h"

// System headers
#include <fstream>

// Forward declarations
class TH1D;
class TH2D;
class QwRootFile;

/**
 * \class QwCorrelator
 * \ingroup QwAnalysis
 * \brief Data handler computing correlations and linear-regression coefficients
 *
 * Uses Bevington/Pebay algorithms to estimate correlations between independent
 * and dependent variables selected from subsystem arrays. Produces summary
 * histograms and optional output trees/files for further analysis.
 */
class QwCorrelator : public VQwDataHandler, public MQwDataHandlerCloneable<QwCorrelator>
{
 public:
  /// \brief Constructor with name
  QwCorrelator(const TString& name);
  QwCorrelator(const QwCorrelator& name);
  ~QwCorrelator() override;

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

  LinRegBevPeb linReg;

  Int_t fCycleCounter;

  // Default constructor
  QwCorrelator();

};

// Register this handler with the factory
REGISTER_DATA_HANDLER_FACTORY(QwCorrelator);


