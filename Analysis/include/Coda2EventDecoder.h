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
                ~Coda2EventDecoder() { }
public:
        // Encoding Functions
        virtual std::vector<UInt_t> EncodePHYSEventHeader(std::vector<ROCID_t> &ROCList);
        virtual void EncodePrestartEventHeader(int* buffer, int runnumber, int runtype, int localtime);
        virtual void EncodeGoEventHeader(int* buffer, int eventcount, int localtime);
        virtual void EncodePauseEventHeader(int* buffer, int eventcount, int localtime);
        virtual void EncodeEndEventHeader(int* buffer, int eventcount, int localtime);
public:
        // Decoding Functions
        virtual Int_t DecodeEventIDBank(UInt_t *buffer);
        virtual void PrintDecoderInfo(QwLog& out);

private:
        // Event Information (CODA 2 Specific)
        UInt_t fEvtClass;
        UInt_t fStatSum;
        UInt_t fIDBankNum;
};
#endif
