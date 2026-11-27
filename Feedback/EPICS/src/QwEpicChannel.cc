#include "QwEpicChannel.h"
#include "QwLog.h"


void EpicChannel::connection_callback(connection_handler_args arg) noexcept {
	std::string_view ch_name = ::ca_name(arg.chid);
	bool connected = (arg.op == CA_OP_CONN_UP);
	if(connected) {
		auto log = gQwLogger::Instance().Log(QwLogLevel::kMessage);
		log << "[Callback] Channel '" << ch_name << "' status changed: ";
		log << "Connected to IOC (Host: " << ::ca_host_name(arg.chid) <<")\n";
	} else {
		auto log = gQwLogger::Instance().Log(QwLogLevel::kWarning);
		log << "[Callback] Channel '" << ch_name << "' status changed: ";
		log << "Disconnected from IOC\n";
	}
    if (void* private_data = ::ca_puser(arg.chid)) {
        auto* instance = static_cast<EpicChannel*>(private_data);
        instance->is_conn.store(arg.op == CA_OP_CONN_UP);
		if( connected ) {
			instance->th_map.ForEach([](::chtype key, AsyncContextBase* ctx) {
				if(ctx) ctx->Start();
			});
		} else {
			instance->th_map.ForEach([](::chtype key, AsyncContextBase* ctx) {
				if(ctx) ctx->Stop();
			});
		}
    }
}

void EpicChannel::monitor_callback(event_handler_args arg) noexcept
{
	if( arg.status == ECA_NORMAL && arg.dbr != nullptr ) {
		auto const* data = static_cast<dbr_time_string const*>( arg.dbr );
		QwMessage << ">>> Live PV Update: " << data->value << '\n';
	}
}

void EpicChannel::async_get_callback(::event_handler_args arg)
{
	std::unique_ptr<AsyncContextBase> context(
		static_cast<AsyncContextBase*>(arg.usr));
	if(!context) return;
	context->Fulfill(arg.status, arg.type, arg.dbr);
}
void EpicChannel::async_put_callback(::event_handler_args arg)
{
	std::unique_ptr<AsyncContextBase> context(
		static_cast<AsyncContextBase*>(arg.usr));
	if(!context) return;
	context->Fulfill(arg.status, arg.type, arg.dbr);
}

EpicChannel::EpicChannel(char const* pv_name, ::capri priority)
: chan{nullptr}, is_conn{false}
{
	::ca_create_channel(pv_name, &EpicChannel::connection_callback, this, priority, &chan);
}

EpicChannel::~EpicChannel()
{
	if(chan) {
		th_map.Clear();
		::ca_clear_channel(chan);
	}
}


EpicChannel::EpicChannel(EpicChannel && other) noexcept
: chan(nullptr)
, is_conn(false)
{
	std::lock_guard<std::mutex> lk(other.io_mut);
	th_map = std::move(other.th_map);
	chan = std::exchange(other.chan, nullptr);
	is_conn.store(other.is_conn.exchange(false, std::memory_order_relaxed),
					std::memory_order_relaxed);
}


::channel_state EpicChannel::CheckConnection() { return ::ca_state(chan); }
std::string_view EpicChannel::GetName() { return ::ca_name(chan); }


[[nodiscard]]
::evid EpicChannel::StartMonitoring() noexcept
{
	return StartMonitoring(DBR_STRING, monitor_callback, nullptr);
}

[[nodiscard]]
::evid EpicChannel::StartMonitoring(::chtype dbr_type, MonitorCallback func, void* pArg) noexcept
{
	// We do not support arrays at this time
	constexpr int elem_size = 1;
	::evid monitor_id{nullptr};
	int result{ECA_NORMAL};
	{
		std::lock_guard<std::mutex> lk(io_mut);
		result = ca_create_subscription(dbr_type, elem_size, chan, DBE_VALUE, func, pArg, &monitor_id);
	}
	if(result != ECA_NORMAL) {
		std::cout << "Start Monitoring Error: " << ::ca_message(result) << '\n';
	}
	::ca_flush_io();
	return monitor_id;
}

void EpicChannel::StopMonitoring(::evid monitor_id) noexcept
{
	// No-op if monitor_id is not set
	if(monitor_id)
	{
		int result = ECA_NORMAL;
		{
			std::lock_guard<std::mutex> lk(io_mut);
			result = ca_clear_subscription(monitor_id);
		}
		if(result != ECA_NORMAL) {
			std::cout << "Stop Monitoring Error: " << ::ca_message(result) << '\n';
		}
		::ca_flush_io();
	}
}

