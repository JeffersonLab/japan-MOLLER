/*------------------------------------------------------------------------*//*!

 \file QwMockDataGenerator.cc

 \brief Parity mock data generator

*//*-------------------------------------------------------------------------*/

// C and C++ headers
#include <iostream>
#include <random>

// Qweak headers
#include "QwLog.h"
#include "QwBeamLine.h"
#include "QwOptionsParity.h"
#include "QwEventBuffer.h"
#include "QwHelicity.h"
#include "QwHelicityPattern.h"
#include "QwBlindDetectorArray.h"
#include "QwSubsystemArrayParity.h"
#include "QwDetectorArray.h"

// ROOT headers
#include "TStopwatch.h"

// Number of variables to correlate
#define NVARS 3


// Multiplet structure
static const int kMultiplet = 64;

// Beam trips on qwk_bcm0l03
static const bool kBeamTrips = true;

// Debug
static const bool kDebug = false;

// Stringify
inline std::string stringify(int i) {
  std::ostringstream stream;
  stream << i;
  return stream.str();
}

int main(int argc, char* argv[])
{
  // Define the command line options
  DefineOptionsParity(gQwOptions);

  ///  Without anything, print usage
  if (argc == 1) {
    gQwOptions.Usage();
    exit(0);
  }

  // Fill the search paths for the parameter files; this sets a static
  // variable within the QwParameterFile class which will be used by
  // all instances.
  // The "scratch" directory should be first.
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QW_PRMINPUT"));
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QWANALYSIS") + "/Analysis/prminput");
  QwParameterFile::AppendToSearchPath(getenv_safe_string("QWANALYSIS") + "/Parity/prminput");

  // Set the command line arguments and the configuration filename,
  // and we define the options that can be used in them (using QwOptions).
  gQwOptions.SetCommandLine(argc, argv);
  gQwOptions.SetConfigFile("qwmockdataanalysis.conf");

  // Event buffer
  QwEventBuffer eventbuffer;
  eventbuffer.ProcessOptions(gQwOptions);

  // Detector array
  QwSubsystemArrayParity detectors(gQwOptions);
  detectors.ProcessOptions(gQwOptions);

  // Get the helicity
  QwHelicity* helicity = dynamic_cast<QwHelicity*>(detectors.GetSubsystemByName("Helicity Info"));
  if (! helicity) QwWarning << "No helicity subsystem defined!" << QwLog::endl;


  // Get the beamline channels we want to correlate
  detectors.LoadMockDataParameters("mock_parameters_list.map");

  // new vectors for GetSubsystemByType
  std::vector <QwDetectorArray*> detchannels;
  std::vector <VQwSubsystem*> tempvector = detectors.GetSubsystemByType("QwDetectorArray");
  //  detectors.GetSubsystemByType;

  for (std::size_t i = 0; i < tempvector.size(); i++){
    detchannels.push_back(dynamic_cast<QwDetectorArray*>(tempvector[i]));

  //return detchannels;
  }

  // Initialize the stopwatch
  TStopwatch stopwatch;

  // Loop over all runs
  UInt_t runnumber_min = (UInt_t) gQwOptions.GetIntValuePairFirst("run");
  UInt_t runnumber_max = (UInt_t) gQwOptions.GetIntValuePairLast("run");
  for (UInt_t run  = runnumber_min;
              run <= runnumber_max;
              run++) {

    // Set the random seed for this run
    MQwMockable::Seed(run);
    QwCombinedBCM<QwVQWK_Channel>::SetTripSeed(0x56781234 ^ (run*run));

    // Open new output file
    // (giving run number as argument to OpenDataFile confuses the segment search)


    TString filename = Form("%sQwMock_%u.log", eventbuffer.GetDataDirectory().Data(), run);
    if (eventbuffer.IsOnline()) {
      if (eventbuffer.ReOpenStream() != CODA_OK) {
        std::cout << "Error: could not open ET stream!" << std::endl;
        return 0;
      }
    } else {
      if (eventbuffer.OpenDataFile(filename,"W") != CODA_OK) {
        std::cout << "Error: could not open file!" << std::endl;
        return 0;
      }
    }
    eventbuffer.ResetControlParameters();
    eventbuffer.EncodePrestartEvent(run, 0); // prestart: runnumber, runtype
    eventbuffer.EncodeGoEvent();


    // Helicity initialization loop
    helicity->SetEventPatternPhase(-1, -1, -1);
    // 24-bit seed, should be larger than 0x1, 0x55 = 0101 0101
    // Consecutive runs should have no trivially related seeds:
    // e.g. with 0x2 * run, the first two files will be just 1 MPS offset...
    unsigned int seed = 0x1234 ^ run;
    helicity->SetFirstBits(24, seed & 0xFFFFFF);


    // Retrieve the requested range of event numbers
    if (kDebug) std::cout << "Starting event loop..." << std::endl;
    Int_t eventnumber_min = gQwOptions.GetIntValuePairFirst("event");
    Int_t eventnumber_max = gQwOptions.GetIntValuePairLast("event");

    // Warn when only few events are requested, probably a problem in the input
    if (abs(eventnumber_max - eventnumber_min) < 10)
      QwWarning << "Only " << abs(eventnumber_max - eventnumber_min)
                << " events will be generated." << QwLog::endl;

    // Event generation loop
    for (Int_t event = eventnumber_min; event <= eventnumber_max; event++) {

      // First clear the event
      detectors.ClearEventData();

      // Set the event, pattern and phase number
      // - event number increments for every event
      // - pattern number increments for every multiplet
      // - phase number gives position in multiplet
      helicity->SetEventPatternPhase(event, event / kMultiplet, event % kMultiplet + 1);

      // Run the helicity predictor
      helicity->RunPredictor();
      // Concise helicity printout
      if (kDebug) {
        // - actual helicity
        if      (helicity->GetHelicityActual() == 0) std::cout << "-";
        else if (helicity->GetHelicityActual() == 1) std::cout << "+";
        else std::cout << "?";
        // - delayed helicity
        if      (helicity->GetHelicityDelayed() == 0) std::cout << "(-) ";
        else if (helicity->GetHelicityDelayed() == 1) std::cout << "(+) ";
        else std::cout << "(?) ";
        if (event % kMultiplet + 1 == 4) {
          std::cout << std::hex << helicity->GetRandomSeedActual()  << std::dec << ",  \t";
          std::cout << std::hex << helicity->GetRandomSeedDelayed() << std::dec << std::endl;
        }
      }

      // Calculate the time assuming one ms for every helicity window
      double time = event * detectors.GetWindowPeriod();

      // Fill the detectors with randomized data

      int myhelicity = helicity->GetHelicityActual() ? +1 : -1;
      //std::cout << myhelicity << std::endl;

      // Randomize data for this event
      detectors.RandomizeEventData(myhelicity, time);
//      detectors.ProcessEvent();
//      beamline-> ProcessEvent(); //Do we need to keep this line now?  Check the maindetector correlation with beamline devices with and without it.

     for (std::size_t i = 0; i < detchannels.size(); i++){
      detchannels[i]->ExchangeProcessedData();
      detchannels[i]->RandomizeMollerEvent(myhelicity);
      }

      // Write this event to file
      Int_t status = eventbuffer.EncodeSubsystemData(detectors);
      if (status != CODA_OK) {
        QwError << "Error: could not write event " << event << QwLog::endl;
        break;
      }

      // Periodically print event number
      constexpr int nevents = kDebug ? 1000 : 10000;
      if (event % nevents == 0) {
        QwMessage << "Generated " << event << " events ";
        stopwatch.Stop();
        QwMessage << "(" << stopwatch.RealTime()*1e3/nevents << " ms per event)";
        stopwatch.Reset();
        stopwatch.Start();
        QwMessage << QwLog::endl;
      }

    } // end of event loop


    eventbuffer.EncodeEndEvent();
    eventbuffer.CloseDataFile();
    eventbuffer.ReportRunSummary();

    if (eventbuffer.IsOnline()) {
      QwMessage << "Wrote mock data run to ET stream successfully." << QwLog::endl;
    } else {
      QwMessage << "Wrote mock data run " << filename << " successfully." << QwLog::endl;
    }

  } // end of run loop

} // end of main
