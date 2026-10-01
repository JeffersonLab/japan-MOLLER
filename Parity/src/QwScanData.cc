#include "QwScanData.h"
#include "QwRootFile.h"
#include <algorithm>

char const* const QwScanData::CleanDataIndex::fCleanWordName = "cleandata";
QwScanData::QwScanData(TString const& name)
: VQwSubsystem(name)
, VQwSubsystemParity(name) {}

QwScanData::QwScanData(QwScanData const& other)
: VQwSubsystem(other)
, VQwSubsystemParity(other)
, fWords(other.fWords)
, fCleanDataIndex(other.fCleanDataIndex)
, fTreeArrayIndex(other.fTreeArrayIndex)
, fSaveType(other.fSaveType) {}

Int_t QwScanData::LoadChannelMap(TString mapfile)
{
	// Open the Param file
	QwParameterFile mapstr(mapfile.Data());
  	mapstr.EnableGreediness();
  	mapstr.SetCommentChars("!");

  	fDetectorMaps.insert(mapstr.GetParamFileNameContents());

  	while (mapstr.ReadNextLine())  {
  		// This sets fCurrentROC_ID and fCurrentBank_ID
		RegisterRocBankMarker(mapstr);
		mapstr.TrimComment();       // Remove everything after a comment character.
		mapstr.TrimWhitespace();    // Get rid of leading and trailing whitespace
    	if (mapstr.LineIsEmpty())  continue;
		
		//  Break this line into tokens to process it.
		TString modtype = mapstr.GetTypedNextToken<TString>();
		UInt_t  modnum  = mapstr.GetTypedNextToken<UInt_t>();
		UInt_t  channum = mapstr.GetTypedNextToken<UInt_t>();
		TString name    = mapstr.GetTypedNextToken<TString>();
		modtype.ToUpper();

		if(modtype != "WORD") {
			QwError << "Unrecognized module type " << modtype << QwLog::endl;
			continue;
		}
		// Register data channel type
		Int_t subbank = GetSubbankIndex();
        QwVerbose << "Registering " << modtype << " " << name
                  << std::hex
                  << " in ROC 0x" << fCurrentROC_ID << ", bank 0x" << fCurrentBank_ID
                  << std::dec
                  << " at mod " << modnum << ", chan " << channum
                  << QwLog::endl;
		fWords.emplace_back( QwWord{subbank, 0, modtype, name, "", -1} );
		if(name == CleanDataIndex::fCleanWordName) {
			SetCleanDataIndex(fWords.size()-1);
		}
	}
	if(CheckCleanDataIndex() == false) {
		QwError << "fCleanDataIndex is not set inside QwScanData Subsystem" << QwLog::endl;
	}
	return 0;
}

VQwSubsystem&  QwScanData::operator=  (VQwSubsystem *value) 
{
	if(Compare(value))
	{
		VQwSubsystem::operator=(value);
		QwScanData* input = dynamic_cast<QwScanData*>(value);
		fWords = input->fWords;
	}
	return *this;
}

// Goal: If any of the scan data values changed during a pattern,
//       mark the whole pattern as not clean
VQwSubsystem&  QwScanData::operator+= (VQwSubsystem *value)
{
	if(Compare(value)){
		QwScanData* input = dynamic_cast<QwScanData*>(value);
		if( !std::equal(
				fWords.cbegin(),
				fWords.cend(),
				input->fWords.cbegin(),
				input->fWords.cend(),
				[](QwWord const& q1, QwWord const& q2){
					return q1.fValue == q2.fValue;
				})) {
			SetCleanData(CleanDataVal::kUNCLEAN);
		}
	}
	return *this;
}

// Goal: If any of the scan data values changed during a pattern,
//       mark the whole pattern as not clean
VQwSubsystem&  QwScanData::operator-= (VQwSubsystem *value)
{
	if(Compare(value)){
		QwScanData* input = dynamic_cast<QwScanData*>(value);
		if( !std::equal(
				fWords.cbegin(),
				fWords.cend(),
				input->fWords.cbegin(),
				input->fWords.cend(),
				[](QwWord const& q1, QwWord const& q2){
					return q1.fValue == q2.fValue;
				})) {
			SetCleanData(CleanDataVal::kUNCLEAN);
		}
	}
	return *this;
}


void  QwScanData::ClearEventData()
{
	for(auto & word : fWords) word.ClearEventData();
}

Int_t QwScanData::ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t *buffer, UInt_t num_words)
{
	UInt_t words_read = 0;

	// Get the subbank index (or -1 when no match)
	Int_t subbank = GetSubbankIndex(roc_id, bank_id);
	if (subbank >= 0 && num_words > 0) {
		words_read++;
		for(std::size_t i = 0; i < fWords.size(); i++)
			fWords[i].fValue = buffer[i];
		words_read = num_words;
	}

	return words_read;
}

void QwScanData::ProcessEvent()
{
	// Do post-processing here
	// By Default, we have none
	return;
}

Bool_t QwScanData::ApplySingleEventCuts()
{
	// Apply cuts here
	// By Default, we have none
	return true;
}

UInt_t QwScanData::GetEventcutErrorFlag()
{
	// Return errors here
	// By default, we have none
	return 0;
}
void QwScanData::AccumulateRunningSum(VQwSubsystem* value, Int_t count, Int_t ErrorMask)
{
	// No-op
	return;
}
Bool_t QwScanData::CheckForBurpFail(const VQwSubsystem *subsys)
{
	// Check for Burb failure
	// By default, we succeed
	return kFALSE;
}

void QwScanData::DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask)
{
	// No-op
	return;
}

void QwScanData::IncrementErrorCounters()
{
	// No-op
	return;
}
void QwScanData::Scale(Double_t factor)
{
	// No-op
	return;
}

void QwScanData::UpdateErrorFlag(const VQwSubsystem *ev_error)
{
	// No-op
	return;
}

void QwScanData::Ratio(VQwSubsystem *numer, VQwSubsystem *denom)
{
	// No-op
	return;
}

void QwScanData::CalculateRunningAverage()
{
	// No-op
	return;
}

void QwScanData::PrintErrorCounters() const
{
	// No-op
	return;
}

void QwScanData::SetSaveType(TString const& prefix)
{
	Ssiz_t len;
	if (TRegexp("diff_").Index(prefix,&len) == 0
			|| TRegexp("asym[1-9]+_").Index(prefix,&len) == 0
		    || TRegexp("yield_").Index(prefix,&len) == 0)
		fSaveType = RootSaveType::kNO_SAVE;
	else if (TRegexp("asym").Index(prefix,&len) == 0)
		fSaveType = RootSaveType::kSAVE_ASYM;
	else
		fSaveType = RootSaveType::kSAVE_EVT;
}
void QwScanData::ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values)
{
	SetSaveType(prefix);
	if(fSaveType == RootSaveType::kSAVE_EVT || fSaveType == RootSaveType::kSAVE_ASYM) {
		fTreeArrayIndex  = values.size();
		for (size_t i=0; i<fWords.size(); i++) {
			TString basename = fWords[i].fWordName;
			values.push_back(basename.Data(), 'I');
			tree->Branch(basename, &(values[fTreeArrayIndex + i]), values.LeafList(fTreeArrayIndex + i).c_str());
		}
	}
}

void QwScanData::FillTreeVector(QwRootTreeBranchVector &values) const 
{

	if(fSaveType == RootSaveType::kSAVE_EVT || fSaveType == RootSaveType::kSAVE_ASYM) {
		int index = fTreeArrayIndex;
		for (auto& word : fWords){
			values.SetValue(index++, word.fValue);
		}
	}
}

#ifdef HAS_RNTUPLE_SUPPORT
void QwScanData::ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs)
{
	SetSaveType(prefix);
	if(fSaveType == RootSaveType::kSAVE_EVT || fSaveType == RootSaveType::kSAVE_ASYM) {
		fTreeArrayIndex  = values.size();
		for (auto const& word : fWords) {
			TString basename = word.fWordName;
			values.push_back(0.0);
			fieldPtrs.push_back(model->MakeField<Double_t>(basename.Data()));
		}
	}
}

void QwScanData::FillNTupleVector(std::vector<Double_t>& values) const
{
	if(fSaveType == RootSaveType::kSAVE_EVT || fSaveType == RootSaveType::kSAVE_ASYM) {
		int index = fTreeArrayIndex;
		for (auto& word : fWords){
			values[index++] = word.fValue;
		}
	}
}
#endif // HAS_RNTUPLE_SUPPORT

bool QwScanData::CleanDataIndex::operator==(CleanDataIndex const& other) const
{
	return fIndex == other.fIndex;
}

/* \brief Sets Clean data word index
 * \param index -- index to set
 *
 * Sets the CleanData index. Will return if already set and print an error
 */
void QwScanData::SetCleanDataIndex(Int_t index)
{
	if(CheckCleanDataIndex() == false) {
		 fCleanDataIndex.fIndex= fWords.size()-1;
	} else {
		QwWarning << "ScanData Word already Set! " << '\n';
		QwWarning << "\tCurrent:  " << fCleanDataIndex.fIndex << '\n';
		QwWarning << "\tFound:  "   << index   << QwLog::endl;
	}

}
/*
 * \brief Checks to see if CleanDataIndex is set
 *
 * Returns true if set, false otherwise
 */
bool QwScanData::CheckCleanDataIndex() const
{
	return !(fCleanDataIndex == CleanDataIndex{});
}

/* \brief Sets Clean data word value
 * \param clean_flag -- clean data enum value {0, 1}
 *
 * Sets the CleanData value and returns true if CleanDataIndex is set.
 * Will return false if CleanDataIndex is not set
 */
bool QwScanData::SetCleanData(CleanDataVal clean_flag)
{
	bool status = false;
	if(CheckCleanDataIndex()) {
		fWords[fCleanDataIndex.fIndex].fValue = static_cast<int>(clean_flag);
		status = true;
	}
	return status;
}
