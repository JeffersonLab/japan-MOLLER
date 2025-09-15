#include "QwEventBuffer.h"

#include "QwOptions.h"
#include "QwEPICSEvent.h"
#include "VQwSubsystem.h"
#include "QwSubsystemArray.h"

#include <TMath.h>

#include <vector>
#include <glob.h>

#include <csignal>
Bool_t globalEXIT;
Bool_t onlineRestart;
void sigint_handler(int sig)
{
  std::cout << "handling signal no. " << sig << " ";
  std::cout << "(press ctrl-\\ to abort now)\n";
  globalEXIT=1;
}
void sigusr_handler(int sig)
{
  std::cout << "handling signal no. " << sig << "\n";
  std::cout << "Restarts the event loop in online mode." << std::endl;
  onlineRestart = 1;
}

#include "THaCodaFile.h"
#ifdef __CODA_ET
#include "THaEtClient.h"
#endif

std::string QwEventBuffer::fDefaultDataDirectory = "/adaq1/data1/apar";
std::string QwEventBuffer::fDefaultDataFileStem = "QwRun_";
std::string QwEventBuffer::fDefaultDataFileExtension = "log";

const Int_t QwEventBuffer::kRunNotSegmented = -20;
const Int_t QwEventBuffer::kNoNextDataFile  = -30;
const Int_t QwEventBuffer::kFileHandleNotConfigured  = -40;

/// This is the ASCII character array 'NULL', and is used by the
/// DAQ to indicate a known empty buffer.
const UInt_t QwEventBuffer::kNullDataWord = 0x4e554c4c;


/// Default constructor
QwEventBuffer::QwEventBuffer()
  :    fRunListFile(NULL),
       fDataFileStem(fDefaultDataFileStem),
       fDataFileExtension(fDefaultDataFileExtension),
       fDataDirectory(fDefaultDataDirectory),
       fEvStreamMode(fEvStreamNull),
       fEvStream(NULL),
       fCurrentRun(-1),
       fNumPhysicsEvents(0),
       fSingleFile(kFALSE),
			 decoder(NULL)
{
  //  Set up the signal handler.
  globalEXIT=0;
  signal(SIGINT,  sigint_handler);// ctrl+c
  signal(SIGTERM, sigint_handler);// kill in shell // 15
  //  signal(SIGTSTP, sigint_handler);// ctrl+z // 20
  onlineRestart=0;
  signal(SIGUSR1, sigusr_handler);

  fCleanParameter[0] = 0.0;
  fCleanParameter[1] = 0.0;
  fCleanParameter[2] = 0.0;
}

/**
 * Defines configuration options for QwEventBuffer class using QwOptions
 * functionality.
 *
 * @param options Options object
 */
void QwEventBuffer::DefineOptions(QwOptions &options)
{
  // Define the execution options
  options.AddDefaultOptions()
    ("online", po::value<bool>()->default_bool_value(false),
     "use online data stream");
  options.AddDefaultOptions()
    ("online.RunNumber", po::value<int>()->default_bool_value(0),
     "Effective run number to be used by online system to find the parameter files");
  options.AddDefaultOptions()
    ("run,r", po::value<string>()->default_value("0:0"),
     "run range in format #[:#]");
  options.AddDefaultOptions()
    ("data,d", po::value<string>()->default_value(fDefaultDataDirectory),
     "data directory, also $QW_DATA");
  options.AddDefaultOptions()
    ("runlist", po::value<string>()->default_value(""),
     "run list file name");
  options.AddDefaultOptions()
    ("event,e", po::value<string>()->default_value("0:"),
     "event range in format #[:#]");
  options.AddDefaultOptions()
    ("segment,s", po::value<string>()->default_value("0:"),
     "run segment range in format #[:#]");
  options.AddDefaultOptions()
    ("chainfiles", po::value<bool>()->default_bool_value(false),
     "chain file segments together, do not analyze them separately");
  options.AddDefaultOptions()
    ("codafile-stem", po::value<string>()->default_value(fDefaultDataFileStem),
     "stem of the input CODA filename");
  options.AddDefaultOptions()
    ("codafile-ext", po::value<string>()->default_value(fDefaultDataFileExtension),
     "extension of the input CODA filename");
  options.AddOptions()
    ("directfile", po::value<string>(), 
    "Run over single event file");
  //  Special flag to allow sub-bank IDs less than 31
  options.AddDefaultOptions()
    ("allow-low-subbank-ids", po::value<bool>()->default_bool_value(false),
     "allow the sub-bank ids to be 31 or less, when using this flag, all ROCs must be sub-banked");
  //  Options specific to the ET clients
  options.AddOptions("ET system options")
    ("ET.hostname", po::value<string>(),
     "Name of the ET session's host machine --- Only used in online mode\nDefaults to the environment variable $HOSTNAME"); 
  options.AddOptions("ET system options")
    ("ET.session", po::value<string>(),
     "ET session name --- Only used in online mode\nDefaults to the environment variable $SESSION"); 
  options.AddOptions("ET system options")
    ("ET.station", po::value<string>(),
     "ET station name --- Only used in online mode"); 
  options.AddOptions("ET system options")
    ("ET.waitmode", po::value<int>()->default_value(0),
     "ET system wait mode: 0 is wait-forever, 1 is timeout \"quickly\"  --- Only used in online mode"); 
  options.AddOptions("ET system options")
    ("ET.exit-on-end", po::value<bool>()->default_value(false),
     "Exit the event loop if the end event is found. JAPAN remains open and waits for the next run. --- Only used in online mode");
  options.AddOptions("CodaVersion")
    ("coda-version", po::value<int>()->default_value(3),
     "Sets the Coda Version. Allowed values = {2,3}. \nThis is needed for writing and reading mock data. Mock data needs to be written and read with the same Coda Version.");
}

void QwEventBuffer::ProcessOptions(QwOptions &options)
{
  fOnline     = options.GetValue<bool>("online");
  if (fOnline){
    fETWaitMode  = options.GetValue<int>("ET.waitmode");
    fExitOnEnd  = options.GetValue<bool>("ET.exit-on-end");
#ifndef __CODA_ET
    QwError << "Online mode will not work without the CODA libraries!"
	    << QwLog::endl;
    exit(EXIT_FAILURE);
#else
    if (options.HasValue("online.RunNumber")) {
      fCurrentRun = options.GetValue<int>("online.RunNumber");
    }
    if (options.HasValue("ET.station")) {
      fETStationName = options.GetValue<string>("ET.station");
    } else {
      fETStationName = "";
    }
    if (options.HasValue("ET.hostname")) {
      fETHostname = options.GetValue<string>("ET.hostname");
    } else {
      fETHostname = getenv("HOSTNAME");
    }
    if (options.HasValue("ET.session")) {
      fETSession = options.GetValue<string>("ET.session");
    } else {
      fETSession = getenv("SESSION");
    }
    if (fETHostname.Length() == 0 || fETSession.Length() == 0) {
      TString tmp = "";
      if (fETHostname == NULL || fETHostname.Length() == 0)
	tmp += " \"HOSTNAME\"";
      if (fETSession == NULL ||  fETSession.Length() == 0){
	if (tmp.Length() > 0)
	  tmp += " and";
	tmp += " ET \"SESSION\"";
      }
      QwError << "The" << tmp
	      << " variable(s) is(are) not defined in your environment.\n"
	      << "        This is needed to run the online analysis."
	      << QwLog::endl;
      exit(EXIT_FAILURE);
    }
#endif
  }
  if(options.HasValue("directfile")){
      fSingleFile = kTRUE;
      fDataFile = options.GetValue<string>("directfile");
  } else {
      fSingleFile = kFALSE;
  }
  fDataDirectory = options.GetValue<string>("data");
  if (fDataDirectory.Length() == 0){
    QwError << "ERROR:  Can't get the data directory in the QwEventBuffer creator."
	    << QwLog::endl;
  } else if (! fDataDirectory.EndsWith("/")) {
      fDataDirectory.Append("/");
  }

  fRunRange = options.GetIntValuePair("run");
  fEventRange = options.GetIntValuePair("event");
  fSegmentRange = options.GetIntValuePair("segment");
  fRunListFileName = options.GetValue<string>("runlist");
  fChainDataFiles = options.GetValue<bool>("chainfiles");
  fDataFileStem = options.GetValue<string>("codafile-stem");
  fDataFileExtension = options.GetValue<string>("codafile-ext");
	fDataVersion = options.GetValue<int>("coda-version");
	
	if(fDataVersion == 2){
		decoder = new Coda2EventDecoder();
	} else if(fDataVersion == 3) {
		decoder = new Coda3EventDecoder();
	} else{
		QwError << "Invalid Coda Version. Only versions 2 and 3 are supported. "
						<< "Please set using --coda-version 2(3)" << QwLog::endl;
    exit(EXIT_FAILURE);
	}
	
	decoder->SetAllowLowSubbankIDs( options.GetValue<bool>("allow-low-subbank-ids") );

  // Open run list file
  /* runlist file format example:
     [5253]
     234
     246
     256
     345:456
     567:789
     [5259]
     [5260]
     0:10000
     [5261:5270]
     9000:10000
    - for run 5253 it will analyze three individual events, and two event ranges
    - for run 5259 it will analyze the entire run (all segments)
    - for run 5260 it will analyze the first 10000 events
    - for runs 5261 through 5270 it will analyze the events 9000 through 10000)
  */
  if (fRunListFileName.size() > 0) {
    fRunListFile = new QwParameterFile(fRunListFileName);
    fEventListFile = 0;
    if (! GetNextRunRange()) {
      QwWarning << "No run range found in run list file: " << fRunListFile->GetLine() << QwLog::endl;
    }
  } else {
    fRunListFile = NULL;
    fEventListFile = NULL;
  }
}

void QwEventBuffer::PrintRunTimes()
{
  UInt_t nevents = fNumPhysicsEvents - fStartingPhysicsEvent;
  if (nevents==0) nevents=1;
  QwMessage << QwLog::endl
	    << "Analysis of run " << GetRunNumber() << QwLog::endl
	    << fNumPhysicsEvents << " physics events were processed"<< QwLog::endl
	    << "CPU time used:  " << fRunTimer.CpuTime() << " s "
	    << "(" << 1000.0 * fRunTimer.CpuTime() / nevents << " ms per event)" << QwLog::endl
	    << "Real time used: " << fRunTimer.RealTime() << " s "
	    << "(" << 1000.0 * fRunTimer.RealTime() / nevents << " ms per event)" << QwLog::endl
	    << QwLog::endl;
}



/// Read the next requested event range, return true if success
Bool_t QwEventBuffer::GetNextEventRange() {
  // If there is an event list, open the next section
  if (fEventListFile && !fEventListFile->IsEOF()) {
    std::string eventrange;
    // Find next non-whitespace, non-comment, non-empty line, before EOF
    do {
      fEventListFile->ReadNextLine(eventrange);
      fEventListFile->TrimWhitespace();
      fEventListFile->TrimComment('#');
    } while (fEventListFile->LineIsEmpty() && !fEventListFile->IsEOF());
    // If EOF return false; no next event range
    if (fEventListFile->IsEOF()) return kFALSE;
    // Parse the event range
    fEventRange = QwParameterFile::ParseIntRange(":",eventrange);
    QwMessage << "Next event range is " << eventrange << QwLog::endl;
    return kTRUE;
  }
  return kFALSE;
}

/// Read the next requested run range, return true if success
Bool_t QwEventBuffer::GetNextRunRange() {
  // Delete any open event list file before moving to next run
  if (fEventListFile) delete fEventListFile;
  // If there is a run list, open the next section
  std::string runrange;
  if (fRunListFile && !fRunListFile->IsEOF() &&
     (fEventListFile = fRunListFile->ReadNextSection(runrange))) {
    // Parse the run range
    fRunRange = QwParameterFile::ParseIntRange(":",runrange);
    QwMessage << "Next run range is " << runrange << QwLog::endl;
    // If there is no event range for this run range, set to default of 0:MAXINT
    if (! GetNextEventRange()) {
      QwWarning << "No valid event range in run list file: "
                << fEventListFile->GetLine() << ". "
                << "Assuming the full event range." << QwLog::endl;
      fEventRange = QwParameterFile::ParseIntRange(":","0:");
    }
    return kTRUE;
  }
  return kFALSE;
}

/// Get the next run in the active run range, proceed to next range if needed
Bool_t QwEventBuffer::GetNextRunNumber() {
  // First run
  if (fCurrentRun == -1) {
    fCurrentRun = fRunRange.first;
    return kTRUE;
  // Run is in range
  } else if (fCurrentRun < fRunRange.second) {
    fCurrentRun++;
    return kTRUE;
  // Run is not in range, get new range
  } else if (GetNextRunRange()) {
    fCurrentRun = fRunRange.first;
    return kTRUE;
  }
  return kFALSE;
}

TString QwEventBuffer::GetRunLabel() const
{
  TString runlabel = Form("%d",fCurrentRun);
  if (fRunIsSegmented && ! fChainDataFiles){
    runlabel += Form(".%03d",*fRunSegmentIterator);
  }
  return runlabel;
}

Int_t QwEventBuffer::ReOpenStream()
{
  Int_t status = CODA_ERROR;
  //  Reset the physics event counter
  fNumPhysicsEvents = fStartingPhysicsEvent;

  if (fOnline) {
    // Online stream
    status = OpenETStream(fETHostname, fETSession, fETWaitMode, fETStationName);
  } else {
    // Offline data file
    if (fRunIsSegmented)
      // Segmented
      status = OpenNextSegment();
    else
      // Not segmented
      status = OpenDataFile(fCurrentRun);
  }
  return status;
}

Int_t QwEventBuffer::OpenNextStream()
{
  Int_t status = CODA_ERROR;
  if (globalEXIT==1) {
    //  We want to exit, so don't open the next stream.
    status = CODA_ERROR;
  } else if (fOnline) {
    /* Modify the call below for your ET system, if needed.
       OpenETStream( ET host name , $SESSION , mode)
       mode=0: wait forever
       mode=1: timeout quickly
    */
    QwMessage << "Try to open the ET station with HOSTNAME=="
	      << fETHostname
	      << ", SESSION==" << fETSession << "."
	      << QwLog::endl;
    status = OpenETStream(fETHostname, fETSession, fETWaitMode, fETStationName);

  } else {
    //  Try to open the next data file for the current run,
    //  but only if we haven't hit the event limit.
    if (fCurrentRun != -1 && !fChainDataFiles
	&& decoder->GetEvtNumber() <= fEventRange.second) {
      status = OpenNextSegment();
    }
    while (status != CODA_OK && GetNextRunNumber()) {
      status = OpenDataFile(fCurrentRun);
      if (status == CODA_ERROR){
	//  The data file can't be opened.
	//  Get ready to process the next run.
	QwError << "ERROR:  Unable to find data files for run "
		<< fCurrentRun << ".  Moving to the next run.\n"
		<< QwLog::endl;
      }
    }

  }
  //  Grab the starting event counter
  fStartingPhysicsEvent = fNumPhysicsEvents;
  //  Start the timers.
  fRunTimer.Reset();
  fRunTimer.Start();
  fStopwatch.Start();
  return status;
}

Int_t QwEventBuffer::CloseStream()
{
  //  Stop the timers.
  fRunTimer.Stop();
  fStopwatch.Stop();
  QwWarning << "Starting CloseStream."
	    << QwLog::endl;
  Int_t status = kFileHandleNotConfigured;
  if (fEvStreamMode==fEvStreamFile
      && (fRunIsSegmented && !fChainDataFiles) ){
    //  The run is segmented and we are not chaining the
    //  segments together in the event loop, so close
    //  the current segment.
    status = CloseThisSegment();
  } else if (fEvStreamMode==fEvStreamFile) {
    status = CloseDataFile();
  } else if (fEvStreamMode==fEvStreamFile){
    status = CloseETStream();
  }
  return status;
}



Int_t QwEventBuffer::GetNextEvent()
{
  //  This will return for read errors,
  //  non-physics events, and for physics
  //  events that are within the event range.
  Int_t status = CODA_OK;
  do {
    status = GetEvent();
    if (globalEXIT == 1) {
      //  QUESTION:  Should we continue to loop once we've
      //  reached the maximum event, to allow access to
      //  non-physics events?
      //  For now, mock up EOF if we've reached the maximum event.
      status = EOF;
    }
    if (decoder->GetEvtNumber() > fEventRange.second) {
      do {
        if (GetNextEventRange()) status = CODA_OK;
        else status = EOF;
      } while (decoder->GetEvtNumber() < fEventRange.first);
    }
    //  While we're in a run segment which was not requested (which
    //  should happen only when reading the zeroth segment for startup
    //  information), pretend that there's an event cut causing us to
    //  ignore events.  Read configuration events only from the first
    //  part of the file.
    if (fRunIsSegmented && GetSegmentNumber() < fSegmentRange.first) {
      fEventRange.first = decoder->GetEvtNumber() + 1;
      if (decoder->GetEvtNumber() > 1000) status = EOF;
    }
    if (fOnline && fExitOnEnd && decoder->GetEndTime()>0){
      // fExitOnEnd exits the event loop only and does not exit JAPAN. 
      // The root file gets processed and JAPAN immediately waits for the next run.
      // We considered adding a exit-JAPAN-on-end flag that quits JAPAN but decided 
      // we didn't have a use case for it. If quitting JAPAN is desired, just set:
      // globalEXIT = 1
      // -- mrc (01/21/25)
      QwMessage << "Caught End Event (end time=="<< decoder->GetEndTime()
		<< ").  Exit event loop." << QwLog::endl;
      status = EOF;
    }
    if (fOnline && onlineRestart){
      onlineRestart=0;
      status = EOF;
    }
  } while (status == CODA_OK  &&
           IsPhysicsEvent()   &&
           (decoder->GetEvtNumber() < fEventRange.first
         || decoder->GetEvtNumber() > fEventRange.second)
          );
  if (status == CODA_OK  && IsPhysicsEvent()) fNumPhysicsEvents++;

  //  Progress meter (this should probably produce less output in production)
  int nevents = 10000;
  if (IsPhysicsEvent() && decoder->GetEvtNumber() > 0 && decoder->GetEvtNumber() % nevents == 0) {
    QwMessage << "Processing event " << decoder->GetEvtNumber() << " ";
    fStopwatch.Stop();
    double efficiency = 100.0 * fStopwatch.CpuTime() / fStopwatch.RealTime();
    QwMessage << "(" << fStopwatch.CpuTime()*1e3/nevents << " ms per event with ";
    QwMessage << efficiency << "% efficiency)";
    fStopwatch.Reset();
    fStopwatch.Start();
    QwMessage << QwLog::endl;
  } else if (decoder->GetEvtNumber() > 0 && decoder->GetEvtNumber() % 100 == 0) {
    QwVerbose << "Processing event " << decoder->GetEvtNumber() << QwLog::endl;
  }

  return status;
}


Int_t QwEventBuffer::GetEvent()
{
  Int_t status = kFileHandleNotConfigured;
  ResetFlags();
  if (fEvStreamMode==fEvStreamFile){
    status = GetFileEvent();
  } else if (fEvStreamMode==fEvStreamET){
    status = GetEtEvent();
  }
  if (status == CODA_OK){
    // Coda Data was loaded correctly
    UInt_t* evBuffer = (UInt_t*)fEvStream->getEvBuffer();
	if(fDataVersionVerify == 0){ // Default = 0 => Undetermined
		VerifyCodaVersion(evBuffer);
	}
	decoder->DecodeEventIDBank(evBuffer);
  }
  return status;
}

// Tries to figure out what Coda Version the Data is
// fDataVersionVerify = 
//  	      2 -- Coda Version 2
//		      3 -- Coda Version 3
// 	   	      0 -- Default (Unknown, Could be a EPICs Event or a ROCConfiguration)
void QwEventBuffer::VerifyCodaVersion( const UInt_t *buffer )
{
	if(buffer[0] == 0) return;
	UInt_t header = buffer[1];
	int top = (header & 0xff000000) >> 24;
	int bot = (header & 0xff      );
	fDataVersionVerify = 0;     // Default
	if( (top == 0xff) && (bot != 0xcc) ){
		fDataVersionVerify = 3; // Coda 3	
	} else if( (top != 0xff) && (bot == 0xcc) ){
		fDataVersionVerify = 2; // Coda 2
	} 
	// Validate
	if( (fDataVersion != fDataVersionVerify) && (fDataVersionVerify != 0 ) ){
    	QwError << "QwEventBuffer::GetEvent:  Coda Version is not recognized" << QwLog::endl;
		QwError << "fDataVersion == " << fDataVersion 
	   	 	    << ", but it looks like the data is from Coda Version "
			    << fDataVersionVerify
			    << "\nTry running with --coda-version " << fDataVersionVerify
			    << "\nExiting ... " << QwLog::endl;
    	globalEXIT = 1;
	}
	return;
}

Int_t QwEventBuffer::GetFileEvent(){
  Int_t status = CODA_OK;
  //  Try to get a new event.  If the EOF occurs,
  //  and the run is segmented, try to open the
  //  next segment and read a new event; repeat
  //  if needed.
  do {
    status = fEvStream->codaRead();
    if (fChainDataFiles && status == EOF){
      CloseThisSegment();
      //  Crash out of the loop if we can't open the
      //  next segment!
      if (OpenNextSegment()!=CODA_OK) break;
    }
  } while (fChainDataFiles && status == EOF);
  return status;
}

Int_t QwEventBuffer::GetEtEvent(){
  Int_t status = CODA_OK;
  //  Do we want to have any loop here to wait for a bad
  //  read to be cleared?
  status = fEvStream->codaRead();
  if (status != CODA_OK) {
    globalEXIT = 1;
	}
  return status;
}


Int_t QwEventBuffer::WriteEvent(int* buffer)
{
  Int_t status = kFileHandleNotConfigured;
  ResetFlags();
  if (fEvStreamMode==fEvStreamFile){
    status = WriteFileEvent(buffer);
  } else if (fEvStreamMode==fEvStreamET) {
    QwMessage << "No support for writing to ET streams" << QwLog::endl;
    status = CODA_ERROR;
  }
  return status;
}

Int_t QwEventBuffer::WriteFileEvent(int* buffer)
{
  Int_t status = CODA_OK;
  //  fEvStream is of inherited type THaCodaData,
  //  but codaWrite is only defined for THaCodaFile.
  status = ((THaCodaFile*)fEvStream)->codaWrite((UInt_t*) buffer);
  return status;
}


Int_t QwEventBuffer::EncodeSubsystemData(QwSubsystemArray &subsystems)
{
  // Encode the data in the elements of the subsystem array
  std::vector<UInt_t> buffer;
  std::vector<ROCID_t> ROCList;
  subsystems.EncodeEventData(buffer);
  subsystems.GetROCIDList(ROCList);
  // Add CODA event header
 	std::vector<UInt_t> header = decoder->EncodePHYSEventHeader(ROCList);

  // Copy the encoded event buffer into an array of integers,
  // as expected by the CODA routines.
  // Size of the event buffer in long words
  int* codabuffer = new int[header.size() + buffer.size() + 1];
  // First entry contains the buffer size
  int k = 0;
  codabuffer[k++] = header.size() + buffer.size();
  for (size_t i = 0; i < header.size(); i++)
    codabuffer[k++] = header.at(i);
  for (size_t i = 0; i < buffer.size(); i++)
    codabuffer[k++] = buffer.at(i);

  // Now write the buffer to the stream
  Int_t status = WriteEvent(codabuffer);
  // delete the buffer
  delete[] codabuffer;
  // and report success or fail
  return status;
}


void QwEventBuffer::ResetControlParameters()
{
	decoder->ResetControlParameters();
}
void QwEventBuffer::ReportRunSummary()
{
	decoder->ReportRunSummary();
}

TString QwEventBuffer::GetStartSQLTime()
{
	return decoder->GetStartSQLTime();
}

TString QwEventBuffer::GetEndSQLTime()
{
	return decoder->GetEndSQLTime();
}

time_t  QwEventBuffer::GetStartUnixTime()
{
	return decoder->GetStartUnixTime();
}

time_t  QwEventBuffer::GetEndUnixTime()
{
	return decoder->GetEndUnixTime();
}

Int_t QwEventBuffer::EncodePrestartEvent(int runnumber, int runtype)
{
  int buffer[5];
  int localtime  = (int)time(0);
  decoder->EncodePrestartEventHeader(buffer, runnumber, runtype, localtime);
  return WriteEvent(buffer);
}
Int_t QwEventBuffer::EncodeGoEvent()
{
  int buffer[5];
  int localtime  = (int)time(0);
  int eventcount = 0;
  decoder->EncodeGoEventHeader(buffer, eventcount, localtime);
  return WriteEvent(buffer);
}
Int_t QwEventBuffer::EncodePauseEvent()
{
  int buffer[5];
  int localtime  = (int)time(0);
  int eventcount = 0;
  decoder->EncodePauseEventHeader(buffer, eventcount, localtime);
  return WriteEvent(buffer);
}
Int_t QwEventBuffer::EncodeEndEvent()
{
  int buffer[5];
  int localtime  = (int)time(0);
  int eventcount = 0;
  decoder->EncodeEndEventHeader(buffer, eventcount, localtime);
  return WriteEvent(buffer);
}


void QwEventBuffer::ResetFlags(){
}

Bool_t QwEventBuffer::FillSubsystemConfigurationData(QwSubsystemArray &subsystems)
{
  ///  Passes the data for the configuration events into each subsystem
  ///  object.  Each object is responsible for recognizing the configuration
  ///  data which it ought to decode.
  ///  NOTE TO DAQ PROGRAMMERS:
  ///      The configuration event for a ROC must have the same
  ///      subbank structure as the physics events for that ROC.
  Bool_t okay = kTRUE;
  UInt_t rocnum = decoder->GetEvtType() - 0x90;
  QwMessage << "QwEventBuffer::FillSubsystemConfigurationData:  "
	    << "Found configuration event for ROC"
	    << rocnum
	    << QwLog::endl;
	decoder->PrintDecoderInfo(QwMessage);
  //  Loop through the data buffer in this event.
  UInt_t *localbuff = (UInt_t*)(fEvStream->getEvBuffer());
	decoder->DecodeEventIDBank(localbuff);
  while ((okay = decoder->DecodeSubbankHeader(&localbuff[decoder->GetWordsSoFar()]))){
    //  If this bank has further subbanks, restart the loop.
    if (decoder->GetSubbankType() == 0x10) {
      QwMessage << "This bank has further subbanks, restart the loop" << QwLog::endl;
      continue;
    }
    //  If this bank only contains the word 'NULL' then skip
    //  this bank.
    if (decoder->GetFragLength()==1 && localbuff[decoder->GetWordsSoFar()]==kNullDataWord){
      decoder->AddWordsSoFarAndFragLength();
      QwMessage << "Skip this bank" << QwLog::endl;
      continue;
    }

    //  Subsystems may be configured to accept data in formats
    //  other than 32 bit integer (banktype==1), but the
    //  bank type is not provided.  Subsystems must be able
    //  to process their data knowing only the ROC and bank tags.
    //
    //  After trying the data in each subsystem, bump the
    //  fWordsSoFar to move to the next bank.

    subsystems.ProcessConfigurationBuffer(rocnum, decoder->GetSubbankTag(),
					  &localbuff[decoder->GetWordsSoFar()],
					  decoder->GetFragLength());
    decoder->AddWordsSoFarAndFragLength();
    QwDebug << "QwEventBuffer::FillSubsystemConfigurationData:  "
	    << "Ending loop: fWordsSoFar=="<<decoder->GetWordsSoFar()
	    <<QwLog::endl;
  }

  return okay;
}

Bool_t QwEventBuffer::FillSubsystemData(QwSubsystemArray &subsystems)
{
  //  Initialize local flag
  Bool_t okay = kTRUE;

  //  Reload the data buffer and decode the header again, this allows
  //  multiple calls to this function for different subsystem arrays.
  UInt_t *localbuff = (UInt_t*)(fEvStream->getEvBuffer());
  
	decoder->DecodeEventIDBank(localbuff);

  //  Clear the old event information from the subsystems.
  subsystems.ClearEventData();

  //  Pass CODA run, segment, event number and type to the subsystem array.
  subsystems.SetCodaRunNumber(fCurrentRun);
  subsystems.SetCodaSegmentNumber(fRunIsSegmented? *fRunSegmentIterator: 0);
  subsystems.SetCodaEventNumber(decoder->GetEvtNumber());
  subsystems.SetCodaEventType(decoder->GetEvtType());

  // If this event type is masked for the subsystem array, return right away
  if (((0x1 << (decoder->GetEvtType() - 1)) & subsystems.GetEventTypeMask()) == 0) {
    return kTRUE;
  }

  UInt_t offset;

  //  Loop through the data buffer in this event.
  while ((okay = decoder->DecodeSubbankHeader(&localbuff[decoder->GetWordsSoFar()]))){

    //  If this bank has further subbanks, restart the loop.
    if (decoder->GetSubbankType() == 0x10) continue;

    //  If this bank only contains the word 'NULL' then skip
    //  this bank.
    if (decoder->GetFragLength() == 1 && localbuff[decoder->GetWordsSoFar()]==kNullDataWord) {
      decoder->AddWordsSoFarAndFragLength();
      continue;
    }

    // if (GetSubbankType() == 0x85) {
    //   std::cout << "ProcessEventBuffer: , SubbankTag= "<< GetSubbankTag()<<" FragLength="<<GetFragLength() <<std::endl;
    // }

//     QwDebug << "QwEventBuffer::FillSubsystemData:  "
// 	    << "Beginning loop: fWordsSoFar=="<<GetWordsSoFar()
// 	    <<QwLog::endl;

    //  Loop through the subsystems and try to store the data
    //  from this bank in each subsystem.
    //
    //  Subsystems may be configured to accept data in formats
    //  other than 32 bit integer (banktype==1), but the
    //  bank type is not provided.  Subsystems must be able
    //  to process their data knowing only the ROC and bank tags.
    //
    //  After trying the data in each subsystem, bump the
    //  fWordsSoFar to move to the next bank.
		
		// TODO:
		// What is special about this subbank?
    if( decoder->GetROC() == 0 && decoder->GetSubbankTag()==0x6101) {
      //std::cout << "ProcessEventBuffer: ROC="<<GetROC()<<", SubbankTag="<< GetSubbankTag()<<", FragLength="<<GetFragLength() <<std::endl;
      fCleanParameter[0]=localbuff[decoder->GetWordsSoFar()+decoder->GetFragLength()-4];//clean data
      fCleanParameter[1]=localbuff[decoder->GetWordsSoFar()+decoder->GetFragLength()-3];//scan data 1
      fCleanParameter[2]=localbuff[decoder->GetWordsSoFar()+decoder->GetFragLength()-2];//scan data 2
      //std::cout << "ProcessEventBuffer: ROC="<<GetROC()<<", SubbankTag="<< GetSubbankTag()
      //		<<", FragLength="<<GetFragLength() << " " <<fCleanParameter[0]<< " " <<fCleanParameter[1]<< " " <<fCleanParameter[2]<<std::endl;

    }
    
    subsystems.SetCleanParameters(fCleanParameter);

    Int_t nmarkers = CheckForMarkerWords(subsystems);
    if (nmarkers>0) {
      //  There are markerwords for this ROC/Bank
      for (size_t i=0; i<nmarkers; i++){
	offset = FindMarkerWord(i,&localbuff[decoder->GetWordsSoFar()],decoder->GetFragLength());
	BankID_t tmpbank = GetMarkerWord(i);
	tmpbank = ((tmpbank)<<32) + decoder->GetSubbankTag();
	if (offset != -1){
	  offset++; //  Skip the marker word
	  subsystems.ProcessEvBuffer(decoder->GetEvtType(), decoder->GetROC(), tmpbank,
				     &localbuff[decoder->GetWordsSoFar()+offset],
				     decoder->GetFragLength()-offset);
	}
      }
    } else {
      QwDebug << "QwEventBuffer::FillSubsystemData:  "
	      << "fROC=="<<decoder->GetROC() << ", GetSubbankTag()==" << decoder->GetSubbankTag()
	      << QwLog::endl;	
      subsystems.ProcessEvBuffer(decoder->GetEvtType(), decoder->GetROC(), decoder->GetSubbankTag(),
				 &localbuff[decoder->GetWordsSoFar()],
				 decoder->GetFragLength());
    }
    decoder->AddWordsSoFarAndFragLength();
//     QwDebug << "QwEventBuffer::FillSubsystemData:  "
// 	    << "Ending loop: fWordsSoFar=="<<GetWordsSoFar()
// 	    <<QwLog::endl;
  }
  return okay;
}


// added all this method for QwEPICSEvent class
Bool_t QwEventBuffer::FillEPICSData(QwEPICSEvent &epics)
{
  // QwVerbose << "QwEventBuffer::FillEPICSData:  "
// 	  << Form("Length: %d; Tag: 0x%x; Bank ID num: 0x%x; ",
// 		  fEvtLength, fEvtTag, fIDBankNum)
// 	  << Form("Evt type: 0x%x; Evt number %d; Evt Class 0x%.8x; ",
// 		  fEvtType(), fEvtNumber, fEvtClass)
// 	  << Form("Status Summary: 0x%.8x; Words so far %d",
// 		  fStatSum, fWordsSoFar)
// 	  << QwLog::endl;


  ///
  Bool_t okay = kTRUE;
  if (! IsEPICSEvent()){
    okay = kFALSE;
    return okay;
  }
  QwVerbose << "QwEventBuffer::FillEPICSData:  "
	    << QwLog::endl;
  //  Loop through the data buffer in this event.
  UInt_t *localbuff = (UInt_t*)(fEvStream->getEvBuffer());
  if (decoder->GetBankDataType()==0x10){
    while ((okay = decoder->DecodeSubbankHeader(&localbuff[decoder->GetWordsSoFar()]))){
      //  If this bank has further subbanks, restart the loop.
      if (decoder->GetSubbankType() == 0x10) continue;
      //  If this bank only contains the word 'NULL' then skip
      //  this bank.
      if (decoder->GetFragLength()==1 && localbuff[decoder->GetWordsSoFar()]==kNullDataWord){
	decoder->AddWordsSoFarAndFragLength();
	continue;
      }

      if (decoder->GetSubbankType() == 0x3){
	//  This is an ASCII string bank.  Try to decode it and
	//  pass it to the EPICS class.
	char* tmpchar = (Char_t*)&localbuff[decoder->GetWordsSoFar()];
	
	epics.ExtractEPICSValues(string(tmpchar), GetEventNumber());
	QwVerbose << "test for GetEventNumber =" << GetEventNumber() << QwLog::endl;// always zero, wrong.
	
      }


      decoder->AddWordsSoFarAndFragLength();

//     QwDebug << "QwEventBuffer::FillEPICSData:  "
// 	    << "Ending loop: fWordsSoFar=="<<GetWordsSoFar()
// 	    <<QwLog::endl;
//     QwMessage<<"\nQwEventBuffer::FillEPICSData: fWordsSoFar = "<<GetWordsSoFar()<<QwLog::endl;


    }
  } else {
    // Single bank in the event, use event headers.
    if (decoder->GetBankDataType() == 0x3){
      //  This is an ASCII string bank.  Try to decode it and
      //  pass it to the EPICS class.
      Char_t* tmpchar = (Char_t*)&localbuff[decoder->GetWordsSoFar()];
      
      QwError << tmpchar << QwLog::endl;
      
      epics.ExtractEPICSValues(string(tmpchar), GetEventNumber());
      
    }

  }

  //std::cout<<"\nEpics data coming!! "<<GetWordsSoFar()<<std::endl;
  QwVerbose << "QwEventBuffer::FillEPICSData:  End of routine"
	    << QwLog::endl;
  return okay;
}

const TString&  QwEventBuffer::DataFile(const UInt_t run, const Short_t seg = -1)
{
  if(!fSingleFile){
    TString basename = fDataFileStem + Form("%u.",run) + fDataFileExtension;
    if(seg == -1){
      fDataFile = fDataDirectory + basename;
    } else {
      fDataFile = fDataDirectory + basename + Form(".%d",seg);
    }
  }
  return fDataFile;
}


Bool_t QwEventBuffer::DataFileIsSegmented()
{
  glob_t globbuf;

  TString searchpath;
  TString scanvalue;
  Int_t   local_segment;

  std::vector<Int_t> tmp_segments;
  std::vector<Int_t> local_index;

  /*  Clear and set up the fRunSegments vector. */
  tmp_segments.clear();
  fRunSegments.clear();
  fRunSegments.resize(0);
  fRunIsSegmented = kFALSE;

  searchpath = fDataFile;
  glob(searchpath.Data(), GLOB_ERR, NULL, &globbuf);

  if(fSingleFile){

    fRunIsSegmented = kFALSE;

  } else if (globbuf.gl_pathc == 1){
    /* The base file name exists.                               *
     * Do not look for file segments.                           */
    fRunIsSegmented = kFALSE;

  } else {
    /* The base file name does not exist.                       *
     * Look for file segments.                                  */
    QwWarning << "File " << fDataFile << " does not exist!\n"
	      << "      Trying to find run segments for run "
	      << fCurrentRun << "...  ";

    searchpath.Append(".[0-9]*");
    glob(searchpath.Data(), GLOB_ERR, NULL, &globbuf);

    if (globbuf.gl_pathc == 0){
      /* There are no file segments and no base file            *
       * Produce and error message and exit.                    */
      QwError << "\n      There are no file segments either!!" << QwLog::endl;

      // This could mean a single gzipped file!
      fRunIsSegmented = kFALSE;

    } else {
      /* There are file segments.                               *
       * Determine the segment numbers and fill fRunSegments    *
       * to indicate the existing file segments.                */

      QwMessage << "OK" << QwLog::endl;
      scanvalue = fDataFile + ".%d";

      /*  Get the list of segment numbers in file listing       *
       *  order.                                                */
      for (size_t iloop=0;iloop<globbuf.gl_pathc;++iloop){
        /*  Extract the segment numbers from the file name.     */
        sscanf(globbuf.gl_pathv[iloop], scanvalue.Data(), &local_segment);
        tmp_segments.push_back(local_segment);
      }
      local_index.resize(tmp_segments.size(),0);
      /*  Get the list of segments sorted numerically in        *
       *  increasing order.                                     */
      TMath::Sort(static_cast<int>(tmp_segments.size()),&(tmp_segments[0]),&(local_index[0]),
                  kFALSE);
      /*  Put the segments into numerical order in fRunSegments.  Add  *
       *  only those segments requested (though always add segment 0). */
      QwMessage << "      Found the segment(s): ";
      size_t printed = 0;
      for (size_t iloop=0; iloop<tmp_segments.size(); ++iloop){
        local_segment = tmp_segments[local_index[iloop]];
        if (printed++) QwMessage << ", ";
	QwMessage << local_segment ;
	if (local_segment == 0 ||
	    ( fSegmentRange.first <= local_segment &&
	      local_segment <= fSegmentRange.second ) ) {
	  fRunSegments.push_back(local_segment);
	} else {
	  QwMessage << " (skipped)" ;
	}
      }
      QwMessage << "." << QwLog::endl;
      fRunSegmentIterator = fRunSegments.begin();

      fRunIsSegmented = kTRUE;

      /* If the first requested segment hasn't been found,
	 forget everything. */
      if ( local_segment < fSegmentRange.first ) {
	QwError << "First requested run segment "
		<< fSegmentRange.first << " not found.\n";
	fRunSegments.pop_back();
	fRunSegmentIterator = fRunSegments.begin();
	fRunIsSegmented = kTRUE; // well, it is true.
      }
    }
  }
  globfree(&globbuf);
  return fRunIsSegmented;
}

//------------------------------------------------------------

Int_t QwEventBuffer::CloseThisSegment()
{
  Int_t status = kFileHandleNotConfigured;
  Int_t last_runsegment;
  if (fRunIsSegmented){
    last_runsegment = *fRunSegmentIterator;
    fRunSegmentIterator++;
    if (fRunSegmentIterator <= fRunSegments.end()){
      QwMessage << "Closing run segment " << last_runsegment <<"."
		<< QwLog::endl;
      status = CloseDataFile();
    }
  } else {
    //  Don't try to close a nonsegmented file; we will explicitly
    //  use CloseDataFile() later.
  }
  return status;
}

//------------------------------------------------------------

Int_t QwEventBuffer::OpenNextSegment()
{
  Int_t status;
  if (! fRunIsSegmented){
    /*  We are processing a non-segmented run.            *
     *  We should not have entered this routine, but      *
     *  since we are here, don't do anything.             */
    status = kRunNotSegmented;

  } else if (fRunSegments.size()==0){
    /*  There are actually no file segments located.      *
     *  Return "kNoNextDataFile", but don't print an      *
     *  error message.                                    */
    status = kNoNextDataFile;

  } else if (fRunSegmentIterator >= fRunSegments.begin() &&
             fRunSegmentIterator <  fRunSegments.end() ) {
    QwMessage << "Trying to open run segment " << *fRunSegmentIterator << QwLog::endl;
    status = OpenDataFile(DataFile(fCurrentRun,*fRunSegmentIterator),"R");

  } else if (fRunSegmentIterator == fRunSegments.end() ) {
    /*  We have reached the last run segment. */
    QwMessage << "There are no run segments remaining." << QwLog::endl;
    status = kNoNextDataFile;

  } else {
    QwError << "QwEventBuffer::OpenNextSegment(): Unrecognized error" << QwLog::endl;
    status = CODA_ERROR;
  }
  return status;
}


//------------------------------------------------------------
//call this routine if we've selected the run segment by hand
Int_t QwEventBuffer::OpenDataFile(UInt_t current_run, Short_t seg)
{
  fCurrentRun = current_run;

  fRunSegments.clear();
  fRunIsSegmented = kTRUE;

  fRunSegments.push_back(seg);
  fRunSegmentIterator = fRunSegments.begin();
  return OpenNextSegment();
}

//------------------------------------------------------------
//call this routine if the run is not segmented
Int_t QwEventBuffer::OpenDataFile(UInt_t current_run, const TString rw)
{
  Int_t status;
  fCurrentRun = current_run;
  DataFile(fCurrentRun);
  if (DataFileIsSegmented()){
    status = OpenNextSegment();
  } else {
    status = OpenDataFile(DataFile(fCurrentRun),rw);
  }
  return status;
}



//------------------------------------------------------------
Int_t QwEventBuffer::OpenDataFile(const TString filename, const TString rw)
{
  if (fEvStreamMode==fEvStreamNull){
    QwDebug << "QwEventBuffer::OpenDataFile:  File handle doesn't exist.\n"
	    << "                              Try to open a new file handle!"
	    << QwLog::endl;
    fEvStream = new THaCodaFile();
    fEvStreamMode = fEvStreamFile;
  } else if (fEvStreamMode!=fEvStreamFile){
    QwError << "QwEventBuffer::OpenDataFile:  The stream is not configured as an input\n"
	    << "                              file stream!  Can't deal with this!\n"
	    << QwLog::endl;
    exit(1);
  }
  fDataFile = filename;

  if (rw.Contains("w",TString::kIgnoreCase)) {
    // If we open a file for write access, let's suppose
    // we've given the path we want to use.
    QwMessage << "Opening data file:  " << fDataFile << QwLog::endl;
  } else {
    //  Let's try to find the data file for read access.
    glob_t globbuf;
    glob(fDataFile.Data(), GLOB_ERR, NULL, &globbuf);
    if (globbuf.gl_pathc == 0){
      //  Can't find the file; try in the "fDataDirectory".
      fDataFile = fDataDirectory + filename;
      glob(fDataFile.Data(), GLOB_ERR, NULL, &globbuf);
    }
    if (globbuf.gl_pathc == 0){
      //  Can't find the file; try gzipped.
      fDataFile = filename + ".gz";
      glob(fDataFile.Data(), GLOB_ERR, NULL, &globbuf);
    }
    if (globbuf.gl_pathc == 0){
      //  Can't find the file; try gzipped in the "fDataDirectory".
      fDataFile = fDataDirectory + filename + ".gz";
      glob(fDataFile.Data(), GLOB_ERR, NULL, &globbuf);
    }
    if (globbuf.gl_pathc == 1){
      QwMessage << "Opening data file:  " << fDataFile << QwLog::endl;
    } else {
      fDataFile = filename;
      QwError << "Unable to find "
              << filename.Data()  << " or "
              << (fDataDirectory + filename).Data()  << QwLog::endl;
    }
    globfree(&globbuf);
  }
  return fEvStream->codaOpen(fDataFile, rw);
}


//------------------------------------------------------------
Int_t QwEventBuffer::CloseDataFile()
{
  Int_t status = kFileHandleNotConfigured;
  if (fEvStreamMode==fEvStreamFile){
    status = fEvStream->codaClose();
  }
  return status;
}

//------------------------------------------------------------
Int_t QwEventBuffer::OpenETStream(TString computer, TString session, int mode,
				  const TString stationname)
{
  Int_t status = CODA_OK;
  if (fEvStreamMode==fEvStreamNull){
#ifdef __CODA_ET
    if (stationname != ""){
      fEvStream = new THaEtClient(computer, session, mode, stationname.Data());
    } else {
      fEvStream = new THaEtClient(computer, session, mode);
    }
    fEvStreamMode = fEvStreamET;
#endif
  }
  return status;
}

//------------------------------------------------------------
Int_t QwEventBuffer::CloseETStream()
{
  Int_t status = kFileHandleNotConfigured;
  if (fEvStreamMode==fEvStreamET){
    status = fEvStream->codaClose();
  }
  return status;
}

//------------------------------------------------------------
Int_t QwEventBuffer::CheckForMarkerWords(QwSubsystemArray &subsystems)
{
  QwDebug << "QwEventBuffer::GetMarkerWordList:  start function" <<QwLog::endl;
  fThisRocBankLabel = decoder->GetROC();
  fThisRocBankLabel = fThisRocBankLabel<<32;
  fThisRocBankLabel += decoder->GetSubbankTag();
  if (fMarkerList.count(fThisRocBankLabel)==0){
    std::vector<UInt_t> tmpvec;
    subsystems.GetMarkerWordList(decoder->GetROC(), decoder->GetSubbankTag(), tmpvec);
    fMarkerList.emplace(fThisRocBankLabel, tmpvec);
    fOffsetList.emplace(fThisRocBankLabel, std::vector<UInt_t>(tmpvec.size(),0));
  }
  QwDebug << "QwEventBuffer::GetMarkerWordList:  fMarkerList.count(fThisRocBankLabel)==" 
	  << fMarkerList.count(fThisRocBankLabel) 
	  << " fMarkerList.at(fThisRocBankLabel).size()==" 
	  << fMarkerList.at(fThisRocBankLabel).size()
	  << QwLog::endl;
  return fMarkerList.at(fThisRocBankLabel).size();
}

UInt_t QwEventBuffer::GetMarkerWord(UInt_t markerID){
  return fMarkerList.at(fThisRocBankLabel).at(markerID);
};


UInt_t QwEventBuffer::FindMarkerWord(UInt_t markerindex, UInt_t* buffer, UInt_t num_words){
  UInt_t markerpos  = fOffsetList.at(fThisRocBankLabel).at(markerindex);
  UInt_t markerval  = fMarkerList.at(fThisRocBankLabel).at(markerindex);
  if (markerpos < num_words && buffer[markerpos] == markerval){
    // The marker word is where it was last time
    return markerpos;
  } else {
    for (size_t i=0; i<num_words; i++){
      if (buffer[i] == markerval){
	fOffsetList.at(fThisRocBankLabel).at(markerindex) = i;
	markerpos = i;
	break;
      }
    }
  }
  return markerpos;
}
