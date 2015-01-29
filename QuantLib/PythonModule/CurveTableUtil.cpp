#include "StdAfx.h"

#include "CurveTableUtil.h"

#include "CQuery.h"

#include "GlobalSetting.h"
#include "ParamParseUtil.h"
#include "StringUtil.h"
#include "PricingSetting.h"
#include "ShiftOption.h"
#include "CurveTable.h"
#include "InterestRateCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"

#include "yield_builder.hpp"
#include "yield_curve_bootstrapping.hpp"
#include <ql/instruments/bonds/fixedratebond.hpp>
#include "pricing_functions/single_hull_white_calibration.hpp"
#include "pricing_functions/hull_white_calibration.hpp"

#include <atlstr.h>

void QueryRecentCurveData( const std::wstring& code, OUT CQuery& dbConnector, OUT Date& rcvDate )
{
	BOOL conRes = dbConnector.Connect( 4, DB_SERVER_NAME, DB_ID, DB_PASSWORD );

	CString query_str;
	query_str.Format( L"select count(*) as cnt, max(ReceiveDate) as RcvDate from `ficc_drvs`.`curves` where ReceiveDate<='%s' and CurveCode='%s'", ::ToWString( PricingSetting::instance().GetDataDate() ).c_str(), code.c_str() );
	dbConnector.Exec( (const wchar_t*)query_str );

	dbConnector.Fetch();
	if( !boost::lexical_cast<int>( dbConnector.GetStr( L"cnt" ) ) )
	{
		QL_ASSERT( false, "존재하지 않는 커브" + ::ToString( code ) + "가 입력되었습니다." );
	}

	rcvDate = ::ConvertToDate( dbConnector.GetStr( L"RcvDate" ) );
	dbConnector.Clear();

	query_str.Format( L"select B.CurveCode, B.Tenor, A.Param, B.Value, A.ChunkIndex from ficc_drvs.curves as B left outer join ficc_drvs.curve_type as A on ( A.CurveCode=B.CurveCode and ( A.Tenor = B.Tenor or A.Tenor='all' ) ) where B.CurveCode='%s' and ReceiveDate='%s'", code.c_str(), ::ToWString( rcvDate ).c_str() );

	dbConnector.Exec( (const wchar_t*)query_str );
	if( !dbConnector.GetNumRow() )
	{
		QL_ASSERT( false,  "존재하지 않는 커브" + ::ToString( code ) + "가 입력되었습니다." );
	}
}

enum E_CurrencyType
{
	CT_KRW,
	CT_USD,
	CT_HKD,
	CT_JPY,
};

class CurrencyTypeParser : public EnumParser<CurrencyTypeParser, E_CurrencyType>
{
public:
	CurrencyTypeParser() { }

public:
	void BuildEnumMap()
	{
		AddEnum( L"KRW", CT_KRW );
		AddEnum( L"USD", CT_USD );
		AddEnum( L"HKD", CT_HKD );
		AddEnum( L"JPY", CT_JPY );
	}
};

Period ParsePeriod( const std::wstring& period )
{
	wchar_t code = period.back();
	int num = boost::lexical_cast<int>( period.substr( 0, period.length() - 1 ) );
	switch( code )
	{
	case L'd':
	case L'D':
		return Period( num, Days );
	case L'w':
	case L'W':
		return Period( num, Weeks );
	case L'm':
	case L'M':
		return Period( num, Months );
	case L'y':
	case L'Y':
		return Period( num, Years );
	}

	QL_ASSERT( false, "DB에서 금리의 Period가 이상합니다. d, w, m, y 중 하나여야 합니다" );
	return Period( num, Days );
}

typedef std::map<std::wstring, std::wstring> ParamMap;
void ParseDepoRateData( const std::wstring& tenor, const ParamMap& paramMap, OUT DepoRateData& out, Real value )
{
	out.depoRateQuote = value;
	out.maturity = ::ParsePeriod( tenor );

	out.endOfMonth = false;
	out.bdc = BusinessDayConventionParser::ParseEnum( paramMap.find( L"BDC" )->second );
	out.calendar = CalendarParser::ParseEnum( paramMap.find( L"Calendar" )->second );
	out.dayCounter = DayCounterParser::ParseEnum( paramMap.find( L"DayCounter" )->second );
	out.fixingDays = boost::lexical_cast<int>( paramMap.find( L"FixingDays" )->second );
}

void ParseFuturesRateData( const std::wstring& tenor, const ParamMap& paramMap, OUT FutRateData& out, Real value )
{
	out.futPriceQuote = ( 1. - value ) * 100.;

	out.imm = Date( boost::lexical_cast<BigInteger>( paramMap.find( L"Date" )->second ) );
	out.matMonths = 3;
	out.bdc = ModifiedFollowing;

	out.endOfMonth = false;
	out.calendar = CalendarParser::ParseEnum( paramMap.find( L"Calendar" )->second );
	out.dayCounter = DayCounterParser::ParseEnum( paramMap.find( L"DayCounter" )->second );
}

void ParseFRARateData( const std::wstring& tenor, const ParamMap& paramMap, OUT FraRateData& out, Real value )
{
	out.fraRateQuote = value;

	out.monthsToStart = 3;
	out.monthsToEnd = ::ParsePeriod( tenor ).length();

	out.endOfMonth = false;
	out.bdc = BusinessDayConventionParser::ParseEnum( paramMap.find( L"BDC" )->second );
	out.calendar = CalendarParser::ParseEnum( paramMap.find( L"Calendar" )->second );
	out.dayCounter = DayCounterParser::ParseEnum( paramMap.find( L"DayCounter" )->second );
}

void ParseSwapRateData( const std::wstring& tenor, const ParamMap& paramMap, OUT SwapRateData& out, Real value )
{
	out.swapRateQuote = value;
	out.maturity = ::ParsePeriod( tenor );

	out.bdc = BusinessDayConventionParser::ParseEnum( paramMap.find( L"BDC" )->second );
	out.dayCounter = DayCounterParser::ParseEnum( paramMap.find( L"DayCounter" )->second );
	out.fixedFreq = FrequencyParser::ParseEnum( paramMap.find( L"FixedFreq" )->second );
	out.iborIndex = IborIndexParser::ParseEnum( paramMap.find( L"IborIndex" )->second );

	out.calendar = out.iborIndex->fixingCalendar();
}

void ParseBondRateData( const std::wstring& tenor, const ParamMap& paramMap, OUT BondRateData& out, Real value )
{
	out.maturity = ::ConvertToDate( tenor );
	out.issueDate = ::ConvertToDate( paramMap.find( L"IssueDate" )->second );
	out.cpn = boost::lexical_cast<Real>( paramMap.find( L"CPN" )->second );
	out.calendar = CalendarParser::ParseEnum( paramMap.find( L"Calendar" )->second );
	out.dayCounter = DayCounterParser::ParseEnum( paramMap.find( L"DayCounter" )->second );
	out.cpnFreq = FrequencyParser::ParseEnum( paramMap.find( L"CPNFreq" )->second );
	out.settle = boost::lexical_cast<Natural>( paramMap.find( L"Settle" )->second );

	Schedule schedule(out.issueDate, out.maturity, Period( out.cpnFreq ), out.calendar,
		Unadjusted, Unadjusted, DateGeneration::Backward, false);

	std::vector<Rate> cpns( schedule.size(), out.cpn );

	FixedRateBond b( out.settle, 100., schedule, cpns, out.dayCounter );
	RelinkableHandle<YieldTermStructure> discountingTermStructure;
	boost::shared_ptr<PricingEngine> bondEngine(
		new DiscountingBondEngine(discountingTermStructure));

	b.setPricingEngine( bondEngine );	
	out.quote = b.cleanPrice( value, out.dayCounter, Compounded, out.cpnFreq );
}

void ParseBondFairYieldData( const std::wstring& tenor, const ParamMap& paramMap, OUT BondRateData& out, Real value )
{
	out.issueDate = PricingSetting::instance().GetEvaluationDate();
	out.cpn = value;
	out.maturity = out.issueDate + ::ParsePeriod( tenor );

	out.calendar = CalendarParser::ParseEnum( paramMap.find( L"Calendar" )->second );
	out.dayCounter = DayCounterParser::ParseEnum( paramMap.find( L"DayCounter" )->second );
	out.cpnFreq = FrequencyParser::ParseEnum( paramMap.find( L"CPNFreq" )->second );
	out.settle = boost::lexical_cast<Natural>( paramMap.find( L"Settle" )->second );

	out.quote = 100;
}



enum E_YieldDataType
{
	YDT_Deposit,
	YDT_Futures,
	YDT_FRA,
	YDT_Swap,
	YDT_Bond,
	YDT_BondFairYield,
};

class YieldDataTypeParser : public EnumParser<YieldDataTypeParser, E_YieldDataType>
{
public:
	YieldDataTypeParser() { }

public:
	void BuildEnumMap()
	{
		AddEnum( L"D", YDT_Deposit );
		AddEnum( L"F", YDT_Futures );
		AddEnum( L"FR", YDT_FRA );
		AddEnum( L"S", YDT_Swap );
		AddEnum( L"B", YDT_Bond );
		AddEnum( L"BFY", YDT_BondFairYield );
	}
};

void ParseParam( const std::wstring& paramStr, OUT ParamMap& res )
{
	std::vector<std::wstring> params;
	boost::algorithm::split( params, paramStr, boost::is_any_of( L"," ), boost::algorithm::token_compress_on );

	for each( const std::wstring& param in params )
	{
		std::wstring::size_type commaPos = param.find( L"=" );
		std::wstring paramName = param.substr( 0, commaPos );
		std::wstring value = param.substr( commaPos + 1, std::wstring::npos );

		boost::trim( paramName );
		boost::trim( value );

		res.insert( std::make_pair( paramName, value ) );
	}
}

std::pair<boost::shared_ptr<YieldCurveData>, std::vector<Real> > LoadCurve( const std::wstring& code, const ShiftOption& shiftOption ) 
{
	CQuery dbConnector;
	Date rcvDate;
	::QueryRecentCurveData( code, dbConnector, rcvDate );

	boost::shared_ptr<YieldCurveData> curveData( new YieldCurveData );

	curveData->dates.reserve( dbConnector.GetNumRow() );
	curveData->yields.reserve( dbConnector.GetNumRow() );
	curveData->dc = Actual365Fixed();

	std::vector<Real> yieldData;
	std::vector<Size> chunkIndicies;

	std::vector<DepoRateData> deposits;
	std::vector<FutRateData> futures;
	std::vector<FraRateData> fras;
	std::vector<SwapRateData> swaps;
	std::vector<BondRateData> bonds;
	int tenorIdx = 0;

	while( !dbConnector.Fetch() )
	{
		ParamMap paramMap;
		::ParseParam( dbConnector.GetStr( L"Param" ), paramMap );

		Size chunkIdx = boost::lexical_cast<Size>( dbConnector.GetStr( L"ChunkIndex" ) );
		Real delta = shiftOption.GetDelta( tenorIdx, chunkIdx );

		E_YieldDataType type = YieldDataTypeParser::ParseEnum( paramMap[ L"Type" ] );

		Real value = CurveTable::instance().GetNullableManualInputYTM( std::make_pair( code, dbConnector.GetStr( L"Tenor" ) ) );
		if( value == Null<Real>() )
		{
			value = boost::lexical_cast<double>( dbConnector.GetStr( L"Value" ) );
		}

		switch( type )
		{
		case YDT_Deposit:
			yieldData.push_back( value + delta );
			deposits.resize( deposits.size() + 1 );
			::ParseDepoRateData( dbConnector.GetStr( L"Tenor" ), paramMap, deposits.back(), yieldData.back() );
			break;
		case YDT_Futures:
			yieldData.push_back( ( 100. - value ) / 100. + delta );
			futures.resize( futures.size() + 1 );
			::ParseFuturesRateData( dbConnector.GetStr( L"Tenor" ), paramMap, futures.back(), yieldData.back() );
			break;
		case YDT_FRA:
			yieldData.push_back( value + delta );
			fras.resize( fras.size() + 1 );
			::ParseFRARateData( dbConnector.GetStr( L"Tenor" ), paramMap, fras.back(), yieldData.back() );;
			break;
		case YDT_Swap:
			yieldData.push_back( value + delta );
			swaps.resize( swaps.size() + 1 );
			::ParseSwapRateData( dbConnector.GetStr( L"Tenor" ), paramMap, swaps.back(), yieldData.back() );
			break;
		case YDT_Bond:
			yieldData.push_back( value + delta );
			bonds.resize( bonds.size() + 1 );
			::ParseBondRateData( dbConnector.GetStr( L"Tenor" ), paramMap, bonds.back(), yieldData.back() );
			break;
		case YDT_BondFairYield:
			yieldData.push_back( value + delta );
			bonds.resize( bonds.size() + 1 );
			::ParseBondFairYieldData( dbConnector.GetStr( L"Tenor" ), paramMap, bonds.back(), yieldData.back() );
			break;
		}
		chunkIndicies.push_back( chunkIdx );
		tenorIdx++;
	}

	std::vector<std::pair<Integer, Rate> > temp = yield_curve_bootstrapping( rcvDate, deposits, futures, fras, swaps, bonds );
	curveData->dates.reserve( temp.size() );
	curveData->yields.reserve( temp.size() );

	for each( std::pair<Integer, Rate> v in temp )
	{
		curveData->dates.push_back( Date( PricingSetting::instance().GetEvaluationDate() + v.first ) );
		curveData->yields.push_back( v.second );
	}

	Date minDate = PricingSetting::instance().GetEvaluationDate();

	// 가장 최근 이자율을 이자율받은날의 이자율로 박아준다!
	if( curveData->dates.front().serialNumber() >= minDate.serialNumber() )
	{
		curveData->dates.insert( curveData->dates.begin(), Date( minDate.serialNumber(), 0 ) );

		volatile Real firstYield = yieldData.front();
		yieldData.insert( yieldData.begin(), firstYield );

		volatile Real firstSpot = curveData->yields.front();
		curveData->yields.insert( curveData->yields.begin(), firstSpot );
		chunkIndicies.insert( chunkIndicies.begin(), 0 );
	}

	curveData->dc = Actual365Fixed();
	curveData->chunkIndex = chunkIndicies;
	return std::make_pair( curveData, yieldData );
}

boost::shared_ptr<CDSCurveData> LoadCDSCurve( const std::wstring& code )
{
	CQuery dbConnector;
	Date rcvDate;
	try
	{
		::QueryRecentCurveData( code, dbConnector, rcvDate );
	}
	catch( const std::exception& )
	{
		return LoadCDSCurve( L"MeanCDS00000" );
	}


	boost::shared_ptr<CDSCurveData> curveData = boost::shared_ptr<CDSCurveData>( new CDSCurveData() );

	while( !dbConnector.Fetch() )
	{
		curveData->tenors.push_back( ::ParsePeriod( dbConnector.GetStr( L"Tenor" ) ) );
		curveData->quotedSpreads.push_back( boost::lexical_cast<Real>( dbConnector.GetStr( L"Value" ) ) );
	}

	return curveData;
}

enum E_DateType
{
	DT_Tenor,
	DT_Date,
};

class DateTypeParser : public EnumParser<DateTypeParser, E_DateType>
{
public:
	void BuildEnumMap()
	{
		AddEnum( L"T", DT_Tenor );
		AddEnum( L"D", DT_Date );
	}
};

boost::shared_ptr<FXCurveData> LoadFXCurve( const std::wstring& code, const ShiftOption& shiftOption )
{
	CQuery dbConnector;
	Date rcvDate;
	::QueryRecentCurveData( code, dbConnector, rcvDate );

	boost::shared_ptr<FXCurveData> curveData = boost::shared_ptr<FXCurveData>( new FXCurveData() );
	int tenorIdx = 0;
	while( !dbConnector.Fetch() )
	{
		ParamMap paramMap;
		::ParseParam( dbConnector.GetStr( L"Param" ), paramMap );

		Date date;
		std::wstring dateValue = dbConnector.GetStr( L"Tenor" );
		switch( DateTypeParser::ParseEnum( paramMap[ L"DateType" ] ) )
		{
		case DT_Tenor:
			date = rcvDate + ::ParsePeriod( dateValue );
			break;
		case DT_Date:
			date = ::ConvertToDate( dateValue );
			break;
		}
		
		Size chunkIdx = boost::lexical_cast<Size>( dbConnector.GetStr( L"ChunkIndex" ) );
		Real delta = shiftOption.GetDelta( tenorIdx, chunkIdx );

		curveData->fwdDate.push_back( date );
		curveData->fwdValue.push_back( boost::lexical_cast<Real>( dbConnector.GetStr( L"Value" ) ) + delta );
		curveData->chunkIndex.push_back( chunkIdx );

		tenorIdx++;
	}

	if( !curveData->fwdDate.empty() )
	{
		curveData->fwdDate.push_back( curveData->fwdDate.back() + 20 * Years );
		curveData->fwdValue.push_back( curveData->fwdValue.back()	);
	}

	return curveData;
}

std::pair<boost::shared_ptr<YieldCurveData>, std::vector<Real> > LoadCRSCurve( const std::wstring& code, const ShiftOption& shiftOption, const ShiftOption& fxShiftOption )
{
	std::vector<std::wstring> codeVec;
	boost::split( codeVec, code, boost::is_any_of( L"_" ), boost::algorithm::token_compress_on );

	boost::shared_ptr<FXCurveData> fxCurve( CurveTable::instance().GetFXCurve( codeVec[ 0 ] + L"_FX", fxShiftOption ) );
	Real spot = fxCurve->fwdValue.front();

	boost::shared_ptr<YieldCurveData> res( new YieldCurveData );
	boost::shared_ptr<YieldTermStructure> foreignIRS( build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( codeVec[ 0 ], ShiftOption::ShiftNothing ) )->GetCurveData() ) );

	res->dates.resize( fxCurve->fwdValue.size() );
	res->yields.resize( fxCurve->fwdValue.size() );
	res->dc = Actual365Fixed();

	for( size_t i = 1; i < fxCurve->fwdValue.size() - 1; i++ )
	{
		Real t = Actual365Fixed().yearFraction( fxCurve->fwdDate[ 0 ], fxCurve->fwdDate[ i ] );
		res->dates[ i ] = fxCurve->fwdDate[ i ];
		res->yields[ i ] = std::log( fxCurve->fwdValue[ i ] / spot * std::exp( foreignIRS->zeroRate( t, Continuous ) * t ) ) / t + shiftOption.GetDelta( i, 0 );
	}

	res->dates.back() = fxCurve->fwdDate.back();
	res->yields.back() = *( res->yields.rbegin() + 1 );

	res->dates[ 0 ] = Date( fxCurve->fwdDate.front().serialNumber(), 0 );
	res->yields[ 0 ] = res->yields[ 1 ];
	res->chunkIndex = fxCurve->chunkIndex;

	return std::make_pair( res, res->yields );
}


boost::shared_ptr<HullWhiteParameters> CalcFromSwaption( boost::shared_ptr<SwaptionVolData> volDataOrg, Period tenor, boost::shared_ptr<InterestRateCurveInfoWrapper> ircw )
{
	boost::shared_ptr<SwaptionVolData> volData( new SwaptionVolData );
	std::vector<Size> dataIdx = ::GetDataIndexFromSwaption( *volDataOrg, tenor );

	for each( Size idx in dataIdx )
	{
		volData->maturities.push_back( volDataOrg->maturities[ idx ] );
		volData->lengths.push_back( volDataOrg->lengths[ idx ] );
		volData->vols.push_back( volDataOrg->vols[ idx ] );
	}

	volData->fixedDC = volDataOrg->fixedDC;
	volData->floatingDC = volDataOrg->floatingDC;
	volData->fixedFreq = volDataOrg->fixedFreq;
	volData->index = volDataOrg->index;
	volData->initialSigma = volData->vols;
	volData->fixedA = 0.03;

	return boost::shared_ptr<HullWhiteParameters>( new HullWhiteParameters( ::single_calibration_hull_white( Date( PricingSetting::instance().GetEvaluationDate().serialNumber() ), *volData ) ) );	
}

boost::shared_ptr<CapVolData> LoadCapVol( const std::wstring& code, const ShiftOption& so, boost::shared_ptr<InterestRateCurveInfoWrapper> ircw )
{
	CQuery dbConnector;
	Date rcvDate;
	::QueryRecentCurveData( code + L"_CAPVOL", dbConnector, rcvDate );

	std::vector<Size> tenors;
	std::vector<Real> vols;

	std::wstring paramStr;

	boost::shared_ptr<CapVolData> capVolData( new CapVolData );

	while( !dbConnector.Fetch() )
	{
		Period tenor = ::ParsePeriod( dbConnector.GetStr( L"Tenor" ) );
		capVolData->tenors.push_back( tenor.length() );
		capVolData->vols.push_back( boost::lexical_cast<Real>( dbConnector.GetStr( L"Value" ) ) + so.GetVolDelta( capVolData->tenors.size() - 1 ) );

		paramStr = dbConnector.GetStr( L"Param" );
	}

	ParamMap paramMap;
	::ParseParam( paramStr, paramMap );

	boost::shared_ptr<IborIndex> iborIdx( IborIndexParser::ParseEnum( paramMap[ L"IborIndex" ] ) );
	capVolData->fixedFreq = FrequencyParser::ParseEnum( paramMap[ L"FixedFreq" ] );
	capVolData->fixedDC = DayCounterParser::ParseEnum( paramMap[ L"DayCounter" ] );

	boost::shared_ptr<YieldTermStructure> yieldTS = ::build_yield_curve( *ircw->GetCurveData() );

	capVolData->index = iborIdx->clone( Handle<YieldTermStructure>( yieldTS ) );
	capVolData->index->clearFixings();
	capVolData->index->addFixing( iborIdx->fixingCalendar().advance( ircw->GetCurveData()->dates.front(), 0, Days, Preceding ), ircw->GetCurveData()->yields.front() );
	capVolData->initialSigma = capVolData->vols;
	capVolData->fixedA = 0.03;

	return capVolData;
}

boost::shared_ptr<HullWhiteParameters> CalcFromCapVol( const CapVolData& volDataOrg, Period tenor )
{
	std::vector<Size> dataIdx = ::GetDataIndexFromCapVol( volDataOrg, tenor );
	CapVolData capVolData( volDataOrg );
	capVolData.tenors.clear();
	capVolData.vols.clear();

	for each( Size idx in dataIdx )
	{
		capVolData.tenors.push_back( volDataOrg.tenors[ idx ] );
		capVolData.vols.push_back( volDataOrg.vols[ idx ] );
	}

	return boost::shared_ptr<HullWhiteParameters>( new HullWhiteParameters( ::single_calibration_hull_white( Date( PricingSetting::instance().GetEvaluationDate().serialNumber() ), capVolData ) ) );	
}

boost::shared_ptr<SwaptionVolData> LoadSwaptionVol( const std::wstring& code, const ShiftOption& so, boost::shared_ptr<InterestRateCurveInfoWrapper> ircw )
{
	CQuery dbConnector;
	Date rcvDate;	

	try
	{
		::QueryRecentCurveData( code + L"_SWTVOL", dbConnector, rcvDate );
	}
	catch ( ... )
	{
		return boost::shared_ptr<SwaptionVolData>();
	}

	std::vector<Size> tenors;
	std::vector<Real> vols;

	std::wstring paramStr;

	boost::shared_ptr<SwaptionVolData> swaptionVolData( new SwaptionVolData );

	while( !dbConnector.Fetch() )
	{
		std::vector<std::wstring> termTenor = ::Split( dbConnector.GetStr( L"Tenor" ), boost::is_any_of( L"_" ) );
		Period term = ::ParsePeriod( termTenor[ 0 ] );
		Period tenor = ::ParsePeriod( termTenor[ 1 ] );
		swaptionVolData->maturities.push_back( term );
		swaptionVolData->lengths.push_back( tenor );
		swaptionVolData->vols.push_back( boost::lexical_cast<Real>( dbConnector.GetStr( L"Value" ) ) + so.GetVolDelta( swaptionVolData->maturities.size() - 1 ) );

		paramStr = dbConnector.GetStr( L"Param" );
	}

	ParamMap paramMap;
	::ParseParam( paramStr, paramMap );

	boost::shared_ptr<IborIndex> iborIdx( IborIndexParser::ParseEnum( paramMap[ L"IborIndex" ] ) );
	swaptionVolData->fixedFreq = FrequencyParser::ParseEnum( paramMap[ L"FixedFreq" ] );
	swaptionVolData->fixedDC = DayCounterParser::ParseEnum( paramMap[ L"FixedDayCounter" ] );
	swaptionVolData->floatingDC = DayCounterParser::ParseEnum( paramMap[ L"FloatingDayCounter" ] );

	boost::shared_ptr<YieldTermStructure> yieldTS = ::build_yield_curve( *ircw->GetCurveData() );

	swaptionVolData->index = iborIdx->clone( Handle<YieldTermStructure>( yieldTS ) );
	swaptionVolData->index->clearFixings();
	swaptionVolData->index->addFixing( iborIdx->fixingCalendar().advance( ircw->GetCurveData()->dates.front(), 0, Days, Preceding ), ircw->GetCurveData()->yields.front() );
	swaptionVolData->initialSigma = swaptionVolData->vols;
	swaptionVolData->fixedA = 0.03;

	return swaptionVolData;
}


std::vector<Size> GetDataIndexFromSwaption( const SwaptionVolData& volDataOrg, Period tenor )
{
	std::vector<Size> dataIdx;
	Real threshold = 0.;
	if( tenor > 365 * Days )
	{
		threshold = 300.;
	}
	else
	{
		threshold = 730.;
	}

	for( size_t i = 0; i < volDataOrg.maturities.size(); i++ )
	{
		if( std::abs( ( PricingSetting::instance().GetEvaluationDate() + volDataOrg.maturities[ i ] + volDataOrg.lengths[ i ] ) - ( PricingSetting::instance().GetEvaluationDate() + tenor ) ) < threshold )
		{
			dataIdx.push_back( i );
		}
	}


	return dataIdx;
}

std::vector<Size> GetDataIndexFromCapVol( const CapVolData& volDataOrg, Period tenor )
{
	std::vector<Size> dataIdx;
	for( Size i = 0; i < volDataOrg.vols.size(); i++ )
	{
		dataIdx.push_back( i );
		if( volDataOrg.tenors[ i ] * 365 > (unsigned)tenor.length() )
		{
			break;
		}
	}

	return dataIdx;
}
