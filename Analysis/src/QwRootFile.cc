#include "QwRootFile.h"
#include "QwRunCondition.h"
#include "TH1.h"

#include <unistd.h>
#include <cstdio>

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
  : fRootFile(nullptr), fMakePermanent(0),
    fMapFile(nullptr), fEnableMapFile(kFALSE),
    fUpdateInterval(-1)
#ifdef HAS_RNTUPLE_SUPPORT
    , fEnableRNTuples(kFALSE)
#endif // HAS_RNTUPLE_SUPPORT
{
  // Process the configuration options
  ProcessOptions(gQwOptions);

#ifdef QW_ENABLE_MAPFILE
  // Check for the memory-mapped file flag
  if (fEnableMapFile) {

    TString mapfilename = "/dev/shm/";

    mapfilename += "/QwMemMapFile.map";

    fMapFile = TMapFile::Create(mapfilename,"UPDATE", kMaxMapFileSize, "RealTime Producer File");

    if (fMapFile == nullptr) {
      QwError << "Memory-mapped file " << mapfilename
              << " could not be opened!" << QwLog::endl;
      return;
    }

    QwMessage << "================== RealTime Producer Memory Map File =================" << QwLog::endl;
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
    } else {
      rootfilename = fPermanentName;
    }
    fRootFile = new TFile(rootfilename.Data(), "RECREATE", "myfile1");
    if (fRootFile == nullptr) {
      QwError << "ROOT file " << rootfilename
              << " could not be opened!" << QwLog::endl;
      return;
    }       QwMessage << "Opened "<< (fUseTemporaryFile?"temporary ":"")
		<<"rootfile " << rootfilename << QwLog::endl;
   

    TString run_condition_name = Form("condition_%s", run_label.Data());
    auto *run_cond_list = dynamic_cast<TList*>( fRootFile -> FindObjectAny(run_condition_name));
    if (run_cond_list == nullptr) {
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

    fRootFile->SetCompressionLevel(fCompressionLevel);
  }
}


/**
 * Destructor
 */
QwRootFile::~QwRootFile()
{
  // Keep the file on disk if any trees or histograms have been filled.
  // Also respect any other requests to keep the file around.
  if (!fMakePermanent) { fMakePermanent = HasAnyFilled();
}

  // Close the map file
  if (fMapFile != nullptr) {
    fMapFile->Close();
    // TMapFiles may not be deleted
    fMapFile = nullptr;
  }

  // Close the ROOT file.
  // Rename if permanence is requested, remove otherwise
  if (fRootFile != nullptr) {
    TString rootfilename = fRootFile->GetName();

    fRootFile->Close();
    delete fRootFile;
    fRootFile = nullptr;

    int err = 0;
    const char* action = nullptr;
    if (fUseTemporaryFile){
      if (fMakePermanent) {
	action = " rename ";
	err = rename( rootfilename.Data(), fPermanentName.Data() );
      } else {
	action = " remove ";
	err = remove( rootfilename.Data() );
      }
      // It'd be proper to "extern int errno" and strerror() here,
      // but that doesn't seem very C++-ish.
      if (err != 0) {
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

#ifdef HAS_RNTUPLE_SUPPORT
  // Define the RNTuple options
  options.AddOptions("ROOT output options")
    ("enable-rntuples", po::value<bool>()->default_bool_value(false),
     "enable RNTuple output");
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
    ("compression-level", po::value<int>()->default_value(1),
     "TFile compression level");
}


/**
 * Parse the configuration options and store in class fields
 * @param options Options object
 */
void QwRootFile::ProcessOptions(QwOptions &options)
{
  // Option 'rootfiles' to specify ROOT files dir
  fRootFileDir = TString(options.GetValue<std::string>("rootfiles"));

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
  fUseTemporaryFile = options.GetValue<bool>("write-temporary-rootfiles");

#ifdef HAS_RNTUPLE_SUPPORT
  // Option 'enable-rntuples' to enable RNTuple output
  fEnableRNTuples = options.GetValue<bool>("enable-rntuples");
#endif // HAS_RNTUPLE_SUPPORT

  // Options 'disable-trees' and 'disable-histos' for disabling
  // tree and histogram output
  auto v = options.GetValueVector<std::string>("disable-tree");
  std::for_each(v.begin(), v.end(), [&](const std::string& s){ this->DisableTree(s); });
  if (options.GetValue<bool>("disable-trees")) {  DisableTree(".*");
}
  if (options.GetValue<bool>("disable-histos")) { DisableHisto(".*");
}

  // Options 'disable-mps' and 'disable-hel' for disabling
  // helicity window and helicity pattern output
  if (options.GetValue<bool>("disable-mps-tree")) {  DisableTree("^evt$");
}
  if (options.GetValue<bool>("disable-pair-tree")) {  DisableTree("^pr$");
}
  if (options.GetValue<bool>("disable-hel-tree")) {  DisableTree("^mul$");
}
  if (options.GetValue<bool>("disable-burst-tree")) {  DisableTree("^burst$");
}
  if (options.GetValue<bool>("disable-slow-tree")) { DisableTree("^slow$");
}

  // Options 'num-accepted-events' and 'num-discarded-events' for
  // prescaling of the tree output
  fNumMpsEventsToSave = options.GetValue<int>("num-mps-accepted-events");
  fNumMpsEventsToSkip = options.GetValue<int>("num-mps-discarded-events");
  fNumHelEventsToSave = options.GetValue<int>("num-mps-accepted-events");
  fNumHelEventsToSkip = options.GetValue<int>("num-mps-discarded-events");

  // Update interval for the map file
  fCircularBufferSize = options.GetValue<int>("circular-buffer");
  fUpdateInterval = options.GetValue<int>("mapfile-update-interval");
  fCompressionLevel = options.GetValue<int>("compression-level");
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
}

/**
 * Determine whether the rootfile object has any non-empty trees or
 * histograms.
 */
Bool_t QwRootFile::HasAnyFilled() {
  return this->HasAnyFilled(fRootFile);
}
Bool_t QwRootFile::HasAnyFilled(TDirectory* d) {
  if (d == nullptr) {

    return false;
  }
  
  // First check if any in-memory trees have been filled
  for (auto& pair : fTreeByName) {
    for (auto& tree : pair.second) {
      if ((tree != nullptr) && (tree->GetTree() != nullptr)) {
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
#endif // HAS_RNTUPLE_SUPPORT

  TList* l = d->GetListOfKeys();


  for( int i=0; i < l->GetEntries(); ++i) {
    const char* name = l->At(i)->GetName();
    TObject* obj = d->FindObjectAny(name);



    // Objects which can't be found don't count.
    if (obj == nullptr) {

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
      if (this->HasAnyFilled( dynamic_cast<TDirectory*>(obj) )) { return true;
}
    }

    if (obj->IsA()->InheritsFrom( "TTree" )) {
      Long64_t entries = (dynamic_cast<TTree*>( obj))->GetEntries();
      if ( entries != 0 ) { return true;
}
    }

    if (obj->IsA()->InheritsFrom( "TH1" )) {
      Double_t entries = (dynamic_cast<TH1*>( obj))->GetEntries();
      if ( entries != 0.0 ) { return true;
}
    }
  }
  return false;
}
