#pragma once

// Singleton을 상속받지 않은 이유는 thread별로 다르더라도 같은 객체를 봐야 하기 때문이다.
// Singleton을 상속받으면 thread별로 다른 객체가 생긴다.
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