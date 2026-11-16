#pragma once
#include <limits>
template<typename T>
struct QwLimits
{
	T lower_limit{std::numeric_limits<T>::lowest()};
	T upper_limit{std::numeric_limits<T>::max()};
};

using QwSetpointLimits = QwLimits<double>;

