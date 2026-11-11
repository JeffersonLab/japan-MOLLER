#include "QwFeedbackHandler.h"
#include <string_view>
#include <string>
#include <limits>
#include "QwFeedbackConfig.h"
#include "QwFeedbackImpl.h"

constexpr double PPM_UNIT = 1e6;
constexpr double  UM_UNIT = 1e3;
QwFeedbackHandler::QwFeedbackHandler(TString const& name)
: VQwDataHandler(name)
, fMaxPattern(std::numeric_limits<std::size_t>::max())
, fPatternCounter(0)
, fFeedback{}
{
	fKeepRunningSum = false;
}
QwFeedbackHandler::QwFeedbackHandler(QwFeedbackHandler const& source)
: VQwDataHandler(source)
, fMaxPattern(source.fMaxPattern)
, fPatternCounter(source.fPatternCounter)
, fDeviceSet(source.fDeviceSet)
, fFeedback(source.fFeedback)
{ }


void QwFeedbackHandler::ParseConfigFile(QwParameterFile& file)
{
	VQwDataHandler::ParseConfigFile(file);
	std::string feedback_type, logfile;

	// Instantiate the feedback type first before setting slopes
  	file.PopValue("impl", feedback_type); 
	auto feedback_type_enum = fFeedback.ConfigureFeedbackType(feedback_type);
	if(feedback_type_enum == QwFeedback::TYPE::UNKNOWN) {
		THROW_ERROR("Unsupported Feedback Type: " + std::string(feedback_type));
	}
	if(feedback_type_enum != QwFeedback::TYPE::POSXY) {
		double slope_in = 0, slope_out = 0;
		if(!file.PopValue("slope_ihwp_in", slope_in)) {
			THROW_ERROR("Missing Required Slope (slope_ihwp_in)!");
		}
		if(!file.PopValue("slope_ihwp_out", slope_out)) {
			THROW_ERROR("Missing Required Slope (slope_ihwp_out)!");
		}
		fFeedback.SetSlope(IHWP::kIN , slope_in );
		fFeedback.SetSlope(IHWP::kOUT, slope_out);
	} else {
		double slope_xu = 0, slope_xv = 0,
		       slope_yu = 0, slope_yv = 0;
		if(!file.PopValue("slope_xu", slope_xu)) {
			THROW_ERROR("Missing Required Slope (slope_xu)!");
		}
		if(!file.PopValue("slope_xv", slope_xv)) {
			THROW_ERROR("Missing Required Slope (slope_xv)!");
		}
		if(!file.PopValue("slope_yu", slope_yu)) {
			THROW_ERROR("Missing Required Slope (slope_yu)!");
		}
		if(!file.PopValue("slope_yv", slope_yv)) {
			THROW_ERROR("Missing Required Slope (slope_yv)!");
		}
		fFeedback.SetSlope(QwPOSXYSlope::POS_TYPE::kXU , slope_xu);
		fFeedback.SetSlope(QwPOSXYSlope::POS_TYPE::kXV , slope_xv);
		fFeedback.SetSlope(QwPOSXYSlope::POS_TYPE::kYU , slope_yu);
		fFeedback.SetSlope(QwPOSXYSlope::POS_TYPE::kYV , slope_yv);
	}

  	file.PopValue("patterns",fMaxPattern);
	file.PopValue("log", logfile);
	if( !fFeedback.ConfigureLogger(logfile) ) {
		QwWarning << '(' << logfile << ") Logfile Not Set for "
		          << feedback_type << " Feedback!" << QwLog::endl;
	}
	
}
Int_t QwFeedbackHandler::LoadChannelMap(std::string const& mapfile)
{
	// Open the file
	QwParameterFile map(mapfile);

	while (map.ReadNextLine()) {
		// Throw away comments, whitespace, empty lines
		map.TrimComment();
		map.TrimWhitespace();
		if (map.LineIsEmpty()) continue;
		std::string token = map.GetNextToken(" ");
		QwFeedbackConfig config;
		while ( !token.empty() ) {
			config.parse_pair(token);
			token = map.GetNextToken(" ");
		}
		if(!config.isValid()) {
			QwWarning << "Invalid Feedback Config:" << config << '\n';
			continue;
		}
		fFeedback.ConfigureFeedback( config );
	}

	return 0;
}

Int_t QwFeedbackHandler::ConnectChannels(QwSubsystemArrayParity& yield, QwSubsystemArrayParity& asym, QwSubsystemArrayParity& diff)
{
	for( auto& device : fFeedback.RequestTargetDevices() ) {
		auto const& device_name    = device.fName;
		auto const& device_type    = device.fQwType;
		auto const& device_fb_type = device.fFbType;
		double scale_factor = 1.0;
		VQwHardwareChannel const* observer{nullptr};
		QwMessage << fFeedback.RequestFeedbackName() << " is requesting device: " << device_name << '\n';
		switch (device_type) {
			case kHandleTypeYield:
				// Unsupported
				continue;	
				break;
			case kHandleTypeAsym:
				SetEventcutErrorFlagPointer(asym.GetEventcutErrorFlagPointer());
				observer = asym.ReturnInternalValue(TString(device_name));
				scale_factor = PPM_UNIT; 
				break;
			case kHandleTypeDiff:
				SetEventcutErrorFlagPointer(diff.GetEventcutErrorFlagPointer());
				observer = diff.ReturnInternalValue(TString(device_name));
				scale_factor = UM_UNIT;
				break;
			default:
				QwWarning << "Warning: QwFeedbackHander::ConnectChannels():\n"
				          << "Unknown data type (" << device_type << ") for device (" << device_name << ")"
				          << QwLog::endl;
				break;
		}

		if(observer == nullptr) {
			QwWarning << "Feedback Target Device (" 
				      << device_name
				      << ") Not Found";
			continue;
		} 
		auto const& [it, success] = fDeviceSet.emplace(DeviceMap{device_name, device_fb_type, observer, observer->Clone(VQwDataElement::kDerived), scale_factor});
		if(!success) {
			QwWarning << "Feedback Target Device (" 
				      << device_name
				      << ") Not included... Likely already set for FB Type: "
					  << stringify(device_fb_type) << QwLog::endl;
		}
	}
	


	return 0;
}

void QwFeedbackHandler::ProcessData()
{
	// Perform accumulation here
	// VQwDataHandler::AccumulateRunningSum is
	//    a) Not virtual and
	//    b) unergonomic (nested class structure)
	if(GetEventcutErrorFlag() == 0) {
		for( auto& device : fDeviceSet) {
			device.Accumulate();
		}
		fPatternCounter++;
	}
	if(fPatternCounter >= fMaxPattern) {
		// Apply correction
		QwOut << fFeedback.RequestFeedbackName() << " -- Pattern " << fPatternCounter << ":\n";
		QwFeedbackCorrectionPayload payload{};
		for( auto const& device : fDeviceSet) {
			device.CalculateAverage();
			payload.Set(device.GetFbType(), device.GetValue());
			QwOut << "\tDevice = "     <<  device.GetName()       <<   '\n';
			QwOut << "\tValue = "      <<  device.GetValue()      << " [ppm]\n";
			QwOut << "\tValueError = " << device.GetValueError() << " [ppm]\n";
			QwOut << "\tValueWidth = " << device.GetValueWidth() << " [ppm]\n";
		}
		fFeedback.ApplyCorrection(payload);
		fPatternCounter = 0;
	}
}
void QwFeedbackHandler::FinishDataHandler()
{
	ClearEventData();
}

void QwFeedbackHandler::ClearEventData()
{
	for( auto& device : fDeviceSet) {
			device.ClearEventData();
	}
}


void QwFeedbackHandler::ConstructTreeBranches( QwRootFile *treerootfile, const std::string& treeprefix, const std::string& branchprefix)
{
	// No-op
	return;
}
void QwFeedbackHandler::FillTreeBranches(QwRootFile *treerootfile)
{
	// No-op
	return;
}
void QwFeedbackHandler::ConstructNTupleFields( QwRootFile *treerootfile, const std::string& treeprefix, const std::string& branchprefix)
{
	// No-op
	return;
}
void QwFeedbackHandler::FillNTupleFields(QwRootFile *treerootfile)
{
	// No-op
	return;
}

void  QwFeedbackHandler::ConstructHistograms(TDirectory * /*folder*/, TString & /*prefix*/)
{
	// No-op
	return;
}
void  QwFeedbackHandler::FillHistograms()
{
	// No-op
	return;
}

