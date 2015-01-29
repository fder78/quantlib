#include "StdAfx.h"

#include "RASingleTreeParam.h"
#include "pricing_functions/range_accrual_note_tree.hpp"
#include "pricing_functions/hull_white_calibration.hpp"

#include "PricingSetting.h"

#include "ProductIndex.h"
#include "XMLValue.h"

#include "ParamParseUtil.h"

#include "InterestRateCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "BloombergCaller.h"

#include "CurveTable.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "PricingSetting.h"

#include "yield_builder.hpp"

//#include "PerProcessSingleton.h"
#include "StringUtil.h"

#include "EnumParser.h"

#include "CurveTableUtil.h"

#include <ql/instruments/vanillaswap.hpp>

RASingleTreeParam::RASingleTreeParam()
{
}

void RASingleTreeParam::LoadCallScheduleInfo( const TiXmlElement* record )
{
	std::vector<std::wstring> calendars = Split( XMLValue( record, PI_Calendar ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	Calendar calendar;
	switch( calendars.size() )
	{
	case 1:
		calendar = CalendarParser::ParseEnum( XMLValue( record, PI_Calendar ) );
		break;
	case 2:
		calendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ) );
		break;
	case 3:
		calendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ) );
		break;
	case 4:
		calendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ), CalendarParser::ParseEnum( calendars[ 3 ] ) );
		break;
	}

	BusinessDayConvention tTerminationBDC = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDCTerminal ) );
	BusinessDayConvention bdc = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDC ) );
	DateGeneration::Rule tRule = DateGenerationRuleParser::ParseEnum( XMLValue( record, PI_DayGenerationRule ) );
	bool tEOM = XMLValue( record, PI_EOM );

	Date callStartDate = XMLValue( record, PI_RACallStartDate );
	Date callEndDate = XMLValue( record, PI_RACallEndDate );
	Period tCallTenor = TenorParser::ParseEnum( XMLValue( record, PI_CallTenor ) );
	m_callSchedule = Schedule( callStartDate, callEndDate, tCallTenor, calendar, bdc, tTerminationBDC, tRule, tEOM );
	if( XMLValue( record, PI_CallValue ).GetType() == L"string" )
	{
		m_callValues = SplitStrToRealVector( XMLValue( record, PI_CallValue ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	}
	else
	{
		m_callValues.clear();
	}
	
}

void RASingleTreeParam::LoadAccrualInfo( const TiXmlElement* record )
{
	Schedule::const_iterator iter = m_schedule.lower_bound( PricingSetting::instance().GetEvaluationDate() );
	Date termStartDate = m_schedule.dates().front();
	Date termEndDate = m_schedule.dates().back();
	if( iter != m_schedule.begin() )
	{
		termStartDate = *( iter - 1 );
	}
	if( iter != m_schedule.end() )
	{
		termEndDate = *iter;
	}

	size_t termIdx = std::distance( m_schedule.begin(), iter );

	// TODO: 2 index 구현
	std::vector<Date> inRangeDates;
	inRangeDates = ::CalcNInRange( PricingSetting::instance().GetEvaluationDate(), m_obs1Ticker, m_schedule, m_upperBound1, m_lowerBound1 );
	m_pastAccrualCnt = inRangeDates.size();
}

void RASingleTreeParam::SetDataImpl( TiXmlElement* record )
{
	__super::SetDataImpl(record);

	LoadAccrualInfo( record );
	LoadCallScheduleInfo( record );
}

ResultInfo RASingleTreeParam::DoCalculation()
{
	std::vector<Real> price;
	boost::shared_ptr<GeneralizedHullWhiteProcess> hp = boost::dynamic_pointer_cast<GeneralizedHullWhiteProcess>( m_obs1CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ) );
	QL_ASSERT( hp, "관찰커브는 금리만 가능합니다." );
	
	if( m_isSwap )
	{
		price = single_rangeaccrual( 
			PricingSetting::instance().GetEvaluationDate()
			, m_notional
			, m_accrualRate
			, m_gearing
			, m_schedule
			, m_accrualDayCounter
			, m_bd
			, m_lowerBound1
			, m_upperBound1
			, m_callSchedule
			, m_callValues
			, m_pastAccrualCnt
			, build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_obs1CurveInfo->GetYieldCurveWrapper() )->GetCurveData() )
			, hp->GetParam()
			, build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_discountCurveInfo->GetYieldCurveWrapper() )->GetCurveData() )
			, 1024
			, m_invAlpha, m_invGearing, m_invFixing, m_invCap, m_invFloor
			, m_alpha
			, m_pastFixing
			);
	}
	else
	{
		price = single_rangeaccrual( 
			PricingSetting::instance().GetEvaluationDate()
			, m_notional
			, m_accrualRate
			, m_gearing
			, m_schedule
			, m_accrualDayCounter
			, m_bd
			, m_lowerBound1
			, m_upperBound1
			, m_callSchedule
			, m_callValues
			, m_pastAccrualCnt
			, build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_obs1CurveInfo->GetYieldCurveWrapper() )->GetCurveData() )
			, hp->GetParam()
			, build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_discountCurveInfo->GetYieldCurveWrapper() )->GetCurveData() )
			, 1024
			, m_invAlpha, m_invGearing, m_invFixing, m_invCap, m_invFloor
			);
	}

	if( m_side == RangeAccrualSwap::Payer )
	{
		price[ 0 ] *= -1.;
	}

	ResultInfo info;
	info.npv = price[ 0 ];

	return info;
}

std::vector<std::pair<Date, Real> > RASingleTreeParam::GetCashFlow() const
{
	// time 제거용
	Date evalDate( PricingSetting::instance().GetEvaluationDate().serialNumber() );
	std::vector<Date>::const_iterator iter = std::lower_bound( m_schedule.dates().begin(), m_schedule.dates().end(), evalDate );
	if( iter != m_schedule.dates().end() && *iter == evalDate && iter != m_schedule.dates().begin() )
	{
		Size accrualIdx = std::min<size_t>( m_accrualRate.size() - 1, std::distance( m_schedule.dates().begin(), iter ) );
		std::vector<Date> inRangeDates( ::CalcNInRange( PricingSetting::instance().GetEvaluationDate() - 1, m_obs1Ticker, m_schedule, m_upperBound1, m_lowerBound1 ) );
		std::vector<std::pair<Date, Real> > result;
		result.push_back( std::make_pair( evalDate, m_accrualRate[ accrualIdx ] * static_cast<Real>( inRangeDates.size() ) / static_cast<Real>( m_accrualDayCounter.dayCount( *( iter - 1 ), *iter ) ) * m_notional / static_cast<Real>( m_schedule.tenor().frequency() ) ) );

		return result;
	}

	return std::vector<std::pair<Date, Real> >();
}
