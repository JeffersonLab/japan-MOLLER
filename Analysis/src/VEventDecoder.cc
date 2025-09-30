#include "VEventDecoder.h"
#include "QwOptions.h"

/**
 * @brief Decodes the Subbank Header for PHYS Events
 * @param buffer Event buffer that contains the subbanks
 * @return okay
 */
Bool_t VEventDecoder::DecodeSubbankHeader(UInt_t *buffer){
        //  This function will decode the header information from
        //  either a ROC bank or a subbank.  It will also bump
        //  fWordsSoFar to be referring to the first word of
        //  the subbank's data.
        //
        //  NOTE TO DAQ PROGRAMMERS:
        //      All internal subbank tags MUST be defined to
        //      be greater than 31.
        Bool_t okay = kTRUE;
        if (fWordsSoFar >= fEvtLength){
                //  We have reached the end of this event.
                okay = kFALSE;
        } else if (fBankDataType == 0x10) {
                //  This bank has subbanks, so decode the subbank header.
                fFragLength   = buffer[0] - 1;  // This is the number of words in the data block
                fSubbankTag   = (buffer[1]&0xFFFF0000)>>16; // Bits 16-31
                fSubbankType  = (buffer[1]&0xFF00)>>8;      // Bits 8-15
                fSubbankNum   = (buffer[1]&0xFF);           // Bits 0-7

                QwDebug << "QwEventBuffer::DecodeSubbankHeader: "
                        << "fROC=="<<fROC << ", fSubbankTag==" << fSubbankTag
                        << ", fSubbankType=="<<fSubbankType << ", fSubbankNum==" <<fSubbankNum
                        << ", fAllowLowSubbankIDs==" << fAllowLowSubbankIDs
                        << QwLog::endl;

                if (fSubbankTag<=31
                        && ( (fAllowLowSubbankIDs==kFALSE)
                        || (fAllowLowSubbankIDs==kTRUE && fSubbankType==0x10) ) ){
                        //  Subbank tags between 0 and 31 indicate this is
                        //  a ROC bank.
                        fROC        = fSubbankTag;
                        fSubbankTag = 0;
                }
                if (fWordsSoFar+2+fFragLength > fEvtLength){
                        //  Trouble, because we'll have too many words!
                        QwError << "fWordsSoFar+2+fFragLength=="<<fWordsSoFar+2+fFragLength
                                        << " and fEvtLength==" << fEvtLength
                                        << QwLog::endl;
                        okay = kFALSE;
                }
                fWordsSoFar   += 2;
        }
        QwDebug << "QwEventBuffer::DecodeSubbankHeader: "
                        << "fROC=="<<fROC << ", fSubbankTag==" << fSubbankTag <<": "
                        <<  std::hex
                        << buffer[0] << " "
                        << buffer[1] << " "
                        << buffer[2] << " "
                        << buffer[3] << " "
                        << buffer[4] << std::dec << " "
                        << fWordsSoFar << " "<< fEvtLength
                        << QwLog::endl;
        //  There is no final else, because any bank type other than
        //  0x10 should just return okay.
        return okay;
}

/**
 * @brief Prints internal decoder information
 * @param out Output buffer to use to dispay internal Decoder Information.\n Can be QwMessage, QwVerbose, QwWarning, or QwErrror.
 * @return okay
 */
void VEventDecoder::PrintDecoderInfo(QwLog& out)
{
        out << "\n-------\n" << std::hex <<
                "fWordsSoFar " << fWordsSoFar <<
                "\n fEvtLength; " << fEvtLength <<
                "\n fEvtType " << fEvtType <<
                "\n fEvtTag " << fEvtTag <<
                "\n fBankDataType " << fBankDataType <<
                "\n fPhysicsEventFlag " << fPhysicsEventFlag <<
                "\n fEvtNumber;   " << fEvtNumber <<
                "\n fFragLength " << fFragLength <<
                "\n fSubbankTag " << fSubbankTag <<
                "\n fSubbankType " << fSubbankType <<
                "\n fSubbankNum " << fSubbankNum <<
                "\n fROC " << fROC <<
                "\n fAllowLowSubbankIDs " << fAllowLowSubbankIDs <<
                "\n-------\n" << std::dec <<
                QwLog::endl;
}
