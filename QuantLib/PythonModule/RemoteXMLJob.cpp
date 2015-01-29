#include "StdAfx.h"

#include "RemoteXMLJob.h"
#include "PricingSetting.h"
#include "StringUtil.h"

#include "CalculationProxy.h"

RemoteXMLJob::RemoteXMLJob( const TiXmlElement& data, const TiXmlElement& curveRoot )
	: m_data( data )
	, m_curveRoot( curveRoot )
	, m_calcRoot( "root" )
	, m_event( ::CreateEvent( NULL, false, false, NULL ) )
	, m_terminateEvent( ::CreateEvent( NULL, TRUE, false, NULL ) )
{
	// avoiding 4355 warning
	m_code = ::ToString( static_cast<double>( reinterpret_cast<unsigned long>( this ) ) );

	TiXmlElement paramRoot( "param_root" );
	TiXmlElement dataRoot( "data_root" );
	TiXmlElement funcRoot( "func_root" );
	TiXmlElement evalTime( "eval_time" );
	TiXmlElement dataDateAlias( "data_date_alias" );

	dataRoot.InsertEndChild( m_data );

	evalTime.SetAttribute( "type", "date" );
	evalTime.SetAttribute( "value", ::ConvertToBloombergDate( PricingSetting::instance().GetEvaluationDate() ) );

	dataDateAlias.SetAttribute( "type", "double" );
	dataDateAlias.SetAttribute( "value", PricingSetting::instance().GetDataDateAlias() );

	paramRoot.InsertEndChild( dataDateAlias );
	paramRoot.InsertEndChild( evalTime );
	paramRoot.InsertEndChild( dataRoot );
	paramRoot.InsertEndChild( m_curveRoot );

	m_calcRoot.InsertEndChild( paramRoot );

	funcRoot.SetAttribute( "name", "RunPricing" );
	m_calcRoot.InsertEndChild( funcRoot );

	std::stringstream paramStream;
	paramStream << m_calcRoot;
	m_paramStr = paramStream.str();

	CalculationProxy::instance().AddRemoteJob( this );
}

void RemoteXMLJob::FetchResult()
{
	WaitForSingleObject( m_event, INFINITE );

	if( m_resStr.substr( 0, 2 ) == "OK" )
	{
		m_resStr = m_resStr.substr( 2 );

		TiXmlDocument outDoc;

		std::wstring ucs2Data = ::ToWString( m_resStr.c_str() );
		int n = ::WideCharToMultiByte( CP_UTF8, 0, ucs2Data.c_str(), -1, NULL, 0, NULL, NULL );
		boost::scoped_array<char> utf8Data( new char[ n + 1 ] );
		::WideCharToMultiByte( CP_UTF8, 0, ucs2Data.c_str(), -1, utf8Data.get(), n, NULL, NULL );
		utf8Data[ n ] = '\0';

		outDoc.Parse( utf8Data.get(), NULL, TIXML_ENCODING_UTF8 );
		m_result.reset( static_cast<TiXmlElement*>( outDoc.FirstChildElement()->Clone() ) );

		SetEvent( m_terminateEvent );
	}
	else
	{
		QL_ASSERT( false, m_resStr.c_str() );
	}
}

void RemoteXMLJob::SetResult( const std::string& resStr )
{
	m_resStr = resStr;
	SetEvent( m_event );
}

void RemoteXMLJob::WaitForTerminate()
{
	WaitForSingleObject( m_terminateEvent, INFINITE );
	__super::WaitForTerminate();
}
