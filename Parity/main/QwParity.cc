/*------------------------------------------------------------------------*//*!

 \file QwParity.cc

 \brief main(...) function for the qwparity executable

*//*-------------------------------------------------------------------------*/

// System headers
#include <iostream>
#include <fstream>
#include <vector>
#include <new>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

// ROOT headers
#include "Rtypes.h"
#include "TROOT.h"
#include "TFile.h"
#include "THLimitsFinder.h"
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTuple.hxx"
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RField.hxx"
#endif

// Qweak headers
#include "QwLog.h"
#include "QwRootFile.h"
#include "QwOptionsParity.h"
#include "QwEventBuffer.h"
#ifdef __USE_DATABASE__
#include "QwParityDB.h"
#endif //__USE_DATABASE__
#include "QwHistogramHelper.h"
#include "QwSubsystemArrayParity.h"
#include "QwHelicityPattern.h"
#include "QwEventRing.h"
#include "QwWorkerThread.h"
#include "QwEPICSEvent.h"
#include "QwCombiner.h"
#include "QwCombinerSubsystem.h"
#include "QwPromptSummary.h"
#include "QwCorrelator.h"
#include "QwCorrelatorNew.h"
#include "GrandCorrelator.h"
#include "LRBCorrector.h"
#include "QwExtractor.h"
#include "QwDataHandlerArray.h"

// Qweak subsystems
// (for correct dependency generation)
#include "QwHelicity.h"
#include "QwFakeHelicity.h"
#include "QwBeamLine.h"
#include "QwBeamMod.h"

// Valgrind headers
#if __has_include(<valgrind/callgrind.h>)
#include <valgrind/callgrind.h>
#endif

/**
 * Events handed from the reader thread (first stage of the event loop:
 * decoding, event cuts, event ring) to the main thread (--pipeline).  The
 * events are copied into a few preallocated slots; EPICS events go through
 * the same queue, so that the main thread sees everything in order.
 */
class QwEventPipeline {
  public:
    enum Kind { kEvent, kEPICS, kEnd };
    struct Item {
      Kind kind;
      QwSubsystemArrayParity* event;
    };

    QwEventPipeline(const QwSubsystemArrayParity& prototype, size_t nslots) {
      for (size_t i = 0; i < nslots; i++) {
        fSlots.push_back(std::make_unique<QwSubsystemArrayParity>(prototype));
        fFree.push_back(fSlots.back().get());
      }
    }

    /// Reader thread: a slot to copy the next event into (waits for one)
    QwSubsystemArrayParity* GetFreeSlot() {
      std::unique_lock<std::mutex> lock(fMutex);
      fCond.wait(lock, [this] { return !fFree.empty() || fCancelled; });
      if (fCancelled) throw std::runtime_error("event pipeline cancelled");
      QwSubsystemArrayParity* slot = fFree.back();
      fFree.pop_back();
      return slot;
    }
    /// Reader thread: pass an event (or the end of the events)
    void Push(Kind kind, QwSubsystemArrayParity* event = nullptr) {
      {
        std::lock_guard<std::mutex> lock(fMutex);
        fQueue.push_back(Item{kind, event});
      }
      fCond.notify_all();
    }
    /// Reader thread: pass an EPICS event, and wait until the main thread
    /// has processed it (the reader reuses the EPICS event object)
    void PushEPICSAndWait() {
      std::unique_lock<std::mutex> lock(fMutex);
      const size_t sent = ++fEPICSSent;
      fQueue.push_back(Item{kEPICS, nullptr});
      fCond.notify_all();
      fCond.wait(lock, [&] { return fEPICSDone >= sent || fCancelled; });
      if (fCancelled) throw std::runtime_error("event pipeline cancelled");
    }

    /// Main thread: the next item (waits for it)
    Item Pop() {
      std::unique_lock<std::mutex> lock(fMutex);
      fCond.wait(lock, [this] { return !fQueue.empty(); });
      Item item = fQueue.front();
      fQueue.pop_front();
      return item;
    }
    /// Main thread: the event of a slot has been copied
    void Recycle(QwSubsystemArrayParity* slot) {
      {
        std::lock_guard<std::mutex> lock(fMutex);
        fFree.push_back(slot);
      }
      fCond.notify_all();
    }
    /// Main thread: the EPICS event has been processed
    void EPICSDone() {
      {
        std::lock_guard<std::mutex> lock(fMutex);
        fEPICSDone++;
      }
      fCond.notify_all();
    }
    /// Main thread: stop the reader thread (after an error)
    void Cancel() {
      {
        std::lock_guard<std::mutex> lock(fMutex);
        fCancelled = true;
      }
      fCond.notify_all();
    }

  private:
    std::mutex fMutex;
    std::condition_variable fCond;
    std::vector<std::unique_ptr<QwSubsystemArrayParity>> fSlots;
    std::vector<QwSubsystemArrayParity*> fFree;
    std::deque<Item> fQueue;
    size_t fEPICSSent = 0;
    size_t fEPICSDone = 0;
    bool fCancelled = false;
};

Int_t main(Int_t argc, Char_t* argv[])
{
  ///  Enable implicit multi-threading in e.g. TTree::Fill
  ROOT::EnableImplicitMT();

  ///  Define the command line options
  DefineOptionsParity(gQwOptions);

  ///  Define additional command line arguments and the configuration filename,
  ///  and we define the options that can be used in them (using QwOptions).
  gQwOptions.AddOptions()("single-output-file", po::value<bool>()->default_bool_value(false), "Write a single output file");
  gQwOptions.AddOptions()("print-errorcounters", po::value<bool>()->default_bool_value(true), "Print summary of error counters");
  gQwOptions.AddOptions()("write-promptsummary", po::value<bool>()->default_bool_value(false), "Write PromptSummary");
  gQwOptions.AddOptions()("callgrind-instr-start-event-loop", po::value<bool>()->default_bool_value(false), "Start callgrind instrumentation with main event loop (with --instr-atstart=no)");
  gQwOptions.AddOptions()("callgrind-instr-stop-event-loop", po::value<bool>()->default_bool_value(false), "Stop callgrind instrumentation with main event loop (with --instr-atstart=no)");
  gQwOptions.AddOptions()("async-histograms", po::value<bool>()->default_bool_value(false), "Fill the event histograms in a worker thread while the event is processed further");
  gQwOptions.AddOptions()("histogram-threads", po::value<int>()->default_value(1), "With --async-histograms: number of worker threads that fill the event histograms (each fills whole subsystems)");
  gQwOptions.AddOptions()("pipeline", po::value<bool>()->default_bool_value(false), "Read, decode and cut the events and pass them through the event ring in a reader thread, while the main thread processes the events that leave the ring");

  ///  Without anything, print usage
  if (argc == 1) {
    gQwOptions.Usage();
    exit(0);
  }

  ///  First, fill the search paths for the parameter files; this sets a
  ///  static variable within the QwParameterFile class which will be used by
  ///  all instances.
  ///  The "scratch" directory should be first.
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QW_PRMINPUT"));
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QWANALYSIS") + "/Parity/prminput");
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QWANALYSIS") + "/Analysis/prminput");

  gQwOptions.SetCommandLine(argc, argv);
  gQwOptions.AddConfigFile("qweak_mysql.conf");

  gQwOptions.ListConfigFiles();

  /// Load command line options for the histogram/tree helper class
  gQwHists.ProcessOptions(gQwOptions);
  /// Setup screen and file logging
  gQwLog.ProcessOptions(&gQwOptions);


  ///  Create the event buffer
  QwEventBuffer eventbuffer;
  eventbuffer.ProcessOptions(gQwOptions);

  ///  Create the database connection
  #ifdef __USE_DATABASE__
  QwParityDB database(gQwOptions);
  #endif //__USE_DATABASE__

  //  QwPromptSummary promptsummary;

  ///  Start loop over all runs
  Int_t run_number = 0;
  while (eventbuffer.OpenNextStream() == CODA_OK) {

    ///  Begin processing for the first run

    run_number = eventbuffer.GetRunNumber();
    TString run_label = eventbuffer.GetRunLabel();

    ///  Set the current event number for parameter file lookup
    QwParameterFile::SetCurrentRunNumber(run_number);
    //  Parse the options again, in case there are run-ranged config files
    gQwOptions.Parse(kTRUE);
    eventbuffer.ProcessOptions(gQwOptions);

    //    if (gQwOptions.GetValue<bool>("write-promptsummary")) {
    QwPromptSummary promptsummary(run_number, eventbuffer.GetSegmentNumber());
    //    }
    ///  Create an EPICS event
    QwEPICSEvent epicsevent;
    epicsevent.ProcessOptions(gQwOptions);
    epicsevent.LoadChannelMap("EpicsTable.map");


    ///  Load the detectors from file
    QwSubsystemArrayParity detectors(gQwOptions);
    detectors.ProcessOptions(gQwOptions);
    detectors.ListPublishedValues();

    /// Create event-based correction subsystem
    //    TString name = "EvtCorrector";
    //    QwCombinerSubsystem corrector_sub(gQwOptions, detectors, name);
    //    detectors.push_back(corrector_sub.GetSharedPointerToStaticObject());

    /// Create the helicity pattern
    //    Instead of having run_label in the constructor of helicitypattern, it might
    //    make since to have it be an option for use globally
    QwHelicityPattern helicitypattern(detectors,run_label);
    helicitypattern.ProcessOptions(gQwOptions);

    ///  Create the event ring with the subsystem array
    QwEventRing eventring(gQwOptions,detectors);
    //  Make a copy of the detectors object to hold the
    //  events which pass through the ring.
    QwSubsystemArrayParity ringoutput(detectors);

    ///  Threads that fill the event histograms from ringoutput
    ///  (--async-histograms); declared after ringoutput, so they end first.
    ///  Each takes the next subsystem until all are filled, so the
    ///  histograms of a subsystem are filled by one thread.
    const bool async_histograms = gQwOptions.GetValue<bool>("async-histograms");
    std::vector<std::unique_ptr<QwWorkerThread>> evt_histo_fillers;
    std::atomic<size_t> next_histo_subsystem(0);
    if (async_histograms) {
      const int nthreads = std::max(1, gQwOptions.GetValue<int>("histogram-threads"));
      for (int i = 0; i < nthreads; i++)
        evt_histo_fillers.push_back(std::make_unique<QwWorkerThread>());
      // Created now: a histogram buffer flush in either thread may use it
      THLimitsFinder::GetLimitsFinder();
    }
    auto wait_evt_histos = [&]() {
      for (auto& filler : evt_histo_fillers) filler->Wait();
    };
    auto fill_evt_histos = [&]() {
      // As QwSubsystemArrayParity::FillHistograms
      if (ringoutput.GetEventcutErrorFlag() != 0) return;
      next_histo_subsystem = 0;
      for (auto& filler : evt_histo_fillers) {
        filler->Run([&]() {
          for (size_t i; (i = next_histo_subsystem++) < ringoutput.size(); )
            ringoutput.at(i)->FillHistograms();
        });
      }
    };
    /// Create the data handler arrays
    QwDataHandlerArray datahandlerarray_evt(gQwOptions,ringoutput,run_label);
    QwDataHandlerArray datahandlerarray_mul(gQwOptions,helicitypattern,run_label);
    QwDataHandlerArray datahandlerarray_burst(gQwOptions,helicitypattern,run_label);

    ///  Create the burst sum
    QwHelicityPattern patternsum_per_burst(helicitypattern);
    patternsum_per_burst.DisablePairs();

    ///  Create the running sum
    QwSubsystemArrayParity eventsum(detectors);
    QwHelicityPattern patternsum(helicitypattern);
    patternsum.DisablePairs();
    QwHelicityPattern burstsum(helicitypattern);
    burstsum.DisablePairs();

    //  Initialize the database connection.
    #ifdef __USE_DATABASE__
    database.SetupOneRun(eventbuffer);
    #endif // __USE_DATABASE__

    //  All options are defined now; report typos once
    static bool warned_unrecognized_options = false;
    if (! warned_unrecognized_options) {
      gQwOptions.WarnUnrecognizedOptions();
      warned_unrecognized_options = true;
    }

    //  Open the ROOT file (close when scope ends)
    QwRootFile *treerootfile  = NULL;
    QwRootFile *burstrootfile = NULL;
    QwRootFile *historootfile = NULL;


    if (gQwOptions.GetValue<bool>("single-output-file")) {

      treerootfile  = new QwRootFile(run_label);
      burstrootfile = historootfile = treerootfile;
      //  Construct a tree which contains map file names which are used to analyze data
      treerootfile->WriteParamFileList("mapfiles", detectors);

    } else {

      treerootfile  = new QwRootFile(run_label + ".trees");
      burstrootfile = new QwRootFile(run_label + ".bursts");
      historootfile = new QwRootFile(run_label + ".histos");

      //  Construct a tree which contains map file names which are used to analyze data
      detectors.PrintParamFileList();
      treerootfile->WriteParamFileList("mapfiles", detectors);
      burstrootfile->WriteParamFileList("mapfiles", detectors);
      historootfile->WriteParamFileList("mapfiles", detectors);
    }
    #ifdef __USE_DATABASE__
    if (database.AllowsWriteAccess()) {
      database.FillParameterFiles(detectors);
    }
    #endif // __USE_DATABASE__
    //  Construct histograms
    historootfile->ConstructHistograms("evt_histo", ringoutput);
    historootfile->ConstructHistograms("mul_histo", helicitypattern);
    burstrootfile->ConstructHistograms("burst_histo", patternsum_per_burst);
    detectors.ShareHistograms(ringoutput);

    //  Construct tree branches
    treerootfile->ConstructTreeBranches("evt", "MPS event data tree", ringoutput);
    treerootfile->ConstructTreeBranches("mul", "Helicity event data tree", helicitypattern);
    burstrootfile->ConstructTreeBranches("pr", "Pair tree", helicitypattern.GetPairYield(),"yield_");
    burstrootfile->ConstructTreeBranches("pr", "Pair tree", helicitypattern.GetPairAsymmetry(),"asym_");
    treerootfile->ConstructTreeBranches("slow", "EPICS and slow control tree", epicsevent);
    burstrootfile->ConstructTreeBranches("burst", "Burst level data tree", patternsum_per_burst, "|stat");

    // Construct RNTuple fields if enabled
#ifdef HAS_RNTUPLE_SUPPORT
    treerootfile->ConstructNTupleFields("evt", "MPS event data RNTuple", ringoutput);
    treerootfile->ConstructNTupleFields("mul", "Helicity event data RNTuple", helicitypattern);
    burstrootfile->ConstructNTupleFields("pr", "Pair RNTuple", helicitypattern.GetPairYield(),"yield_");
    burstrootfile->ConstructNTupleFields("pr", "Pair RNTuple", helicitypattern.GetPairAsymmetry(),"asym_");
    treerootfile->ConstructNTupleFields("slow", "EPICS and slow control RNTuple", epicsevent);
    burstrootfile->ConstructNTupleFields("burst", "Burst level data RNTuple", patternsum_per_burst, "|stat");
#endif

    historootfile->ConstructHistograms("evt_histo",   datahandlerarray_evt);
    historootfile->ConstructHistograms("mul_histo",   datahandlerarray_mul);
    burstrootfile->ConstructHistograms("burst_histo", datahandlerarray_burst);

    datahandlerarray_evt.ConstructTreeBranches(treerootfile, "evt_");
    datahandlerarray_mul.ConstructTreeBranches(treerootfile);
    datahandlerarray_burst.ConstructTreeBranches(burstrootfile, "burst_", "|stat");

    // Construct RNTuple fields for data handlers if enabled
#ifdef HAS_RNTUPLE_SUPPORT
    datahandlerarray_evt.ConstructNTupleFields(treerootfile, "evt_");
    datahandlerarray_mul.ConstructNTupleFields(treerootfile);
    datahandlerarray_burst.ConstructNTupleFields(burstrootfile, "burst_", "|stat");
#endif

    treerootfile->ConstructTreeBranches("evts", "Running sum tree", eventsum, "|stat");
    treerootfile->ConstructTreeBranches("muls", "Running sum tree", patternsum, "|stat");
    burstrootfile->ConstructTreeBranches("bursts", "Burst running sum tree", burstsum, "|stat");

    // Construct RNTuple fields for additional data if enabled
#ifdef HAS_RNTUPLE_SUPPORT
    treerootfile->ConstructNTupleFields("evts", "Running sum RNTuple", eventsum, "|stat");
    treerootfile->ConstructNTupleFields("muls", "Running sum RNTuple", patternsum, "|stat");
    burstrootfile->ConstructNTupleFields("bursts", "Burst running sum RNTuple", burstsum, "|stat");
#endif

    // Summarize the ROOT file structure
    //treerootfile->PrintTrees();
    //treerootfile->PrintDirs();


    //  Clear the single-event running sum at the beginning of the runlet
    eventsum.ClearEventData();
    patternsum.ClearEventData();
    burstsum.ClearEventData();
    //  Clear the running sum of the burst values at the beginning of the runlet
    helicitypattern.ClearEventData();
    patternsum_per_burst.ClearEventData();



    //  Load the blinder seed from a random number generator for online mode
    if (eventbuffer.IsOnline() ){
      helicitypattern.UpdateBlinder();//this routine will call update blinder mechanism using a random number
    }else{
      //  Load the blinder seed from the database for this runlet.
#ifdef __USE_DATABASE__
      helicitypattern.UpdateBlinder(&database);
#endif // __USE_DATABASE__
    }


    //  Find the first EPICS event and try to initialize
    //  the blinder, but only for disk files, not online.
    if (! eventbuffer.IsOnline() ){
      QwMessage << "Finding first EPICS event" << QwLog::endl;
      while (eventbuffer.GetNextEvent() == CODA_OK) {
	if (eventbuffer.IsEPICSEvent()) {
	  eventbuffer.FillEPICSData(epicsevent);
	  if (epicsevent.HasDataLoaded()) {
	    helicitypattern.UpdateBlinder(epicsevent);
	    // and break out of this event loop
	    break;
	  }
	}
      }
      epicsevent.ResetCounters();
      //  Rewind stream
      QwMessage << "Rewinding stream" << QwLog::endl;
      eventbuffer.ReOpenStream();
    }

    // Start event loop instrumentation
#ifdef CALLGRIND_START_INSTRUMENTATION
    if (gQwOptions.GetValue<bool>("callgrind-instr-start-event-loop")) {
      QwMessage << "Starting callgrind instrumentation" << QwLog::endl;
      CALLGRIND_START_INSTRUMENTATION;
    }
#endif

    ///  First stage of the event loop: read and decode the events, apply
    ///  the event cuts and pass the events through the event ring.
    ///  on_epics() is called for each EPICS event with data (in
    ///  epicsevent), and on_event() when the ring has an event ready.
    auto read_events = [&](const std::function<void()>& on_epics,
                           const std::function<void()>& on_event) {
      while (eventbuffer.GetNextEvent() == CODA_OK) {

        //  First, do processing of non-physics events...
        if (eventbuffer.IsROCConfigurationEvent()) {
          //  Send ROC configuration event data to the subsystem objects.
          eventbuffer.FillSubsystemConfigurationData(detectors);
        }

        //  Secondly, process EPICS events, but not for online running,
        //  because the EPICS events get messed up by our 32-bit to 64-bit
        //  double ET system.
        if (! eventbuffer.IsOnline() && eventbuffer.IsEPICSEvent()) {
          eventbuffer.FillEPICSData(epicsevent);
          if (epicsevent.HasDataLoaded()){
            epicsevent.CalculateRunningValues();
            on_epics();
          }
        }

        //  Now, if this is not a physics event, go back and get a new event.
        if (! eventbuffer.IsPhysicsEvent()) continue;

        //  Fill the subsystem objects with their respective data for this event.
        eventbuffer.FillSubsystemData(detectors);

        //  Process the subsystem data
        detectors.ProcessEvent();

        // The event pass the event cut constraints
        if (detectors.ApplySingleEventCuts()) {
          // Add event to the ring
          eventring.push(detectors);
          // Check to see ring is ready
          if (eventring.IsReady()) on_event();
        }
      }
    };

    ///  Second stage for EPICS events
    auto process_epics = [&]() {
      helicitypattern.UpdateBlinder(epicsevent);

      treerootfile->FillTreeBranches(epicsevent);
      treerootfile->FillTree("slow");

      // Fill RNTuple if enabled
#ifdef HAS_RNTUPLE_SUPPORT
      treerootfile->FillNTupleFields(epicsevent);
      treerootfile->FillNTuple("slow");
#endif
    };

    ///  Second stage for the events that leave the ring (in ringoutput)
    auto process_event = [&]() {
	  ringoutput.IncrementErrorCounters();


	  // Accumulate the running sum to calculate the event based running average
	  eventsum.AccumulateRunningSum(ringoutput);

	  // Fill the histograms: with --async-histograms in the worker thread,
	  // while the code below only reads ringoutput, until the next event
	  if (async_histograms) {
	    if (historootfile->ShouldFillHistograms(ringoutput)) fill_evt_histos();
	  } else {
	    historootfile->FillHistograms(ringoutput);
	  }

	  // Fill mps tree branches
	  treerootfile->FillTreeBranches(ringoutput);
	  treerootfile->FillTree("evt");

	  // Fill RNTuple if enabled
#ifdef HAS_RNTUPLE_SUPPORT
	  treerootfile->FillNTupleFields(ringoutput);
	  treerootfile->FillNTuple("evt");
#endif

	  // Process data handlers
          datahandlerarray_evt.ProcessDataHandlerEntry();

          // Fill data handler histograms
          historootfile->FillHistograms(datahandlerarray_evt);

          // Fill data handler tree branches
          datahandlerarray_evt.FillTreeBranches(treerootfile);

          // Fill data handler RNTuple fields if enabled
#ifdef HAS_RNTUPLE_SUPPORT
          datahandlerarray_evt.FillNTupleFields(treerootfile);
#endif

          // Load the event into the helicity pattern
          helicitypattern.LoadEventData(ringoutput);

	  if (helicitypattern.PairAsymmetryIsGood()) {
            patternsum.AccumulatePairRunningSum(helicitypattern);

	    // Fill pair tree branches
	    burstrootfile->FillTreeBranches(helicitypattern.GetPairYield());
	    burstrootfile->FillTreeBranches(helicitypattern.GetPairAsymmetry());
	    burstrootfile->FillTreeBranches(helicitypattern.GetPairDifference());
	    burstrootfile->FillTree("pr");

	    // Fill the pair RNTuple if enabled
#ifdef HAS_RNTUPLE_SUPPORT
	    burstrootfile->FillNTupleFields("pr", helicitypattern.GetPairYield());
	    burstrootfile->FillNTupleFields("pr", helicitypattern.GetPairAsymmetry());
	    burstrootfile->FillNTuple("pr");
#endif

	    // Clear the data
	    helicitypattern.ClearPairData();
	  }

          // Check to see if we can calculate helicity pattern asymmetry, do so, and report if it worked
          if (helicitypattern.IsGoodAsymmetry()) {
              patternsum.AccumulateRunningSum(helicitypattern);

              // Fill histograms
              historootfile->FillHistograms(helicitypattern);

              // Fill helicity tree branches
              treerootfile->FillTreeBranches(helicitypattern);
              treerootfile->FillTree("mul");

              // Fill helicity RNTuple if enabled
#ifdef HAS_RNTUPLE_SUPPORT
              treerootfile->FillNTupleFields(helicitypattern);
              treerootfile->FillNTuple("mul");
#endif

              // Process data handlers
              datahandlerarray_mul.ProcessDataHandlerEntry();
              datahandlerarray_burst.ProcessDataHandlerEntry();

              // Fill data handler histograms
              historootfile->FillHistograms(datahandlerarray_mul);

              // Fill data handler tree branches
              datahandlerarray_mul.FillTreeBranches(treerootfile);

              // Fill data handler RNTuple fields if enabled
#ifdef HAS_RNTUPLE_SUPPORT
              datahandlerarray_mul.FillNTupleFields(treerootfile);
#endif

              // Fill the pattern into the sum for this burst
              patternsum_per_burst.AccumulateRunningSum(helicitypattern);

              // Accumulate data handler arrays
              //datahandlerarray_burst.AccumulateRunningSum(datahandlerarray_mul);

              // Burst mode
              if (patternsum_per_burst.IsEndOfBurst()) {

                // Calculate average over this burst
                patternsum_per_burst.CalculateRunningAverage();

                // Fill the burst into the sum over all bursts
                burstsum.AccumulateRunningSum(patternsum_per_burst);

                if (gQwOptions.GetValue<bool>("print-burstsum")) {
                  QwMessage << " Running average of this burst" << QwLog::endl;
                  QwMessage << " =============================" << QwLog::endl;
                  patternsum_per_burst.PrintValue();
                }

                // Fill histograms
                burstrootfile->FillHistograms(patternsum_per_burst);

                // Fill burst tree branches
                burstrootfile->FillTreeBranches(patternsum_per_burst);
                burstrootfile->FillTree("burst");

                // Fill burst RNTuple if enabled
#ifdef HAS_RNTUPLE_SUPPORT
                burstrootfile->FillNTupleFields(patternsum_per_burst);
                burstrootfile->FillNTuple("burst");
#endif

                // Finish data handler for burst
                datahandlerarray_burst.FinishDataHandler();

                // Fill data handler histograms
                burstrootfile->FillHistograms(datahandlerarray_burst);

                // Fill data handler tree branches
                datahandlerarray_burst.FillTreeBranches(burstrootfile);

                // Fill data handler RNTuple fields if enabled
#ifdef HAS_RNTUPLE_SUPPORT
                datahandlerarray_burst.FillNTupleFields(burstrootfile);
#endif

		helicitypattern.IncrementBurstCounter();
		datahandlerarray_mul.UpdateBurstCounter(helicitypattern.GetBurstCounter());
		datahandlerarray_burst.UpdateBurstCounter(helicitypattern.GetBurstCounter());
                // Clear the data
                patternsum_per_burst.ClearEventData();
                datahandlerarray_burst.ClearEventData();
              }

              // Clear the data
              helicitypattern.ClearEventData();

	  } // helicitypattern.IsGoodAsymmetry()
    }; // process_event

    if (! gQwOptions.GetValue<bool>("pipeline")) {
      ///  Loop over events
      read_events(process_epics, [&]() {
        // The histograms of the previous event are filled from ringoutput
        wait_evt_histos();
        ringoutput = eventring.pop();
        process_event();
      });
    } else {
      ///  Loop over events with the first stage in a reader thread; the
      ///  reader copies the events that leave the ring into the slots of
      ///  the pipeline, and the main thread processes them in order
      QwEventPipeline pipeline(detectors, 8);
      std::exception_ptr reader_error;
      std::thread reader([&]() {
        try {
          read_events(
            [&]() { pipeline.PushEPICSAndWait(); },
            [&]() {
              QwSubsystemArrayParity* slot = pipeline.GetFreeSlot();
              *slot = eventring.pop();
              pipeline.Push(QwEventPipeline::kEvent, slot);
            });
        } catch (...) {
          reader_error = std::current_exception();
        }
        pipeline.Push(QwEventPipeline::kEnd);
      });
      try {
        for (;;) {
          QwEventPipeline::Item item = pipeline.Pop();
          if (item.kind == QwEventPipeline::kEnd) break;
          if (item.kind == QwEventPipeline::kEPICS) {
            process_epics();
            pipeline.EPICSDone();
            continue;
          }
          // The histograms of the previous event are filled from ringoutput
          wait_evt_histos();
          ringoutput = *item.event;
          pipeline.Recycle(item.event);
          process_event();
        }
      } catch (...) {
        pipeline.Cancel();
        reader.join();
        throw;
      }
      reader.join();
      if (reader_error) std::rethrow_exception(reader_error);
    }

    // The event histograms of the last event
    wait_evt_histos();

    // Unwind event ring
    QwMessage << "Unwinding event ring" << QwLog::endl;
    eventring.Unwind();

    // Stop event loop instrumentation
#ifdef CALLGRIND_START_INSTRUMENTATION
    if (gQwOptions.GetValue<bool>("callgrind-instr-stop-event-loop")) {
      CALLGRIND_STOP_INSTRUMENTATION;
      QwMessage << "Stapped callgrind instrumentation" << QwLog::endl;
    }
#endif

    //  TODO Drain event run

    //  Finalize burst
    if (patternsum_per_burst.HasBurstData()){
      // Calculate average over this burst
      patternsum_per_burst.CalculateRunningAverage();

      // Fill the burst into the sum over all bursts
      burstsum.AccumulateRunningSum(patternsum_per_burst);

      if (gQwOptions.GetValue<bool>("print-burstsum")) {
	QwMessage << " Running average of this burst" << QwLog::endl;
	QwMessage << " =============================" << QwLog::endl;
	patternsum_per_burst.PrintValue();
      }

      // Fill histograms
      burstrootfile->FillHistograms(patternsum_per_burst);

      // Fill burst tree branches
      burstrootfile->FillTreeBranches(patternsum_per_burst);
      burstrootfile->FillTree("burst");

      // Fill burst RNTuple if enabled
#ifdef HAS_RNTUPLE_SUPPORT
      burstrootfile->FillNTupleFields(patternsum_per_burst);
      burstrootfile->FillNTuple("burst");
#endif

      // Finish data handler for burst
      datahandlerarray_burst.FinishDataHandler();

      // Fill data handler histograms
      burstrootfile->FillHistograms(datahandlerarray_burst);

      // Fill data handler tree branches
      datahandlerarray_burst.FillTreeBranches(burstrootfile);

      // Fill data handler RNTuple fields if enabled
#ifdef HAS_RNTUPLE_SUPPORT
      datahandlerarray_burst.FillNTupleFields(burstrootfile);
#endif
      patternsum_per_burst.PrintIndexMapFile(run_number);
    }

    //  Perform actions at the end of the event loop on the
    //  detectors object, which ought to have handles for the
    //  MPS based histograms.
    ringoutput.AtEndOfEventLoop();

    QwMessage << "Number of events processed at end of run: "
              << eventbuffer.GetPhysicsEventNumber() << QwLog::endl;

    // Finish data handlers
    datahandlerarray_evt.FinishDataHandler();
    datahandlerarray_mul.FinishDataHandler();

    // Calculate running averages
    eventsum.CalculateRunningAverage();
    patternsum.CalculateRunningAverage();
    burstsum.CalculateRunningAverage();

    // This will calculate running averages over single helicity events
    if (gQwOptions.GetValue<bool>("print-runningsum")) {
      QwMessage << " Running average of events" << QwLog::endl;
      QwMessage << " =========================" << QwLog::endl;
      eventsum.PrintValue();
    }
    treerootfile->FillTreeBranches(eventsum);
    treerootfile->FillTree("evts");

    // Fill running sum RNTuple if enabled
#ifdef HAS_RNTUPLE_SUPPORT
    treerootfile->FillNTupleFields(eventsum);
    treerootfile->FillNTuple("evts");
#endif

    if (gQwOptions.GetValue<bool>("print-patternsum")) {
      QwMessage << " Running average of patterns" << QwLog::endl;
      QwMessage << " =========================" << QwLog::endl;
      patternsum.PrintValue();
    }
    treerootfile->FillTreeBranches(patternsum);
    treerootfile->FillTree("muls");

    // Fill pattern sum RNTuple if enabled
#ifdef HAS_RNTUPLE_SUPPORT
    treerootfile->FillNTupleFields(patternsum);
    treerootfile->FillNTuple("muls");
#endif

    if (gQwOptions.GetValue<bool>("print-burstsum")) {
      QwMessage << " Running average of bursts" << QwLog::endl;
      QwMessage << " =========================" << QwLog::endl;
      burstsum.PrintValue();
    }
    burstrootfile->FillTreeBranches(burstsum);
    burstrootfile->FillTree("bursts");

    // Fill burst sum RNTuple if enabled
#ifdef HAS_RNTUPLE_SUPPORT
    burstrootfile->FillNTupleFields(burstsum);
    burstrootfile->FillNTuple("bursts");
#endif

    //  Construct objects
    burstrootfile->ConstructObjects("objects", helicitypattern);

    /*  Write to the root file, being sure to delete the old cycles  *
     *  which were written by Autosave.                              *
     *  Doing this will remove the multiple copies of the ntuples    *
     *  from the root file.                                          *
     *                                                               *
     *  Then, we need to delete the histograms here.                 *
     *  If we wait until the subsystem destructors, we get a         *
     *  segfault; but in addition to that we should delete them      *
     *  here, in case we run over multiple runs at a time.           */
    if (treerootfile == historootfile) {
      // Use different write methods based on output format
#ifdef HAS_RNTUPLE_SUPPORT
      if (gQwOptions.GetValue<bool>("enable-rntuples") && gQwOptions.GetValue<bool>("disable-trees")) {
        // RNTuple-only mode: use Close() for proper RNTuple finalization
        treerootfile->Close();
      } else {
#endif
        // TTree mode or mixed mode: use Write() for explicit tree writing
        treerootfile->Write(0, TObject::kOverwrite);
        treerootfile->Close();
#ifdef HAS_RNTUPLE_SUPPORT
      }
#endif
      delete treerootfile; treerootfile = 0; burstrootfile = 0; historootfile = 0;
    } else {
      // Use different write methods based on output format
#ifdef HAS_RNTUPLE_SUPPORT
      if (gQwOptions.GetValue<bool>("enable-rntuples") && gQwOptions.GetValue<bool>("disable-trees")) {
        // RNTuple-only mode: use Close() for proper RNTuple finalization
        treerootfile->Close();
        burstrootfile->Close();
        historootfile->Close();
      } else {
#endif
        // TTree mode or mixed mode: use Write() for explicit tree writing
        treerootfile->Write(0, TObject::kOverwrite);
        burstrootfile->Write(0, TObject::kOverwrite);
        historootfile->Write(0, TObject::kOverwrite);
        treerootfile->Close();
        burstrootfile->Close();
        historootfile->Close();
#ifdef HAS_RNTUPLE_SUPPORT
      }
#endif
      delete treerootfile; treerootfile = 0;
      delete burstrootfile; burstrootfile = 0;
      delete historootfile; historootfile = 0;
    }

    //  Print the event cut error summary for each subsystem
    if (gQwOptions.GetValue<bool>("print-errorcounters")) {
      QwMessage << " ------------ error counters ------------------ " << QwLog::endl;
      ringoutput.PrintErrorCounters();
    }

    if (gQwOptions.GetValue<bool>("write-promptsummary")) {
      //      runningsum.WritePromptSummary(&promptsummary, "yield");
      // runningsum.WritePromptSummary(&promptsummary, "asymmetry");
      //      runningsum.WritePromptSummary(&promptsummary, "difference");
      datahandlerarray_mul.WritePromptSummary(&promptsummary, "asymmetry");
      patternsum.WritePromptSummary(&promptsummary);
      promptsummary.PrintCSV(eventbuffer.GetPhysicsEventNumber(),eventbuffer.GetStartSQLTime(), eventbuffer.GetEndSQLTime());
    }
    //  Read from the database
    #ifdef __USE_DATABASE__
    database.SetupOneRun(eventbuffer);

    // Each subsystem has its own Connect() and Disconnect() functions.
    if (database.AllowsWriteAccess()) {
      patternsum.FillDB(&database);
      patternsum.FillErrDB(&database);
      epicsevent.FillDB(&database);
      ringoutput.FillDB_MPS(&database, "optics");
    }
    #endif // __USE_DATABASE__

    //epicsevent.WriteEPICSStringValues();

    //  Close event buffer stream
    eventbuffer.CloseStream();



    //  Report run summary
    eventbuffer.ReportRunSummary();
    eventbuffer.PrintRunTimes();
  } // end of loop over runs

  QwMessage << "I have done everything I can do..." << QwLog::endl;

  return 0;
}
