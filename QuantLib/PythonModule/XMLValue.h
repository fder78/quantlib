#pragma once

#include "stringUtil.h"
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
		QL_ASSERT( m_type == L"date", m_param + "의 타입이 date가 아닙니다." );
		return ::ConvertToDateFromBloomberg( m_value );
	}

	Real GetValueT( const Real& ) const
	{
		QL_ASSERT( m_type == L"double", m_param + "의 타입이 double이 아닙니다." );
		return boost::lexical_cast<Real>( m_value );
	}

	int GetValueT( const int& ) const
	{
		QL_ASSERT( m_type == L"double", m_param + "의 타입이 double이 아닙니다." );
		return boost::lexical_cast<int>( m_value );
	}

	bool GetValueT( const bool& ) const
	{
		QL_ASSERT( m_type == L"double", m_param + "의 타입이 double이 아닙니다." );
		return boost::lexical_cast<double>( m_value ) != 0;
	}

	std::wstring GetValueT( const std::wstring& ) const
	{
		QL_ASSERT( m_type == L"string", m_param + "의 타입이 string이 아닙니다." );
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

inline std::string ConvertDateFormat( const Date& bloombergFormat )
{
	return ::ToString( ::ToWString( bloombergFormat ) );
}