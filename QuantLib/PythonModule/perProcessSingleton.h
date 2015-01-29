#pragma once

// Singleton�� ��ӹ��� ���� ������ thread���� �ٸ����� ���� ��ü�� ���� �ϱ� �����̴�.
// Singleton�� ��ӹ����� thread���� �ٸ� ��ü�� �����.
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