#pragma once
#include <vector>
#include <mutex>

#include "QwEpicTypeTraits.h"
#include "QwObserver.h"

// Forward Declaration
template<typename T>
class Observer;

/**
 *@class Publisher
 *@brief Threadsafe publisher implementation for the Observer pattern.
 *       Virtual to allow for derived classes but implementaion is fixed
 *@details
 *       Based off of Tony Van Eerd's C++Now 2016 talk
 *       "Thread-Safe Observer Pattern -- You're Doing it Wrong"
 *       https://www.youtube.com/watch?v=RVvVQpIy6zc
 *       https://github.com/boostcon/cppnow_presentations_2016/tree/master/01_wednesday
 *
 *       Idea is that the thread managing the Publisher logic also executes the the
 *       observers Update function. This increases the chance of deadlock!
 *       * Critical: Observer::Update should not remove or add another observer (or self) during Notify
 *       * Critical: Observer::Update should minimize any additonal locks and limit the time the lock is held
 *
 *       Better Design:
 *       To resolve the two above issues, the observers would need to request the data from the publisher.
 *       Not sure how to implement with regards to the callback scheme implemented inside QwEpicChannel
 *
 *       Japan-MOLLER is single-threaded (EPICS Lib is not), so the above concerns are mitigated
 */
template<typename T>
class Publisher
{
	mutable std::recursive_mutex mut;
	std::vector<Observer<T>*> Observers;

protected:
	void Notify(T const& data) const;
public:
	Publisher() = default;
	virtual ~Publisher() = default;
	Publisher(Publisher const& other);
	void AddObserver(Observer<T>* observer);
	bool RemoveObserver(Observer<T>* observer);
	std::size_t GetObserverCount() const;
};

template<typename T>
Publisher<T>::Publisher(Publisher<T> const& other)
{
	std::lock_guard<std::recursive_mutex> lk(other.mut);
	Observers = other.Observers;
}


template<typename T>
class EPICSPublisher : public Publisher<T>
{
	static_assert(is_epic_supported<T>::value,
				"EPICSPublisher is limited only to EPICS Supported Types");

	using EpicsType = typename EpicsTypeTraits<T>::epics_type;
	using MonitorCallback = void(*)(event_handler_args);
};

template<typename T>
void Publisher<T>::AddObserver(Observer<T>* observer)
{
	std::lock_guard<std::recursive_mutex> lk(mut);
	Observers.push_back(observer);
}

template<typename T>
bool Publisher<T>::RemoveObserver(Observer<T>* observer)
{
	using std::begin, std::end;
	std::lock_guard<std::recursive_mutex> lk(mut);
	for( auto it = end(Observers); it != begin(Observers); ) {
		it--;
		if( *it == observer ) {
			Observers.erase(it);
			return true;
		}
	}
	return false;
}

template<typename T>
void Publisher<T>::Notify(T const& data) const
{
	std::lock_guard<std::recursive_mutex> lk(mut);
	for( auto observer : Observers ) {
		observer->Update(data);
	}
}
template<typename T>
std::size_t Publisher<T>::GetObserverCount() const
{
	std::lock_guard<std::recursive_mutex> lk(mut);
	return Observers.size();

}

