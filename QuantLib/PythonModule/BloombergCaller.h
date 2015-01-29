#pragma once

#include <xmlrpc-c/base.hpp>

xmlrpc_c::value __Blp( const std::wstring& code, const std::wstring& field );
xmlrpc_c::value __Blph( const std::wstring& code, const std::wstring& field, const Date& date );

template<typename T>
T Blp( const std::wstring& code, const std::wstring& field )
{
	return T( __Blp( code, field ) );
}

template<typename T>
T Blph( const std::wstring& code, const std::wstring& field, const Date& date )
{
	// 실제 현재시간보다 미래가격을 불러와야 하면 현재가격을 줌
	if( date > Date::todaysDate() )
	{
		return T( __Blp( code, field ) );
	}

	return T( __Blph( code, field, date ) );
}

std::wstring ConvertToBloombergCode( const std::wstring& code );

typedef std::pair<BigInteger, Real> DayPrice;
typedef std::vector<DayPrice> DayPriceVec;
DayPriceVec Blphvec( const std::wstring& code, const std::wstring& field, const Date& startDate, const Date& endDate, const std::wstring& periodUnit, int numData, const std::wstring& activeDateOption );

double GetHistoricalVolWithBloomberg( const std::wstring& code, int ndata );
double GetHistoricalCorrWithBloomberg( const std::wstring& code1, const std::wstring& code2, const std::wstring& periodUnit, int ndata, const std::wstring& activeDateOption );

