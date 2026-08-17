#include "QwWords.h"
#include "QwRootFile.h"
#include <algorithm>

QwWords::QwWords(TString const& name)
: VQwSubsystem(name)
, VQwSubsystemParity(name) {}

QwWords::QwWords(QwWords const& other)
: VQwSubsystem(other)
, VQwSubsystemParity(other)
, fWords(other.fWords)
, fTreeArrayIndex(other.fTreeArrayIndex) {}

Int_t QwWords::LoadChannelMap(TString mapfile)
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
	}
	return 0;
}

VQwSubsystem&  QwWords::operator=  (VQwSubsystem *value) 
{
	if(Compare(value))
	{
		VQwSubsystem::operator=(value);
		QwWords* input = dynamic_cast<QwWords*>(value);
		fWords = input->fWords;
	}
	return *this;
}

VQwSubsystem&  QwWords::operator+= (VQwSubsystem *value)
{
	if(Compare(value)){
		QwWords* input = dynamic_cast<QwWords*>(value);
		std::transform(fWords.cbegin(), fWords.cend(),
					   input->fWords.cbegin(), fWords.begin(), 
					   std::plus<>{});
	}
	return *this;
}

VQwSubsystem&  QwWords::operator-= (VQwSubsystem *value)
{
	if(Compare(value)){
		QwWords* input = dynamic_cast<QwWords*>(value);
		std::transform(fWords.cbegin(), fWords.cend(),
					   input->fWords.cbegin(), fWords.begin(),
					   std::minus<>{});
	}
	return *this;
}


void  QwWords::ClearEventData()
{
	for(auto & word : fWords) word.ClearEventData();
}

Int_t QwWords::ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t *buffer, UInt_t num_words)
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

void QwWords::ProcessEvent()
{
	// Do post-processing here
	// By Default, we have none
	return;
}

Bool_t QwWords::ApplySingleEventCuts()
{
	// Apply cuts here
	// By Default, we have none
	return true;
}

UInt_t QwWords::GetEventcutErrorFlag()
{
	// Return errors here
	// By default, we have none
	return 0;
}
void QwWords::AccumulateRunningSum(VQwSubsystem* value, Int_t count, Int_t ErrorMask)
{
	// No-op
	return;
}
Bool_t QwWords::CheckForBurpFail(const VQwSubsystem *subsys)
{
	// Check for Burb failure
	// By default, we succeed
	return kFALSE;
}

void QwWords::DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask)
{
	// No-op
	return;
}

void QwWords::IncrementErrorCounters()
{
	// No-op
	return;
}
void QwWords::Scale(Double_t factor)
{
	// No-op
	return;
}

void QwWords::UpdateErrorFlag(const VQwSubsystem *ev_error)
{
	// No-op
	return;
}

void QwWords::Ratio(VQwSubsystem *numer, VQwSubsystem *denom)
{
	// No-op
	return;
}

void QwWords::CalculateRunningAverage()
{
	// No-op
	return;
}

void QwWords::PrintErrorCounters() const
{
	// No-op
	return;
}
void QwWords::ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values)
{
	TString basename;
	fTreeArrayIndex  = values.size();
	for (size_t i=0; i<fWords.size(); i++) {
		basename = prefix(0, (prefix.First("|") >= 0)? prefix.First("|"): prefix.Length());
		basename += fWords[i].fWordName;
		values.push_back(basename.Data(), 'I');
		tree->Branch(basename, &(values[fTreeArrayIndex + i]), values.LeafList(fTreeArrayIndex + i).c_str());
	}

}

void QwWords::FillTreeVector(QwRootTreeBranchVector &values) const 
{

	int index = fTreeArrayIndex;
	for (auto& word : fWords){
		values.SetValue(index++, word.fValue);
	}
}

#ifdef HAS_RNTUPLE_SUPPORT
void QwWords::ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs)
{
	TString basename;
	fTreeArrayIndex  = values.size();
	for (auto const& word : fWords) {
		basename = prefix(0, (prefix.First("|") >= 0)? prefix.First("|"): prefix.Length());
		basename += word.fWordName;
		values.push_back(0.0);
		fieldPtrs.push_back(model->MakeField<Double_t>(basename.Data()));
	}
}

void QwWords::FillNTupleVector(std::vector<Double_t>& values) const
{
  int index = fTreeArrayIndex;
  for (auto& word : fWords){
    values[index++] = word.fValue;
  }
}
#endif // HAS_RNTUPLE_SUPPORT
