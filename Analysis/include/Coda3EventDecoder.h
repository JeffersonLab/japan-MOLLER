/*!
 * \file   Coda3EventDecoder.h
 * \brief  CODA version 3 event decoder implementation
 */

#pragma once

#include "VEventDecoder.h"
#include "Rtypes.h"

#include <vector>

/**
 * \class Coda3EventDecoder
 * \ingroup QwAnalysis
 * \brief CODA version 3 event decoder implementation
 *
 * Concrete decoder for CODA 3.x format event streams, handling the specific
 * data structures, bank formats, and trigger information used in CODA 3.
 * Provides encoding and decoding capabilities for both real and mock data.
 */
class Coda3EventDecoder : public VEventDecoder
{
public:
	Coda3EventDecoder() :
		tsEvType(0),
		block_size(0),
		evt_time(0),
		trigger_bits(0),
		TSROCNumber(0) { }
	~Coda3EventDecoder() override { }
public:
	// Encoding Functions
	/** Create a PHYS event EVIO header.
	 *  @param ROCList List of ROC IDs.
	 *  @return Vector of 32-bit words containing the header.
	 */
	std::vector<UInt_t> EncodePHYSEventHeader(std::vector<ROCID_t> &ROCList) override;
	/** Create a PRESTART event EVIO header.
	 *  @param buffer    Output buffer (>= 5 words).
	 *  @param runnumber Run number.
	 *  @param runtype   Run type.
	 *  @param localtime Event time.
	 */
	void EncodePrestartEventHeader(int* buffer, int runnumber, int runtype, int localtime) override;
	/** Create a GO event EVIO header.
	 *  @param buffer     Output buffer (>= 5 words).
	 *  @param eventcount Number of events.
	 *  @param localtime  Event time.
	 */
	void EncodeGoEventHeader(int* buffer, int eventcount, int localtime) override;
	/** Create a PAUSE event EVIO header.
	 *  @param buffer     Output buffer (>= 5 words).
	 *  @param eventcount Number of events.
	 *  @param localtime  Event time.
	 */
	void EncodePauseEventHeader(int* buffer, int eventcount, int localtime) override;
	/** Create an END event EVIO header.
	 *  @param buffer     Output buffer (>= 5 words).
	 *  @param eventcount Number of events.
	 *  @param localtime  Event time.
	 */
	void EncodeEndEventHeader(int* buffer, int eventcount, int localtime) override;

public:
	// Decoding Functions
	/** Determine if a buffer contains a PHYS, control, or other event.
	 *  @param buffer Event buffer to decode.
	 *  @return CODA_OK on success.
	 */
        Int_t DecodeEventIDBank(UInt_t *buffer) override;
private:
	// Debugging Functions
	/** Print non-PHYS, non-control "user" events. */
	void printUserEvent(const UInt_t *buffer);
	/** Print internal decoder state for diagnostics. */
        void PrintDecoderInfo(QwLog& out) override;
protected:
	// TI Decoding Functions
	/** Determine event type and set control/physics flags based on bank tag. */
	UInt_t InterpretBankTag(UInt_t tag);
	/** Decode the TI trigger bank for PHYS events. */
	Int_t trigBankDecode(UInt_t* buffer);
	/** Display a warning and reset state for a given TI error flag. */
	void trigBankErrorHandler( Int_t flag );

	ULong64_t GetEvTime() const { return evt_time; }
	void SetEvTime(ULong64_t evtime) { evt_time = evtime; }
	UInt_t tsEvType, block_size;
	ULong64_t evt_time; // Event time (for CODA 3.* this is a 250 Mhz clock)
	UInt_t trigger_bits; //  (Not completely sure) The TS# trigger for the TS
public:
	// Ti Specific Functions
	enum { HED_OK = 0, HED_WARN = -63, HED_ERR = -127, HED_FATAL = -255 };
	class coda_format_error : public std::runtime_error {
	public:
		explicit coda_format_error( const std::string& what_arg ) : std::runtime_error(what_arg) {}
		explicit coda_format_error( const char* what_arg )        : std::runtime_error(what_arg) {}
	};
	
	// Trigger Bank OBJect
	class TBOBJ {
	public:
    	TBOBJ() : blksize(0), tag(0), nrocs(0), len(0), tsrocLen(0), evtNum(0),
               	  runInfo(0), start(nullptr), evTS(nullptr), evType(nullptr),
                  TSROC(nullptr) {}
		void     Clear() { memset(this, 0, sizeof(*this)); }
		uint32_t Fill( const uint32_t* evbuffer, uint32_t blkSize, uint32_t tsroc );
		bool     withTimeStamp()   const { return (tag & 1) != 0; }
		bool     withRunInfo()     const { return (tag & 2) != 0; }
		bool     withTriggerBits() const { return (tsrocLen > 2*blksize);}

		uint32_t blksize;          /* total number of triggers in the Bank */
		uint16_t tag;              /* Trigger Bank Tag ID = 0xff2x */
		uint16_t nrocs;            /* Number of ROC Banks in the Event Block (val = 1-256) */
		uint32_t len;              /* Total Length of the Trigger Bank - including Bank header */
		uint32_t tsrocLen;         /* Number of words in TSROC array */
		uint64_t evtNum;           /* Starting Event # of the Block */
		uint64_t runInfo;          /* Run Info Data (optional) */
		const uint32_t *start;     /* Pointer to start of the Trigger Bank */
		const uint64_t *evTS;      /* Pointer to the array of Time Stamps (optional) */
		const uint16_t *evType;    /* Pointer to the array of Event Types */
		const uint32_t *TSROC;     /* Pointer to Trigger Supervisor ROC segment data */
	};

protected:
	/** Load TI trigger bank info for the i-th event in block. */
	Int_t LoadTrigBankInfo( UInt_t index_buffer );
	TBOBJ tbank;

public:
	// Hall A analyzer keywords (analyzer/Decoder.h)
	// Keywords that collide with JAPAN have been removed (deferring to JAPAN's definitions)
	static const UInt_t MAX_PHYS_EVTYPE  = 14;  // Types up to this are physics
	static const UInt_t TS_PRESCALE_EVTYPE  = 120;
	// TODO:
	// Do we need any of these keywords?
	static const UInt_t PRESCALE_EVTYPE  = 133;
	static const UInt_t DETMAP_FILE      = 135; // Most likely do not need this one
	static const UInt_t DAQCONFIG_FILE1  = 137;
	static const UInt_t DAQCONFIG_FILE2  = 138;
	static const UInt_t TRIGGER_FILE     = 136;
	static const UInt_t SCALER_EVTYPE    = 140;
	static const UInt_t SBSSCALER_EVTYPE = 141;
	static const UInt_t HV_DATA_EVTYPE   = 150;

protected:
	// TODO:
	// How does JAPAN want to handle a TS?
	// Currently implemented as 0
	uint32_t TSROCNumber;
};
