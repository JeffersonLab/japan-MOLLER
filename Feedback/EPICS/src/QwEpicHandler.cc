#include <cadef.h>
#include <string_view>
#include "QwEpicHandler.h"
#include "QwErrorHandling.h"
#include "QwLog.h"
std::string_view stringify(::channel_state const state)
{
	std::string_view sv;
	switch (state) {
		case cs_never_conn:
			sv = "STATE: IOC not found or unavailable.";
			break;
		case cs_prev_conn:
			sv = "STATE: IOC was found, but unavailable (previously connected to server).";
			break;
		case cs_conn:
			sv = "STATE: Valid chid, IOC was found, still available.";
			break;
		case cs_closed:
			sv = "STATE: Channel deleted by user.";
			break;
		default:
			sv = "STATE: Unknown IOC.";
			break;
	}
	return sv;
}
EpicHandler& EpicHandler::Instance()
{
	static EpicHandler singleton;
	return singleton;
}

EpicHandler::EpicHandler()
{
    int result = ::ca_context_create(::ca_enable_preemptive_callback);
	if( result != ECA_NORMAL ) {
		THROW_ERROR("CA Error while opening channel: " + std::string(::ca_message(result)));
	}
}

EpicHandler::~EpicHandler()
{
	// I need to delete the channels first before closing the context
	channels.clear();
	::ca_flush_io();
	ca_context_destroy();
}


EpicChannel* EpicHandler::ConnectChannel(char const* pv_name, capri priority)
{
	EpicChannel* ptr = FindChannel(pv_name);
	if(ptr == nullptr) {
		auto new_ch = std::make_unique<EpicChannel>(pv_name, priority);
		ptr    = new_ch.get();
		channels.push_back( std::move(new_ch) );
	}
	return ptr;
}

EpicChannel* EpicHandler::FindChannel(std::string_view target)
{
	using std::begin, std::end;
	auto it = std::find_if(begin(channels), end(channels), [target](auto const& chan) {
		return (target == chan->GetName());
	});
	return (it != end(channels)) ? it->get() : nullptr;
}

void EpicHandler::GetStatus(unsigned level)
{
	::ca_client_status(level);
}

void EpicHandler::CheckConnection()
{
	auto log = gQwLogger::Instance().Log(QwLogLevel::kMessage,  "Checking channel connections:\n");
	for( auto& ch : channels ) {
		auto name  = ch->GetName();
		auto state = ch->CheckConnection();
		log << name << " -- " << stringify(state) << '\n';
	}
}

