#pragma once
#include <map>
#include <string>
#include <string_view>
#include <shared_mutex>
#include <mutex>
#include <optional>

template<typename Key, typename Value>
class QwThreadSafeMap
{
private:
	std::map<Key, Value> fMap;
	mutable std::shared_mutex fRW_mutex;
public:
	void InsertOrAssign(Key const& key, Value const& value);
	std::optional<Value> Get(Key const& key) const;
	bool Find(Key const& key) const;
	void Remove(Key const& key);
	void Clear();
	template<typename Callable>
	void ForEach(Callable&& callable);
	template<typename Callable>
	void ForEach(Callable&& callable) const;
public:
	QwThreadSafeMap() = default;
	~QwThreadSafeMap() = default;
	QwThreadSafeMap(QwThreadSafeMap const& other);
	QwThreadSafeMap(QwThreadSafeMap&& other) noexcept;
	QwThreadSafeMap& operator=(QwThreadSafeMap const& other);
	QwThreadSafeMap& operator=(QwThreadSafeMap&& other);
};

template<typename Key, typename Value>
void QwThreadSafeMap<Key, Value>::InsertOrAssign(Key const& key, Value const& value)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	fMap[key] = value;
}
template<typename Key, typename Value>
std::optional<Value> QwThreadSafeMap<Key, Value>::Get(Key const& key) const
{
	// Don't Call QwThreadSafeMap::Find as the dropping
	// and re-obtaining the lock introduces potential
	// race conditions
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	auto it = fMap.find(key);
	return (it == fMap.end()) ? std::nullopt : std::optional(it->second);
}
template<typename Key, typename Value>
bool QwThreadSafeMap<Key, Value>::Find(Key const& key) const
{
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	auto it = fMap.find(key);
	return (it != fMap.end());
}
template<typename Key, typename Value>
void QwThreadSafeMap<Key, Value>::Remove(Key const& key)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	fMap.erase(key);
}
template<typename Key, typename Value>
void QwThreadSafeMap<Key, Value>::Clear()
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	if constexpr (std::is_pointer_v<Value>) {
		for (auto const& [key, ptr] : fMap) {
			delete ptr;
		}
	}
	fMap.clear();
}

template<typename Key, typename Value> template<typename Callable>
void QwThreadSafeMap<Key, Value>::ForEach(Callable&& callable)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	for (auto& [key, value] : fMap) {
		callable(key, value);
	}
}

template<typename Key, typename Value> template<typename Callable>
void QwThreadSafeMap<Key, Value>::ForEach(Callable&& callable) const
{
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	for (auto const& [key, value] : fMap) {
		callable(key, value);
	}
}

template<typename Key, typename Value>
QwThreadSafeMap<Key, Value>::QwThreadSafeMap(QwThreadSafeMap<Key, Value> const& other)
{
	std::shared_lock<std::shared_mutex> lk(other.fRW_mutex);
	fMap = other.fMap;
}

template<typename Key, typename Value>
QwThreadSafeMap<Key, Value>::QwThreadSafeMap(QwThreadSafeMap&& other) noexcept
{
	std::unique_lock<std::shared_mutex> lk(other.fRW_mutex);
	fMap = std::move(other.fMap);
}

template<typename Key, typename Value>
QwThreadSafeMap<Key, Value>& QwThreadSafeMap<Key, Value>::operator=(QwThreadSafeMap const& other)
{
	if(this == &other) return *this;
	std::scoped_lock lk(fRW_mutex, other.fRW_mutex);
	fMap = other.fMap;
	return *this;
}
template<typename Key, typename Value>
QwThreadSafeMap<Key, Value>& QwThreadSafeMap<Key, Value>::operator=(QwThreadSafeMap&& other)
{
	if(this == &other) return *this;
	std::scoped_lock lk(fRW_mutex, other.fRW_mutex);
	fMap = std::move(other.fMap);
	return *this;
}

////////////////////////////////////////////////////////////////
//////////// Partial std::string specialization ////////////////
////////////////////////////////////////////////////////////////
template<typename Value>
class QwThreadSafeMap<std::string, Value>
{
private:
	std::map<std::string, Value, std::less<>> fMap;
	mutable std::shared_mutex fRW_mutex;
public:
	void InsertOrAssign(std::string_view key, Value const& value);
	std::optional<Value> Get(std::string_view key) const;
	bool Find(std::string_view key) const;
	void Remove(std::string_view key);
	void Clear();
	template<typename Callable>
	void ForEach(Callable&& callable);
	template<typename Callable>
	void ForEach(Callable&& callable) const;
public:
	QwThreadSafeMap() = default;
	~QwThreadSafeMap() = default;
	QwThreadSafeMap(QwThreadSafeMap const& other);
	QwThreadSafeMap(QwThreadSafeMap&& other) noexcept;
	QwThreadSafeMap& operator=(QwThreadSafeMap const& other);
	QwThreadSafeMap& operator=(QwThreadSafeMap&& other);
};

template<typename Value>
void QwThreadSafeMap<std::string, Value>::InsertOrAssign(std::string_view key, Value const& value)
{
	std::string tmp{key};
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	fMap.insert_or_assign(std::move(tmp), value);
}
template<typename Value>
std::optional<Value> QwThreadSafeMap<std::string, Value>::Get(std::string_view key) const
{
	// Don't Call QwThreadSafeMap::Find as the dropping
	// and re-obtaining the lock introduces potential
	// race conditions
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	auto it = fMap.find(key);
	return (it == fMap.end()) ? std::nullopt : std::optional(it->second);
}
template<typename Value>
bool QwThreadSafeMap<std::string, Value>::Find(std::string_view key) const
{
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	auto it = fMap.find(key);
	return (it != fMap.end());
}
template<typename Value>
void QwThreadSafeMap<std::string, Value>::Remove(std::string_view key)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	fMap.erase(key);
}
template<typename Value>
void QwThreadSafeMap<std::string, Value>::Clear()
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	if constexpr (std::is_pointer_v<Value>) {
		for (auto const& [key, ptr] : fMap) {
			delete ptr;
		}
	}
	fMap.clear();
}

template<typename Value> template<typename Callable>
void QwThreadSafeMap<std::string, Value>::ForEach(Callable&& callable)
{
	std::unique_lock<std::shared_mutex> lk(fRW_mutex);
	for (auto& [key, value] : fMap) {
		callable(key, value);
	}
}

template<typename Value> template<typename Callable>
void QwThreadSafeMap<std::string, Value>::ForEach(Callable&& callable) const
{
	std::shared_lock<std::shared_mutex> lk(fRW_mutex);
	for (auto const& [key, value] : fMap) {
		callable(key, value);
	}
}

template<typename Value>
QwThreadSafeMap<std::string, Value>::QwThreadSafeMap(QwThreadSafeMap const& other)
{
	std::shared_lock<std::shared_mutex> lk(other.fRW_mutex);
	fMap = other.fMap;
}

template<typename Value>
QwThreadSafeMap<std::string, Value>::QwThreadSafeMap(QwThreadSafeMap&& other) noexcept
{
	std::unique_lock<std::shared_mutex> lk(other.fRW_mutex);
	fMap = std::move(other.fMap);
}

template<typename Value>
QwThreadSafeMap<std::string, Value>& QwThreadSafeMap<std::string, Value>::operator=(QwThreadSafeMap const& other)
{
	if(this == &other) return *this;
	std::scoped_lock lk(fRW_mutex, other.fRW_mutex);
	fMap = other.fMap;
	return *this;
}
template<typename Value>
QwThreadSafeMap<std::string, Value>& QwThreadSafeMap<std::string, Value>::operator=(QwThreadSafeMap&& other)
{
	if(this == &other) return *this;
	std::scoped_lock lk(fRW_mutex, other.fRW_mutex);
	fMap = std::move(other.fMap);
	return *this;
}
