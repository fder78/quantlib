#include "StdAfx.h"

#include "SwaptionParam.h"

#include "PricingSetting.h"

#include "ProductIndex.h"
#include "XMLValue.h"

#include "ParamParseUtil.h"

#include "InterestRateCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "BloombergCaller.h"

#include "CurveTable.h"
#include "PricingSetting.h"

#include "yield_builder.hpp"

//#include "PerProcessSingleton.h"
#include "StringUtil.h"

#include "EnumParser.h"

#include <ql/instruments/vanillaswap.hpp>
#include "pricing_functions/cap_floor_swaption.hpp"
#include "pricing_functions/hull_white_calibration.hpp"
#include "VanillaSwapParam.h"



class SwaptionSettlementParser : public EnumParser<SwaptionSettlementParser, Settlement::Type >
{
public:
	void BuildEnumMap();
};

void SwaptionSettlementParser::BuildEnumMap()
{
	AddEnum( L"Physical", Settlement::Physical );
	AddEnum( L"Cash", Settlement::Cash );
}

SwaptionParam::SwaptionParam()
{
}

void SwaptionParam::LoadScheduleInfo( const TiXmlElement* record )
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
	
	m_BDCpayment = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDCSwapPayment ) );
}

void SwaptionParam::LoadLegInfo( const TiXmlElement* record)
{
	m_fixedDayCounter = DayCounterParser::ParseEnum( XMLValue( record, PI_FixedDayCounter ) );
	m_BDCcoupon = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDC ) );

	m_spreadFloating = XMLValue( record, PI_FloatingSpread );
	m_floatingDayCounter = DayCounterParser::ParseEnum( XMLValue( record, PI_FloatingDayCounter ) );
}

void SwaptionParam::LoadCurveInfo( const TiXmlElement* record )
{
	m_discountCurveInfo = m_discountCurveProxy = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_DiscountRateCurve ) );

	AddCurve( m_discountCurveProxy->GetYieldCurveWrapper()->GetCurveName(), m_discountCurveProxy, true );
}	

void SwaptionParam::SetDataImpl( TiXmlElement* record )
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

	m_notional = XMLValue( record, PI_Notional );
	m_calcDelta = XMLValue( record, PI_CalcDelta ) == L"Y";
	m_calcChunkDelta = XMLValue( record, PI_CalcChunkDelta ) == L"Y";
	m_calcVega = XMLValue( record, PI_CalcVega ) == L"Y";
	m_calcXGamma = XMLValue( record, PI_CalcXGamma ) == L"Y";

	// General Info
	//m_isSwap = XMLValue( record, PI_Type ) == L"VanillaSwap";
	m_strTraderID = XMLValue(record, PI_TraderID).GetValue<const char*>();
	m_strCode = XMLValue(record, PI_Code).GetValue<const char*>();
	m_strBook = XMLValue(record, PI_Book).GetValue<const char*>();

	m_strBacktoBack = XMLValue(record,PI_BacktoBack).GetValue<const char*>();
	m_strExtCalculator = XMLValue(record,PI_ExtCalculator).GetValue<const char*>();
	m_strCounterParty = XMLValue(record,PI_CounterParty).GetValue<const char*>();

	m_side = VanillaSwapSideParser::ParseEnum( XMLValue( record, PI_PayRec ).GetValue<std::wstring>() );

	m_evalDate = XMLValue( record, PI_EvalDate );
	m_strCurrency = XMLValue( record, PI_Currency ).GetValue<const char*>();

	m_effectiveDate = XMLValue( record, PI_EffectiveDate ) ;
	m_terminationDate = XMLValue( record, PI_TerminationDate ) ;

	LoadScheduleInfo( record );
	LoadLegInfo( record );
	LoadCurveInfo( record );

	m_settlementType = SwaptionSettlementParser::ParseEnum( XMLValue( record, PI_SettlementType ).GetValue<std::wstring>() );
	m_strike = XMLValue( record, PI_Strike );
	m_exerciseDate = XMLValue( record, PI_ExerciseDate );
	m_fixingDays = XMLValue( record, PI_FixingDays );

	m_lbortmp = IborIndexParser::ParseEnum(XMLValue( record, PI_FloatingRate ) );
}

ResultInfo SwaptionParam::DoCalculation()
{
	ResultInfo res;

	boost::shared_ptr<YieldTermStructure> dc = build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_discountCurveProxy->GetYieldCurveWrapper() )->GetCurveData() );
	boost::shared_ptr<IborIndex> floatingIdx = m_lbortmp->clone( Handle<YieldTermStructure>( dc ) );

	Real vol = GetVolatility();

	floatingIdx->clearFixings();

	if( floatingIdx->isValidFixingDate( PricingSetting::instance().GetEvaluationDate() ) )
	{
		floatingIdx->addFixing( PricingSetting::instance().GetEvaluationDate(), boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_discountCurveProxy->GetYieldCurveWrapper() )->GetPastFixing() );
	}

	if( floatingIdx->isValidFixingDate( floatingIdx->fixingCalendar().advance( PricingSetting::instance().GetEvaluationDate(), -1, Days, Preceding, false ) ) )
	{
		floatingIdx->addFixing( floatingIdx->fixingCalendar().advance( PricingSetting::instance().GetEvaluationDate(), -1, Days, Preceding, false ), boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_discountCurveProxy->GetYieldCurveWrapper() )->GetPastFixing() );
	}

	if( m_lbortmp->fixingDays() > 1 )
	{
		if( floatingIdx->isValidFixingDate( floatingIdx->fixingCalendar().advance( PricingSetting::instance().GetEvaluationDate(), -1 * ( (int)m_lbortmp->fixingDays() ), Days, Preceding, false ) ) )
		{
			floatingIdx->addFixing( floatingIdx->fixingCalendar().advance( PricingSetting::instance().GetEvaluationDate(), -1 * ( (int)m_lbortmp->fixingDays() ), Days, Preceding, false ), boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_discountCurveProxy->GetYieldCurveWrapper() )->GetPastFixing() );
		}

	}

	try
	{
		floatingIdx->addFixing( m_recentFixDate, m_pastFixingFloating );
	}
	catch( ... )
	{
	}

	
	res.npv = swaption( m_evalDate
										, m_side
										, m_settlementType
										, m_strike
										, m_notional
										, m_exerciseDate
										, *m_scheduleFixed
										, m_fixedDayCounter
										, *m_scheduleFloating
										, m_floatingDayCounter
										, m_fixingDays
										, m_BDCpayment
										, floatingIdx
										, dc
										, vol
										);
	return res;
}

QuantLib::Period SwaptionParam::GetRemainingTime() const 
{
	return static_cast<int>( m_terminationDate - m_evalDate ) * Days;
}

std::vector<std::pair<Date, Real> > SwaptionParam::GetCashFlow() const 
{
	return std::vector<std::pair<Date, Real> >();
}

class ToDayPeriodFunctor : std::unary_function<Period, Period>
{
public:
	ToDayPeriodFunctor( const Date& today )
		: m_today( today )
	{
	}

	Period operator() ( const Period& in )
	{
		return static_cast<Size>( ( m_today + in ) - m_today ) * Days;
	}

private:
	Date m_today;
};

QuantLib::Real SwaptionParam::GetVolatility() const
{
	boost::shared_ptr<SwaptionVolData> swtVolData = CurveTable::instance().GetSwaptionVolData( m_discountCurveProxy->GetCurveName(), m_discountCurveProxy->GetShiftOption() );
	Date evalDate = m_evalDate;

	typedef std::set<Period> PeriodSet;
	typedef std::vector<Period> PeriodVec;
	PeriodVec matVec( swtVolData->maturities.begin(), swtVolData->maturities.end() );
	PeriodVec lenVec( swtVolData->lengths.begin(), swtVolData->lengths.end() );

	std::transform( matVec.begin(), matVec.end(), matVec.begin(), ToDayPeriodFunctor( m_evalDate ) );
	std::transform( lenVec.begin(), lenVec.end(), lenVec.begin(), ToDayPeriodFunctor( m_evalDate ) );

	PeriodSet matSet( matVec.begin(), matVec.end() );
	PeriodSet lenSet( lenVec.begin(), lenVec.end() );

	Size mat = static_cast<Size>( m_exerciseDate - evalDate );
	Size len = static_cast<Size>( m_terminationDate - m_exerciseDate );

	PeriodSet::iterator matIter = matSet.lower_bound( mat * Days );
	PeriodSet::iterator lenIter = lenSet.lower_bound( len * Days );

	int matIdx[ 2 ], lenIdx[ 2 ];
	matIdx[ 1 ] = static_cast<int>( std::distance( matSet.begin(), matIter ) );
	lenIdx[ 1 ] = static_cast<int>( std::distance( lenSet.begin(), lenIter ) );

	matIdx[ 0 ] = std::max( 0, matIdx[ 1 ] - 1 );
	lenIdx[ 0 ] = std::max( 0, lenIdx[ 1 ] - 1 );
	matIdx[ 1 ] = std::min( matIdx[ 1 ], static_cast<int>( matSet.size() ) - 1 );
	lenIdx[ 1 ] = std::min( lenIdx[ 1 ], static_cast<int>( lenSet.size() ) - 1 );

	Size lenSize = lenSet.size();
	Real vol = 0.;
	Real sumWeight = 0.;
	for( Size i = 0; i < 2; i++ )
	{
		for( Size j = 0; j < 2; j++ )
		{
			Size dataIdx = matIdx[ i ] * lenSize + lenIdx[ j ];
			Period nowMat = swtVolData->maturities[ dataIdx ];
			Period nowLen = swtVolData->lengths[ dataIdx ];
			Time matDiff = ( evalDate + nowMat ) - m_exerciseDate;
			Time lenDiff = ( ( evalDate + nowLen + nowMat ) - ( evalDate + nowMat ) ) - ( m_terminationDate - m_exerciseDate );
			Real dist = std::sqrt( matDiff * matDiff + lenDiff + lenDiff );
			if( dist == 0. )
			{
				return swtVolData->vols[ dataIdx ];
			}
			sumWeight += 1. / dist;
			vol += 1. / dist * swtVolData->vols[ dataIdx ];
		}
	}

	return vol / sumWeight;
}
