#pragma once
#include <string_view>
#include <atomic>
#include <mutex>
#include <memory>
#include <future>
#include <cadef.h>
#include <stdexcept>
#include "QwEpicTypeTraits.h"
#include "QwObserver.h"
#include "QwPublisher.h"
#include "QwThreadSafeMap.h"

/**
 *@class EpicChannel
 *@brief Encapsulates all the business logic for Epics channels.
 */
class EpicChannel
{
	/*@var chid chan opaque type (use as if void*) */
	chid chan;

	/*@var is_conn atomic connection flag. 
	 *      Is set inside connection_callback
	 */
	std::atomic<bool> is_conn;
	std::mutex io_mut;

	/*@brief Connection callback on conn. status change.
	 *@param [in] arg EPICS data passed into callback
	 *@details
	 *     see https://docs.epics-controls.org/projects/base/en/latest/cadef_h.html#_CPPv423connection_handler_args
	 */
	static void connection_callback(::connection_handler_args arg) noexcept;

	/*@brief Live Value Monitoring callback on value change
	 *@param [in] arg EPICS data passed into callback
	 *@details
	 *      Barebone callback used in StartMonitoring() overload;
	 *      Prints the status update as a string
	 */
	static void monitor_callback(::event_handler_args arg) noexcept;

	/*@brief Channel value callback on value change.
	 *@param [in] arg EPICS data passed into callback
	 */
	template<typename T>
	static void monitor_value_callback(event_handler_args arg) noexcept;

	/*@brief Async Read Callback
	 *@param [in] arg EPICS data passed into callback
	 */
	static void async_get_callback(::event_handler_args arg);

	/*@brief Async Write Callback
	 *@param [in] arg EPICS data passed into callback
	 */
	static void async_put_callback(::event_handler_args arg);
public:
	/**
	 *@brief EpicChannel Ctor
	 *@param [in] pv_name Process variable
	 *@param [in] priority level for EPICS dispatch
	 *@details Creates an EPICS Channel associated with the given PV and creates a
	 *         connection subscription (see connection_callback(connection_handler_args))
	 *
	 *         Currently the priority level has no effect.
	 *         See description for ca_create_channel
	 *         https://docs.epics-controls.org/projects/base/en/latest/cadef_h.html
	 */
	EpicChannel(char const* pv_name, ::capri priority);

	/**
	 *@brief EpicChannel Dtor
	 *@details Removes an monitor subscriptions and disconnects the channel
	 */
	~EpicChannel();
	EpicChannel(EpicChannel const&)            = delete;
	EpicChannel& operator=(EpicChannel const&) = delete;
	EpicChannel(EpicChannel && other) noexcept;
	EpicChannel& operator=(EpicChannel && other) = delete;
public:
	/**
	 *@brief Get the EpicChannel PV
	 *@return Name of the EpicChannel
	 */
	std::string_view GetName();

	/**
	 *@brief Get the current connection status
	 *@retval Connection status. See stringify(::channel_state const)
	 *        or https://docs.epics-controls.org/projects/base/en/latest/cadef_h.html#_CPPv413channel_state
	 *        for enumeration description
	 */
	::channel_state CheckConnection();

	/**
	 *@brief Monitor Callback function signature
	 */
	using MonitorCallback = void(*)(::event_handler_args);

	/**
	 *@brief Start monitoring a this channel with a user defined callback
	 *@param [in] dbr_type EPICS primitive C type
	 *@param [in] func User supplied callback function. See MonitorCallback for required signature
	 *@param [in] pArg Pointer to user args (accessible through event_handler_args)
	 *@return evid Subscrption ID, needed to stop subscription
	 *@todo Replace explicit use of ::chtype with EpicsTypeTraits (see QwEpicTypeTraits)
	 */
	[[nodiscard]]
	::evid StartMonitoring(::chtype dbr_type, MonitorCallback func, void* pArg) noexcept;

	/**
	 *@brief Start monitoring a this channel with default monitor callback
	 *@return evid Subscrption ID, needed to stop subscription
	 */
	[[nodiscard]]
	::evid StartMonitoring() noexcept;

	/**
	 *@brief Stop monitoring a this channel for a given subscription ID.
	 *       No op if monitor_id is invalid
	 *@param [in] monitor_id Subscrption ID
	 */
	void StopMonitoring(::evid monitor_id) noexcept;

	/**
	 *@brief Start monitoring the channel for value change using Observer Pattern
	 *@param [in] observer Observer type limited to EPICS supported types
	 */
	template <typename T>
	void StartMonitoring(EPICSObserver<T>* observer) noexcept;

	/**
	 *@brief Stop monitoring the channel for value change using Observer Pattern
	 *@param [in] observer Observer type limited to EPICS supported types
	 */
	template <typename T>
	void StopMonitoring(EPICSObserver<T>* observer) noexcept;

	/**
	 *@brief Primitive Async Read Method
	 *@tparam T Standard C++ type that has a corresponding EPICS type
	 *          (see QwEpicsTypeTraits)
	 *@return Result of async read or exception.
	 *details This is a primitive read method for standalone requests.
	 *        If doing a lot of repetitive requests, using the callback
	 *        feature is likely a better option
	 */
	template<typename T>
	std::future<T> GetAsync();

	/**
	 *@brief Primitive Async Write Method
	 *@tparam T Standard C++ type that has a corresponding EPICS type
	 *          (see QwEpicsTypeTraits)
	 *@return Request acknowlegement or exception.
	 */
	template<typename T>
	std::future<void> PutAsync(T&& value);
private:

	/**
	 *@class Async Context Interface
	 */
	class AsyncContextBase {
	public:
		virtual ~AsyncContextBase() = default;
		virtual void Fulfill(int status, chtype dbr_type, const void* dbr_ptr) = 0;
		//@brief Interface needed for PublisherContext
		virtual void Start(){};
		virtual void Stop(){};
	};

	/**
	 *@class Async Read Request Context
	 *brief Encapsulates the Async Get callback logic. Uses std::promise / future pair
	 */
	template<typename T>
	class GetRequestContext : public AsyncContextBase
	{
		std::promise<T> promise;
	public:
		void Fulfill(int status, chtype dbr_type, const void* dbr_ptr) override;
		void SetException(std::exception_ptr except);
		std::future<T> GetFuture();
	};

	/**
	 *@class Async Write Request Context
	 *brief Encapsulates the Async Put callback logic. Uses std::promise / future pair
	 */
	template<typename T>
	class PutRequestContext : public AsyncContextBase
	{
		std::promise<void> promise;
		T data;
	public:
		template<typename ...Args>
		PutRequestContext(Args&&... args);
		void Fulfill(int status, chtype dbr_type, const void* dbr_ptr) override;
		void SetException(std::exception_ptr except);
		std::future<void> GetFuture();
		void const* GetValuePtr() const noexcept;

	};

	/**
	 *@class Async Monitor Context
	 *@brief Encapsulates the Async Monitor logic using the Observer pattern
	 *@details Idea is to have a many-to-one pairing for each channel (one) mapped
	 *        to each epics supported type, ::chtype, (many). 
	 *        A single instantiation of this base class corresponds to a unique
	 *        ::chtype type (mapped via EpicTypeTraits)
	 */
	template<typename T>
	class PublisherContext : public AsyncContextBase, public EPICSPublisher<T>
	{
		// Publisher contains all the 'Subject' logic
		// Maybe this also should hold onto the ::evid monitor_id too
		EpicChannel* fChannel;
		std::mutex ctx_mut;
		::evid monitor_id;
	public:
		PublisherContext(EpicChannel* chan);
		~PublisherContext();
		PublisherContext(PublisherContext const& other) = delete;
		PublisherContext& operator=(PublisherContext const& other) = delete;
		PublisherContext(PublisherContext&& other) = delete;
		PublisherContext& operator=(PublisherContext&& other) = delete;
		// Loops over all of the Subscribed values and updates them
		void Fulfill(int status, ::chtype dbr_type, const void* dbr_ptr) override;
		// No Op if already monitoring
		void Start() override;
		void Stop() override;
	};
	/*
	 *@var map a unique PublisherContext (derived) to ::ch_type
	 *@details Lifetime of contents is tied to the lifetime of the th_map
	 *        any resources will not be released until th_map is destroyed.
	 *        see ~EpicChannel()
	 *        This avoid having to deal with deallocation in an aync context.
	 */
	QwThreadSafeMap<::chtype, AsyncContextBase*> th_map;

};
template <typename T>
void EpicChannel::monitor_value_callback(event_handler_args arg) noexcept
{
	auto* context( static_cast<PublisherContext<T>*>(arg.usr));
	if(!context) return;
	context->Fulfill(arg.status, arg.type, arg.dbr);
}


template<typename T>
std::future<T> EpicChannel::GetAsync()
{
	static_assert(is_epic_supported<T>::value,
			"EpicChannel::GetAsync is limited to EPICS Supported Types");
	constexpr ::chtype dbr_type = EpicsTypeTraits<T>::dbr_type;

	auto context = std::make_unique<GetRequestContext<T>>();
	auto future  = context->GetFuture();
	try {
		if(!is_conn.load()) {
			throw std::runtime_error("Async Read Error: Channel is disconnected.");
		}
		int result;
		{	
			std::lock_guard<std::mutex> lk(io_mut);
			result = ::ca_get_callback(dbr_type, chan, &EpicChannel::async_get_callback, context.get());
		}
		if(result != ECA_NORMAL) {
			throw std::runtime_error("Async Read Error: " + std::string(::ca_message(result)));
		}
		context.release();
		::ca_flush_io();
	} catch(...) {
		// Return the exception to the caller
		context->SetException(std::current_exception());
	}
	return future;
}

template<typename T>
std::future<void> EpicChannel::PutAsync(T&& value)
{
	using decay_type = std::decay_t<T>;
	static_assert(is_epic_supported<decay_type>::value,
			"EpicChannel::PutAsync is limited to EPICS Supported Types");
	constexpr ::chtype dbr_type = EpicsTypeTraits<decay_type>::dbr_type;

	auto context = std::make_unique<PutRequestContext<decay_type>>(std::forward<T>(value));
	auto future  = context->GetFuture();
	try {
		if(!is_conn.load()) {
			throw std::runtime_error("Async Write Error: Channel is disconnected.");
		}
		int result;
		{	
			std::lock_guard<std::mutex> lk(io_mut);
			result = ::ca_put_callback(dbr_type, chan, context->GetValuePtr(), &EpicChannel::async_put_callback, context.get());
		}
		if(result != ECA_NORMAL) {
			throw std::runtime_error("Async Write Error: " + std::string(::ca_message(result)));
		}
		context.release();
		::ca_flush_io();
	} catch(...) {
		// Return the exception to the caller
		context->SetException(std::current_exception());
	}
	return future;
}

template<typename T>
std::future<T> EpicChannel::GetRequestContext<T>::GetFuture() { return promise.get_future(); }

template<typename T>
void EpicChannel::GetRequestContext<T>::SetException(std::exception_ptr except) { promise.set_exception(except); }

template<typename T>
void EpicChannel::GetRequestContext<T>::Fulfill(int status, chtype dbr_type, const void* dbr_ptr)
{
	using Traits = EpicsTypeTraits<T>;
	using epics_type = typename Traits::epics_type;
	try {
		if(status != ECA_NORMAL) {
			throw std::runtime_error("Network Read Error: " + std::string(::ca_message(status)));
		}
		if(!dbr_ptr) {
			throw std::runtime_error("Network read error: payload is empty");
		}
		auto epics_data = static_cast<epics_type const*>( dbr_ptr );
		T data;
		Traits::from_epics(data, *epics_data);
		promise.set_value(data);
	} catch(...) {
		// Return the exception to the caller
		promise.set_exception(std::current_exception());
	}
}

template<typename T> template<typename ...Args>
EpicChannel::PutRequestContext<T>::PutRequestContext(Args&&... args)
: data(std::forward<Args>(args)...)
{ }
template<typename T>
std::future<void> EpicChannel::PutRequestContext<T>::GetFuture()
{
	return promise.get_future();
}
template<typename T>
void const* EpicChannel::PutRequestContext<T>::GetValuePtr() const noexcept
{
	return EpicsTypeTraits<T>::get_ptr(data);
}
template<typename T>
void EpicChannel::PutRequestContext<T>::Fulfill(int status, chtype dbr_type, const void* dbr_ptr)
{
	try {
		if(status != ECA_NORMAL) {
			throw std::runtime_error("Network Write Error: " + std::string(::ca_message(status)));
		}
		promise.set_value();
	} catch(...) {
		promise.set_exception(std::current_exception());
	}
}
template<typename T>
void EpicChannel::PutRequestContext<T>::SetException(std::exception_ptr except)
{
	promise.set_exception(except);
}

template <typename T>
void EpicChannel::StartMonitoring(EPICSObserver<T>* observer) noexcept
{
	constexpr auto key = EpicsTypeTraits<T>::dbr_type;
	if(!th_map.Find(key)) {
		th_map.InsertOrAssign(key, new PublisherContext<T>(this));
	}
	auto publisher_context = static_cast<PublisherContext<T>*>(*th_map.Get(key));
	publisher_context->AddObserver(observer);
	publisher_context->Start();
}
template <typename T>
void EpicChannel::StopMonitoring(EPICSObserver<T>* observer) noexcept
{
	constexpr auto key = EpicsTypeTraits<T>::dbr_type;
	if(th_map.Find(key)) {
		auto publisher_context = static_cast<PublisherContext<T>*>(*th_map.Get(key));
		publisher_context->RemoveObserver(observer);
		publisher_context->Stop();
	}
}
template <typename T>
EpicChannel::PublisherContext<T>::PublisherContext(EpicChannel* chan)
: fChannel(chan)
, monitor_id{nullptr}
{ }

template <typename T>
EpicChannel::PublisherContext<T>::~PublisherContext()
{
	::evid active_id{nullptr};
	{
		std::lock_guard<std::mutex> lk(ctx_mut);
		active_id = monitor_id;
		monitor_id = nullptr;
	}
	fChannel->StopMonitoring(active_id);
}

template <typename T>
void EpicChannel::PublisherContext<T>::Fulfill(int status, chtype dbr_type, const void* dbr_ptr)
{
	if(status == ECA_NORMAL && dbr_ptr ) {
		T const* data = static_cast<T const*>( dbr_ptr );
		this->Notify(*data);
	}
}

template <typename T>
void EpicChannel::PublisherContext<T>::Start()
{
	constexpr ::chtype dbr_type = EpicsTypeTraits<T>::dbr_type;
	if(!fChannel->is_conn.load()) { return; }
	std::lock_guard<std::mutex> lk(ctx_mut);
	if(monitor_id != nullptr) { return; }
	monitor_id = fChannel->StartMonitoring(EpicsTypeTraits<T>::dbr_type, monitor_value_callback<T>, this);
}

template <typename T>
void EpicChannel::PublisherContext<T>::Stop()
{
	std::lock_guard<std::mutex> lk(ctx_mut);
	if(this->GetObserverCount() > 0 ) return;
	if( monitor_id )  {
		fChannel->StopMonitoring(monitor_id);
		monitor_id = nullptr;
	}
}

