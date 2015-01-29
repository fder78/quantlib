#pragma once

#include "StringUtil.h"
#include "tinyxml.h"

// By Hyun Chul
class XMLValue
{
public:
	XMLValue( const TiXmlElement* record, const std::string& param )
		: m_record( record )
	{
		m_param = param;
		const TiXmlElement* firstElement = record->FirstChildElement( param );
		if( firstElement )
		{
			m_type = ::ToWString( firstElement->Attribute( "type" ) );
			m_value = ::ToWString( record->FirstChildElement( param )->Attribute( "value" ) );
		}
		else
		{
			m_type = L"empty";
			m_value = L"";
		}
	}

	std::wstring GetType() const { return m_type; }

	Date GetValueT( const Date& ) const
	{
		QL_ASSERT( m_type == L"date", m_param + "�� Ÿ���� date�� �ƴմϴ�." );
		return ::ConvertToDateFromBloomberg( m_value );
	}

	Real GetValueT( const Real& ) const
	{
		QL_ASSERT( m_type == L"double", m_param + "�� Ÿ���� double�� �ƴմϴ�." );
		try
		{
			return boost::lexical_cast<Real>( m_value );
		}
		catch (...)
		{
			QL_ASSERT( false, m_param + "���� lexical cast������ �߻��Ͽ����ϴ�." );
		}
	}

	int GetValueT( const int& ) const
	{
		QL_ASSERT( m_type == L"double", m_param + "�� Ÿ���� double�� �ƴմϴ�." );
		try
		{
			return boost::lexical_cast<int>( m_value );
		}
		catch (...)
		{
			QL_ASSERT( false, m_param + "���� lexical cast������ �߻��Ͽ����ϴ�." );
		}
	}

	bool GetValueT( const bool& ) const
	{
		QL_ASSERT( m_type == L"double", m_param + "�� Ÿ���� double�� �ƴմϴ�." );
		return boost::lexical_cast<double>( m_value ) != 0;
	}

	std::wstring GetValueT( const std::wstring& ) const
	{
		QL_ASSERT( m_type == L"string", m_param + "�� Ÿ���� string�� �ƴմϴ�." );
		return m_value;
	}

	const char* GetValueT( const char* ) const
	{
		return m_record->FirstChildElement( m_param )->Attribute( "value" );
	}

	template<typename T>
	T GetNullableValue() const
	{
		if( m_type == L"empty" || m_value == L"" )
		{
			return T();
		}

		return GetValueT( T() );
	}

	template<typename T>
	T GetValue() const
	{
		if( m_type == L"empty" || m_value == L"" )
		{
			return T();
		}

		return GetValueT( T() );
	}

	template<typename T>
	operator T() const
	{
		return GetValueT( T() );
	}

private:
	std::string m_param;
	std::wstring m_type;
	std::wstring m_value;
	const TiXmlElement* m_record;
};

// = �����ڿ� Str �Ͻ�����ȯ�� �� �ȵǾ ����
class XMLStrValue
{
public:
	XMLStrValue( const TiXmlElement* record, const std::string& param )
		: m_value( record, param )
	{
	}

	operator std::wstring() const
	{
		return GetValue();
	}

	std::wstring GetValue() const
	{
		return m_value.GetValueT( std::wstring() );
	}

private:
	XMLValue m_value;
};

inline bool operator == ( const XMLValue& lhs, const wchar_t* rhs )
{
	return lhs.GetValue<std::wstring>() == std::wstring( rhs );
}

inline bool operator != ( const XMLValue& lhs, const wchar_t* rhs )
{
	return lhs.GetValue<std::wstring>() != std::wstring( rhs );
}


template<typename T>
bool operator == ( const XMLValue& lhs, const T& rhs )
{
	return lhs.GetValue<T>() == rhs;
}

template<typename T>
bool operator != ( const XMLValue& lhs, const T& rhs )
{
	return lhs.GetValue<T>() != rhs;
}

template<typename T>
bool operator > ( const XMLValue& lhs, const T& rhs )
{
	return lhs.GetValue<T>() > rhs;
}

inline bool operator == ( const XMLStrValue& lhs, const std::wstring& rhs )
{
	return lhs.GetValue() == rhs;
}

inline bool operator != ( const XMLStrValue& lhs, const std::wstring& rhs )
{
	return lhs.GetValue() != rhs;
}

inline std::string ConvertDateFormat( const Date& bloombergFormat )
{
	return ::ToString( ::ToWString( bloombergFormat ) );
}
