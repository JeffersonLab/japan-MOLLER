#include "Coda3EventDecoder.h"
#include "THaCodaFile.h"
#include "QwOptions.h"

#include <vector>
#include <ctime>

#include "TError.h"

// Encoding Functions

/*
	Creates a PHYS Header Buffer
*/
std::vector<UInt_t> Coda3EventDecoder::EncodePHYSEventHeader(std::vector<ROCID_t> &ROCList)
{
	int localtime = (int) time(0);
	int ROCCount = ROCList.size();
	int wordcount = (8 + ROCCount*3);
	std::vector<UInt_t> header;
	// TODO:
	// Could we make this more dynamic by reversing the TBOBJ::Fill mechanism?
	header.push_back(0xFF501001);
	header.push_back(wordcount); 						 // word count for Trigger Bank
	header.push_back(0xFF212000 | ROCCount); // # of ROCs 
	
	header.push_back(0x010a0004); 
	// evtnum is held by a 64 bit ... for now we set the upper 32 bits to 0
	header.push_back(++fEvtNumber );
	header.push_back(0x0);

	// evttime is held by a 64 bit (bits 0-48 is the time) ... for now we set the upper 32 bits to 0
	header.push_back(localtime); 
	header.push_back(0x0);

	header.push_back(0x1850001);
	header.push_back(0xc0da); // TS# Trigger
	for(auto it = ROCList.begin(); it != ROCList.end(); it++){
		int base = 0x010002;
		int roc  = (*it << 24);
		header.push_back(roc | base);
		header.push_back(0x4D6F636B); // ASCII for 'MOCK'
		header.push_back(0x4D6F636B); // ASCII for 'MOCK'
	}

	return header;
}


/*
	Creates an Event Header Buffer
	Note: It expects a buffer size of 5. Giving a buffer size other than 5 results in UB
*/
void Coda3EventDecoder::EncodePrestartEventHeader(int* buffer, int runnumber, int runtype, int localtime)
{
	buffer[0] = 4; // Prestart event length
	buffer[1] = ((0xffd1 << 16) | (0x01 << 8));
	buffer[2] = localtime;
	buffer[3] = runnumber;
	buffer[4] = runtype;
}

/*
	Creates an Event Header Buffer
	Note: It expects a buffer size of 5. Giving a buffer size other than 5 results in UB
*/
void Coda3EventDecoder::EncodeGoEventHeader(int* buffer, int eventcount, int localtime)
{
	buffer[0] = 4; // Go event length
	buffer[1] = ((0xffd2 << 16) | (0x01 << 8) );
	buffer[2] = localtime;
	buffer[3] = 0; // unused
	buffer[4] = eventcount;
}

/*
	Creates an Event Header Buffer
	Note: It expects a buffer size of 5. Giving a buffer size other than 5 results in UB
*/
void Coda3EventDecoder::EncodePauseEventHeader(int* buffer, int eventcount, int localtime)
{
	buffer[0] = 4; // Pause event length
	buffer[1] = ((0xffd3 << 16) | (0x01 << 8) );
	buffer[2] =	localtime;
	buffer[3] = 0; // unused
	buffer[4] = eventcount;
}

/*
	Creates an Event Header Buffer
	Note: It expects a buffer size of 5. Giving a buffer size other than 5 results in UB
*/
void Coda3EventDecoder::EncodeEndEventHeader(int* buffer, int eventcount, int localtime)
{
	buffer[0] = 4; // End event length
	buffer[1] = ((0xffd4 << 16) | (0x01 << 8) );
	buffer[2] = localtime;
	buffer[3] = 0; // unused
	buffer[4] = eventcount; 
}

// Decoding Functions

/*
	Main Decoding Function
*/
Int_t Coda3EventDecoder::DecodeEventIDBank(UInt_t *buffer)
{
	// TODO:
	// How should we handle bad events??
	fPhysicsEventFlag = kFALSE;
	fControlEventFlag = kFALSE;
	Int_t ret = HED_OK;

	// Main engine for decoding, called by public LoadEvent() methods
	// this assert checks to see if buffer points to NULL
	assert(buffer);

	// General Event information
	fEvtLength = buffer[0]+1;  // in longwords (4 bytes)
	fEvtType = 0;
	fEvtTag = 0;
	fBankDataType = 0;

	// Prep TBOBJ variables
	tbank.Clear();
	tsEvType = 0;
	evt_time = 0;
	trigger_bits = 0;
	block_size = 0;

	// Start Filling Data
	fEvtTag     = (buffer[1] & 0xffff0000) >> 16;
	fBankDataType = (buffer[1] & 0xff00) >> 8;
	block_size  =	(buffer[1] & 0xff); 

	if(block_size > 1) {
		QwWarning << "MultiBlock is not properly supported! block_size = "
		<< block_size << QwLog::endl;
	}

	// Determine the event type by the call
	fEvtType = InterpretBankTag(fEvtTag);		
	fWordsSoFar = (2);
	if(fEvtTag < 0xff00) { /* user event */ printUserEvent(buffer); }
	else if(fControlEventFlag) {
		fEvtNumber = 0;	ProcessControlEvent(fEvtType, &buffer[fWordsSoFar]);
	}
	else if(fPhysicsEventFlag) { 
		ret = trigBankDecode( buffer );
		if(ret != HED_OK) { trigBankErrorHandler( ret ); }
		else { 
			fEvtNumber = tbank.evtNum;
			fWordsSoFar = 2 + tbank.len;
		}
	}
	else { // Not a control event, user event, nor physics event. Not sure what it is
		//  Arbitrarily set the event type to "fEvtTag".
		//  The first two words have been examined.
		QwWarning << "!!! Cannot determine the event type !!!" << QwLog::endl;
		QwMessage << "Printing Event Buffer:";
		QwMessage << "\n------------\n" << QwLog::endl;
		for(size_t index = 0; fEvtLength; index++){
			// TODO: // what if fEvtLength is gibberish because the event is gibberish?
			QwMessage << "\t" << buffer[index];
			if(index % 4 == 0){ QwMessage << QwLog::endl; }
		}	
		QwMessage << "\n------------\n" << QwLog::endl;
		fEvtType = fEvtTag;	fEvtNumber = 0;
	}

	fFragLength = fEvtLength - fWordsSoFar;	
	QwDebug << Form("buffer[0-1] 0x%x 0x%x ; ", buffer[0], buffer[1]);
	PrintDecoderInfo(QwDebug);

	return CODA_OK;
}

//_____________________________________________________________________________
UInt_t Coda3EventDecoder::InterpretBankTag( UInt_t tag )
{
	UInt_t evtyp{};
	if( tag >= 0xff00 ) { // CODA Reserved bank type
		switch( tag ) {
			case 0xffd1:
				evtyp = kPRESTART_EVENT;
				fControlEventFlag = kTRUE;
				break;
			case 0xffd2:
				evtyp = kGO_EVENT;
				fControlEventFlag = kTRUE;
				break;
			case 0xffd4:
				evtyp = kEND_EVENT;
				fControlEventFlag = kTRUE;
				break;
			case 0xff50:
			case 0xff58:      // Physics event with sync bit
			case 0xFF78:
			case 0xff70:
				evtyp = 1;      // for CODA 3.* physics events are type 1.
				fPhysicsEventFlag=kTRUE;
				break;
			default:          // Undefined CODA 3 event type
				QwWarning << "CodaDecoder:: WARNING:  Undefined CODA 3 event type, tag = "
						  << "0x" << std::hex << tag << std::dec << QwLog::endl;
				evtyp = 0;
				//FIXME evtyp = 0 could also be a user event type ...
				// maybe throw an exception here?
		}
	} else {              // User event type
		evtyp = tag;      // EPICS, ROC CONFIG, ET-insertions, etc.
	}
	return evtyp;
}


void Coda3EventDecoder::printUserEvent(const UInt_t *buffer)
{
	// checks of ET-inserted data
	Int_t print_it=0;

	switch( fEvtType ) {

		case EPICS_EVTYPE:
			// QwMessage << "EPICS data "<<QwLog::endl;
			// print_it=1;
			break;
			// Do we need this event?
		case PRESCALE_EVTYPE:
			QwMessage << "Prescale data "<<QwLog::endl;
			print_it=1;
			break;
			// Do we need this event?
		case DAQCONFIG_FILE1:
			QwMessage << "DAQ config file 1 "<<QwLog::endl;
			print_it=1;
			break;
			// Do we need this event?
		case DAQCONFIG_FILE2:
			QwMessage << "DAQ config file 2 "<<QwLog::endl;
			print_it=1;
			break;
			// Do we need this event?
		case SCALER_EVTYPE:
			QwMessage << "LHRS scaler event "<<QwLog::endl;
			print_it=1;
			break;
			// Do we need this event?
		case SBSSCALER_EVTYPE:
			QwMessage << "SBS scaler event "<<QwLog::endl;
			print_it=1;
			break;
			// Do we need this event?
		case HV_DATA_EVTYPE:
			QwMessage << "High voltage data event "<<QwLog::endl;
			print_it=1;
			break;
		default:
			// something else ?
			QwMessage << "\n--- Special event type: " << fEvtTag << "\n" << QwLog::endl;
	}
	if(print_it) {
		char *cbuf = (char *)buffer; // These are character data
		size_t elen = sizeof(int)*(buffer[0]+1);
		QwMessage << "Dump of event buffer .  Len = "<<elen<<QwLog::endl;
		// This dump will look exactly like the text file that was inserted.
		for (size_t ii=0; ii<elen; ii++) QwMessage << cbuf[ii];
	}
}

void Coda3EventDecoder::PrintDecoderInfo(QwLog& out)
{

	out << Form("Event Number: %d; Length: %d; Tag: 0x%x; Bank data type: 0x%x ",
			fEvtNumber, fEvtLength, fEvtTag, fBankDataType)
		<< Form("Evt type: 0x%x; Evt number %d; fWordsSoFar %d",
				fEvtType, fEvtNumber, fWordsSoFar )
		<< QwLog::endl;
}

Int_t Coda3EventDecoder::trigBankDecode( UInt_t* buffer)
{
	const char* const HERE = "Coda3EventDecoder::trigBankDecode";
	if(block_size == 0) {
		QwError << HERE << ": CODA 3 Format Error: Physics event #" << fEvtNumber
			<< " with block size 0" << QwLog::endl;
		return HED_ERR;
	}
	// Set up exception handling for the PHYS Bank
	try {
		tbank.Fill(&buffer[fWordsSoFar], block_size, TSROCNumber);
	} 
	catch( const coda_format_error& e ) {
		Error(HERE, "CODA 3 format error: %s", e.what() );
		return HED_ERR;
	}
	// Copy pertinent data to member variables for faster retrieval
	LoadTrigBankInfo(0);  // Load data for first event in block
	return HED_OK;
}


//_____________________________________________________________________________
uint32_t Coda3EventDecoder::TBOBJ::Fill( const uint32_t* evbuffer,
		uint32_t blkSize, uint32_t tsroc )
{
	if( blkSize == 0 )
		throw std::invalid_argument("CODA block size must be > 0");
	start = evbuffer;
	blksize = blkSize;
	len = evbuffer[0] + 1;
	tag = (evbuffer[1] & 0xffff0000) >> 16;
	nrocs = evbuffer[1] & 0xff;

	const uint32_t* p = evbuffer + 2;
	// Segment 1:
	//  uint64_t event_number
	//  uint64_t run_info                if withRunInfo
	//  uint64_t time_stamp[blkSize]     if withTimeStamp
	{
		uint32_t slen = *p & 0xffff;
		if( slen != 2*(1 + (withRunInfo() ? 1 : 0) + (withTimeStamp() ? blkSize : 0)))
			throw coda_format_error("Invalid length for Trigger Bank seg 1");
		const auto* q = (const uint64_t*) (p + 1);
		evtNum  = *q++;
		runInfo = withRunInfo()   ? *q++ : 0;
		evTS    = withTimeStamp() ? q    : nullptr;
		p += slen + 1;
	}
	if( p-evbuffer >= len )
		throw coda_format_error("Past end of bank after Trigger Bank seg 1");

	// Segment 2:
	//  uint16_t event_type[blkSize]
	//  padded to next 32-bit boundary
	{
		uint32_t slen = *p & 0xffff;
		if( slen != (blkSize-1)/2 + 1 )
			throw coda_format_error("Invalid length for Trigger Bank seg 2");
		evType = (const uint16_t*) (p + 1);
		p += slen + 1;
	}

	// nroc ROC segments containing timestamps and optional
	// data like trigger latch bits:
	// struct {
	//   uint64_t roc_time_stamp;     // Lower 48 bits only seem to be the time.
	//   uint32_t roc_trigger_bits;   // Optional. Typically only in TSROC.
	// } roc_segment[blkSize];
	TSROC = nullptr;
	tsrocLen = 0;
	for( uint32_t i = 0; i < nrocs; ++i ) {
		if( p-evbuffer >= len )
			throw coda_format_error("Past end of bank while scanning trigger bank segments");
		uint32_t slen = *p & 0xffff;
		uint32_t rocnum = (*p & 0xff000000) >> 24;
		// TODO:
		// tsroc is the crate # of the TS
		// This is filled with the THaCrateMap class which we are not using
		// tsroc is currently always 0
		if( rocnum == tsroc ) {
			TSROC = p + 1;
			tsrocLen = slen;
			break;
		}
		p += slen + 1;
	}

	return len;
}

Int_t Coda3EventDecoder::LoadTrigBankInfo( UInt_t i )
{
	// CODA3: Load tsEvType, evt_time, and trigger_bits for i-th event
	// in event block buffer. index_buffer must be < block size.

	assert(i < tbank.blksize);
	if( i >= tbank.blksize )
		return -1;
	tsEvType = tbank.evType[i];      // event type (configuration-dependent)
	if( tbank.evTS )
		evt_time = tbank.evTS[i];      // event time (4ns clock, I think)
	else if( tbank.TSROC ) {
		UInt_t struct_size = tbank.withTriggerBits() ? 3 : 2;
		evt_time = *(const uint64_t*) (tbank.TSROC + struct_size * i);
		// Only the lower 48 bits seem to contain the time
		evt_time &= 0x0000FFFFFFFFFFFF;
	}
	if( tbank.withTriggerBits() ){
		// Trigger bits. Only the lower 6 bits seem to contain the actual bits
		trigger_bits = tbank.TSROC[2 + 3 * i] & 0x3F;
	}
	return 0;
}


void Coda3EventDecoder::trigBankErrorHandler( Int_t flag )
{
	// TODO:
	// How should we handle bad events?
	//QwError << "trigBankErrorHandling is not yet fully supported!" << QwLog::endl;
	switch(flag){
		case HED_OK:
			QwWarning << "TrigBankDecode() returned HED_OK... why are we here?" << QwLog::endl;
			break;
		case HED_WARN:
			QwError << "TrigBankDecode() returned HED_WARN" << QwLog::endl;
			break;
		case HED_ERR:
			QwError << "TrigBankDecode() returned HED_ERR" << QwLog::endl;
			break;
		case HED_FATAL:
			QwError << "TrigBankDecoder() returned HED_FATAL" << QwLog::endl;
			break;
		default:
			QwError << "TrigBankDecoder() returned an Unknown Error" << QwLog::endl;
			break;
	}
	// Act as if we are at the end of the event and set everything to false (0)
	QwWarning << "Skipping to the end of the event and setting everything to false (0)!" << QwLog::endl;
	fPhysicsEventFlag = kFALSE;
	fControlEventFlag = kFALSE;

	fEvtType = 0;
	fEvtTag = 0;
	fBankDataType = 0;
	tbank.Clear();
	tsEvType = 0;
	evt_time = 0;
	trigger_bits = 0;
	block_size = 0;

	fWordsSoFar = fEvtLength;
}



