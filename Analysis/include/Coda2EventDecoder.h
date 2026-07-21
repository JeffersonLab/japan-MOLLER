/*!
 * \file   Coda2EventDecoder.h
 * \brief  CODA version 2 event decoder implementation
 */

#pragma once

#include "VEventDecoder.h"
#include "Rtypes.h"

#include <vector>

/**
 * \class Coda2EventDecoder
 * \ingroup QwAnalysis
 * \brief CODA version 2 event decoder implementation
 *
 * Concrete decoder for CODA 2.x format event streams, handling the legacy
 * data structures and bank formats. Maintains compatibility with older
 * data files while providing the same encoding/decoding interface.
 */
class Coda2EventDecoder : public VEventDecoder
{
public:
		Coda2EventDecoder() :
			fEvtClass(0),
			fStatSum(0),
			fIDBankNum(0) { }
		~Coda2EventDecoder() override { }
public:
	// Encoding Functions
	/** Create a PHYS event EVIO header.
	 *  @param ROCList List of ROC IDs (unused in CODA2 headers).
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
	 *  @param buffer    Output buffer (>= 5 words).
	 *  @param eventcount Number of events.
	 *  @param localtime  Event time.
	 */
	void EncodeGoEventHeader(int* buffer, int eventcount, int localtime) override;
	/** Create a PAUSE event EVIO header.
	 *  @param buffer    Output buffer (>= 5 words).
	 *  @param eventcount Number of events.
	 *  @param localtime  Event time.
	 */
	void EncodePauseEventHeader(int* buffer, int eventcount, int localtime) override;
	/** Create an END event EVIO header.
	 *  @param buffer    Output buffer (>= 5 words).
	 *  @param eventcount Number of events.
	 *  @param localtime  Event time.
	 */
	void EncodeEndEventHeader(int* buffer, int eventcount, int localtime) override;
public:
	// Decoding Functions
	/** Determine whether a buffer contains a PHYS, control, or other event.
	 *  @param buffer Event buffer to decode.
	 *  @return CODA_OK on success.
	 */
	Int_t DecodeEventIDBank(UInt_t *buffer) override;
	/** Print internal decoder state for diagnostics.
	 *  @param out Logging stream.
	 */
	void PrintDecoderInfo(QwLog& out) override;

private:
	// Event Information (CODA 2 Specific)
	UInt_t fEvtClass;
	UInt_t fStatSum;
	UInt_t fIDBankNum;
};
