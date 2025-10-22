/*!
 * \file   VEventDecoder.h
 * \brief  Virtual base class for event decoders to encode and decode CODA data
 */

#pragma once

#include <vector>


#include "Rtypes.h"
#include "QwTypes.h"
#include "MQwCodaControlEvent.h"
#include "QwOptions.h"


/**
 * \class VEventDecoder
 * \ingroup QwAnalysis
 * \brief Abstract base for CODA event encoding and decoding
 *
 * Provides the interface for encoding mock CODA events and decoding
 * real CODA event streams. Concrete implementations handle version-specific
 * differences (CODA 2 vs CODA 3) while exposing a common API for event
 * type detection, bank decoding, and header processing.
 */
class VEventDecoder : public MQwCodaControlEvent {
public:
	VEventDecoder() : 
		fWordsSoFar(0),
		fEvtLength(0),
		fEvtNumber(0),
		fFragLength(0),
		fEvtType(0),
		fEvtTag(0),
		fBankDataType(0),
		fSubbankTag(0),
		fSubbankType(0),
		fSubbankNum(0),
		fROC(0),
    	fPhysicsEventFlag(kFALSE),
		fControlEventFlag(kFALSE),
		fAllowLowSubbankIDs(kFALSE) { }

	virtual ~VEventDecoder()  { }

public:
	// Encoding Functions
	/**
	 * \brief Create a physics-event (PHYS) header bank for the given ROCs.
	 *
	 * Encodes a minimal CODA-compliant PHYS event header for one trigger,
	 * suitable for mock-data generation and unit tests.
	 *
	 * @param ROCList List of ROC IDs to include in the header/banks.
	 * @return A vector of 32-bit words containing the encoded header.
	 */
	virtual std::vector<UInt_t> EncodePHYSEventHeader(std::vector<ROCID_t> &ROCList) = 0;
	/**
	 * \brief Encode a PRESTART control-event header.
	 * @param buffer Output buffer (size >= 5 words) to receive the header.
	 * @param runnumber Run number to store in the header.
	 * @param runtype   Run type identifier.
	 * @param localtime Wall-clock time (seconds) for the header timestamp.
	 */
	virtual void EncodePrestartEventHeader(int* buffer, int runnumber, int runtype, int localtime) = 0;
	/**
	 * \brief Encode a GO control-event header.
	 * @param buffer Output buffer (size >= 5 words) to receive the header.
	 * @param eventcount Current event count.
	 * @param localtime  Wall-clock time (seconds) for the header timestamp.
	 */
	virtual void EncodeGoEventHeader(int* buffer, int eventcount, int localtime)     = 0;
	/**
	 * \brief Encode a PAUSE control-event header.
	 * @param buffer Output buffer (size >= 5 words) to receive the header.
	 * @param eventcount Current event count.
	 * @param localtime  Wall-clock time (seconds) for the header timestamp.
	 */
	virtual void EncodePauseEventHeader(int* buffer, int eventcount, int localtime)  = 0;
	/**
	 * \brief Encode an END control-event header.
	 * @param buffer Output buffer (size >= 5 words) to receive the header.
	 * @param eventcount Final event count.
	 * @param localtime  Wall-clock time (seconds) for the header timestamp.
	 */
	virtual void EncodeEndEventHeader(int* buffer, int eventcount, int localtime)    = 0;

public:
	// Decoding Functions
	/**
	 * \brief Decode the event ID bank and classify the event type.
	 * @param buffer Pointer to the start of the event buffer.
	 * @return Non-negative on success (implementation-specific), negative on error.
	 */
	virtual Int_t DecodeEventIDBank(UInt_t *buffer) = 0;
	/**
	 * \brief Decode the subbank header for the current event/bank context.
	 *
	 * Updates internal fields (subbank tag/type/num, ROC, fragment length)
	 * and advances the internal word counter to the first subbank data word.
	 *
	 * @param buffer Pointer to the current position in the event buffer.
	 * @return kTRUE if a valid header was decoded; kFALSE at end-of-event or on error.
	 */
	virtual Bool_t DecodeSubbankHeader(UInt_t *buffer);
	/**
	 * \brief Print internal decoder state for diagnostics.
	 * @param out Logging stream to receive the formatted state (QwMessage/QwWarning/etc.).
	 */
	virtual void PrintDecoderInfo(QwLog& out);

public:
	// Boolean Functions
	virtual Bool_t IsPhysicsEvent(){
		return fPhysicsEventFlag;
	}

	virtual Bool_t IsROCConfigurationEvent(){
		return (fEvtType>=0x90 && fEvtType<=0x18f);
	}

	virtual Bool_t IsEPICSEvent(){
		return (fEvtType==EPICS_EVTYPE); // Defined in CodaDecoder.h
	}
// Accessor Functions
	// Generic Information
	UInt_t GetWordsSoFar()const  { return fWordsSoFar;  }
	UInt_t GetEvtNumber()const   { return fEvtNumber;   }
	UInt_t GetEvtLength()const   { return fEvtLength;   }
	UInt_t GetFragLength()const  { return fFragLength;  }
	// Event Information
	UInt_t GetEvtType()const     { return fEvtType;     }
	UInt_t GetBankDataType()const{ return fBankDataType;}
	UInt_t GetSubbankTag()const  { return fSubbankTag;  }
	UInt_t GetSubbankType()const { return fSubbankType; }
	ROCID_t GetROC()const        { return fROC;         }

// Mutator Functions
	void SetWordsSoFar(UInt_t val)                  { fWordsSoFar = val;         }
	void AddWordsSoFarAndFragLength()               { fWordsSoFar += fFragLength;}
	void SetFragLength(UInt_t val)					{ fFragLength = val;		 }
	void SetAllowLowSubbankIDs(Bool_t val = kFALSE) { fAllowLowSubbankIDs = val; }
	

protected:
	// Generic Information
	UInt_t fWordsSoFar;
	UInt_t fEvtLength;	
	UInt_t fEvtNumber;   ///< CODA event number; only defined for physics events
	UInt_t fFragLength;

	// Event Information
	UInt_t fEvtType;
	UInt_t fEvtTag;
	UInt_t fBankDataType;
	BankID_t fSubbankTag;
	UInt_t fSubbankType;
	UInt_t fSubbankNum;
	ROCID_t fROC;

	// Booleans
	Bool_t fPhysicsEventFlag;
	Bool_t fControlEventFlag;
	Bool_t fAllowLowSubbankIDs;

protected:
	enum KEYWORDS {
		EPICS_EVTYPE = 131
	};

};
