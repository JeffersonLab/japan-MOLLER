#include "QwEpicHandler.h"
#include "QwFeedbackSetpoints.h"

void QwFeedbackSetpoint::Update(double const& data)
{
	fPrev = fCurr.Exchange(data, std::memory_order_acq_rel);
	std::string log_payload = "[Callback] Channel '" + fName + "' value changed: New ="
	                          + std::to_string(data) + ", Old = " + std::to_string(fPrev) + '\n';
	Notify(QwFeedbackLogPayload{QwLogLevel::kMessage, std::move(log_payload)}); 
}

void QwFeedbackSetpoint::Attach(const char* pv_name)
{
	if(fChannel) fChannel->StopMonitoring(this);
	fChannel = EpicHandler::Instance().ConnectChannel(pv_name);
	fChannel->StartMonitoring(this);
	fName = fChannel->GetName();
}

void QwFeedbackSetpoint::SetLimits(QwSetpointLimits const& limits) { fLimits = limits; }

QwFeedbackSetpoint::QwFeedbackSetpoint()
: fChannel(nullptr)
, fCorr{}
, fPrev{}
, fCurr{}
, fLimits{}
{}

bool QwFeedbackSetpoint::IsWithinLimits(double const input) const noexcept
{
	bool status = true;
	if(input > fLimits.upper_limit || input < fLimits.lower_limit )
		status = false;
	return status;
}

QwFeedbackSetpoint::QwFeedbackSetpoint(EpicChannel* channel)
: fChannel(channel)
, fCorr{}
, fPrev{}
, fCurr{}
, fLimits{}
{
	if(channel) {
		fChannel->StartMonitoring(this);
	}
}

QwFeedbackSetpoint::~QwFeedbackSetpoint()
{
	if(fChannel) {
		fChannel->StopMonitoring(this);
	}
}

QwFeedbackSetpoint::QwFeedbackSetpoint(QwFeedbackSetpoint const& other)
: fChannel(other.fChannel)
, fLimits(other.fLimits)
{
	double current = other.fCurr.Load(std::memory_order_acquire);
	fCorr = other.fCorr;
	fPrev = other.fPrev;
	fCurr.Store(current, std::memory_order_release);
}


