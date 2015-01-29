#pragma once

#include "StringUtil.h"

namespace QuantLib
{
	struct YieldCurveData;
}

class XMLOStream
{
public:
	XMLOStream( const std::string& name );
	void AddRecord( const TiXmlElement& elem );
	const TiXmlElement* GetResult() const { return &m_root; }
	std::string GetResultStr() const
	{
		std::stringstream paramStream;
		paramStream << m_root;
		return paramStream.str();
	}

private:
	TiXmlElement m_root;
};

class XMLIStream
{
public:
	XMLIStream( const TiXmlElement& root );
	const TiXmlElement* PopRecord();

private:
	TiXmlElement m_root;
	const TiXmlElement* m_iterator;
};

template<typename T>
XMLOStream& operator << ( XMLOStream& out, const T& val )
{
	TiXmlElement record( "record" );
	record.SetAttribute( "value", ::ToString( ::ToWString( val ) ) );
	out.AddRecord( record );

	return out;
}

XMLOStream& operator << ( XMLOStream& out, bool val );

template<typename T>
XMLIStream& operator >> ( XMLIStream& in, T& val )
{
	const  TiXmlElement* record( in.PopRecord() );
	val = boost::lexical_cast<T>( record->Attribute( "value" ) );

	return in;
}

XMLOStream& operator << ( XMLOStream& out, const Date& val );
XMLIStream& operator >> ( XMLIStream& in, Date& val );

XMLOStream& operator << ( XMLOStream& out, const std::vector<Real>& val );
XMLIStream& operator >> ( XMLIStream& in, std::vector<Real>& val );

XMLOStream& operator << ( XMLOStream& out, const std::vector<Date>& val );
XMLIStream& operator >> ( XMLIStream& in, std::vector<Date>& val );

XMLOStream& operator << ( XMLOStream& out, boost::shared_ptr<YieldCurveData> val );
XMLIStream& operator >> ( XMLIStream& in, boost::shared_ptr<YieldCurveData>& val );

