#include "QwIHWP.h"

const char* IHWP_IOC::IHWP_PV = "IGL1I00DI24_24M";

IHWP IHWP_IOC::GetState() const
{
	int value = fCurrState.Load(std::memory_order_acquire);
	return (value == static_cast<int>(IHWP::kIN)) ? IHWP::kIN : IHWP::kOUT;
}

void IHWP_IOC::Update(int const& data)
{
	auto prev = fCurrState.Exchange(data, std::memory_order_acq_rel);
	std::string log_payload = "IHWP STATE CHANGED: " + stringify(static_cast<IHWP>(prev))
	                      + " -> " + stringify(static_cast<IHWP>(data)) + '\n';
	Notify(QwFeedbackLogPayload{QwLogLevel::kMessage, std::move(log_payload)});
}


IHWP_IOC::IHWP_IOC()
: IHWP_IOC(EpicHandler::Instance().ConnectChannel(IHWP_IOC::IHWP_PV))
{ }

IHWP_IOC::IHWP_IOC(EpicChannel* channel)
: fChannel{channel}
, fCurrState{0}
{
	fChannel->StartMonitoring(this);
}
IHWP_IOC::~IHWP_IOC()
{
	fChannel->StopMonitoring(this);
}

IHWP_IOC::IHWP_IOC(IHWP_IOC const& other)
: fChannel{other.fChannel}
, fCurrState{0}
{
	fCurrState.Store(other.fCurrState.Load(std::memory_order_relaxed),
						std::memory_order_relaxed);
	fChannel->StartMonitoring(this);
}

std::string stringify(IHWP ihwp)
{
	return (ihwp == IHWP::kIN) ? "IN" : "OUT";
}
