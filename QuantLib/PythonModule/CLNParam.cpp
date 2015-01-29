#include "StdAfx.h"

#include "CLNParam.h"

#include "PricingSetting.h"

#include "ProductIndex.h"
#include "XMLValue.h"

#include "ParamParseUtil.h"

#include "CurveTable.h"
#include "PricingSetting.h"

#include "InterestRateCurveInfoWrapper.h"

#include "yield_builder.hpp"
#include "../../Daishin_Credit_Derivatives/cln_NtD.hpp"
#include "../../Daishin_Credit_Derivatives/cln_singlename.hpp"
#include "../../Daishin_Credit_Derivatives/credit_result.hpp"

void CLNParam::SetDataImpl( TiXmlElement* record )
{
	m_book = XMLValue( record, PI_Book ).GetValue<const char*>();
	m_todaysDate = PricingSetting::instance().GetEvaluationDate();

	// Nominal
	m_nominal = std::make_pair<Real, Real>( XMLValue( record, PI_Notional1 ), XMLValue( record, PI_Notional2 ) );
	m_cpnRate = XMLValue( record, PI_Coupon );
	m_dayCounter = DayCounterParser::ParseEnum( XMLValue( record, PI_DayCounter ) );

	// Scheduling Info
	m_effectiveDate = XMLValue( record, PI_EffectiveDate );
	m_terminationDate = XMLValue( record, PI_TerminationDate );

	m_tenor = TenorParser::ParseEnum( XMLValue( record, PI_Tenor ) );
	std::vector<std::wstring> calendars = Split( XMLValue( record, PI_Calendar ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );

	switch( calendars.size() )
	{
	case 1:
		m_calendar = CalendarParser::ParseEnum( XMLValue( record, PI_Calendar ) );
		break;
	case 2:
		m_calendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ) );
		break;
	case 3:
		m_calendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ) );
		break;
	case 4:
		m_calendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ), CalendarParser::ParseEnum( calendars[ 3 ] ) );
		break;
	}	

	m_convention = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDC ) );
	m_terminationDateConvention = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDCTerminal ) );

	m_rule = DateGenerationRuleParser::ParseEnum( XMLValue( record, PI_DayGenerationRule ) );
	m_endOfMonth = XMLValue( record, PI_EOM );

	m_firstDate = XMLValue( record, PI_FirstDate );
	m_nextToLastDate = XMLValue( record, PI_NextToLastdate );

	// Market Info
	m_tsCurve_rf = Handle<YieldTermStructure>( ::build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_RfRateCurve ), ShiftOption::ShiftNothing ) )->GetCurveData() ) );
	m_tsCurve_disc = Handle<YieldTermStructure>( ::build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_DiscountRateCurve ), ShiftOption::ShiftNothing ) )->GetCurveData() ) );

	boost::shared_ptr<FXCurveData> fxCurve = CurveTable::instance().GetFXCurve( XMLValue( record, PI_FXFwdCurve ), ShiftOption::ShiftNothing );
	m_fxVol = XMLValue( record, PI_FXVol );

	m_date_fxFwd = fxCurve->fwdDate;
	m_value_fxFwd = fxCurve->fwdValue;

	// Product Info
	std::pair<Real, Real> corrRcvry = CurveTable::instance().GetCorrRcvy( ::ToWString( GetProductName() ) );

	m_correlation = corrRcvry.first;
	m_recoveryRate = corrRcvry.second;

	std::wstring cdsPremiumStr( static_cast<const std::wstring&>( XMLValue( record, PI_CDSPremiumCurve ) ) );
	std::vector<std::wstring> cdsPremiumStrVec;

	boost::algorithm::split( cdsPremiumStrVec, cdsPremiumStr, boost::is_any_of( L"-/" ), boost::algorithm::token_compress_on );

	for each( const std::wstring& premimumStr in cdsPremiumStrVec )
	{
		std::wstring trimmed = premimumStr;
		boost::algorithm::trim( trimmed );

		boost::shared_ptr<CDSCurveData> cdsPremium = CurveTable::instance().GetCDSCurve( trimmed );

		m_tenors.push_back( cdsPremium->tenors );
		m_quoted_spreads.push_back( cdsPremium->quotedSpreads );
	}
}

void CLNParam::Calculate()
{
	CLNPricingResult res = CalcRes();

	{
		TiXmlElement resNode( "price" );

		resNode.SetAttribute( "value", ::ToString( res.price ) );
		resNode.SetAttribute( "type", "double" );

		GetResultObject()->InsertEndChild( resNode );
	}

	// CLN은 Protection/Premium이 뒤바뀌어 있음
	{
		TiXmlElement notional2( "notional2" );

		notional2.SetAttribute( "value", ::ToString( m_nominal.first ) );
		notional2.SetAttribute( "type", "double" );

		GetResultObject()->InsertEndChild( notional2 );
	}

	{
		TiXmlElement notional1( "notional1" );

		notional1.SetAttribute( "value", ::ToString( m_nominal.second ) );
		notional1.SetAttribute( "type", "double" );

		GetResultObject()->InsertEndChild( notional1 );
	}

	{
		TiXmlElement bookName( "book" );

		bookName.SetAttribute( "value", m_book );
		bookName.SetAttribute( "type", "string" );

		GetResultObject()->InsertEndChild( bookName );
	}

	{
		for( size_t i = 0; i < m_value_fxFwd.size(); i++ )
		{
			m_value_fxFwd[ i ] += 1.;
		}

		CLNPricingResult resp1 = CalcRes();

		for( size_t i = 0; i < m_value_fxFwd.size(); i++ )
		{
			m_value_fxFwd[ i ] -= 2.;
		}

		CLNPricingResult resm1 = CalcRes();

		Real delta = ( resp1.price - resm1.price ) * .5;
		Real gamma = ( resp1.price + resm1.price - 2 * res.price );

		{
			TiXmlElement fxDelta( "fxDelta" );

			fxDelta.SetDoubleAttribute( "value", delta );
			fxDelta.SetAttribute( "type", "double" );

			GetResultObject()->InsertEndChild( fxDelta );
		}

		{
			TiXmlElement fxGamma( "fxGamma" );

			fxGamma.SetDoubleAttribute( "value", gamma );
			fxGamma.SetAttribute( "type", "double" );

			GetResultObject()->InsertEndChild( fxGamma );
		}

		for( size_t i = 0; i < m_value_fxFwd.size(); i++ )
		{
			m_value_fxFwd[ i ] += 1.;
		}

		m_fxVol += 0.01;

		CLNPricingResult resv1 = CalcRes();

		{
			TiXmlElement fxVega( "fxVega" );

			fxVega.SetDoubleAttribute( "value", resv1.price - res.price );
			fxVega.SetAttribute( "type", "double" );

			GetResultObject()->InsertEndChild( fxVega );
		}
	}
}

QuantLib::CLNPricingResult CLNParam::CalcRes()
{
	CLNPricingResult res;
	switch( m_quoted_spreads.size() )
	{
	case 1:
		res = cln_singlename( m_todaysDate, m_nominal, m_cpnRate, m_dayCounter, m_tsCurve_rf, m_tsCurve_disc, m_date_fxFwd, m_value_fxFwd, m_fxVol, m_recoveryRate, m_tenors.front(), m_quoted_spreads.front(), m_effectiveDate, m_terminationDate, m_tenor, m_calendar, m_convention, m_terminationDateConvention, m_rule, m_endOfMonth, m_firstDate, m_nextToLastDate );
	default:
		res = cln_NtD( m_todaysDate, 1, m_nominal, m_cpnRate, m_dayCounter, m_tsCurve_rf, m_tsCurve_disc, m_date_fxFwd, m_value_fxFwd, m_fxVol, m_recoveryRate, m_correlation, m_tenors, m_quoted_spreads, m_effectiveDate, m_terminationDate, m_tenor, m_calendar, m_convention, m_terminationDateConvention, m_rule, m_endOfMonth, m_firstDate, m_nextToLastDate );
		break;
	}
	res.price *= -1;
	return res;
}
