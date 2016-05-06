#ifndef _KEY_MAX_MIN_H_
#define _KEY_MAX_MIN_H_

#include <limits>
template <typename T>
class KeyMaxMin
{
	public:
	static T max_value()
	{
		return std::numeric_limits<T>::max();
	}

	static T min_value()
	{
		return std::numeric_limits<T>::min();
	}
};

#endif
