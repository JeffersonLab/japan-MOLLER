/*!
 * \file   QwTrendHandler.h
 * \brief  Data handler that books time-ordered TProfile "strip charts"
 * \author GitHub Copilot
 * \date   2026-05-14
 */

#pragma once

// Parent Class
#include "VQwDataHandler.h"

// Forward declarations
class TH1;
class TProfile;

/**
 * \class QwTrendHandler
 * \ingroup QwAnalysis
 * \brief Data handler that replaces time-ordered strip-chart trees with TProfiles
 *
 * Traditional monitoring plots draw a channel value versus event (or pattern)
 * number by streaming every event into a TTree and later projecting
 * `value:CodaEventNumber`.  That keeps a full, unbinned copy of the data on
 * disk purely so a strip chart can be drawn.
 *
 * QwTrendHandler instead pre-books one TProfile per configured channel with the
 * time-ordering index (a monotonically increasing counter that mirrors the
 * event/pattern number) on the x-axis and the channel value on the y-axis.
 * Each bin then stores the mean and RMS of the channel over that slice of the
 * run, which is exactly what a strip chart conveys, at a tiny fraction of the
 * storage cost and with no TTree required.
 *
 * The x-axis auto-extends (TH1::kXaxis) so runs of arbitrary length are
 * handled without knowing the event count in advance.  Because the profiles
 * are ordinary ROOT histograms written into the histogram file, Panguin renders
 * them with the existing `-type prof` option with no plotting-side changes.
 */
class QwTrendHandler : public VQwDataHandler,
                       public MQwDataHandlerCloneable<QwTrendHandler>
{
 public:
  /// \brief Constructor with name
  QwTrendHandler(const TString& name);
  /// \brief Copy constructor
  QwTrendHandler(const QwTrendHandler& source);
  /// Virtual destructor
  ~QwTrendHandler() override;

  /// \brief Load the list of channels to trend (and binning) from the map file
  Int_t LoadChannelMap(const std::string& mapfile) override;

  /// \brief Connect to channels from the per-MPS (event) subsystem array
  /// \param event Subsystem array providing per-MPS yields/mps channels
  /// \return 0 on success
  Int_t ConnectChannels(QwSubsystemArrayParity& event) override;

  // The asym/diff connection uses the VQwDataHandler base implementation,
  // which populates fDependentVar for kHandleTypeAsym/kHandleTypeDiff tokens.

  /// \brief Read the current value of each connected channel
  void ProcessData() override;

  /// \brief Book one TProfile strip chart per connected channel
  void ConstructHistograms(TDirectory* folder, TString& prefix) override;
  /// \brief Fill each strip chart at the current time-ordering index
  void FillHistograms() override;

 protected:
  /// Default constructor (protected for factory/child access)
  QwTrendHandler() { }

  /// Number of x bins for the strip-chart profiles (bounded storage)
  Int_t fNbins;
  /// Initial event/pattern indices per bin; the axis auto-extends (doubling the
  /// bin width) once the run grows past fNbins*fBinWidth events
  Double_t fBinWidth;
  /// Skip filling when (errorflag & mask) != 0 (0 disables the check)
  UInt_t fErrorFlagMask;
  /// Time-ordering index incremented once per filled event/pattern
  Long64_t fCounter;

  /// True when booked into a fixed-size live TMapFile (ConstructHistograms was
  /// called with folder == NULL).  In that mode the strip chart is a rolling
  /// window drawn with a plain TH1D value trace (SetBinContent): the time index
  /// wraps back to the left edge and each new event overwrites its bin.
  /// TProfile's several internal arrays (fArray/fSumw2/fBinEntries/fBinSumw2)
  /// are not marshalled robustly by TMapFile::Update() and crash the live
  /// monitor, so TProfiles are used only for the (auto-extending) file output.
  Bool_t fRolling;

  /// One strip chart per connected channel (owned by the ROOT output
  /// directory).  Held as the TH1 base so file mode can use a TProfile
  /// (mean +/- RMS) while live TMapFile mode uses a robust TH1D value trace.
  std::vector<TH1*> fTrend;
};

// Register this handler with the factory
REGISTER_DATA_HANDLER_FACTORY(QwTrendHandler);
