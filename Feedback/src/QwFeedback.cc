#include "QwFeedback.h"
#include "QwFeedbackImpl.h"
#include "QwFeedbackConfig.h"

QwFeedback::QwFeedback()
: fPimpl(nullptr)
, fSlope{}
{}

QwFeedback::QwFeedback(QwFeedback const& other)
: fPimpl( other.fPimpl ? other.fPimpl->Clone() : nullptr )
, fSlope( other.fSlope ? other.fSlope->Clone() : nullptr )
, fLogFile{}
{
	
}

QwFeedback::TYPE QwFeedback::GetTypeOfFeedback(std::string_view sv) const
{
	auto fb_type = QwFeedback::TYPE::UNKNOWN;
	if(sv.find("PITA") != std::string_view::npos) {
		fb_type = TYPE::PITA;
	} else if(sv.find("APOSU") != std::string_view::npos) {
		fb_type = TYPE::APOSU;
	} else if(sv.find("APOSV") != std::string_view::npos) {
		fb_type = TYPE::APOSV;
	} else if(sv.find("IA") != std::string_view::npos) {
		fb_type = TYPE::IA;
	} else if(sv.find("POSXY") != std::string_view::npos) {
		fb_type = TYPE::POSXY;
	}
	return fb_type;
}

QwFeedback::TYPE QwFeedback::ConfigureFeedbackType(std::string_view sv)
{
	auto fb_type = GetTypeOfFeedback(sv);
	ConfigureFeedbackType(fb_type);
	return fb_type;
}
void QwFeedback::ConfigureFeedbackType(TYPE type)
{
	if(fPimpl) {
		QwWarning << "FeedbackType is already configured! Ignoring...\n";
		return;
	}
	switch(type) {
		case TYPE::PITA:
			fPimpl = std::make_unique<QwFeedbackImpl<QwPitaStrategy>>(fLogFile);
			fSlope = std::make_unique<QwIHWPSlope>(fLogFile);
			break;
		case TYPE::APOSU:
			fPimpl = std::make_unique<QwFeedbackImpl<QwAposUStrategy>>(fLogFile);
			fSlope = std::make_unique<QwIHWPSlope>(fLogFile);
			break;
		case TYPE::APOSV:
			fPimpl = std::make_unique<QwFeedbackImpl<QwAposVStrategy>>(fLogFile);
			fSlope = std::make_unique<QwIHWPSlope>(fLogFile);
			break;
		case TYPE::IA:
			fPimpl = std::make_unique<QwFeedbackImpl<QwIAStrategy>>(fLogFile);
			fSlope = std::make_unique<QwIHWPSlope>(fLogFile);
			break;
		case TYPE::POSXY:
			fPimpl = std::make_unique<QwFeedbackImpl<QwPosXYStrategy>>(fLogFile);
			fSlope = std::make_unique<QwPOSXYSlope>(fLogFile);
			break;
		default:
			break;
	}
	return;
}

bool QwFeedback::ConfigureLogger(std::string const& str, std::ios_base::openmode mode)
{
	fLogFile.SetSink(str);
	return static_cast<bool>(fLogFile);
}


void QwFeedback::ConfigureFeedback(QwFeedbackConfig const& config)
{
	switch(config.getType()) {
		case QwFeedbackConfig::TYPE::kTARGET:
			fDevices.emplace_back(config.getName(), config.getDescr());
			break;
		case QwFeedbackConfig::TYPE::kIOC:
			fPimpl->AddSetpoint(config.getName(), config.getLimits());
			break;
		default:
			break;
	}
}

void QwFeedback::ApplyCorrection(QwFeedbackCorrectionPayload const& payload)
{
	if(fPimpl && fSlope) fPimpl->ApplyCorrectionImpl(payload, fSlope.get());
}


std::vector<Device> const& QwFeedback::RequestTargetDevices() const { return fDevices; }

std::string_view QwFeedback::RequestFeedbackName() const
{
	return (fPimpl) ? fPimpl->GetName() : std::string_view{};
}
