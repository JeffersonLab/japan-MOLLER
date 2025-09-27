#ifndef CODA2EVENTDECODER_H
#define CODA2EVENTDECODER_H

#include "VEventDecoder.h"
#include "Rtypes.h"

#include <vector>

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
	std::vector<UInt_t> EncodePHYSEventHeader(std::vector<ROCID_t> &ROCList) override;
	void EncodePrestartEventHeader(int* buffer, int runnumber, int runtype, int localtime) override;
	void EncodeGoEventHeader(int* buffer, int eventcount, int localtime) override;
	void EncodePauseEventHeader(int* buffer, int eventcount, int localtime) override;
	void EncodeEndEventHeader(int* buffer, int eventcount, int localtime) override;
public:
	// Decoding Functions
	Int_t DecodeEventIDBank(UInt_t *buffer) override;
	void PrintDecoderInfo(QwLog& out) override;

private:
	// Event Information (CODA 2 Specific)
	UInt_t fEvtClass;
	UInt_t fStatSum;  
	UInt_t fIDBankNum;
};
#endif
