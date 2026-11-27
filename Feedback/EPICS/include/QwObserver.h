#pragma once
#include "QwEpicTypeTraits.h"
#include "QwAtomicEpicTypes.h"
#include <string_view>

/**
 *@class Observer
 *@brief Interface for Observer class (Observer pattern).
 */
template<typename T>
class Observer
{
public:
	virtual ~Observer() = default;
	virtual void Update(T const& data) = 0;
};


/**
 *@class EPICSObserver
 *@brief Interface for Observer class limited to EPICS
 *       supported types
 */
template<typename T>
class EPICSObserver : public Observer<T>
{
	static_assert(is_epic_supported<T>::value,
				"EPICSObserver is limited only to EPICS Supported Types");
	using EpicsType = typename EpicsTypeTraits<T>::epics_type;
	using MonitorCallback = void(*)(event_handler_args);
};

/**
 *@class ChannelObserver
 *@brief Example usage of Observer Pattern
 */
template<typename T>
class ChannelObserver : public EPICSObserver<T>
{
	using EpicsType = typename EpicsTypeTraits<T>::epics_type;
	AtomicEpicsType<T> fValue;
public:
	ChannelObserver();
	void Update(EpicsType const& data) override;
	T GetValue() const;
};

template<typename T>
ChannelObserver<T>::ChannelObserver()
: fValue{}
{ }

template<typename T>
void ChannelObserver<T>::Update(EpicsType const& data)
{
	fValue.Store(data);	
	// Do other things
}

template<typename T>
T ChannelObserver<T>::GetValue() const
{
	return fValue.Load();
}
