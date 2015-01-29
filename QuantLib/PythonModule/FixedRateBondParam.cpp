#include "StdAfx.h"

#include "FixedRateBondParam.h"
#include "PricingSetting.h"

#include "ProductIndex.h"
#include "XMLValue.h"

#include "ParamParseUtil.h"

#include "YieldCurveInfoWrapperProxy.h"
#include "InterestRateCurveInfoWrapper.h"
#include "BloombergCaller.h"

#include "CurveTable.h"
#include "PricingSetting.h"

#include "yield_builder.hpp"

//#include "PerProcessSingleton.h"
#include "StringUtil.h"

#include "EnumParser.h"

#include <ql/instruments/vanillaswap.hpp>



class FixedRateBondSideParser : public EnumParser<FixedRateBondSideParser, FixedRateBondParam::Type >
{
public:
	void BuildEnumMap();
};

void FixedRateBondSideParser::BuildEnumMap()
{
	AddEnum( L"Buy", FixedRateBondParam::Buy );
	AddEnum( L"Sell", FixedRateBondParam::Sell );
}

FixedRateBondParam::FixedRateBondParam()
{
}

void FixedRateBondParam::LoadScheduleInfo( const TiXmlElement* record )
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

	m_schedule.reset( new Schedule(m_effectiveDate, m_terminationDate, tTenor, m_Calendar, m_BDCschedule, tTerminationBDC, tRule, tEOM)); //, tFirstDate, tNextToLastDate ));

	m_issueDate = XMLValue( record, PI_ISSUEDATE );
}


void FixedRateBondParam::LoadLegInfo( const TiXmlElement* record)
{
	m_fixedRate = XMLValue( record, PI_FixedRate );
	m_FixedDayCounter = DayCounterParser::ParseEnum( XMLValue( record, PI_FixedDayCounter ) );
}


void FixedRateBondParam::LoadCurveInfo( const TiXmlElement* record )
{
	m_discountCurveInfo = m_DCCurveProxy = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_DiscountRateCurve ) );
	AddCurve( m_DCCurveProxy->GetYieldCurveWrapper()->GetCurveName(), m_DCCurveProxy, false );
}

void FixedRateBondParam::SetDataImpl( TiXmlElement* record )
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

	m_side = FixedRateBondSideParser::ParseEnum( XMLValue( record, PI_BuySell ).GetValue<std::wstring>() );

	m_calcDelta = XMLValue( record, PI_CalcDelta ) == L"Y";
	m_calcChunkDelta = XMLValue( record, PI_CalcChunkDelta ) == L"Y";
	m_calcVega = XMLValue( record, PI_CalcVega ) == L"Y";
	m_notional = XMLValue( record, PI_Notional );

	m_strCurrency = XMLValue( record, PI_Currency ).GetValue<const char*>();

	m_effectiveDate = XMLValue( record, PI_EffectiveDate ) ;
	m_terminationDate = XMLValue( record, PI_TerminationDate ) ;
	
	LoadScheduleInfo( record );
	LoadLegInfo( record );
	LoadCurveInfo( record );
}

ResultInfo FixedRateBondParam::DoCalculation()
{
	boost::shared_ptr<YieldTermStructure> dcTermStructure = build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_DCCurveProxy->GetYieldCurveWrapper() )->GetCurveData() );
	boost::shared_ptr<PricingEngine> bondEngine( new DiscountingBondEngine( Handle<YieldTermStructure>( dcTermStructure ) ) );

	Settings::instance().evaluationDate() = PricingSetting::instance().GetEvaluationDate();
	FixedRateBond fixedRateBond(
		1,
		m_notional,
		*m_schedule,
		std::vector<Rate>(1,m_fixedRate),
		m_FixedDayCounter,
		m_BDCschedule,
		100.0,
		m_issueDate,
		m_Calendar);

	fixedRateBond.setPricingEngine(bondEngine);

	ResultInfo price;

	price.npv = fixedRateBond.NPV();
	price.error = fixedRateBond.dirtyPrice();

	if (m_side == FixedRateBondParam::Sell)
	{
		price.npv *= -1.;
	}

	return price;
}

QuantLib::Period FixedRateBondParam::GetRemainingTime() const 
{
	return static_cast<Size>( m_terminationDate - PricingSetting::instance().GetEvaluationDate() ) * Days;
}

std::vector<std::pair<Date, Real> > FixedRateBondParam::GetCashFlow() const 
{
	boost::shared_ptr<YieldTermStructure> dcTermStructure = build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_DCCurveProxy->GetYieldCurveWrapper() )->GetCurveData() );
	boost::shared_ptr<PricingEngine> bondEngine( new DiscountingBondEngine( Handle<YieldTermStructure>( dcTermStructure ) ) );

	Settings::instance().evaluationDate() = PricingSetting::instance().GetEvaluationDate();
	FixedRateBond fixedRateBond(
		1,
		m_notional,
		*m_schedule,
		std::vector<Rate>(1,m_fixedRate),
		m_FixedDayCounter,
		m_BDCschedule,
		100.0,
		m_issueDate,
		m_Calendar);

	fixedRateBond.setPricingEngine(bondEngine);

	const Leg& cashFlows = fixedRateBond.cashflows();

	std::vector<std::pair<Date, Real> > result;

	result.push_back( std::make_pair( PricingSetting::instance().GetEvaluationDate(), fixedRateBond.accruedAmount() ) );
	for each( boost::shared_ptr<CashFlow> cashFlow in cashFlows )
	{
		if( !cashFlow->hasOccurred() )
		{
			result.push_back( std::make_pair( cashFlow->date(), cashFlow->amount() ) );
		}		
	}

	return result;
}
