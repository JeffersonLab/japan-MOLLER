/*!
 * \file   QwRootFile.cc
 * \brief  Implementation for ROOT file and tree management wrapper classes
 */

#include "QwRootFile.h"
#include "QwRunCondition.h"
#include "TH1.h"
#include "TFileCacheWrite.h"
#include "TROOT.h"

#include <unistd.h>
#include <cstdio>

#include <filesystem>
#include <algorithm>
namespace fs = std::filesystem;

std::string QwRootFile::fDefaultRootFileDir = ".";
std::string QwRootFile::fDefaultRootFileStem = "Qweak_";

const Long64_t QwRootFile::kMaxTreeSize = 100000000000LL;
const Int_t QwRootFile::kMaxMapFileSize = 0x3fffffff; // 1 GiB

const TString QwRootTree::kUnitsName = "ppm/D:ppb/D:um/D:mm/D:mV_uA/D:V_uA/D";
Double_t QwRootTree::kUnitsValue[] = { 1e-6, 1e-9, 1e-3, 1 , 1e-3, 1};

/**
 * Constructor with relative filename
 */
QwRootFile::QwRootFile(const TString& run_label)
  : fRootFile(0), fMakePermanent(0),
    fMapFile(0), fEnableMapFile(kFALSE),
    fUpdateInterval(-1)
#ifdef HAS_RNTUPLE_SUPPORT
    , fEnableRNTuples(kFALSE)
#endif // HAS_RNTUPLE_SUPPORT
{
  // Process the configuration options
  ProcessOptions(gQwOptions);
#ifdef HAS_RNTUPLE_SUPPORT
  if (fEnableRNTuples && gQwOptions.GetValue<bool>("rntuple-writer-thread")) {
    // The writer thread and the event loop use ROOT at the same time
    ROOT::EnableThreadSafety();
    fNTupleWriterThread = std::make_unique<QwRootNTupleWriterThread>(kNTupleWriterQueueSize);
  }
#endif // HAS_RNTUPLE_SUPPORT

#ifdef QW_ENABLE_MAPFILE
  // Check for the memory-mapped file flag
  if (fEnableMapFile) {

    // Verify the target directory exists and is writable before asking ROOT
    // to mmap a file there.  On platforms without /dev/shm (e.g. macOS) the
    // default location is absent; give an actionable message instead of a
    // cryptic TMapFile failure.
    if (gSystem->AccessPathName(fMapFileDir, kWritePermission)) {
      QwError << "Memory-mapped file directory '" << fMapFileDir
              << "' does not exist or is not writable." << QwLog::endl;
      QwError << "Set --mapfile-dir to a writable directory (ideally a RAM "
                 "disk).  /dev/shm does not exist on macOS; a normal directory "
                 "or a hdiutil-mounted RAM disk works there." << QwLog::endl;
      return;
    }

    TString mapfilename = fMapFileDir + "/QwMemMapFile.map";

    fMapFile = TMapFile::Create(mapfilename,"UPDATE", kMaxMapFileSize, "RealTime Producer File");

    if (not fMapFile) {
      QwError << "Memory-mapped file " << mapfilename
              << " could not be opened!" << QwLog::endl;
      return;
    }

    QwMessage << "================== RealTime Producer Memory Map File =================" << QwLog::endl;
    QwMessage << "Memory-mapped file: " << mapfilename << QwLog::endl;
    fMapFile->Print();
    QwMessage << "======================================================================" << QwLog::endl;
  } else
#endif
  {

    TString rootfilename = fRootFileDir;
    TString hostname = gSystem -> HostName();

    // Use a probably-unique temporary file name.
    pid_t pid = getpid();

    fPermanentName = rootfilename
      + Form("/%s%s.root", fRootFileStem.Data(), run_label.Data());
    if (fUseTemporaryFile){
      rootfilename += Form("/%s%s.%s.%d.root",
                           fRootFileStem.Data(), run_label.Data(),
                           hostname.Data(), pid);
      // Delete permanent file if it exists to prevent accumulation across segments
      if (gSystem->AccessPathName(fPermanentName.Data()) == 0) {
        QwVerbose << "Removing existing permanent file: " << fPermanentName << QwLog::endl;
        gSystem->Unlink(fPermanentName.Data());
      }
      // CRITICAL: Also delete the temporary file if it exists!
      // RECREATE mode doesn't properly clear files that contain RNTuples,
      // so we must manually delete before opening
      if (gSystem->AccessPathName(rootfilename.Data()) == 0) {
        QwVerbose << "Removing existing temporary file before RECREATE: " << rootfilename << QwLog::endl;
        gSystem->Unlink(rootfilename.Data());
      }
    } else {
      rootfilename = fPermanentName;
      // Delete permanent file if it exists to ensure RECREATE truly starts fresh
      // This is especially important for RNTuple files where RECREATE doesn't properly clear
      if (gSystem->AccessPathName(rootfilename.Data()) == 0) {
        QwMessage << "File exists before RECREATE, deleting: " << rootfilename << QwLog::endl;
        int unlink_result = gSystem->Unlink(rootfilename.Data());
        if (unlink_result == 0) {
          QwMessage << "Successfully deleted file" << QwLog::endl;
        } else {
          QwError << "Failed to delete file! Error code: " << unlink_result << QwLog::endl;
        }
      } else {
        QwMessage << "File does not exist before RECREATE: " << rootfilename << QwLog::endl;
      }
    }
    QwMessage << "Opening file with RECREATE mode: " << rootfilename << QwLog::endl;
    QwMessage << "QwRootFile constructor called for: " << rootfilename << QwLog::endl;
    // Use TFile::Open instead of `new TFile(...)` so the ROOT plug-in manager
    // can dispatch remote URLs (e.g. root://host/path) to TNetXNGFile etc.
    // The TFile(name, opt, title) constructor refuses remote paths and returns
    // a half-built object, which then segfaults at first use.
    fRootFile = TFile::Open(rootfilename.Data(), "RECREATE", "myfile1");
    if (!fRootFile || fRootFile->IsZombie()) {
      QwError << "ROOT file " << rootfilename
              << " could not be opened!" << QwLog::endl;
      delete fRootFile;
      fRootFile = nullptr;
      return;
    } else {
      QwMessage << "Opened "<< (fUseTemporaryFile?"temporary ":"")
                <<"rootfile " << rootfilename << QwLog::endl;
    }

    TString run_condition_name = Form("condition_%s", run_label.Data());
    TList *run_cond_list = (TList*) fRootFile -> FindObjectAny(run_condition_name);
    if (not run_cond_list) {
      QwRunCondition run_condition(
          gQwOptions.GetArgc(),
          gQwOptions.GetArgv(),
          run_condition_name
      );

      fRootFile -> WriteObject(
          run_condition.Get(),
          run_condition.GetName()
      );
    }

    fRootFile->SetCompressionAlgorithm(fCompressionAlgorithm);
    fRootFile->SetCompressionLevel(fCompressionLevel);

    // Coalesce the many small sequential writes (RNTuple pages, TTree
    // baskets) into large ones; this matters on filesystems with a high
    // per-call cost (network shares, WSL /mnt drives).  ROOT already attaches
    // a write cache to remote files.  The TFile takes ownership of the cache.
    if (fWriteCacheSize > 0 && fRootFile->GetCacheWrite() == nullptr) {
      new TFileCacheWrite(fRootFile, fWriteCacheSize * 1024);
    }
  }
}


/**
 * Destructor
 */
QwRootFile::~QwRootFile()
{
#ifdef HAS_RNTUPLE_SUPPORT
  // Write the queued RNTuple entries before the file is closed
  if (fNTupleWriterThread) fNTupleWriterThread->Stop();
#endif // HAS_RNTUPLE_SUPPORT

  // Keep the file on disk if any trees or histograms have been filled.
  // Also respect any other requests to keep the file around.
  if (!fMakePermanent) fMakePermanent = HasAnyFilled();

  // Close the map file
  if (fMapFile) {
    fMapFile->Close();
    // TMapFiles may not be deleted
    fMapFile = 0;
  }

  // Close the ROOT file.
  // Rename if permanence is requested, remove otherwise
  if (fRootFile) {
    TString rootfilename = fRootFile->GetName();

#ifdef HAS_RNTUPLE_SUPPORT
    // RNTuple writers must be finalized while their file is still open
    fNTupleWriters.clear();
#endif // HAS_RNTUPLE_SUPPORT
    fRootFile->Close();
    delete fRootFile;
    fRootFile = 0;

    int err;
    const char* action;
    if (fUseTemporaryFile){
      if (fMakePermanent) {
        // Delete existing permanent file first to avoid accumulation
        if (gSystem->AccessPathName(fPermanentName.Data()) == 0) {
          remove(fPermanentName.Data());
        }
        action = " rename ";
        err = rename( rootfilename.Data(), fPermanentName.Data() );
      } else {
        action = " remove ";
        err = remove( rootfilename.Data() );
      }
      // It'd be proper to "extern int errno" and strerror() here,
      // but that doesn't seem very C++-ish.
      if (err) {
        QwWarning << "Couldn't" << action << rootfilename << QwLog::endl;
      } else {
        QwMessage << "Was able to" << action << rootfilename << QwLog::endl;
        QwMessage << "Root file is " << fPermanentName << QwLog::endl;
      }
    }
  }

  // Delete Qweak ROOT trees
  std::map< const std::string, std::vector<QwRootTree*> >::iterator map_iter;
  std::vector<QwRootTree*>::iterator vec_iter;
  for (map_iter = fTreeByName.begin(); map_iter != fTreeByName.end(); map_iter++) {
    for (vec_iter = map_iter->second.begin(); vec_iter != map_iter->second.end(); vec_iter++) {
      delete *vec_iter;
    }
  }
}

/**
 * Defines configuration options using QwOptions functionality.
 * @param options Options object
 */
void QwRootFile::DefineOptions(QwOptions &options)
{
  // Define the ROOT files directory
  options.AddOptions("Default options")
    ("rootfiles", po::value<std::string>()->default_value(fDefaultRootFileDir),
     "directory of the output ROOT files");

  // Define the ROOT filename stem
  options.AddOptions("Default options")
    ("rootfile-stem", po::value<std::string>()->default_value(fDefaultRootFileStem),
     "stem of the output ROOT filename");

  // Define the memory map option
  options.AddOptions()
    ("enable-mapfile", po::value<bool>()->default_bool_value(false),
     "enable output to memory-mapped file\n(likely requires circular-buffer too)");
  options.AddOptions()
    ("mapfile-dir", po::value<std::string>()->default_value("/dev/shm"),
     "directory that holds the memory-mapped file (QwMemMapFile.map).  Defaults "
     "to /dev/shm (Linux tmpfs).  Set this on platforms without /dev/shm "
     "(e.g. macOS) to a writable directory, ideally a RAM disk.");
  options.AddOptions()
    ("write-temporary-rootfiles", po::value<bool>()->default_bool_value(true),
     "When writing ROOT files, use the PID to create a temporary filename");

  // Define the histogram and tree options
  options.AddOptions("ROOT output options")
    ("disable-tree", po::value<std::vector<std::string>>()->composing(),
     "disable output to tree regex");
  options.AddOptions("ROOT output options")
    ("disable-trees", po::value<bool>()->default_bool_value(false),
     "disable output to all trees");
  options.AddOptions("ROOT output options")
    ("disable-histos", po::value<bool>()->default_bool_value(false),
     "disable output to all histograms");

  // Define the helicity window versus helicity pattern options
  options.AddOptions("ROOT output options")
    ("disable-mps-tree", po::value<bool>()->default_bool_value(false),
     "disable helicity window output");
  options.AddOptions("ROOT output options")
    ("disable-pair-tree", po::value<bool>()->default_bool_value(false),
     "disable helicity pairs output");
  options.AddOptions("ROOT output options")
    ("disable-hel-tree", po::value<bool>()->default_bool_value(false),
     "disable helicity pattern output");
  options.AddOptions("ROOT output options")
    ("disable-burst-tree", po::value<bool>()->default_bool_value(false),
     "disable burst tree");
  options.AddOptions("ROOT output options")
    ("disable-slow-tree", po::value<bool>()->default_bool_value(false),
     "disable slow control tree");
  options.AddOptions("ROOT output options")
    ("disable-redundant-trees", po::value<bool>()->default_bool_value(false),
     "disable the per-entry trees whose content is already captured by a "
     "matching histogram directory (evt/mul/burst).  Histograms, the "
     "running-sum trees (evts/muls/bursts), the pair tree (pr) and the slow "
     "control tree (slow) are kept.");

#ifdef HAS_RNTUPLE_SUPPORT
  // Define the RNTuple options
  options.AddOptions("ROOT output options")
    ("enable-rntuples", po::value<bool>()->default_bool_value(false),
     "enable RNTuple output");
  options.AddOptions("ROOT output options")
    ("rntuple-writer-thread", po::value<bool>()->default_bool_value(false),
     "write the RNTuple entries in a separate thread (one per output file): "
     "the event loop queues a copy of each entry's values and continues "
     "while the thread serializes, compresses and writes them");
#endif // HAS_RNTUPLE_SUPPORT

  // Define the tree output prescaling options
  options.AddOptions("ROOT output options")
    ("num-mps-accepted-events", po::value<int>()->default_value(0),
     "number of accepted consecutive MPS events");
  options.AddOptions("ROOT output options")
    ("num-mps-discarded-events", po::value<int>()->default_value(0),
     "number of discarded consecutive MPS events");
  options.AddOptions("ROOT output options")
    ("num-hel-accepted-events", po::value<int>()->default_value(0),
     "number of accepted consecutive pattern events");
  options.AddOptions("ROOT output options")
    ("num-hel-discarded-events", po::value<int>()->default_value(0),
     "number of discarded consecutive pattern events");
  options.AddOptions("ROOT output options")
    ("mapfile-update-interval", po::value<int>()->default_value(-1),
     "Events between a map file update");
  options.AddOptions("ROOT output options")
    ("fill-prescale", po::value<int>()->default_value(1),
     "Fill trees and histograms only on every Nth processed event (physics is "
     "still computed every event).  Roughly multiplies live (mapfile) throughput "
     "by N at the cost of fewer accumulated monitoring entries.  Default 1 = "
     "fill every event.");

  // Define the autoflush and autosave option (default values by ROOT)
  options.AddOptions("ROOT performance options")
    ("autoflush", po::value<int>()->default_value(0),
     "TTree autoflush");
  options.AddOptions("ROOT performance options")
    ("autosave", po::value<int>()->default_value(300000000),
     "TTree autosave");
  options.AddOptions("ROOT performance options")
    ("basket-size", po::value<int>()->default_value(16000),
     "TTree basket size");
  options.AddOptions("ROOT performance options")
    ("circular-buffer", po::value<int>()->default_value(0),
     "TTree circular buffer");
  options.AddOptions("ROOT performance options")
    ("compression-algorithm", po::value<int>()->default_value(1),
     "TFile compression algorithm (1=ZLIB, 2=LZMA, 4=LZ4, 5=ZSTD, default=1 ZLIB)");
  options.AddOptions("ROOT performance options")
    ("compression-level", po::value<int>()->default_value(1),
     "TFile compression level (default = 1, no compression = 0)");
  options.AddOptions("ROOT performance options")
    ("rntuple-compression-algorithm", po::value<int>()->default_value(5),
     "RNTuple compression algorithm (1=ZLIB, 2=LZMA, 4=LZ4, 5=ZSTD, default=5 ZSTD)");
  options.AddOptions("ROOT performance options")
    ("rntuple-compression-level", po::value<int>()->default_value(5),
     "RNTuple compression level (0-12, default=5; 0 disables compression)");
  options.AddOptions("ROOT performance options")
    ("rntuple-cluster-size", po::value<int>()->default_value(0),
     "RNTuple approximate compressed cluster size in MiB (0 = ROOT default)");
  options.AddOptions("ROOT performance options")
    ("rntuple-max-unzipped-cluster-size", po::value<int>()->default_value(0),
     "RNTuple maximum uncompressed cluster size in MiB (0 = ROOT default, "
     "or 10x rntuple-cluster-size when that is set)");
  options.AddOptions("ROOT performance options")
    ("rntuple-page-buffer-budget", po::value<int>()->default_value(1024),
     "RNTuple memory budget in MiB for buffered pages of one RNTuple; wide "
     "RNTuples with many fields need a large budget to avoid tiny pages "
     "(default=1024, 0 = ROOT default of 256)");
  options.AddOptions("ROOT performance options")
    ("rntuple-max-page-size", po::value<int>()->default_value(0),
     "RNTuple maximum uncompressed page size in KiB (0 = ROOT default)");
  options.AddOptions("ROOT performance options")
    ("rntuple-channel-arrays", po::value<bool>()->default_bool_value(false),
     "Store each channel's values in one std::array<double,N> RNTuple field "
     "named after the channel (like the TTree leaflist branch), with the "
     "element names in the field description, instead of one field per "
     "value; much faster to write for wide RNTuples");
  options.AddOptions("ROOT performance options")
    ("write-cache-size", po::value<int>()->default_value(4096),
     "Size in KiB of the TFile write cache that coalesces small writes of "
     "TTree baskets and RNTuple pages (default=4096, 0 = disabled)");
}


/**
 * Parse the configuration options and store in class fields
 * @param options Options object
 */
void QwRootFile::ProcessOptions(QwOptions &options)
{
  // Option 'rootfiles' to specify ROOT files dir
  fRootFileDir = TString(options.GetValue<std::string>("rootfiles"));
  fs::path tmppath(fRootFileDir.Data());
  if( ! fs::exists(tmppath) || ! fs::is_directory(tmppath)) {
    QwError << "ERROR:  The rootfile directory path, " << fRootFileDir
            << ", does not exist.  Exiting."
            << QwLog::endl;
    exit(2);
  }

  // Option 'root-stem' to specify ROOT file stem
  fRootFileStem = TString(options.GetValue<std::string>("rootfile-stem"));

  // Option 'mapfile' to enable memory-mapped ROOT file
  fEnableMapFile = options.GetValue<bool>("enable-mapfile");
#ifndef QW_ENABLE_MAPFILE
  if( fEnableMapFile ) {
    QwMessage << QwLog::endl;
    QwWarning << "QwRootFile::ProcessOptions:  "
              << "The 'enable-mapfile' flag is not supported by the ROOT "
                 "version with which this app is built. Disabling it."
              << QwLog::endl;
    fEnableMapFile = false;
  }
#endif
  fMapFileDir = TString(options.GetValue<std::string>("mapfile-dir"));
  while (fMapFileDir.Length() > 1 && fMapFileDir.EndsWith("/"))
    fMapFileDir.Remove(fMapFileDir.Length() - 1);
  fUseTemporaryFile = options.GetValue<bool>("write-temporary-rootfiles");

#ifdef HAS_RNTUPLE_SUPPORT
  // Option 'enable-rntuples' to enable RNTuple output
  fEnableRNTuples = options.GetValue<bool>("enable-rntuples");
  // RNTuples require a TFile (RNTupleWriter::Append takes a TDirectory&);
  // they cannot be hosted by a TMapFile.  If both flags are requested,
  // disable RNTuples and warn loudly so the run does not crash later in
  // QwRootNTuple::InitializeWriter with a null TFile*.
  if (fEnableMapFile && fEnableRNTuples) {
    QwMessage << QwLog::endl;
    QwWarning << "QwRootFile::ProcessOptions:  "
              << "RNTuple output is not supported alongside --enable-mapfile "
                 "(TMapFile is not a TDirectory). Disabling RNTuples."
              << QwLog::endl;
    fEnableRNTuples = false;
  }
#endif // HAS_RNTUPLE_SUPPORT

  // Options 'disable-trees' and 'disable-histos' for disabling
  // tree and histogram output
  auto v = options.GetValueVector<std::string>("disable-tree");
  std::for_each(v.begin(), v.end(), [&](const std::string& s){ this->DisableTreeAndNTuple(s); });
  if (options.GetValue<bool>("disable-trees"))  DisableTree(".*");
  if (options.GetValue<bool>("disable-histos")) DisableHisto(".*");

  // Read --circular-buffer up front so the mapfile-mode logic below can use
  // it (the original ProcessOptions read it further down, but we now need
  // it during the mapfile-tree decision).
  fCircularBufferSize = options.GetValue<int>("circular-buffer");

  // TMapFile-mode tree publishing.
  //
  // TTrees can be published into the mapfile: TDirectoryFile::Append
  // auto-calls TMapFile::Add() when the directory's mother is a TMapFile,
  // so a TTree created while gDirectory == fMapFile->GetDirectory()
  // becomes a TMapRec on its own.  TMapFile::Update() then re-streams the
  // tree (header + baskets) into the 1 GiB mmap on every interval.
  //
  // The hazard is that an unbounded TTree's serialized size grows
  // monotonically and CustomReAlloc2 aborts as soon as it overruns the
  // mmap.  TTree::SetCircular(N) caps the in-memory entry count, which
  // caps the serialized size.  Require a non-zero circular buffer in
  // mapfile mode; if the user did not pass one, force a safe default
  // and warn loudly rather than silently dropping all trees.
  if (fEnableMapFile && fCircularBufferSize == 0) {
    const UInt_t kMapFileCircularDefault = 100;
    QwMessage << QwLog::endl;
    QwWarning << "QwRootFile::ProcessOptions:  "
              << "--enable-mapfile requires a bounded TTree to avoid "
                 "overrunning the " << (kMaxMapFileSize >> 20)
              << " MiB mmap region.  "
                 "Forcing --circular-buffer=" << kMapFileCircularDefault
              << " (pass --circular-buffer=N to override; pass "
                 "--disable-trees to suppress tree output entirely)."
              << QwLog::endl;
    fCircularBufferSize = kMapFileCircularDefault;
  }

#ifdef HAS_RNTUPLE_SUPPORT
  // TTree and RNTuple writers share per-channel state (fTreeArrayIndex,
  // fTreeArrayNumEntries, fDataToSave, b* flags). When both are active,
  // the second Construct*AndVector() call clobbers the first writer's
  // layout, and subsequent Fill*Vector() then walks a vector whose entry
  // types no longer match what was pushed (e.g. SetValue throws
  // "entry type 'D' cannot store unsigned int value 'block2'").
  // Until per-writer layout state is added, make the two writers
  // mutually exclusive: keep RNTuples and silence TTrees.
  if (fEnableRNTuples) {
    QwMessage << QwLog::endl;
    QwMessage << "QwRootFile::ProcessOptions:  "
              << "--enable-rntuples is set; disabling tree output "
                 "(channels share layout state between TTree and RNTuple "
                 "writers, so the two cannot be produced in the same run)."
              << QwLog::endl;
    DisableTree(".*");
  }
#endif // HAS_RNTUPLE_SUPPORT

  // Options 'disable-mps' and 'disable-hel' for disabling
  // helicity window and helicity pattern output (TTrees and RNTuples)
  if (options.GetValue<bool>("disable-mps-tree"))  DisableTreeAndNTuple("^evt$");
  if (options.GetValue<bool>("disable-pair-tree"))  DisableTreeAndNTuple("^pr$");
  if (options.GetValue<bool>("disable-hel-tree"))  DisableTreeAndNTuple("^mul$");
  if (options.GetValue<bool>("disable-burst-tree"))  DisableTreeAndNTuple("^burst$");
  if (options.GetValue<bool>("disable-slow-tree")) DisableTreeAndNTuple("^slow$");

  // Option 'disable-redundant-trees' disables the per-entry trees that are
  // fully captured by their matching histogram directories
  // (evt->evt_histo, mul->mul_histo, burst->burst_histo).  The running-sum
  // trees (evts/muls/bursts) hold the unbinned moments, and the pair (pr)
  // and slow (EPICS) trees have no histogram equivalent, so they are left
  // untouched.
  if (options.GetValue<bool>("disable-redundant-trees")) {
    DisableTreeAndNTuple("^evt$");
    DisableTreeAndNTuple("^mul$");
    DisableTreeAndNTuple("^burst$");
  }

  // Options 'num-accepted-events' and 'num-discarded-events' for
  // prescaling of the tree output
  fNumMpsEventsToSave = options.GetValue<int>("num-mps-accepted-events");
  fNumMpsEventsToSkip = options.GetValue<int>("num-mps-discarded-events");
  fNumHelEventsToSave = options.GetValue<int>("num-mps-accepted-events");
  fNumHelEventsToSkip = options.GetValue<int>("num-mps-discarded-events");

  // Live-output fill prescale.  Filling trees and histograms is a sizeable
  // fraction of the per-event cost, so thinning it to 1-in-N raises throughput
  // (e.g. for live mapfile monitoring) while physics is still processed every
  // event.  The histogram side is gated in FillHistograms() via
  // fHistoFillPrescale; the tree side reuses the existing per-tree prescale.
  // If the user explicitly set a tree prescale we leave it untouched.
  int fillPrescale = options.GetValue<int>("fill-prescale");
  if (fillPrescale < 1) fillPrescale = 1;
  fHistoFillPrescale = fillPrescale;
  if (fillPrescale > 1) {
    if (fNumMpsEventsToSave == 0 && fNumMpsEventsToSkip == 0) {
      fNumMpsEventsToSave = 1;
      fNumMpsEventsToSkip = fillPrescale - 1;
    }
    if (fNumHelEventsToSave == 0 && fNumHelEventsToSkip == 0) {
      fNumHelEventsToSave = 1;
      fNumHelEventsToSkip = fillPrescale - 1;
    }
  }

  // Update interval for the map file
  fUpdateInterval = options.GetValue<int>("mapfile-update-interval");
  fCompressionAlgorithm = options.GetValue<int>("compression-algorithm");
  fCompressionLevel = options.GetValue<int>("compression-level");
  fRNTupleCompressionAlgorithm = options.GetValue<int>("rntuple-compression-algorithm");
  fRNTupleCompressionLevel = options.GetValue<int>("rntuple-compression-level");
  fRNTupleClusterSize = options.GetValue<int>("rntuple-cluster-size");
  fRNTupleMaxUnzippedClusterSize = options.GetValue<int>("rntuple-max-unzipped-cluster-size");
  fRNTuplePageBufferBudget = options.GetValue<int>("rntuple-page-buffer-budget");
  fRNTupleMaxPageSize = options.GetValue<int>("rntuple-max-page-size");
#ifdef HAS_RNTUPLE_SUPPORT
  QwRootNTupleFields::SetLayout(options.GetValue<bool>("rntuple-channel-arrays")
                                ? QwRootNTupleFields::Layout::kArrayPerChannel
                                : QwRootNTupleFields::Layout::kFieldPerValue);
#endif // HAS_RNTUPLE_SUPPORT
  fWriteCacheSize = options.GetValue<int>("write-cache-size");
  fBasketSize = options.GetValue<int>("basket-size");

  // Autoflush and autosave
  fAutoFlush = options.GetValue<int>("autoflush");
  if ((ROOT_VERSION_CODE < ROOT_VERSION(5,26,00)) && fAutoFlush != -30000000){
    QwMessage << QwLog::endl;
    QwWarning << "QwRootFile::ProcessOptions:  "
              << "The 'autoflush' flag is not supported by ROOT version "
              << ROOT_RELEASE
              << QwLog::endl;
  }
  fAutoSave  = options.GetValue<int>("autosave");
  return;
}

#ifdef HAS_RNTUPLE_SUPPORT
/**
 * Build the RNTuple write options from the configured compression and
 * cluster/page settings.  Invalid combinations fall back to the ROOT
 * defaults for the cluster/page settings.
 */
ROOT::RNTupleWriteOptions QwRootFile::GetRNTupleWriteOptions() const
{
  const auto algorithm =
    static_cast<ROOT::RCompressionSetting::EAlgorithm::EValues>(fRNTupleCompressionAlgorithm);
  ROOT::RNTupleWriteOptions options;
  options.SetCompression(algorithm, fRNTupleCompressionLevel);

  constexpr std::size_t kMiB = 1024 * 1024;
  try {
    if (fRNTupleMaxUnzippedClusterSize > 0) {
      options.SetMaxUnzippedClusterSize(fRNTupleMaxUnzippedClusterSize * kMiB);
    }
    if (fRNTupleClusterSize > 0) {
      // Keep ROOT's default ratio of 10 between unzipped and zipped cluster size
      if (fRNTupleMaxUnzippedClusterSize <= 0) {
        options.SetMaxUnzippedClusterSize(
          std::max(options.GetMaxUnzippedClusterSize(), 10 * fRNTupleClusterSize * kMiB));
      }
      options.SetApproxZippedClusterSize(fRNTupleClusterSize * kMiB);
    }
    if (fRNTupleMaxPageSize > 0) {
      options.SetMaxUnzippedPageSize(fRNTupleMaxPageSize * std::size_t(1024));
    }
    if (fRNTuplePageBufferBudget > 0) {
      options.SetPageBufferBudget(fRNTuplePageBufferBudget * kMiB);
    }
  } catch (const std::exception& e) {
    QwError << "QwRootFile::GetRNTupleWriteOptions:  invalid RNTuple cluster/page "
            << "settings (" << e.what() << "); using ROOT defaults" << QwLog::endl;
    options = ROOT::RNTupleWriteOptions();
    options.SetCompression(algorithm, fRNTupleCompressionLevel);
  }
  return options;
}

/**
 * Create an RNTuple with a custom model in this file.  The writer is owned by
 * this file and finalized in Close() (or the destructor) before the file is
 * closed.
 */
ROOT::RNTupleWriter* QwRootFile::NewNTupleWriter(
    const std::string& name,
    std::unique_ptr<ROOT::RNTupleModel> model)
{
  if (!fEnableRNTuples || !fRootFile || !model || IsNTupleDisabled(name)) return nullptr;
  // Creating the writer writes the RNTuple header to the file
  DrainNTupleWriterThread();
  if (HasNTupleByName(name) ||
      std::find(fNTupleWriterNames.begin(), fNTupleWriterNames.end(), name) != fNTupleWriterNames.end()) {
    QwError << "Cannot create duplicate RNTuple: " << name << QwLog::endl;
    return nullptr;
  }
  size_t nfields = 0;
  for (auto it = model->GetDefaultEntry().begin(); it != model->GetDefaultEntry().end(); ++it) nfields++;
  try {
    fNTupleWriters.push_back(
      ROOT::RNTupleWriter::Append(std::move(model), name, *fRootFile, GetRNTupleWriteOptions()));
    fNTupleWriterNames.push_back(name);
  } catch (const std::exception& e) {
    QwError << "Failed to create RNTuple writer for '" << name << "': " << e.what() << QwLog::endl;
    return nullptr;
  }
  QwMessage << "Created RNTuple '" << name << "' with " << nfields
            << " fields in file " << fRootFile->GetName() << QwLog::endl;
  return fNTupleWriters.back().get();
}

void QwRootNTupleWriterThread::Push(QwRootNTuple* ntuple, const std::vector<Double_t>& values)
{
  std::vector<Double_t> buffer;
  {
    std::unique_lock<std::mutex> lock(fMutex);
    if (!fThread.joinable() && !fStop) fThread = std::thread(&QwRootNTupleWriterThread::Run, this);
    fSpace.wait(lock, [this] { return fQueue.size() < fCapacity; });
    if (!fFree.empty()) {
      buffer = std::move(fFree.back());
      fFree.pop_back();
    }
  }
  // Copy outside the lock; a reused buffer keeps its memory
  buffer.assign(values.begin(), values.end());
  bool wake;
  {
    std::lock_guard<std::mutex> lock(fMutex);
    fQueue.push_back(Item{ntuple, std::move(buffer)});
    wake = fWaiting && fQueue.size() >= kBatch;
  }
  if (wake) fWork.notify_one();
  ReportError();
}

void QwRootNTupleWriterThread::Run()
{
  for (;;) {
    Item item;
    {
      std::unique_lock<std::mutex> lock(fMutex);
      if (fQueue.empty()) {
        // Everything is written: sleep until a batch of entries is queued
        fIdle.notify_all();
        fWaiting = true;
        fWork.wait(lock, [this] {
          return fQueue.size() >= kBatch || (fFlush && !fQueue.empty()) || fStop;
        });
        fWaiting = false;
        if (fQueue.empty()) break;  // stopped, and everything is written
      }
      item = std::move(fQueue.front());
      fQueue.pop_front();
      fBusy = true;
    }
    fSpace.notify_one();

    std::string error;
    try {
      item.ntuple->WriteEntry(item.values);
    } catch (const std::exception& e) {
      error = "RNTuple " + item.ntuple->GetName() + ": " + e.what();
    }

    {
      std::lock_guard<std::mutex> lock(fMutex);
      if (!error.empty() && fError.empty()) fError = error;
      fFree.push_back(std::move(item.values));
      fBusy = false;
    }
    fIdle.notify_all();
  }
  fIdle.notify_all();
}

void QwRootNTupleWriterThread::Drain()
{
  {
    std::unique_lock<std::mutex> lock(fMutex);
    fFlush = true;
    fWork.notify_one();
    fIdle.wait(lock, [this] { return (fQueue.empty() && !fBusy) || !fThread.joinable(); });
    fFlush = false;
  }
  ReportError();
}

void QwRootNTupleWriterThread::Stop()
{
  {
    std::lock_guard<std::mutex> lock(fMutex);
    fStop = true;
  }
  fWork.notify_all();
  if (fThread.joinable()) fThread.join();
  ReportError();
}

void QwRootNTupleWriterThread::ReportError()
{
  // Logging is done by the calling (main) thread only
  std::string error;
  {
    std::lock_guard<std::mutex> lock(fMutex);
    if (fError.empty() || fErrorReported) return;
    fErrorReported = true;
    error = fError;
  }
  QwError << "RNTuple writer thread: " << error << QwLog::endl;
}
#endif // HAS_RNTUPLE_SUPPORT

/**
 * Determine whether the rootfile object has any non-empty trees or
 * histograms.
 */
Bool_t QwRootFile::HasAnyFilled(void) {
  return this->HasAnyFilled(fRootFile);
}
Bool_t QwRootFile::HasAnyFilled(TDirectory* d) {
  if (!d) {

    return false;
  }

  // First check if any in-memory trees have been filled
  for (auto& pair : fTreeByName) {
    for (auto& tree : pair.second) {
      if (tree && tree->GetTree()) {
        Long64_t entries = tree->GetTree()->GetEntries();
        if (entries > 0) {

          return true;
        }
      }
    }
  }

#ifdef HAS_RNTUPLE_SUPPORT
  // Then check if any RNTuples have been filled
  for (auto& pair : fNTupleByName) {
    for (auto& ntuple : pair.second) {
      if (ntuple && ntuple->fCurrentEvent > 0) {

        return true;
      }
    }
  }
  for (auto& writer : fNTupleWriters) {
    if (writer && writer->GetNEntries() > 0) return true;
  }
#endif // HAS_RNTUPLE_SUPPORT

  TList* l = d->GetListOfKeys();


  for( int i=0; i < l->GetEntries(); ++i) {
    const char* name = l->At(i)->GetName();
    TObject* obj = d->FindObjectAny(name);



    // Objects which can't be found don't count.
    if (!obj) {

      continue;
    }

    // Lists of parameter files, map files, and job conditions don't count.
    if ( TString(name).Contains("parameter_file") ) {

      continue;
    }
    if ( TString(name).Contains("mapfile") ) {
      continue;
    }
    if ( TString(name).Contains("_condition") ) {
      continue;
    }
    //  The EPICS tree doesn't count
    if ( TString(name).Contains("slow") ) {
      continue;
    }

    // Recursively check subdirectories.
    if (obj->IsA()->InheritsFrom( "TDirectory" )) {
      if (this->HasAnyFilled( (TDirectory*)obj )) return true;
    }

    if (obj->IsA()->InheritsFrom( "TTree" )) {
      Long64_t entries = ((TTree*) obj)->GetEntries();
      if ( entries ) return true;
    }

    if (obj->IsA()->InheritsFrom( "TH1" )) {
      Double_t entries = ((TH1*) obj)->GetEntries();
      if ( entries ) return true;
    }
  }
  return false;
}
