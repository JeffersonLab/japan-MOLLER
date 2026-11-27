#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <string_view>
#include <cadef.h>
#include "QwEpicTypeTraits.h"


/**
 *@class AtomicEpicsType
 *@brief Wrapper around Epics C primitives and standard types and provides atomic operations
 */
template<typename T>
class AtomicEpicsType
{
	static_assert(std::is_trivially_copyable_v<T>,
		"Primary template must be trivially copyable.");
	static_assert(is_epic_supported<T>::value,
			"AtomicEpicsType is limited to EPICS Supported Types");
	using EpicsType = typename EpicsTypeTraits<T>::epics_type;
	std::atomic<T> data;
public:
	AtomicEpicsType(T const& input = T{});
	void Store(T const& input, std::memory_order mem_order = std::memory_order_seq_cst);
	T Load(std::memory_order mem_order = std::memory_order_seq_cst) const;
	T Exchange(T desired, std::memory_order mem_order = std::memory_order_seq_cst);
	bool CompareExchangeWeak(T& expected, T desired, std::memory_order success, std::memory_order failure);
	bool CompareExchangeWeak(T& expected, T desired, std::memory_order mem_order = std::memory_order_seq_cst);
};

/**
 *@class AtomicEpicsType<std::string>
 *@brief std::string atomic specialization
 *@details Uses std::mutex to provide thread safety. The EPICS primitve, dbr_string_t,
 *         is of type char[40], so interface uses std::string_view to allow for ease of use
 *         with any string-like object
 *         Interface is limited wrt AtomicEpicsType<T>
 */
template<>
class AtomicEpicsType<std::string>
{
	static constexpr std::size_t MAX_STR_LENGTH = EpicsTypeTraits<std::string>::MAX_STR_LENGTH;
	using EpicsType = typename EpicsTypeTraits<std::string>::epics_type;
	std::string data;
	mutable std::mutex  mut;

	std::string_view SliceImpl(std::string_view sv);
	void StoreImpl(std::string_view sv);
public:
	AtomicEpicsType(std::string_view sv = std::string_view{}, std::memory_order mem_order = std::memory_order_seq_cst);
	void Store(std::string_view sv, std::memory_order mem_order = std::memory_order_seq_cst);
	std::string Load(std::memory_order mem_order = std::memory_order_seq_cst) const;
	std::string Exchange(std::string_view desired, std::memory_order mem_order = std::memory_order_seq_cst);
};




template<typename T>
AtomicEpicsType<T>::AtomicEpicsType(T const& input) : data{input} {}


template<typename T>
void AtomicEpicsType<T>::Store(T const& val, std::memory_order mem_order) {
	data.store(val, mem_order);
}
template<typename T>
T AtomicEpicsType<T>::Load(std::memory_order mem_order) const {
	return data.load(mem_order);
}

template<typename T>
T AtomicEpicsType<T>::Exchange(T desired, std::memory_order mem_order)
{
	return data.exchange(desired, mem_order);
}

template<typename T>
bool AtomicEpicsType<T>::CompareExchangeWeak(T& expected, T desired, std::memory_order success, std::memory_order failure)
{
	return data.compare_exchange_weak(expected, desired, success, failure);
}

template<typename T>
bool AtomicEpicsType<T>::CompareExchangeWeak(T& expected, T desired, std::memory_order mem_order)
{
	return data.compare_exchange_weak(expected, desired, mem_order);
}

