#include "StdAfx.h"

#include "BloombergCaller.h"

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/girerr.hpp>
#include <xmlrpc-c/client_simple.hpp>

#include "StringUtil.h"
#include "PricingSetting.h"

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ws2_32.lib")

xmlrpc_c::value __Blp( const std::wstring& code, const std::wstring& field )
{
	xmlrpc_c::value result;
	try {
		std::string const serverUrl("http://128.1.106.203:5500");
		std::string const methodName("blp");

		xmlrpc_c::clientSimple myClient;
		myClient.call(serverUrl, methodName, "ss", &result, ::ToString( code ).c_str(), ::ToString( field ).c_str() );

		return result;
	} 
	catch (...)
	{
		QL_ASSERT( false, "Bloomberg 서버에 연결하는데 실패했습니다. 현철에게 Bloomberg 서버를 켜라고 시켜주세요" );
	}

	return result;
}

xmlrpc_c::value __Blph( const std::wstring& code, const std::wstring& field, const Date& dt )
{
	xmlrpc_c::value result;

	Date date( dt );

	try {
		std::string const serverUrl("http://128.1.106.203:5500");
		std::string const methodName("blph");

		xmlrpc_c::clientSimple myClient;

		myClient.call(serverUrl, methodName, "ssiii", &result, ::ToString( code ).c_str(), ::ToString( field ).c_str(), date.year(), date.month(), date.dayOfMonth() );

		return result;
	} 
	catch (...)
	{
		QL_ASSERT( false, "Bloomberg 서버에 연결하는데 실패했습니다. 현철에게 Bloomberg 서버를 켜라고 시켜주세요" );
	}

	return result;
}

std::wstring ConvertToBloombergCode( const std::wstring& code )
{
	if( code == L"U180" )
	{
		return L"KOSPI2 INDEX";
	}
	if( code == L"HSCEI" )
	{
		return L"HSCEI INDEX";
	}
	if( code == L"SPX" )
	{
		return L"SPX INDEX";
	}
	if( code == L"TWSE" )
	{
		return L"TWSE INDEX";
	}
	if( code == L"SX5E" )
	{
		return L"SX5E INDEX";
	}

	// 국내주식이면
	if( code.substr( 0, 1 ) == L"A" )
	{
		return code.substr( 1, std::wstring::npos ) + L" KS Equity";
	}

	return code;
}

struct BlpQueryKey
{
	BlpQueryKey( 	std::wstring code,
	std::wstring field,
	Date startDate,
	Date endDate,
	std::wstring periodUnit,
	int numData,
	std::wstring activeDateOption )
	: code( code )
	, field( field )
	, startDate( startDate )
	, endDate( endDate )
	, periodUnit( periodUnit )
	, numData( numData )
	, activeDateOption( activeDateOption )
	{ }

	std::wstring code;
	std::wstring field;
	Date startDate;
	Date endDate;
	std::wstring periodUnit;
	int numData;
	std::wstring activeDateOption;
};

bool operator < ( const BlpQueryKey& lhs, const BlpQueryKey& rhs )
{
	return std::make_pair( std::make_pair( std::make_pair( std::make_pair( std::make_pair( std::make_pair( lhs.code, lhs.field ), lhs.startDate ), lhs.endDate ), lhs.periodUnit ), lhs.numData ), lhs.activeDateOption )
		   < std::make_pair( std::make_pair( std::make_pair( std::make_pair( std::make_pair( std::make_pair( rhs.code, rhs.field ), rhs.startDate ), rhs.endDate ), rhs.periodUnit ), rhs.numData ), rhs.activeDateOption );
}

std::vector<DayPrice> Blphvec( const std::wstring& code, const std::wstring& field, const Date& startDate, const Date& ed, const std::wstring& periodUnit, int numData, const std::wstring& activeDateOption )
{
	// 막날이 평가시점과 같으면서 평가시간이 장열리기 전이면 전날로 세팅
	Date endDate( ed );

	if( periodUnit == L"WEEKLY" )
	{
		endDate -= 7;
		endDate += ( Friday - endDate.weekday() + 7 ) % 7;
	}

	BlpQueryKey queryKey( code, field, startDate, endDate, periodUnit, numData, activeDateOption );
	typedef std::map<BlpQueryKey, boost::shared_ptr<DayPriceVec> > HistoricalDataMap;
	static HistoricalDataMap s_historicaDataMap;
	HistoricalDataMap::iterator iter = s_historicaDataMap.find( queryKey );

	if( iter != s_historicaDataMap.end()	)
	{
		return *iter->second;
	}

	try {
		boost::shared_ptr<DayPriceVec> resVec( new DayPriceVec() );

		xmlrpc_c::value result;
		std::string const serverUrl("http://128.1.106.203:5500");
		std::string const methodName("blph2");

		xmlrpc_c::clientSimple myClient;

		myClient.call( serverUrl, methodName, "ssiiiiiisis", &result, ::ToString( code ).c_str(), ::ToString( field ).c_str(), startDate.year(), startDate.month(), startDate.dayOfMonth(), endDate.year(), endDate.month(), endDate.dayOfMonth(), ::ToString( periodUnit ).c_str(), numData, ::ToString( activeDateOption ).c_str() );

		xmlrpc_c::carray ary = xmlrpc_c::value_array( result ).cvalue();
		resVec->reserve( ary.size() );

		for each( const xmlrpc_c::value& v in ary )
		{
			xmlrpc_c::carray vary = xmlrpc_c::value_array( v ).cvalue();
			resVec->push_back( std::make_pair( ::ConvertToDate( ::ToWString( xmlrpc_c::value_string( vary[ 0 ] ).cvalue() ) ).serialNumber(), xmlrpc_c::value_double( vary[ 1 ] ) ) );
		}

		std::reverse( resVec->begin(), resVec->end() );

		s_historicaDataMap.insert( std::make_pair( queryKey, resVec ) );
		return *resVec;
	} 
	catch (...)
	{
		QL_ASSERT( false, "Bloomberg 서버에 연결하는데 실패했습니다. 현철에게 Bloomberg 서버를 켜라고 시켜주세요" );
	}

	return DayPriceVec();
}



inline void CalcLogReturnRate( const std::vector<DayPrice>& data, OUT std::vector<DayPrice>& res )
{
	res.resize( data.size() - 1 );

	for( size_t i = 1; i < data.size(); i++ )
	{
		// i번째 날짜에 i번째 날의 증가율을 넣는다.
		res[ i - 1 ].first = data[ i ].first;
		res[ i - 1 ].second = log( data[ i - 1 ].second / data[ i ].second );
	}
}

double GetHistoricalVolWithBloomberg( const std::wstring& code, int ndata )
{
	std::vector<DayPrice> data;

	data = ::Blphvec( ::ConvertToBloombergCode( code ), L"last price", PricingSetting::instance().GetEvaluationDate() - 10 * Years, PricingSetting::instance().GetEvaluationDate(), L"DAILY", ndata, L"ACTIVE_DAYS_ONLY" );

	if( data.size() < 2 )
	{
		return -1.;
	}

	std::vector<DayPrice> logReturn;
	::CalcLogReturnRate( data, logReturn );

	if( ndata - 1 < int( logReturn.size() ) )
	{
		logReturn.resize( ndata - 1 );
	}

	Real sum = 0.;
	for each( const DayPrice& pr in logReturn )
	{
		sum += pr.second;
	}

	Real mean = sum / static_cast<Real>( logReturn.size() );
	Real var = 0.;

	for each( const DayPrice& pr in logReturn )
	{
		var += pow( pr.second - mean, 2. );
	}

	var /= static_cast<Real>( logReturn.size() - 1 ) / 252.;

	return sqrt( var );
}

double GetHistoricalCorrWithBloomberg( const std::wstring& code1, const std::wstring& code2, const std::wstring& periodUnit, int ndata, const std::wstring& activeDateOption )
{
	std::vector<DayPrice> data1, data2;

	data1 = ::Blphvec( ::ConvertToBloombergCode( code1 ), L"last price", PricingSetting::instance().GetEvaluationDate() - 10 * Years, PricingSetting::instance().GetEvaluationDate(), periodUnit, ndata, activeDateOption );
	data2 = ::Blphvec( ::ConvertToBloombergCode( code2 ), L"last price", PricingSetting::instance().GetEvaluationDate() - 10 * Years, PricingSetting::instance().GetEvaluationDate(), periodUnit, ndata, activeDateOption );

	if( data1.empty() || data2.empty() )
	{
		return -1.;
	}

	typedef std::vector<std::pair<Real, Real> > CorrDataVec;
	CorrDataVec resData;
	resData.reserve( std::min( data1.size(), data2.size() ) );
	for( DayPriceVec::iterator iter1 = data1.begin(), iter2 = data2.begin(); iter1 != data1.end() && iter2 != data2.end(); )
	{
		if( iter1->first == iter2->first )
		{
			resData.push_back( std::make_pair( iter1->second, iter2->second ) );
			++iter1;
			++iter2;
		}
		else
		{
			if( iter1->first > iter2->first )
			{
				++iter1;
			}
			else
			{
				++iter2;
			}
		}
	}

	if( resData.size() > static_cast<unsigned int>( ndata ) )
	{
		resData.resize( ndata );
	}

	for( size_t i = 0; i < resData.size() - 1; i++ )
	{
		resData[ i ].first = log( resData[ i ].first / resData[ i + 1 ].first );
		resData[ i ].second = log( resData[ i ].second / resData[ i + 1 ].second );
	}

	resData.resize( resData.size() - 1 );

	double mean1 = 0., mean2 = 0.;
	for each( const CorrDataVec::value_type& v in resData )
	{
		mean1 += v.first;
		mean2 += v.second;
	}

	mean1 /= (double)resData.size();
	mean2 /= (double)resData.size();

	Real var1 = 0., var2 = 0., covar = 0.;
	for each( const CorrDataVec::value_type& v in resData )
	{
		var1 += pow( v.first - mean1, 2. );
		var2 += pow( v.second - mean2, 2. );
		covar += ( v.first - mean1 ) * ( v.second - mean2 );
	}

	var1 /= ( (double)resData.size() - 1. );
	var2 /= ( (double)resData.size() - 1. );
	covar /= ( (double)resData.size() - 1. );

	return covar / sqrt( var1 ) / sqrt( var2 );
}
