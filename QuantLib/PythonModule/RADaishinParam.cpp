#include "StdAfx.h"

#include "RADaishinParam.h"
#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "PricingSetting.h"
#include "XMLValue.h"
#include "CurveTable.h"
#include "yield_builder.hpp"
#include "ParamParseUtil.h"
#include "ProductIndex.h"

#include "BloombergCaller.h"

#include "pricing_functions/hull_white_calibration.hpp"

class RASwapSideParser : public EnumParser<RASwapSideParser, RangeAccrualSwap::Side>
{
public:
	void BuildEnumMap()
	{
		AddEnum( L"Pay", RangeAccrualSwap::Payer );
		AddEnum( L"Receive", RangeAccrualSwap::Receiver );
	}
};

RADaishinParam::RADaishinParam()
	: m_side( RangeAccrualSwap::Payer )
{
}

std::vector<Date> CalcNInRange( Date evalDate, const std::wstring& code, const Schedule& schedule, const std::vector<Real>& upper, const std::vector<Real>& lower )
{
	evalDate = Date( evalDate.serialNumber() );
	if( evalDate < *schedule.begin() )
	{
		return std::vector<Date>();
	}

	Date startDate;
	std::vector<Real>::const_iterator iterLower = lower.begin();
	std::vector<Real>::const_iterator iterUpper = upper.begin();

	for( Schedule::const_iterator iter = schedule.begin() + 1; iter != schedule.end(); ++iter )
	{
		if( *iter > evalDate )
		{
				startDate = *( iter - 1 );
				break;
		}
		if( iterLower + 1 != lower.end() )
		{
			iterLower++;
		}
		if( iterUpper + 1 != upper.end() )
		{
			iterUpper++;
		}
	}

	DayPriceVec data = ::Blphvec( ::ConvertToBloombergCode( code ), L"last price", startDate, evalDate, L"DAILY", 400, L"ALL_CALENDAR_DAYS" );
	std::vector<Date> res;
	for each( const DayPrice& pr in data )
	{
		if( pr.second / 100. >= *iterLower && pr.second / 100. <= *iterUpper )
		{
			res.push_back( Date( pr.first ) );
		}
	}
	return res;
}

void RADaishinParam::SetDataImpl( TiXmlElement* record )
{
	std::vector<std::string> codes = ::Split( GetProductName(), boost::is_any_of( "_" ) );
	if( codes.size() == 1 )
	{
		m_biasForScenario = 0.;
	}
	else
	{
		m_biasForScenario = boost::lexical_cast<Real>( codes[ 1 ] ) / 10000.;
	}

	m_isSwap = XMLStrValue( record, PI_Type ) == L"RASwap" || XMLStrValue( record, PI_Type ) == L"RASwapCMS" || XMLStrValue( record, PI_Type ) == L"RASwapSingleTree" || XMLStrValue( record, PI_Type ) == L"RAFDMSwap";

	m_notional = XMLValue( record, PI_Notional );
	m_discountCurveInfo = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_DiscountRateCurve ) );
	AddCurve( m_discountCurveInfo->GetYieldCurveWrapper()->GetCurveName(), m_discountCurveInfo, false );

	m_obs1CurveInfo = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_RAOBS1Currency1 ) );
	AddCurve( m_obs1CurveInfo->GetYieldCurveWrapper()->GetCurveName(), m_obs1CurveInfo, true );

	m_obs1Ticker = XMLStrValue( record, PI_RAOBS1Ticker1 );

	if( record->FirstChildElement( PI_RASRho1Disc ) == NULL )
	{
		m_rho1disc = CurveTable::instance().GetCorr( m_discountCurveInfo->GetYieldCurveWrapper()->GetCurveName(), m_obs1CurveInfo->GetYieldCurveWrapper()->GetCurveName() );
		TiXmlElement rho1Disc( PI_RASRho1Disc );
		rho1Disc.SetAttribute( "type", "double" );
		rho1Disc.SetAttribute( "value", ::ToString( m_rho1disc ) );

		record->InsertEndChild( rho1Disc );
	}
	else
	{
		m_rho1disc = XMLValue( record, PI_RASRho1Disc );
	}	

	Period couponTenor = TenorParser::ParseEnum( XMLValue( GetRecord().get(), PI_Tenor ) );
	Date effectiveDate = XMLValue( record, PI_EffectiveDate );
	Date terminationDate = XMLValue( record, PI_TerminationDate );

	Calendar cal;
	std::vector<std::wstring> calendars = Split( XMLValue( record, PI_Calendar ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );

	switch( calendars.size() )
	{
	case 1:
		cal = CalendarParser::ParseEnum( XMLValue( record, PI_Calendar ) );
		break;
	case 2:
		cal = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ) );
		break;
	case 3:
		cal = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ) );
		break;
	case 4:
		cal = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ), CalendarParser::ParseEnum( calendars[ 3 ] ) );
		break;
	}	

	m_bd = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDC ) );
	BusinessDayConvention tbd = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDCTerminal ) );
	DateGeneration::Rule rule = DateGenerationRuleParser::ParseEnum( XMLValue( record, PI_DayGenerationRule ) );
	bool endOfDate = XMLValue( record, PI_EOM );
	Date firstDate = XMLValue( record, PI_FirstDate );
	Date nextToLastDate = XMLValue( record, PI_NextToLastdate );

	m_schedule = Schedule( effectiveDate, terminationDate, couponTenor, cal, m_bd, tbd, rule, endOfDate, firstDate, nextToLastDate );
	m_accrualDayCounter = DayCounterParser::ParseEnum( XMLValue( record, PI_DayCounterAccrual ) );

	if( XMLValue(record,PI_AccrualRate).GetType() == L"string" )
	{
		m_accrualRate = SplitStrToRealVector( XMLValue( record, PI_AccrualRate ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	}
	else
	{
		m_accrualRate.push_back( XMLValue( record, PI_AccrualRate ) );
	}

	if( XMLValue(record,PI_RAOBS1LowerLevel).GetType() == L"string" )
	{
		m_lowerBound1 = SplitStrToRealVector( XMLValue( record, PI_RAOBS1LowerLevel ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	}
	else
	{
		m_lowerBound1.push_back( XMLValue( record, PI_RAOBS1LowerLevel ) );
	}

	if( XMLValue( record, PI_RAOBS1UpperLevel ).GetType() == L"string" )
	{
		m_upperBound1 = SplitStrToRealVector( XMLValue( record, PI_RAOBS1UpperLevel ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	}
	else
	{
		m_upperBound1.push_back( XMLValue( record, PI_RAOBS1UpperLevel ) );
	}

	if(	XMLValue( record, PI_RAOBS2Index1 ).GetNullableValue<std::wstring>() != L"" )
	{
		m_obs2CurveInfo = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_RAOBS2Currency1 ) );
		AddCurve( m_obs2CurveInfo->GetCurveName(), m_obs2CurveInfo, true );

		m_obs2Ticker = XMLStrValue( record, PI_RAOBS2Ticker1 );

		if( XMLValue(record,PI_RAOBS2LowerLevel).GetType() == L"string" )
		{
			m_lowerBound2 = SplitStrToRealVector( XMLValue( record, PI_RAOBS2LowerLevel ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
		}
		else
		{
			m_lowerBound2.push_back( XMLValue( record, PI_RAOBS2LowerLevel ) );
		}

		if( XMLValue( record, PI_RAOBS2UpperLevel ).GetType() == L"string")
		{
			m_upperBound2 = SplitStrToRealVector( XMLValue( record, PI_RAOBS2UpperLevel ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
		}
		else
		{
			m_upperBound2.push_back( XMLValue( record, PI_RAOBS2UpperLevel ) );
		}

		if( record->FirstChildElement( PI_RASRho2Disc ) == NULL )
		{
			m_rho2disc = CurveTable::instance().GetCorr( m_obs2CurveInfo->GetYieldCurveWrapper()->GetCurveName(), m_discountCurveInfo->GetYieldCurveWrapper()->GetCurveName() );
			TiXmlElement rho2Disc( PI_RASRho2Disc );
			rho2Disc.SetAttribute( "type", "double" );
			rho2Disc.SetAttribute( "value", ::ToString( m_rho2disc ) );

			record->InsertEndChild( rho2Disc );
		}
		else
		{
			m_rho2disc = XMLValue( record, PI_RASRho2Disc );
		}	

		if( record->FirstChildElement( PI_RASRho12 ) == NULL )
		{
			m_rho12 = CurveTable::instance().GetCorr( m_obs1CurveInfo->GetYieldCurveWrapper()->GetCurveName(), m_obs2CurveInfo->GetYieldCurveWrapper()->GetCurveName() );
			TiXmlElement rho12( PI_RASRho12 );
			rho12.SetAttribute( "type", "double" );
			rho12.SetAttribute( "value", ::ToString( m_rho12 ) );

			record->InsertEndChild( rho12 );
		}
		else
		{
			m_rho12 = XMLValue( record, PI_RASRho12 );
		}		
	}

	m_side = RASwapSideParser::ParseEnum( XMLValue( record, PI_RASPayRec ) );

	if( m_isSwap || XMLValue( record, PI_RAInvGearing ) != 0. )
	{
		m_alpha = XMLValue( record, PI_RASPayerPaymentSpread );

		Period swapCouponTenor = TenorParser::ParseEnum( XMLValue( GetRecord().get(), PI_RASPayerTenor ) );
		Date swapEffectiveDate = XMLValue( record, PI_RASPayerEffectiveDate );
		Date swapTerminationDate = XMLValue( record, PI_RASPayerTerminationDate );

		Calendar swapCal = CalendarParser::ParseEnum( XMLValue( record, PI_RASPayerCalendar ) );
		BusinessDayConvention swapbd = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_RASPayerBDC ) );
		BusinessDayConvention swaptbd = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_RASPayerBDC_Terminal ) );
		DateGeneration::Rule swapRule = DateGenerationRuleParser::ParseEnum( XMLValue( record, PI_RASPayerRule ) );
		bool swapEndOfDate = XMLValue( record, PI_RASPayerEOM );
		Date swapFirstDate = XMLValue( record, PI_RASPayerFirstDate );
		Date swapNextToLastDate = XMLValue( record, PI_RASPayerNextToLastDate );

		m_swapSchedule = Schedule( swapEffectiveDate, swapTerminationDate, swapCouponTenor, swapCal, swapbd, swaptbd, swapRule, swapEndOfDate, swapFirstDate, swapNextToLastDate );

		Date recentFixDate = m_swapSchedule.dates().front();
		Date today( PricingSetting::instance().GetEvaluationDate().serialNumber() );
		for( std::vector<Date>::const_reverse_iterator iter = m_swapSchedule.dates().rbegin(); iter != m_swapSchedule.dates().rend(); ++iter )
		{
			if( *iter <= PricingSetting::instance().GetEvaluationDate() )
			{
				recentFixDate = *iter;
				break;
			}
		}

		if( record->FirstChildElement( PI_RASPayerPastFixing ) == NULL )
		{
			m_pastFixing = ::Blph<xmlrpc_c::value_double>( m_obs1Ticker, L"last price", recentFixDate ) / 100.;
			TiXmlElement pastFixing( PI_RASPayerPastFixing );
			pastFixing.SetAttribute( "type", "double" );
			pastFixing.SetAttribute( "value", ::ToString( m_pastFixing ) );

			record->InsertEndChild( pastFixing );
		}
		else
		{
			m_pastFixing = XMLValue( record, PI_RASPayerPastFixing );
		}

		if( XMLValue( record, PI_RAInvGearing ) != 0. )
		{
			m_invGearing = XMLValue( record, PI_RAInvGearing );
			m_invCap = XMLValue( record, PI_RAInvCap );
			m_invFloor = XMLValue( record, PI_RAInvFloor );

			m_invFixing = ::Blph<xmlrpc_c::value_double>( m_obs1Ticker, L"last price", recentFixDate ) / 100.;;
		}
		else
		{
			m_invGearing = m_invFixing = m_invCap = m_invFloor = Null<Real>();
		}
	}
	else
	{
		m_pastFixing = 0.;
		m_invGearing = m_invFixing = m_invCap = m_invFloor = Null<Real>();
	}

	if( XMLValue( record, PI_RAInvAlpha ) != 0. )
	{
		m_invAlpha = XMLValue( record, PI_RAInvAlpha );
	}
	else
	{
		m_invAlpha = 0.;
	}

	if( XMLValue( record, PI_Gearings ).GetType() == L"string" )
	{
		m_gearing = SplitStrToRealVector( XMLValue( record, PI_Gearings ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	}
	else
	{
		m_gearing = std::vector<Real>( 1, XMLValue( record, PI_Gearings ).GetNullableValue<Real>() );
		if( m_gearing.front() == 0. || m_gearing.front() == Null<Real>() )
		{
			m_gearing.front() = QL_MIN_POSITIVE_REAL;
		}
	}
	
	m_calcDelta = XMLValue( record, PI_CalcDelta ) == L"Y";
	m_calcChunkDelta = XMLValue( record, PI_CalcChunkDelta ) == L"Y";
	m_calcVega = XMLValue( record, PI_CalcVega ) == L"Y";
	m_calcXGamma = XMLValue( record, PI_CalcXGamma ) == L"Y";
}

QuantLib::Period RADaishinParam::GetRemainingTime() const 
{
	return static_cast<int>( m_schedule.endDate() - PricingSetting::instance().GetEvaluationDate() + 1. ) * Days;
}

