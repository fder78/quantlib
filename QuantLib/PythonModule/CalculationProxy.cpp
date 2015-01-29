#include "StdAfx.h"

#include "CalculationProxy.h"
#include "BloombergCaller.h"
#include "PricingSetting.h"

#include "StringUtil.h"

#include "IProductParam.h"
#include "RemoteXMLJob.h"

#include <xmlrpc-c/girerr.hpp>
#include <xmlrpc-c/client_simple.hpp>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ws2_32.lib")

void CalculationProxy::AddJob( boost::shared_ptr<IJob> job )
{
	m_jobs.push_back( job );
}

void CalculationProxy::FetchResult()
{
	for each( const JobVec::value_type& job in m_jobs )
	{
		job->CreateThread();
	}

	for each( const JobVec::value_type& job in m_jobs )
	{
		job->WaitForTerminate();
	}
}

void CalculationProxy::Init()
{
	m_jobs.clear();
	m_remoteJobs.clear();
}

void CalculationProxy::AddRemoteJob( RemoteXMLJob* job )
{
	m_remoteJobs.insert( std::make_pair( job->GetCode(), job ) );
}

void CalculationProxy::RemoteCalculate()
{
	int connErrCnt = 0;
	std::vector<std::vector<std::string> > paramListVec;
	paramListVec.reserve( m_remoteJobs.size() );
	for each( const RemoteJobMap::value_type& v in m_remoteJobs )
	{
		std::vector<std::string> strVec( 2 );
		strVec[ 0 ] = v.first;
		strVec[ 1 ] = v.second->GetParamStr();

		paramListVec.push_back( strVec );
	}
	
	xmlrpc_c::paramList paramList;
	paramList.addc( paramListVec );

	std::string const serverUrl( PricingSetting::instance().GetProxyAddress() );
	std::string const methodName("ficccalc");

	while( true )
	{
		xmlrpc_c::value result;
		try
		{
			xmlrpc_c::clientSimple myClient;
			myClient.call( serverUrl, methodName, paramList, &result );
			m_jobID = xmlrpc_c::value_int( result ).cvalue();
			break;
		} 
		catch (const std::exception& e)
		{
			std::string errStr( e.what() );
			if( errStr.find( "12002" ) != std::string::npos )
			{
				continue;
			}
			QL_ASSERT( false, errStr + "계산서버에 연결하는데 실패했습니다. 현철에게 계산서버를 켜라고 시켜주세요" );
		}
	}

	while( true )
	{
		std::string const serverUrl( PricingSetting::instance().GetProxyAddress() );
		std::string const methodName("getprogress");
		int progress;
		while( true )
		{
			xmlrpc_c::value result;
			try
			{
				xmlrpc_c::clientSimple myClient;
				myClient.call( serverUrl, methodName, "i", &result, m_jobID );
				progress = xmlrpc_c::value_int( result ).cvalue();
				break;
			} 
			catch (const std::exception& e)
			{
				std::string errStr( e.what() );
				if( errStr.find( "12002" ) != std::string::npos )
				{
					continue;
				}
				QL_ASSERT( false, errStr + "계산서버에 연결하는데 실패했습니다. 현철에게 계산서버를 켜라고 시켜주세요" );
			}
		}

		if( progress == m_remoteJobs.size() )
		{
			std::string const serverUrl( PricingSetting::instance().GetProxyAddress() );
			std::string const methodName("getresult");
			xmlrpc_c::value result;
			while( true )
			{
				try
				{
					xmlrpc_c::clientSimple myClient;
					size_t limit = ::xmlrpc_limit_get( XMLRPC_XML_SIZE_LIMIT_ID );

					::xmlrpc_limit_set( XMLRPC_XML_SIZE_LIMIT_ID, 256 * 1024 * 1024 );
					myClient.call( serverUrl, methodName, "i", &result, m_jobID );
					::xmlrpc_limit_set( XMLRPC_XML_SIZE_LIMIT_ID, limit );
					break;
				} 
				catch (const std::exception& e)
				{
					std::string errStr( e.what() );
					if( errStr.find( "12002" ) != std::string::npos )
					{
						continue;
					}
					QL_ASSERT( false, errStr + "계산서버에 연결하는데 실패했습니다. 현철에게 계산서버를 켜라고 시켜주세요" );
				}
			}

			xmlrpc_c::carray array = xmlrpc_c::value_array( result ).cvalue();
			for each( const xmlrpc_c::value& v in array )
			{
				xmlrpc_c::carray pair = xmlrpc_c::value_array( v ).cvalue();
				m_remoteJobs[ xmlrpc_c::value_string( pair[ 0 ] ).cvalue() ]->SetResult( xmlrpc_c::value_string( pair[ 1 ] ) );
			}

			return;
		}

		Sleep( 1000 );
	}
}
