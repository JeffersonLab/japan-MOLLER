#pragma once
#include <source_location>
#include <string>
#include <ostream>
#include <optional>
#include <set>
#include "VQwDataHandler.h"
#include "QwParameterFile.h"
#include "QwFeedback.h"
#include "QwErrorHandling.h"


class QwFeedbackHandler : public VQwDataHandler, public MQwDataHandlerCloneable<QwFeedbackHandler>
{
public:
	QwFeedbackHandler(TString const& name);
	QwFeedbackHandler(QwFeedbackHandler const& source);
    ~QwFeedbackHandler() {}
public:
	// All inherited functions
	void ParseConfigFile(QwParameterFile& file) override;
	Int_t ConnectChannels(QwSubsystemArrayParity& yield, QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override;
    // Subsystems with support for subsystem arrays should override this
    Int_t ConnectChannels(QwSubsystemArrayParity& /*detectors*/) override { THROW_ERROR("NOT SUPPORTED"); }
    void ProcessData() override;
    void UpdateBurstCounter(Short_t burstcounter) override { THROW_ERROR("NOT SUPPORTED"); }
    void FinishDataHandler() override;
    void ClearEventData() override;
    void AccumulateRunningSum(VQwDataHandler &value, Int_t count = 0, Int_t ErrorMask = 0xFFFFFFF) override { THROW_ERROR("NOT SUPPORTED"); }

    void ConstructTreeBranches( QwRootFile *treerootfile, const std::string& treeprefix = "", const std::string& branchprefix = "") override;
    void FillTreeBranches(QwRootFile *treerootfile) override;
    void ConstructNTupleFields( QwRootFile *treerootfile, const std::string& treeprefix = "", const std::string& branchprefix = "") override;
    void FillNTupleFields(QwRootFile *treerootfile) override;

    /// \brief Construct the histograms in a folder with a prefix
    void  ConstructHistograms(TDirectory * /*folder*/, TString & /*prefix*/) override;
    /// \brief Fill the histograms
    void  FillHistograms() override;

protected:
    Int_t LoadChannelMap(const std::string&) override;
    Int_t ConnectChannels(QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff) override { THROW_ERROR("NOT SUPPORTED"); }

private:
	/**
	 * @class DeviceMap
	 * @brief Encapsulates the VQwHardwareChannel ptr with accumulator + meta data
	 */
	class DeviceMap
	{

		/*@var fFbType Feedback Correction Type */
		FbCorrType fFbType;
		std::string fName;
		VQwHardwareChannel const* fDeviceObserver;
		mutable VQwHardwareChannel* fDeviceAccum;
		/*@var fScale Scale factor to convert to desired units */
		double fScale;
	public:
		DeviceMap(std::string name, FbCorrType fb, VQwHardwareChannel const* observ, VQwHardwareChannel* accum, double factor)
		: fName(std::move(name)), fFbType(fb), fDeviceObserver(observ), fDeviceAccum(accum), fScale(factor){}
		bool operator<(DeviceMap const& other) const {
			return static_cast<int>(fFbType) < static_cast<int>(other.fFbType);
		}
		std::string const& GetName() const { return fName; }
		void Accumulate() const {
			fDeviceAccum->AccumulateRunningSum(fDeviceObserver);
		}
		void CalculateAverage() const{
			fDeviceAccum->CalculateRunningAverage();
		}
		double GetValue() const {
			return fDeviceAccum->GetValue() * fScale;
		}
		double GetValueError() const {
			return fDeviceAccum->GetValueError() * fScale;
		}
		double GetValueWidth() const {
			return fDeviceAccum->GetValueWidth() * fScale;
		}
		FbCorrType GetFbType() const { return fFbType; }
		void ClearEventData() const {  fDeviceAccum->ClearEventData(); }
	};
	std::size_t fMaxPattern;
	std::size_t fPatternCounter;
	std::set<DeviceMap> fDeviceSet;
    QwFeedback fFeedback;
};

// Register this handler with the factory
REGISTER_DATA_HANDLER_FACTORY(QwFeedbackHandler);
