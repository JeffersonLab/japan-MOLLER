#pragma once


#include "VQwSubsystemParity.h"
#include "QwWord.h"
#include "QwFactory.h"
#include <vector>
#include <TString.h>

// Forward Declerations
class QwRootTreeBranchVector;


class QwScanData : public VQwSubsystemParity, public MQwSubsystemCloneable<QwScanData>
{
	struct CleanDataIndex
	{
		Int_t fIndex{-1};
		static char const* const fCleanWordName;
		bool operator==(CleanDataIndex const& other) const;
	};
	enum class CleanDataVal : bool { kUNCLEAN = false, kCLEAN   = true };
	std::vector<QwWord> fWords;
	CleanDataIndex fCleanDataIndex{};
	Int_t fTreeArrayIndex{-1};

public:
	QwScanData(TString const& name);
	QwScanData(QwScanData const& other);
	~QwScanData() = default;



	void FillDB_MPS(QwParityDB*, TString) override { throw std::runtime_error("QwScanData::FillDB_MPS() is not supported"); }
	void FillDB(QwParityDB*    , TString) override { throw std::runtime_error("QwScanData::FillDB() is not supported"    ); }
	void FillErrDB(QwParityDB* , TString) override { throw std::runtime_error("QwScanData::FillErrDB() is not supported");  }

	void Ratio(VQwSubsystem *numer, VQwSubsystem *denom) override;
	void Scale(Double_t factor) override;
	void AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
	void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF) override;
	void CalculateRunningAverage() override;
	using VQwSubsystemParity::LoadEventCuts;
	using VQwSubsystemParity::LoadEventCuts_Init;
	using VQwSubsystemParity::LoadEventCuts_Line;
	using VQwSubsystemParity::LoadEventCuts_Fin;

	VQwSubsystem&  operator=  (VQwSubsystem *value) override;
	VQwSubsystem&  operator+= (VQwSubsystem *value) override;
	VQwSubsystem&  operator-= (VQwSubsystem *value) override;


	Int_t LoadChannelMap(TString mapfile) override;
	/// Mandatory parameter file definition
	Int_t LoadInputParameters(TString mapfile) override { throw std::runtime_error("QwScanData::LoadInputParameters() is not supported"); return 0; }
	Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override { throw std::runtime_error("QwScanData::ProcessConfigurationBuffer() is not supported"); return 0; }
    Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t *buffer, UInt_t num_words) override;
	void ProcessEvent() override;
    void ConstructHistograms() override { std::runtime_error("QwScanData::ConstructHistograms() is not supported"); }
    void ConstructHistograms(TDirectory *folder, TString &prefix) override { std::runtime_error("QwScanData::ConstructHistograms() is not supported"); }
	void FillHistograms() override { std::runtime_error("QwScanData::FillHistograms() is not supported"); }

private:
	enum class RootSaveType { kSAVE_EVT, kSAVE_ASYM, kSAVE_YIELD, kNO_SAVE };
	void SetSaveType(TString const& prefix);
	RootSaveType fSaveType{RootSaveType::kNO_SAVE};

public:
    void ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values) override;
	void FillTreeVector(QwRootTreeBranchVector &values) const override;
	void ConstructBranch(TTree *tree, TString& prefix) override { throw std::runtime_error("QwScanData::ConstructBranch() is not supported"); };
	void ConstructBranch(TTree *tree, TString& prefix, QwParameterFile& trim_file) override { throw std::runtime_error("QwScanData::ConstructBranch() is not supported"); };

#ifdef HAS_RNTUPLE_SUPPORT
	void ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override;
	void FillNTupleVector(std::vector<Double_t>& values) const override;
#endif

	void  ClearEventData() override;
	Bool_t ApplySingleEventCuts() override;
	Bool_t CheckForBurpFail(const VQwSubsystem *subsys) override;
	void PrintErrorCounters() const override;
	void IncrementErrorCounters() override;
	UInt_t GetEventcutErrorFlag() override;
	void UpdateErrorFlag(const VQwSubsystem *ev_error) override;
private:
	// Clean Data Utility Functions
	bool CheckCleanDataIndex() const;
	void SetCleanDataIndex(Int_t index);
	bool SetCleanData(CleanDataVal clean_flag);
};
// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwScanData);
