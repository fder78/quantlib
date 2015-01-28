#include "StdAfx.h"

#include "CurveTable.h"

#include "ParamParseUtil.h"
#include "PricingSetting.h"

#include "structuredproduct_ir.h"

#include "CQuery.h"
#include "GlobalSetting.h"

#include "stringUtil.h"
#include "EnumParser.h"

#include <yield_builder.hpp>
#include <yield_curve_bootstrapping.hpp>

#include <ql/instruments/bonds/fixedratebond.hpp>

#include <atlstr.h>

#include <ql/indexes/ibor/hibor3m.hpp>
#include <ql/indexes/ibor/cd91.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <hull_white_calibration.hpp>

CurveTable::CurveTable()
{
}

enum E_YieldDataType
{
	YDT_Deposit,
	YDT_Futures,
	YDT_FRA,
	YDT_Swap,
	YDT_Bond,
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
	}
};

typedef std::map<std::wstring, std::wstring> ParamMap;

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

enum E_CurrencyType
{
	CT_KRW,
	CT_USD,
	CT_HKD,
	CT_JPY,
};

void CurveTable::Init()
{
	m_table.clear();
	m_fxTable.clear();
	m_cdsTable.clear();
	m_corrMap.clear();
	m_hwTable.clear();
}

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
	out.calendar = CalendarParser::ParseEnum( paramMap.find( L"Calendar" )->second );
	out.dayCounter = DayCounterParser::ParseEnum( paramMap.find( L"DayCounter" )->second );
	out.fixedFreq = FrequencyParser::ParseEnum( paramMap.find( L"FixedFreq" )->second );
	out.iborIndex = IborIndexParser::ParseEnum( paramMap.find( L"IborIndex" )->second );
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

void QueryRecentCurveData( const std::wstring& code, OUT CQuery& dbConnector, OUT Date& rcvDate )
{
	BOOL conRes = dbConnector.Connect( 4, DB_SERVER_NAME, DB_ID, DB_PASSWORD );

	CString query_str;
	query_str.Format( L"select count(*) as cnt, max(ReceiveDate) as RcvDate from `ficc_drvs`.`curves` where ReceiveDate<='%s' and CurveCode='%s'", ::ToWString( PricingSetting::instance().GetEvaluationDate() ).c_str(), code.c_str() );
	dbConnector.Exec( (const wchar_t*)query_str );

	dbConnector.Fetch();
	if( !boost::lexical_cast<int>( dbConnector.GetStr( L"cnt" ) ) )
	{
		QL_ASSERT( false, "존재하지 않는 커브" + ::ToString( code ) + "가 입력되었습니다." );
	}

	rcvDate = ::ConvertToDate( dbConnector.GetStr( L"RcvDate" ) );
	dbConnector.Clear();

	query_str.Format( L"select B.CurveCode, B.Tenor, A.Param, B.Value from ficc_drvs.curves as B left outer join ficc_drvs.curve_type as A on ( A.CurveCode=B.CurveCode and ( A.Tenor = B.Tenor or A.Tenor='all' ) ) where B.CurveCode='%s' and ReceiveDate='%s'", code.c_str(), ::ToWString( rcvDate ).c_str() );

	dbConnector.Exec( (const wchar_t*)query_str );
	if( !dbConnector.GetNumRow() )
	{
		QL_ASSERT( false,  "존재하지 않는 커브" + ::ToString( code ) + "가 입력되었습니다." );
	}
}

std::pair<boost::shared_ptr<YieldCurveData>, std::vector<Real> > LoadCurve( const std::wstring& code ) 
{
	if( boost::find_first( code, L"_CRS" ) )
	{
		std::vector<std::wstring> codeVec;
		boost::split( codeVec, code, boost::is_any_of( L"_" ), boost::algorithm::token_compress_on );

		boost::shared_ptr<FXCurveData> fxCurve( CurveTable::instance().GetFXCurve( codeVec[ 0 ] + L"_FX" ) );
		Real spot = fxCurve->m_fwdValue.front();
		
		boost::shared_ptr<YieldCurveData> res( new YieldCurveData );
		boost::shared_ptr<YieldTermStructure> foreignIRS( build_yield_curve( *CurveTable::instance().GetYieldCurve( codeVec[ 0 ] )->GetCurveData() ) );

		res->dates.resize( fxCurve->m_fwdValue.size() );
		res->yields.resize( fxCurve->m_fwdValue.size() );
		res->dc = Actual365Fixed();

		for( size_t i = 1; i < fxCurve->m_fwdValue.size(); i++ )
		{
			Real t = Actual365Fixed().yearFraction( fxCurve->m_fwdDate[ 0 ], fxCurve->m_fwdDate[ i ] );
			res->dates[ i ] = fxCurve->m_fwdDate[ i ];
			res->yields[ i ] = std::log( fxCurve->m_fwdValue[ i ] / spot * std::exp( foreignIRS->zeroRate( t, Continuous ) * t ) ) / t;
		}

		res->dates[ 0 ] = Date( fxCurve->m_fwdDate.front().serialNumber(), 0 );
		res->yields[ 0 ] = res->yields[ 1 ];

		return std::make_pair( res, res->yields );
	}

	CQuery dbConnector;
	Date rcvDate;
	::QueryRecentCurveData( code, dbConnector, rcvDate );

	boost::shared_ptr<YieldCurveData> curveData( new YieldCurveData );

	curveData->dates.reserve( dbConnector.GetNumRow() );
	curveData->yields.reserve( dbConnector.GetNumRow() );
	curveData->dc = Actual365Fixed();

	std::vector<Real> yieldData;

	std::vector<DepoRateData> deposits;
	std::vector<FutRateData> futures;
	std::vector<FraRateData> fras;
	std::vector<SwapRateData> swaps;
	std::vector<BondRateData> bonds;

	while( !dbConnector.Fetch() )
	{
		ParamMap paramMap;
		::ParseParam( dbConnector.GetStr( L"Param" ), paramMap );

		E_YieldDataType type = YieldDataTypeParser::ParseEnum( paramMap[ L"Type" ] );

		switch( type )
		{
		case YDT_Deposit:
			yieldData.push_back( boost::lexical_cast<double>( dbConnector.GetStr( L"Value" ) ) );
			deposits.resize( deposits.size() + 1 );
			::ParseDepoRateData( dbConnector.GetStr( L"Tenor" ), paramMap, deposits.back(), yieldData.back() );
			break;
		case YDT_Futures:
			yieldData.push_back( ( 100. - boost::lexical_cast<double>( dbConnector.GetStr( L"Value" ) ) ) / 100. );
			futures.resize( futures.size() + 1 );
			::ParseFuturesRateData( dbConnector.GetStr( L"Tenor" ), paramMap, futures.back(), yieldData.back() );
			break;
		case YDT_FRA:
			yieldData.push_back( boost::lexical_cast<double>( dbConnector.GetStr( L"Value" ) ) );
			fras.resize( fras.size() + 1 );
			::ParseFRARateData( dbConnector.GetStr( L"Tenor" ), paramMap, fras.back(), yieldData.back() );;
			break;
		case YDT_Swap:
			yieldData.push_back( boost::lexical_cast<double>( dbConnector.GetStr( L"Value" ) ) );
			swaps.resize( swaps.size() + 1 );
			::ParseSwapRateData( dbConnector.GetStr( L"Tenor" ), paramMap, swaps.back(), yieldData.back() );
			break;
		case YDT_Bond:
			yieldData.push_back( boost::lexical_cast<double>( dbConnector.GetStr( L"Value" ) ) );
			bonds.resize( bonds.size() + 1 );
			::ParseBondRateData( dbConnector.GetStr( L"Tenor" ), paramMap, bonds.back(), yieldData.back() );
			break;
		}
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
	}

	curveData->dc = Actual365Fixed();
	return std::make_pair( curveData, yieldData );
}


boost::shared_ptr<YieldCurveInfoWrapper> CurveTable::GetYieldCurve( const std::wstring& codedCode )
{
	YieldCurveTable::const_iterator iter = m_table.find( codedCode );

	if( iter != m_table.end() )
	{
		return iter->second.first;
	}

	std::vector<std::wstring> codes = ::Split( codedCode, boost::is_any_of( L"|" ) );
	std::wstring code = codes[ 0 ];

	std::pair<boost::shared_ptr<YieldCurveData>, std::vector<Real> > curveData = ::LoadCurve( code );
	boost::shared_ptr<YieldCurveInfoWrapper> wrapper( new YieldCurveInfoWrapper( codedCode, curveData.first ) );

	if( codes.size() > 1 )
	{
		Real delta = boost::lexical_cast<Real>( codes[ 1 ] ) / 10000.;
		wrapper->ShiftCurve( delta );
	}
	
	m_table.insert( std::make_pair( codedCode, std::make_pair( wrapper, curveData.second ) ) );
	return wrapper;
}

CurveTable::yield_mapped_type CurveTable::GetCurveData( const std::wstring& code )
{
	// Data 로드
	GetYieldCurve( code );

	YieldCurveTable::const_iterator iter = m_table.find( code );
	QL_ASSERT( iter != m_table.end(), "KRW/USD/HKD 외의 금리코드가 입력되었습니다." );

	return iter->second;
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
		curveData->m_tenors.push_back( ParsePeriod( dbConnector.GetStr( L"Tenor" ) ) );
		curveData->m_quoted_spreads.push_back( boost::lexical_cast<Real>( dbConnector.GetStr( L"Value" ) ) );
	}

	curveData->m_tenors.insert( curveData->m_tenors.begin(), 1 * Days );
	curveData->m_quoted_spreads.insert( curveData->m_quoted_spreads.begin(), curveData->m_quoted_spreads.front() );

	return curveData;
}

boost::shared_ptr<CDSCurveData> CurveTable::GetCDSCurve( const std::wstring& code )
{
	CDSCurveTable::const_iterator iter = m_cdsTable.find( code );
	if( iter == m_cdsTable.end() )
	{
		boost::shared_ptr<CDSCurveData> curveData = ::LoadCDSCurve( code );
		m_cdsTable.insert( std::make_pair( code, curveData ) );

		return curveData;
	}

	return iter->second;
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

boost::shared_ptr<FXCurveData> LoadFXCurve( const std::wstring& code )
{
	CQuery dbConnector;
	Date rcvDate;
	::QueryRecentCurveData( code, dbConnector, rcvDate );

	boost::shared_ptr<FXCurveData> curveData = boost::shared_ptr<FXCurveData>( new FXCurveData() );

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

		curveData->m_fwdDate.push_back( date );
		curveData->m_fwdValue.push_back( boost::lexical_cast<Real>( dbConnector.GetStr( L"Value" ) ) );
	}

	if( !curveData->m_fwdDate.empty() )
	{
		curveData->m_fwdDate.push_back( curveData->m_fwdDate.back() + 20 * Years );
		curveData->m_fwdValue.push_back( curveData->m_fwdValue.back()	);
	}

	return curveData;
}

boost::shared_ptr<FXCurveData> CurveTable::GetFXCurve( const std::wstring& code )
{
	FXCurveTable::const_iterator iter = m_fxTable.find( code );
	if( iter == m_fxTable.end() )
	{
		boost::shared_ptr<FXCurveData> curveData = ::LoadFXCurve( code );
		m_fxTable.insert( std::make_pair( code, curveData ) );

		return curveData;
	}

	return iter->second;
}

boost::shared_ptr<HullWhiteParameters> CurveTable::GetHWParams( const std::wstring& codedCode, const YieldCurveInfoWrapper* curveData )
{
	std::wstring code = Split( codedCode, boost::is_any_of( "_|" ) )[ 0 ];

	HWTable::const_iterator iter = m_hwTable.find( code );
	if( iter != m_hwTable.end() )
	{
		return iter->second;
	}

	CQuery dbConnector;
	Date rcvDate;
	::QueryRecentCurveData( code + L"_CAPVOL", dbConnector, rcvDate );

	std::vector<Size> tenors;
	std::vector<Real> vols;

	std::wstring paramStr;

	CapVolData volData;

	while( !dbConnector.Fetch() )
	{
		Period tenor = ::ParsePeriod( dbConnector.GetStr( L"Tenor" ) );
		volData.tenors.push_back( tenor.length() );
		volData.vols.push_back( boost::lexical_cast<Real>( dbConnector.GetStr( L"Value" ) ) );

		paramStr = dbConnector.GetStr( L"Param" );
	}

	ParamMap paramMap;
	::ParseParam( paramStr, paramMap );

	boost::shared_ptr<IborIndex> iborIdx( IborIndexParser::ParseEnum( paramMap[ L"IborIndex" ] ) );
	Frequency fixedFreq = FrequencyParser::ParseEnum( paramMap[ L"FixedFreq" ] );
	DayCounter dayCounter = DayCounterParser::ParseEnum( paramMap[ L"DayCounter" ] );

	boost::shared_ptr<YieldTermStructure> yieldTS = ::build_yield_curve( *curveData->GetCurveData() );
	iborIdx = iborIdx->clone( Handle<YieldTermStructure>( yieldTS ) );
	iborIdx->clearFixings();
	iborIdx->addFixing( iborIdx->fixingCalendar().advance( curveData->GetCurveData()->dates.front(), 0, Days, Preceding ), curveData->GetCurveData()->yields.front() );

	boost::shared_ptr<HullWhiteTimeDependentParameters> hwParam( new 
		HullWhiteTimeDependentParameters( ::calibration_hull_white( PricingSetting::instance().GetEvaluationDate(), iborIdx, volData, fixedFreq, dayCounter ) ) );

	m_hwTable.insert( std::make_pair( code, hwParam ) );

	return hwParam;
}

QuantLib::Real CurveTable::GetCorr( const std::wstring& codedCode1, const std::wstring& codedCode2 )
{
	std::vector<std::wstring> codes1 = ::Split( codedCode1, boost::is_any_of( L"|" ) );
	std::wstring code1 = codes1[ 0 ];

	std::vector<std::wstring> codes2 = ::Split( codedCode2, boost::is_any_of( L"|" ) );
	std::wstring code2 = codes2[ 0 ];

	std::wstring minCode = min( code1, code2 );
	std::wstring maxCode = max( code1, code2 );

	CorrMap::const_iterator iter = m_corrMap.find( std::make_pair( minCode, maxCode ) );

	if( iter != m_corrMap.end() )
	{
		return iter->second;
	}

	CQuery dbConnector;
	Date rcvDate;
	::QueryRecentCurveData( minCode + L"_" + maxCode, dbConnector, rcvDate );

	dbConnector.Fetch();

	Real corr = boost::lexical_cast<Real>( dbConnector.GetStr( L"Value" ) );

	m_corrMap.insert( std::make_pair( std::make_pair( minCode, maxCode ), corr ) );

	return corr;
}

std::pair<Real, Real> CurveTable::GetCorrRcvy( const std::wstring& code )
{
	CorrRcvyTable::iterator iter = m_corrRcvyTable.find( code );
	if( iter != m_corrRcvyTable.end() )
	{
		return iter->second;
	}

	CQuery dbConnector;
	BOOL conRes = dbConnector.Connect( 4, DB_SERVER_NAME, DB_ID, DB_PASSWORD );
	CString query_str;
	query_str.Format( L"select count(*) as cnt, max(ReceiveDate) as RcvDate from `ficc_drvs`.`percontractinfo` where ReceiveDate<='%s' and ContractCode='%s'", ::ToWString( PricingSetting::instance().GetEvaluationDate() ).c_str(), code.c_str() );
	dbConnector.Exec( (const wchar_t*)query_str );

	dbConnector.Fetch();

	if( !boost::lexical_cast<int>( dbConnector.GetStr( L"cnt" ) ) )
	{
		return std::make_pair( 0.9, 0.4 );
	}

	Date rcvDate = ::ConvertToDate( dbConnector.GetStr( L"RcvDate" ) );
	dbConnector.Clear();

	query_str.Format( L"select * from ficc_drvs.percontractinfo where ContractCode='%s' and ReceiveDate='%s'", code.c_str(), ::ToWString( rcvDate ).c_str() );
	dbConnector.Exec( (const wchar_t*)query_str );

	dbConnector.Fetch();

	std::pair<Real, Real> res( boost::lexical_cast<Real>( dbConnector.GetStr( L"Corr" ) ), boost::lexical_cast<Real>( dbConnector.GetStr( L"RecoveryRate") ) );

	m_corrRcvyTable.insert( std::make_pair( code, res ) );

	return res;
}

YieldCurveInfoWrapper::YieldCurveInfoWrapper( const std::wstring& curveName, boost::shared_ptr<YieldCurveData> curveData ) : m_curveName( curveName )
	, m_curveData( curveData )
{
	m_daysToMat.reset( new int[ m_curveData->dates.size() ] );
	m_yields.reset( new double[ m_curveData->yields.size() ] );
	m_curveInfo.reset( new YieldCurveInfo() );

	for( size_t i = 0; i < m_curveData->dates.size(); i++ )
	{
		m_daysToMat[ i ] = std::max<int>( m_curveData->dates[ i ].serialNumber() - PricingSetting::instance().GetEvaluationDate().serialNumber(), 1 );
		m_yields[ i ] = m_curveData->yields[ i ];
	}

	m_curveInfo->day = m_daysToMat.get();
	m_curveInfo->rate = m_yields.get();
	m_curveInfo->size = m_curveData->dates.size();

	m_hwParam = CurveTable::instance().GetHWParams( m_curveName, this );
}

YieldCurveInfo* YieldCurveInfoWrapper::GetInfo() const
{
	return m_curveInfo.get();
}

void YieldCurveInfoWrapper::ShiftCurve( Real delta )
{
	for( int i = 0; i < m_curveInfo->size; i++ )
	{
		const_cast<Real&>( m_curveInfo->rate[ i ] ) += delta;
		const_cast<Real&>( m_curveData->yields[ i ] ) += delta;
	}

	m_hwParam = CurveTable::instance().GetHWParams( m_curveName, this );
}
