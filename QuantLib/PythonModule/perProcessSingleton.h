#pragma once
#include <boost/noncopyable.hpp>

template<typename T>
class PerProcessSingleton : private boost::noncopyable
{
public:
	PerProcessSingleton()
	{
		ms_instance = static_cast<const T*>( this );
	}

	static T& instance()
	{
		static T instance;
		return instance;
	}

private:
	static const T* ms_instance;
};

template<typename T>
const T* PerProcessSingleton<T>::ms_instance = NULL;