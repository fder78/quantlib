#include "StdAfx.h"

#include "XMLStream.h"

#include "yield_builder.hpp"
#include "ParamParseUtil.h"

#include "StringUtil.h"

#include "pricing_functions/hull_white_calibration.hpp"

XMLOStream::XMLOStream( const std::string& name )
	: m_root( name )
{
}

void XMLOStream::AddRecord( const TiXmlElement& elem )
{
	m_root.InsertEndChild( elem );
}

XMLIStream::XMLIStream( const TiXmlElement& root )
	: m_root( root )
	, m_iterator( m_root.FirstChildElement() )
{
}

const TiXmlElement* XMLIStream::PopRecord()
{
	const TiXmlElement* now = m_iterator;
	m_iterator = m_iterator->NextSiblingElement();
	return now;
}

inline void ConvertReal( Real val, OUT TiXmlElement& res )
{
	res.SetAttribute( "value", ::ToString( val ) );
}

inline Real ParseReal( const TiXmlElement& data )
{
	return boost::lexical_cast<Real>( ::ToWString( data.Attribute( "value" ) ) );
}

inline void ConvertRealVec( const std::vector<Real>& vec, OUT TiXmlElement& res )
{
	for each( Real v in vec )
	{
		TiXmlElement record( "record" );
		record.SetAttribute( "value", ::ToString( v ) );
		res.InsertEndChild( record );
	}
}

inline void ParseRealVec( const TiXmlElement& data, std::vector<Real>& vec )
{
	for( const TiXmlElement* iter = data.FirstChildElement(); iter != NULL; iter = iter->NextSiblingElement() )
	{
		vec.push_back( ::ParseReal( *iter ) );
	}
}

inline void ConvertDate( const Date& val, OUT TiXmlElement& res )
{
	res.SetAttribute( "value", ::ToString( ::ToWStringWTime( val ) ) );
}

inline Date ParseDate( const TiXmlElement& data )
{
	return ::ConvertToDateWTime( ::ToWString( data.Attribute( "value" ) ) );
}

inline void ConvertDateVec( const std::vector<Date>& vec, OUT TiXmlElement& res )
{
	for each( const Date& v in vec )
	{
		TiXmlElement record( "record" );
		record.SetAttribute( "value", ::ToString( ::ToWStringWTime( v ) ) );
		res.InsertEndChild( record );
	}
}

inline void ParseDateVec( const TiXmlElement& data, std::vector<Date>& vec )
{
	for( const TiXmlElement* iter = data.FirstChildElement(); iter != NULL; iter = iter->NextSiblingElement() )
	{
		vec.push_back( ::ParseDate( *iter ) );
	}
}

XMLOStream& operator<<( XMLOStream& out, const Date& val )
{
	TiXmlElement record( "record" );
	::ConvertDate( val, record );
	out.AddRecord( record );

	return out;
}

XMLIStream& operator>>( XMLIStream& in, Date& val )
{
	const TiXmlElement* record( in.PopRecord() );
	val = ::ParseDate( *record );

	return in;
}

XMLIStream& operator>>( XMLIStream& in, boost::shared_ptr<YieldCurveData>& val )
{
	const TiXmlElement* record( in.PopRecord() );
	const TiXmlElement* dates( record->FirstChildElement( "dates" ) );
	const TiXmlElement* yields( record->FirstChildElement( "yields" ) );

	val.reset( new YieldCurveData() );

	for( const TiXmlElement* iter = dates->FirstChildElement(); iter != NULL; iter = iter->NextSiblingElement() )
	{
		val->dates.push_back( ::ConvertToDateWTime( ::ToWString( iter->Attribute( "value" ) ) ) );
	}

	::ParseRealVec( *yields, val->yields );

	val->dc = Actual365Fixed();
	return in;
}

XMLOStream& operator<<( XMLOStream& out, boost::shared_ptr<YieldCurveData> val )
{
	TiXmlElement dates( "dates" );
	TiXmlElement yields( "yields" );

	for each( const Date& dt in val->dates )
	{
		TiXmlElement date( "record" );
		date.SetAttribute( "value", ::ToString( ::ToWStringWTime( dt ) ) );
		dates.InsertEndChild( date );
	}

	::ConvertRealVec( val->yields, yields );

	TiXmlElement record( "record" );
	record.InsertEndChild( dates );
	record.InsertEndChild( yields );

	out.AddRecord( record );

	return out;
}

XMLIStream& operator>>( XMLIStream& in, std::vector<Real>& val )
{
	::ParseRealVec( *in.PopRecord(), val );

	return in;
}

XMLOStream& operator<<( XMLOStream& out, const std::vector<Real>& val )
{
	TiXmlElement record( "record" );
	::ConvertRealVec( val, record );
	out.AddRecord( record );
	return out;
}

XMLIStream& operator >> ( XMLIStream& in, std::vector<Date>& val )
{
	::ParseDateVec( *in.PopRecord(), val );
	return in;
}

XMLOStream& operator << ( XMLOStream& out, const std::vector<Date>& val )
{
	TiXmlElement record( "record" );
	::ConvertDateVec( val, record );
	out.AddRecord( record );
	return out;	
}

XMLOStream& operator<<( XMLOStream& out, bool val )
{
	TiXmlElement record( "record" );
	if( val )
	{
		record.SetAttribute( "value", "1" );
	}
	else
	{
		record.SetAttribute( "value", "0" );
	}

	out.AddRecord( record );

	return out;
}
