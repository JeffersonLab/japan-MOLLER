#include "QwAtomicEpicTypes.h"
#include <algorithm>

AtomicEpicsType<std::string>::AtomicEpicsType(std::string_view sv, std::memory_order mem_order)
{
	data.reserve(MAX_STR_LENGTH);
	StoreImpl(sv);
}

void AtomicEpicsType<std::string>::Store(std::string_view sv, std::memory_order mem_order) {
	StoreImpl(sv);
}
std::string AtomicEpicsType<std::string>::Load(std::memory_order mem_order) const {
	std::lock_guard lk(mut);
	return data;
}
void AtomicEpicsType<std::string>::StoreImpl(std::string_view sv)
{
	auto slice = SliceImpl(sv);
	std::lock_guard lk(mut);
	data = slice;
}

std::string AtomicEpicsType<std::string>::Exchange(std::string_view desired, std::memory_order mem_order)
{
	auto slice = SliceImpl(desired);
	std::string new_str(slice);

	std::lock_guard lk(mut);
	std::swap(data, new_str);
	return new_str;
}

std::string_view AtomicEpicsType<std::string>::SliceImpl(std::string_view sv)
{
	std::size_t const size = std::min(sv.size(), MAX_STR_LENGTH-1);
	return sv.substr(0, size);
}
