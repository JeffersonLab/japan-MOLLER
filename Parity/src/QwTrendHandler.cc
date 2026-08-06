/*!
 * \file   QwTrendHandler.cc
 * \brief  Implementation of the time-ordered TProfile strip-chart data handler
 * \author GitHub Copilot
 * \date   2026-05-14
 */

#include "QwTrendHandler.h"

// ROOT headers
#include "TDirectory.h"
#include "TH1.h"
#include "TProfile.h"

// Qweak headers
#include "QwLog.h"
#include "QwParameterFile.h"
#include "VQwHardwareChannel.h"


/// \brief Constructor with name
QwTrendHandler::QwTrendHandler(const TString& name)
: VQwDataHandler(name),
  fNbins(200),
  fBinWidth(1.0),
  fErrorFlagMask(0),
  fCounter(0),
  fRolling(kFALSE)
{
  ParseSeparator = ":";
}

/// \brief Copy constructor (histograms are rebuilt, not copied)
QwTrendHandler::QwTrendHandler(const QwTrendHandler& source)
: VQwDataHandler(source),
  fNbins(source.fNbins),
  fBinWidth(source.fBinWidth),
  fErrorFlagMask(source.fErrorFlagMask),
  fCounter(0),
  fRolling(kFALSE)
{
  // The TProfiles are owned by the ROOT output directory and are recreated in
  // ConstructHistograms(); a cloned handler must not share those pointers.
}

/// Destructor
QwTrendHandler::~QwTrendHandler()
{
  // The TProfiles are owned by the ROOT output directory (SetDirectory), so
  // they must not be deleted here.
  fTrend.clear();
}

/** Load the list of channels to trend, plus optional binning.
 *
 * Map format (one channel token per line):
 * \code
 *   nbins = 200          # optional, default 200
 *   binwidth = 1         # optional initial event(s) per bin, default 1
 *   mask  = 0x0          # optional error-flag mask, default 0 (fill always)
 *   asym:bcm_an_ds
 *   diff:bpm1c10WSX
 *   yield:sam1
 * \endcode
 * The type prefix (asym/diff/yield/mps) selects which subsystem array supplies
 * the channel; the remainder is the channel name.
 *
 * @param mapfile Filename of map file
 * @return Zero on success
 */
Int_t QwTrendHandler::LoadChannelMap(const std::string& mapfile)
{
  QwParameterFile map(mapfile);

  // Read optional binning/mask settings (searched over the whole file)
  TString value;
  if (map.FileHasVariablePair("=", "nbins", value)) {
    fNbins = value.Atoi();
  }
  if (map.FileHasVariablePair("=", "binwidth", value)) {
    fBinWidth = value.Atof();
    if (fBinWidth <= 0.0) fBinWidth = 1.0;
  }
  if (map.FileHasVariablePair("=", "mask", value)) {
    fErrorFlagMask = QwParameterFile::GetUInt(value);
  }

  // Read the channel list, one token per line (this map has no [sections])
  map.RewindToFileStart();
  while (map.ReadNextLine()) {
    map.TrimComment();
    map.TrimWhitespace();
    if (map.LineIsEmpty()) continue;
    if (map.LineHasSectionHeader()) continue;
    // Skip the key=value settings lines handled above
    if (map.GetLine().find('=') != std::string::npos) continue;

    std::string token = map.GetNextToken(" \t,");
    if (token.empty()) continue;

    std::pair<EQwHandleType, std::string> type_name = ParseHandledVariable(token);
    fDependentType.push_back(type_name.first);
    fDependentName.push_back(type_name.second);
    fDependentFull.push_back(token);
  }

  QwMessage << "QwTrendHandler " << fName << ": trending "
            << fDependentName.size() << " channel(s) into TProfiles ("
            << fNbins << " bins, initial bin width " << fBinWidth
            << " event(s), x auto-extends)." << QwLog::endl;
  return 0;
}

/** Connect to the requested channels in the per-MPS (event) subsystem array. */
Int_t QwTrendHandler::ConnectChannels(QwSubsystemArrayParity& event)
{
  SetEventcutErrorFlagPointer(event.GetEventcutErrorFlagPointer());

  for (size_t dv = 0; dv < fDependentName.size(); dv++) {
    // Only yield/mps tokens are meaningful for the per-MPS array
    if (fDependentType.at(dv) == kHandleTypeAsym ||
        fDependentType.at(dv) == kHandleTypeDiff) {
      continue;
    }
    const VQwHardwareChannel* dv_ptr =
        event.RequestExternalPointer(fDependentName.at(dv));
    if (dv_ptr == NULL) {
      QwWarning << "QwTrendHandler::ConnectChannels(event): channel "
                << fDependentName.at(dv) << " was not found." << QwLog::endl;
      continue;
    }
    fDependentVar.push_back(dv_ptr);
  }
  return 0;
}

/** Read the current value of each connected channel. */
void QwTrendHandler::ProcessData()
{
  if (fDependentValues.size() != fDependentVar.size()) {
    fDependentValues.resize(fDependentVar.size(), 0.0);
  }
  for (size_t i = 0; i < fDependentVar.size(); i++) {
    fDependentValues.at(i) =
        (fDependentVar.at(i) != NULL) ? fDependentVar.at(i)->GetValue() : 0.0;
  }
}

/** Book one TProfile strip chart per connected channel. */
void QwTrendHandler::ConstructHistograms(TDirectory* folder, TString& prefix)
{
  // folder == NULL means we are booking into a live, fixed-size TMapFile, where
  // the x-axis must not auto-extend; draw a rolling strip chart instead.
  fRolling = (folder == NULL);
  fTrend.assign(fDependentVar.size(), NULL);
  for (size_t i = 0; i < fDependentVar.size(); i++) {
    if (fDependentVar.at(i) == NULL) continue;

    TString channel = fDependentVar.at(i)->GetElementName();
    TString name  = prefix + fName + "_" + channel;
    TString title = TString("Trend ") + channel
                  + ";event index;" + channel;

    // In file mode 'folder' is the target directory (e.g. mul_histo).  In live
    // TMapFile mode QwRootFile passes a NULL folder, so every trend handler
    // books into the single flat mapfile directory.  The same handler is loaded
    // in both the pattern (mul) and burst arrays, which would create two
    // profiles with identical names; the second construction silently replaces
    // the first in the directory and only the survivor is streamed.  Keep the
    // names unique so the first (pattern-scope, per-pattern) profile is not
    // overwritten by the later burst-scope copy.
    if (folder == NULL && gDirectory) {
      TString unique = name;
      int dup = 1;
      while (gDirectory->FindObject(unique) != NULL) {
        unique = name + Form("_%d", ++dup);
      }
      name = unique;
    }

    TH1* chart = NULL;
    if (fRolling) {
      // Live TMapFile mode: use a plain TH1D value trace.  TProfile keeps
      // several independent internal arrays (fArray, fSumw2, fBinEntries,
      // fBinSumw2) that TMapFile::Update() does not marshal robustly and which
      // deterministically corrupt the shared-memory region after a few hundred
      // updates.  A TH1D has a single fixed fArray, is filled with
      // SetBinContent (no reallocation), and streams reliably in a TMapFile
      // exactly like the subsystem monitoring histograms.  The chart stays
      // attached to the current directory (the mapfile) so it is re-streamed.
      chart = new TH1D(name, title, fNbins, 0.0, fNbins * fBinWidth);
    } else {
      // File mode: a TProfile gives the mean +/- RMS per bin and can safely
      // auto-extend its x-axis for runs of arbitrary length.
      TProfile* prof =
          new TProfile(name, title, fNbins, 0.0, fNbins * fBinWidth);
      prof->SetCanExtend(TH1::kXaxis);
      prof->SetDirectory(folder);
      chart = prof;
    }
    fTrend.at(i) = chart;
  }
}

/** Fill each strip chart at the current time-ordering index. */
void QwTrendHandler::FillHistograms()
{
  const bool good =
      !(fErrorFlagMask != 0 && fErrorFlagPtr != NULL &&
        (*fErrorFlagPtr & fErrorFlagMask) != 0);

  // In rolling (live mapfile) mode the fixed x-range spans this many event
  // indices; the time index wraps within it so the chart sweeps continuously.
  const Long64_t span =
      (Long64_t)(fNbins * fBinWidth) > 0 ? (Long64_t)(fNbins * fBinWidth) : 1;
  const Double_t xpos = fRolling ? (Double_t)(fCounter % span) : (Double_t) fCounter;

  // NOTE: In live TMapFile mode we deliberately do NOT call TProfile::Reset()
  // between sweeps.  Reset() (and Fill's lazy buffer/array management) can
  // change the object's serialized footprint inside the fixed-size mmap and
  // crash TMapFile::Update().  Instead the fixed x-range simply accumulates:
  // each bin holds the running mean over all sweeps, which is stable in shared
  // memory.  (DIAGNOSTIC: reset disabled to isolate the TMapFile::Update crash.)

  if (good) {
    for (size_t i = 0; i < fTrend.size(); i++) {
      if (fTrend.at(i) != NULL && i < fDependentValues.size()) {
        if (fRolling) {
          // Live rolling trace: overwrite this event index's bin with the
          // latest value.  SetBinContent never reallocates, so the object's
          // footprint is stable in the fixed-size TMapFile.
          const Int_t bin = fTrend.at(i)->GetXaxis()->FindBin(xpos);
          fTrend.at(i)->SetBinContent(bin, fDependentValues.at(i));
        } else {
          // File mode: accumulate mean +/- RMS in the TProfile.
          static_cast<TProfile*>(fTrend.at(i))->Fill(xpos, fDependentValues.at(i));
        }
      }
    }
  }
  // Advance the time index even for skipped events so the x-axis stays aligned
  // with the true event/pattern ordering.
  fCounter++;
}
