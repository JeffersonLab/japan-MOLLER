#include "Coda2EventDecoder.h"
#include "THaCodaFile.h"
#include "QwOptions.h"

#include <vector>
#include <ctime>

/**
 * @brief Creates PHYS Event EVIO Header
 * @param ROCList List of ROCS. Unused in Coda2 headers.
 * @return PHYS Event Header
 */
std::vector<UInt_t> Coda2EventDecoder::EncodePHYSEventHeader(std::vector<ROCID_t> &ROCList)
{
	std::vector<UInt_t> header;
	header.push_back((0x0001 << 16) | (0x10 << 8) | 0xCC);
	// event type | event data type | event ID (0xCC for CODA event)
	header.push_back(4);	// size of header field
	header.push_back((0xC000 << 16) | (0x01 << 8) | 0x00);
	// bank type | bank data type (0x01 for uint32) | bank ID (0x00 for header event)
	header.push_back(++fEvtNumber); // event number (initialized to 0, so increment before use to agree with CODA number)
	header.push_back(1);	// event class
	header.push_back(0);	// status summary
	return header;
}

/**
 * @brief Creates Prestart Event EVIO Header
 * @param buffer Array of size 5 to store the Prestart Header
 * @param runnumber Number of the run
 * @param runtype Run type
 * @param localtime Event time
 */
void Coda2EventDecoder::EncodePrestartEventHeader(int* buffer, int runnumber, int runtype, int localtime)
{
	buffer[0] = 4; // Prestart event length
	buffer[1] = ((kPRESTART_EVENT << 16) | (0x01 << 8) | 0xCC);
	buffer[2] = localtime;
	buffer[3] = runnumber;
	buffer[4] = runtype;
  ProcessPrestart(localtime, runnumber, runtype);
}

/**
 * @brief Creates Go Event EVIO Header
 * @param buffer Array of size 5 to store the Go Header
 * @param eventcount Number of events 
 * @param localtime Event time
 */
void Coda2EventDecoder::EncodeGoEventHeader(int* buffer, int eventcount, int localtime)
{
	buffer[0] = 4; // Go event length
	buffer[1] = ((kGO_EVENT << 16) | (0x01 << 8) | 0xCC);
	buffer[2] = localtime;
	buffer[3] = 0; // unused
	buffer[4] = eventcount;
  ProcessGo(localtime, eventcount);
}

/**
 * @brief Creates Pause Event EVIO Header
 * @param buffer Array of size 5 to store the Go Header
 * @param eventcount Number of events 
 * @param localtime Event time
 */
void Coda2EventDecoder::EncodePauseEventHeader(int* buffer, int eventcount, int localtime)
{
	buffer[0] = 4; // Pause event length
	buffer[1] = ((kPAUSE_EVENT << 16) | (0x01 << 8) | 0xCC);
	buffer[2] =	localtime;
	buffer[3] = 0; // unused
	buffer[4] = eventcount;
  ProcessPause(localtime, eventcount);
}
 
/**
 * @brief Creates End Event EVIO Header
 * @param buffer Array of size 5 to store the End Header
 * @param eventcount Number of events 
 * @param localtime Event time
 */
void Coda2EventDecoder::EncodeEndEventHeader(int* buffer, int eventcount, int localtime)
{
	buffer[0] = 4; // End event length
	buffer[1] = ((kEND_EVENT << 16) | (0x01 << 8) | 0xCC);
	buffer[2] = localtime;
	buffer[3] = 0; // unused
	buffer[4] = eventcount; 
  ProcessEnd(localtime, eventcount);
}

/**
 * @brief Determines if a buffer contains a PHYS event, Control Event, or some other event
 * @param buffer Event buffer to decode
 * @return CODA_OK
 */
Int_t Coda2EventDecoder::DecodeEventIDBank(UInt_t *buffer)
{

	UInt_t local_datatype;
	UInt_t local_eventtype;

	fPhysicsEventFlag = kFALSE;

	QwDebug << "QwEventBuffer::DecodeEventIDBank: " <<  std::hex
			<< buffer[0] << " "
			<< buffer[1] << " "
			<< buffer[2] << " "
			<< buffer[3] << " "
			<< buffer[4] << std::dec 
			<< QwLog::endl;

	if ( buffer[0] == 0 ){
		/*****************************************************************
		 *  This buffer is empty.                                        *
		 *****************************************************************/
		fEvtLength = (1);     //  Pretend that there is one word.
		fWordsSoFar = (1);      //  Mark that we've read the word already.
		fEvtType = (0);
		fEvtTag       = 0;
		fBankDataType = 0;
		fIDBankNum    = 0;
		fEvtNumber    = 0;
		fEvtClass     = 0;
		fStatSum      = 0;
	} else {
		/*****************************************************************
		 *  This buffer contains data; fill the event ID parameters.     *
		 *****************************************************************/
		//  First word is the number of long-words in the buffer.
		fEvtLength = (buffer[0]+1);

		// Second word contains the event type, for CODA events.
		fEvtTag   = (buffer[1] & 0xFFFF0000) >> 16;  // (bits(31-16));
		local_datatype = (buffer[1] & 0xFF00) >> 8;  // (bits(15-8));
		fIDBankNum = (buffer[1] & 0xFF);             // (bits(7-0));
		if ( fIDBankNum == 0xCC) {
			//  This is a CODA event bank; the event type is equal to
			//  the event tag.
			local_eventtype = fEvtTag;
			fEvtType = (local_eventtype);
			fBankDataType = local_datatype;

			// local_eventtype is unsigned int and always positive
			if (/* local_eventtype >= 0 && */ local_eventtype <= 15) {
				//  This is a physics event; record the event number, event
				//  classification, and status summary.
				fPhysicsEventFlag = kTRUE;
				fEvtNumber = buffer[4];
				fEvtClass  = buffer[5];
				fStatSum   = buffer[6];
				//  Now skip to the first ROC data bank.
				fWordsSoFar = (7);
			} else {
				//  This is not a physics event, but is still in the CODA
				//  event format.  The first two words have been examined.
				fEvtNumber = 0;
				fEvtClass  = 0;
				fStatSum   = 0;
				fWordsSoFar = (2);
				//  Run this event through the Control event processing.
				//  If it is not a control event, nothing will happen.
				ProcessControlEvent(fEvtType, &buffer[fWordsSoFar]);
			}
		} else {
			//  This is not an event in the CODA event bank format,
			//  but it still follows the CEBAF common event format.
			//  Arbitrarily set the event type to "fEvtTag".
			//  The first two words have been examined.
			fEvtType = (fEvtTag);
			fBankDataType = local_datatype;
			fEvtNumber = 0;
			fEvtClass  = 0;
			fStatSum   = 0;
			fWordsSoFar = (2);
		}
	}
	//  Initialize the fragment size to the event size, in case the 
	//  event is not subbanked.
	fFragLength = fEvtLength-fWordsSoFar;
	QwDebug << Form("buffer[0-1] 0x%x 0x%x ; ",
			buffer[0], buffer[1])
		<< Form("Length: %d; Tag: 0x%x; Bank data type: 0x%x; Bank ID num: 0x%x; ",
				fEvtLength, fEvtTag, fBankDataType, fIDBankNum)
		<< Form("Evt type: 0x%x; Evt number %d; Evt Class 0x%.8x; ",
				fEvtType, fEvtNumber, fEvtClass)
		<< Form("Status Summary: 0x%.8x; Words so far %d",
				fStatSum, fWordsSoFar)
		<< QwLog::endl;

	return CODA_OK;
}

/**
 * @brief Prints Internal Decoder Information
 * @param out Output buffer to use to dispay internal Decoder Information.\nCan be QwMessage, QwVerbose, QwWarning, or QwErrror.
 */
void Coda2EventDecoder::PrintDecoderInfo(QwLog& out)
{

	out << Form("Length: %d; Tag: 0x%x; Bank data type: 0x%x; Bank ID num: 0x%x; ",
			fEvtLength, fEvtTag, fBankDataType, fIDBankNum)
		<< Form("Evt type: 0x%x; Evt number %d; Evt Class 0x%.8x; ",
				fEvtType, fEvtNumber, fEvtClass)
		<< QwLog::endl;
}
