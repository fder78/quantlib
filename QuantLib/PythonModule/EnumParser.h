#pragma once

#include "PerProcessSingleton.h"

template<typename Parser, typename EnumType>
class EnumParser : public PerProcessSingleton<Parser>
{
public:
	EnumParser() { static_cast<Parser*>( this )->BuildEnumMap(); }

	static EnumType ParseEnum( const std::wstring& enum_name, bool ignoreError = false, typename EnumType defaultVal = EnumType() )
	{
		EnumParser<Parser, EnumType>& parser = instance();
		std::wstring trimmedEnumName( enum_name );
		boost::algorithm::trim( trimmedEnumName );
		EnumMap::const_iterator iter = parser.m_enumMap.find( trimmedEnumName );
		if( iter == parser.m_enumMap.end() )
		{
			if( !ignoreError )
			{
				QL_ASSERT( false, ::ToString( enum_name ) + "(은/는) 처리가능한 종류가 아닙니다." );
			}

			return defaultVal;
		}

		return iter->second;
	}

	static std::wstring ToString( EnumType type )
	{
		EnumParser<Parser, EnumType>& parser = instance();
		StringMap::const_iterator iter = parser.m_stringMap.find( type );
		if( iter == parser.m_stringMap.end() )
		{
			assert( false );
			return std::wstring();
		}

		return iter->second;
	}

protected:
	void AddEnum( const std::wstring& enum_name, EnumType type )
	{
		m_enumMap.insert( std::make_pair( enum_name, type ) );
		m_stringMap.insert( std::make_pair( type, enum_name ) );
	}

private:
	typedef std::map<std::wstring, EnumType> EnumMap;
	typedef std::map<EnumType, std::wstring> StringMap;
	EnumMap m_enumMap;
	StringMap m_stringMap;
};
