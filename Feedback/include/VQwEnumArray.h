#pragma once
#include <memory>

/**
 * @class VQwEnumArray
 * @brief Interface for enum indexed arrays 
 * @todo: Does it make sense to have T& Get overloads?
 *        Providing only the accessors that copy reduces the
 *        interface needed to implement but is less efficient
 *        for non-trivial types
 */
template<typename T = double>
class VQwEnumArray
{
public:
	using ValueType = T;

	virtual ~VQwEnumArray() = default;
	virtual std::unique_ptr<VQwEnumArray> Clone() const = 0;
	template<typename IndexType>
	void Set(IndexType index, const T val);
	template<typename IndexType>
	T Get(IndexType index = IndexType{}) const;
	T Get(int index = 0) const;
private:
	virtual void    SetImpl(int index, const T val) = 0;
	virtual T  GetImpl(int index) const = 0;
};

template<typename T>
T VQwEnumArray<T>::Get(int index) const
{
	return GetImpl(index);
}

template<typename T>
template<typename IndexType>
void VQwEnumArray<T>::Set(IndexType index, const T val)
{
	SetImpl(static_cast<int>(index), val);
}

template<typename T>
template<typename IndexType>
T VQwEnumArray<T>::Get(IndexType index) const
{
	return GetImpl(static_cast<int>(index));
}

using VQwFeedbackSlopes = VQwEnumArray<double>;
using VQwFeedbackValues = VQwEnumArray<double>;
