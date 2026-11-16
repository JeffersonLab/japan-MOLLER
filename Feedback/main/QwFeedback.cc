/*------------------------------------------------------------------------*//*!

 \file QwParity.cc

 \brief main(...) function for the qwparity executable

*//*-------------------------------------------------------------------------*/

// Qweak headers
#include "QwLog.h"
#include "QwOptionsParity.h"
#include "QwEventBuffer.h"
#include "QwHistogramHelper.h"
#include "QwSubsystemArrayParity.h"
#include "QwHelicityPattern.h"
#include "QwEventRing.h"
#include "QwEPICSEvent.h"
#include "QwCombiner.h"
#include "QwCombinerSubsystem.h"
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

Int_t main(Int_t argc, Char_t* argv[])
{
  ///  Define the command line options
  DefineOptionsParity(gQwOptions);

  ///  Define additional command line arguments and the configuration filename,
  ///  and we define the options that can be used in them (using QwOptions).
  gQwOptions.AddOptions()("callgrind-instr-start-event-loop", po::value<bool>()->default_bool_value(false), "Start callgrind instrumentation with main event loop (with --instr-atstart=no)");
  gQwOptions.AddOptions()("callgrind-instr-stop-event-loop", po::value<bool>()->default_bool_value(false), "Stop callgrind instrumentation with main event loop (with --instr-atstart=no)");

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
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QWANALYSIS") + "/Feedback/prminput");
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QWANALYSIS") + "/Parity/prminput");
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QWANALYSIS") + "/Analysis/prminput");

  gQwOptions.SetCommandLine(argc, argv);
  gQwOptions.AddConfigFile("qweak_mysql.conf");

  gQwOptions.ListConfigFiles();

  /// Load command line options for the histogram/tree helper class
  gQwHists.ProcessOptions(gQwOptions);
  /// Setup screen and file logging
  gQwLogger::Instance().ProcessOptions(&gQwOptions);


  ///  Create the event buffer
  QwEventBuffer eventbuffer;
  eventbuffer.ProcessOptions(gQwOptions);

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

    /// Create the data handler arrays
    QwDataHandlerArray datahandlerarray_mul(gQwOptions,helicitypattern,run_label);

    ///  Create the running sum
    QwSubsystemArrayParity eventsum(detectors);
    QwHelicityPattern patternsum(helicitypattern);
    patternsum.DisablePairs();

    //  Clear the single-event running sum at the beginning of the runlet
    eventsum.ClearEventData();
    patternsum.ClearEventData();
    //  Clear the running sum of the burst values at the beginning of the runlet
    helicitypattern.ClearEventData();



    //  Load the blinder seed from a random number generator for online mode
    if (eventbuffer.IsOnline() ){
      helicitypattern.UpdateBlinder();//this routine will call update blinder mechanism using a random number
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

    ///  Start loop over events
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
	  helicitypattern.UpdateBlinder(epicsevent);


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
        if (eventring.IsReady()) {
	  ringoutput = eventring.pop();
	  ringoutput.IncrementErrorCounters();


	  // Accumulate the running sum to calculate the event based running average
	  eventsum.AccumulateRunningSum(ringoutput);

          // Load the event into the helicity pattern
          helicitypattern.LoadEventData(ringoutput);

	  if (helicitypattern.PairAsymmetryIsGood()) {
            patternsum.AccumulatePairRunningSum(helicitypattern);

	    // Clear the data
	    helicitypattern.ClearPairData();
	  }

          // Check to see if we can calculate helicity pattern asymmetry, do so, and report if it worked
          if (helicitypattern.IsGoodAsymmetry()) {
              patternsum.AccumulateRunningSum(helicitypattern);


              // Process data handlers
              datahandlerarray_mul.ProcessDataHandlerEntry();

              // Clear the data
              helicitypattern.ClearEventData();

	  } // helicitypattern.IsGoodAsymmetry()

        } // eventring.IsReady()

      } // detectors.ApplySingleEventCuts()

    } // end of loop over events

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

    //  Perform actions at the end of the event loop on the
    //  detectors object, which ought to have handles for the
    //  MPS based histograms.
    ringoutput.AtEndOfEventLoop();

    QwMessage << "Number of events processed at end of run: "
              << eventbuffer.GetPhysicsEventNumber() << QwLog::endl;

    // Finish data handlers
    datahandlerarray_mul.FinishDataHandler();

    // Calculate running averages
    eventsum.CalculateRunningAverage();
    patternsum.CalculateRunningAverage();

    // This will calculate running averages over single helicity events
    if (gQwOptions.GetValue<bool>("print-runningsum")) {
      QwMessage << " Running average of events" << QwLog::endl;
      QwMessage << " =========================" << QwLog::endl;
      eventsum.PrintValue();
    }

    if (gQwOptions.GetValue<bool>("print-patternsum")) {
      QwMessage << " Running average of patterns" << QwLog::endl;
      QwMessage << " =========================" << QwLog::endl;
      patternsum.PrintValue();
    }

    //  Print the event cut error summary for each subsystem
    if (gQwOptions.GetValue<bool>("print-errorcounters")) {
      QwMessage << " ------------ error counters ------------------ " << QwLog::endl;
      ringoutput.PrintErrorCounters();
    }

    //  Close event buffer stream
    eventbuffer.CloseStream();


    //  Report run summary
    eventbuffer.ReportRunSummary();
    eventbuffer.PrintRunTimes();
  } // end of loop over runs

  QwMessage << "I have done everything I can do..." << QwLog::endl;

  return 0;
}
