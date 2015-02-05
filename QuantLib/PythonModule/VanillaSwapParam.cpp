#include "StdAfx.h"
#include <iostream>
#include <boost/timer.hpp>

#include "VanillaSwapParam.h"

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

#include <ql/instruments/vanillaswap.hpp>

void VanillaSwapSideParser::BuildEnumMap()
{
	AddEnum( L"Payer", VanillaSwap::Payer );
	AddEnum( L"Receiver", VanillaSwap::Receiver );
}

VanillaSwapParam::VanillaSwapParam()
{
}

void VanillaSwapParam::LoadScheduleInfo( const TiXmlElement* record )
{
	Period tTenor = TenorParser::ParseEnum( XMLValue( record, PI_Tenor ) );
	std::vector<std::wstring> calendars = Split( XMLValue( record, PI_Calendar ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );

	switch( calendars.size() )
	{
	case 1:
		m_Calendar = CalendarParser::ParseEnum( XMLValue( record, PI_Calendar ) );
		break;
	case 2:
		m_Calendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ) );
		break;
	case 3:
		m_Calendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ) );
		break;
	case 4:
		m_Calendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ), CalendarParser::ParseEnum( calendars[ 3 ] ) );
		break;
	}	


	BusinessDayConvention tTerminationBDC = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDCTerminal ) );
	m_BDCschedule = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDC ) );
	DateGeneration::Rule tRule = DateGenerationRuleParser::ParseEnum( XMLValue( record, PI_DayGenerationRule ) );
	bool tEOM = XMLValue( record, PI_EOM );
	Date tFirstDate = Date();//XMLValue( record, PI_FirstDate );
	Date tNextToLastDate = Date();//XMLValue( record, PI_NextToLastdate );

	m_scheduleFixed.reset( new Schedule(m_effectiveDate, m_terminationDate, tTenor, m_Calendar, m_BDCschedule, tTerminationBDC, tRule, tEOM));//, tFirstDate, tNextToLastDate ));
	m_scheduleFloating = m_scheduleFixed;
}


void VanillaSwapParam::LoadLegInfo( const TiXmlElement* record)
{
	m_side = VanillaSwapSideParser::ParseEnum( XMLValue( record, PI_PayRec ).GetValue<std::wstring>() );
	m_fixedRate = XMLValue( record, PI_FixedRate );
	m_FixedDayCounter = DayCounterParser::ParseEnum( XMLValue( record, PI_FixedDayCounter ) );
	m_BDCcoupon = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDC ) );

	m_spreadFloating = XMLValue( record, PI_FloatingSpread );
	m_FloatingDayCounter = DayCounterParser::ParseEnum( XMLValue( record, PI_FloatingDayCounter ) );

	m_recentFixDate = m_scheduleFloating->dates().front();
	m_recentFixDate = m_Calendar.advance( m_recentFixDate, -1 * m_lbortmp->fixingDays(), Days, Preceding );
	Date today( PricingSetting::instance().GetEvaluationDate().serialNumber() );
	Real paymentLength = 0.;
	for( std::vector<Date>::const_reverse_iterator iter = m_scheduleFloating->dates().rbegin(); iter != m_scheduleFloating->dates().rend(); ++iter )
	{
		if( *iter < today )
		{
			m_recentFixDate = m_Calendar.advance( *iter, -1 * m_lbortmp->fixingDays(), Days, Preceding );
			break;
		}

		if( *iter == today && *iter != m_scheduleFloating->dates().front() )
		{
			paymentLength = m_FloatingDayCounter.yearFraction( *( iter + 1 ), *iter );
		}
	}
	boost::timer timer;
	m_pastFixingFloating = ::Blph<xmlrpc_c::value_double>( XMLValue( record, PI_RAOBS1Ticker1 ), L"last price", m_recentFixDate ) / 100.;
	std::cout<<timer.elapsed()<<std::endl;
	m_payment = ( m_pastFixingFloating - m_fixedRate ) * paymentLength * m_notional * ( ( m_side == VanillaSwap::Payer ) ? 1. : -1. );
}

void VanillaSwapParam::LoadCurveInfo( const TiXmlElement* record )
{
	m_discountCurveInfo = m_DCCurveProxy = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_DiscountRateCurve ) );
	AddCurve( m_DCCurveProxy->GetYieldCurveWrapper()->GetCurveName(), m_DCCurveProxy, false );

	m_RefCurveProxy = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_RefCurve ) );
	AddCurve( m_RefCurveProxy->GetYieldCurveWrapper()->GetCurveName(), m_RefCurveProxy, false );
}	

void VanillaSwapParam::SetDataImpl( TiXmlElement* record )
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

	// General Info
	//m_isSwap = XMLValue( record, PI_Type ) == L"VanillaSwap";
	m_strTraderID = XMLValue(record, PI_TraderID).GetValue<const char*>();
	m_strCode = XMLValue(record, PI_Code).GetValue<const char*>();
	m_strBook = XMLValue(record, PI_Book).GetValue<const char*>();

	m_strBacktoBack = XMLValue(record,PI_BacktoBack).GetValue<const char*>();
	m_strExtCalculator = XMLValue(record,PI_ExtCalculator).GetValue<const char*>();
	m_strCounterParty = XMLValue(record,PI_CounterParty).GetValue<const char*>();

	m_EvalDate = XMLValue( record, PI_EvalDate );
	m_notional = XMLValue( record, PI_Notional );
	m_strCurrency = XMLValue( record, PI_Currency ).GetValue<const char*>();

	m_calcDelta = XMLValue( record, PI_CalcDelta ) == L"Y";
	m_calcChunkDelta = XMLValue( record, PI_CalcChunkDelta ) == L"Y";
	m_calcVega = XMLValue( record, PI_CalcVega ) == L"Y";
	m_calcXGamma = XMLValue( record, PI_CalcXGamma ) == L"Y";

	m_effectiveDate = XMLValue( record, PI_EffectiveDate ) ;
	m_terminationDate = XMLValue( record, PI_TerminationDate ) ;

	m_lbortmp = IborIndexParser::ParseEnum(XMLValue( record, PI_FloatingRate ) );

	LoadScheduleInfo( record );
	LoadLegInfo( record );
	LoadCurveInfo( record );
}

Period VanillaSwapParam::GetRemainingTime() const
{
	return static_cast<Size>( m_terminationDate - PricingSetting::instance().GetEvaluationDate() ) * Days;
}

std::vector<std::pair<Date, Real> > VanillaSwapParam::GetCashFlow() const
{
	std::vector<std::pair<Date, Real> > result;
	Date evalDate( PricingSetting::instance().GetEvaluationDate().serialNumber() );
	std::vector<Date>::const_iterator iter = std::lower_bound( m_scheduleFloating->dates().begin(), m_scheduleFloating->dates().end(), evalDate );

	Real res = 0.;
	if( iter != m_scheduleFloating->dates().end() && *iter == evalDate )
	{
		res += m_pastFixingFloating * m_notional / static_cast<Real>( m_scheduleFloating->tenor().frequency() );
	}	

	iter = std::lower_bound( m_scheduleFixed->dates().begin(), m_scheduleFixed->dates().end(), evalDate );

	if( iter != m_scheduleFixed->dates().end() && *iter == evalDate )
	{
		res -= m_fixedRate * m_notional / static_cast<Real>( m_scheduleFixed->tenor().frequency() );
	}

	boost::shared_ptr<VanillaSwap> vanillaSwap = CreateInstrument();
	std::map<Date, Real> dateCashFlowMap;

	boost::shared_ptr<FixedRateCoupon> currentFixedLeg;
	boost::shared_ptr<IborCoupon> currentFloatingLeg;

	Real sign = ( m_side == VanillaSwap::Payer ) ? -1 : 1;

	for each( boost::shared_ptr<CashFlow> cf in vanillaSwap->fixedLeg() )
	{
		if( !cf->hasOccurred() )
		{
			dateCashFlowMap[ cf->date() ] += cf->amount() * sign;
			if( !currentFixedLeg )
			{
				currentFixedLeg = boost::dynamic_pointer_cast<FixedRateCoupon>( cf );
			}
		}
	}

	for each( boost::shared_ptr<CashFlow> cf in vanillaSwap->floatingLeg() )
	{
		if( !cf->hasOccurred() )
		{
			dateCashFlowMap[ cf->date() ] += cf->amount() * sign * -1.;
			if( !currentFloatingLeg )
			{
				currentFloatingLeg = boost::dynamic_pointer_cast<IborCoupon>( cf );
			}
		}
	}

	if( res > 0 )
	{
		result.push_back( std::make_pair( evalDate, ( m_side == VanillaSwap::Payer ) ? res : -res ) );
	}
	else
	{
		if( currentFloatingLeg && currentFixedLeg )
		{
			result.push_back( std::make_pair( evalDate, ( currentFixedLeg->accruedAmount( evalDate ) - currentFloatingLeg->accruedAmount( evalDate ) ) * sign ) );
		}		
	}

	result.insert( result.end(), dateCashFlowMap.begin(), dateCashFlowMap.end() );

	return result;
}

ResultInfo VanillaSwapParam::DoCalculation()
{
	boost::shared_ptr<VanillaSwap> vanillaSwap = CreateInstrument();
	ResultInfo info;
	info.npv = vanillaSwap->NPV();

	return info;
}

boost::shared_ptr<VanillaSwap> VanillaSwapParam::CreateInstrument() const
{
	boost::shared_ptr<YieldTermStructure> dcTermStructure = build_yield_curve( 
		*boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_DCCurveProxy->GetYieldCurveWrapper() )->GetCurveData() );

	boost::shared_ptr<IborIndex> floatingIdx = m_lbortmp->clone( Handle<YieldTermStructure>( dcTermStructure ) );

	floatingIdx->clearFixings();

	if( floatingIdx->isValidFixingDate( PricingSetting::instance().GetEvaluationDate() ) )
	{
		floatingIdx->addFixing( PricingSetting::instance().GetEvaluationDate(), boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_DCCurveProxy->GetYieldCurveWrapper() )->GetPastFixing() );
	}

	if( floatingIdx->isValidFixingDate( floatingIdx->fixingCalendar().advance( PricingSetting::instance().GetEvaluationDate(), -1, Days, Preceding, false ) ) )
	{
		floatingIdx->addFixing( floatingIdx->fixingCalendar().advance( PricingSetting::instance().GetEvaluationDate(), -1, Days, Preceding, false ), boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_DCCurveProxy->GetYieldCurveWrapper() )->GetPastFixing() );
	}

	if( m_lbortmp->fixingDays() > 1 )
	{
		if( floatingIdx->isValidFixingDate( floatingIdx->fixingCalendar().advance( PricingSetting::instance().GetEvaluationDate(), -1 * ( (int)m_lbortmp->fixingDays() ), Days, Preceding, false ) ) )
		{
			floatingIdx->addFixing( floatingIdx->fixingCalendar().advance( PricingSetting::instance().GetEvaluationDate(), -1 * ( (int)m_lbortmp->fixingDays() ), Days, Preceding, false ), boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_DCCurveProxy->GetYieldCurveWrapper() )->GetPastFixing() );
		}

	}

	try
	{
		floatingIdx->addFixing( m_recentFixDate, m_pastFixingFloating );
	}
	catch( ... )
	{
	}

	Settings::instance().evaluationDate() = PricingSetting::instance().GetEvaluationDate();
	boost::shared_ptr<PricingEngine> swapEngine( new DiscountingSwapEngine( Handle<YieldTermStructure>( dcTermStructure ) ) );
	boost::shared_ptr<VanillaSwap> vanillaSwap(new VanillaSwap(
		m_side,
		m_notional,
		*m_scheduleFixed,
		m_fixedRate,
		m_FixedDayCounter,
		*m_scheduleFloating,
		floatingIdx,
		m_spreadFloating,
		m_FloatingDayCounter,
		m_BDCschedule));

	vanillaSwap->setPricingEngine(swapEngine);
	return vanillaSwap;
}

