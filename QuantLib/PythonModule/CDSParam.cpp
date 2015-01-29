#include "StdAfx.h"

#include "CDSParam.h"

#include "PricingSetting.h"

#include "ProductIndex.h"
#include "XMLValue.h"

#include "ParamParseUtil.h"

#include "CurveTable.h"
#include "PricingSetting.h"

#include "yield_builder.hpp"

#include "InterestRateCurveInfoWrapper.h"

#include "../../Daishin_Credit_Derivatives/credit_result.hpp"
#include "../../Daishin_Credit_Derivatives/cds_singlename.hpp"
#include "../../Daishin_Credit_Derivatives/cds_NtD.hpp"

void CDSParam::SetDataImpl( TiXmlElement* record )
{
	m_todaysDate = PricingSetting::instance().GetEvaluationDate();
	m_book = XMLValue( record, PI_Book ).GetValue<const char*>();

	// Nominal
	m_nominal = std::make_pair<Real, Real>( XMLValue( record, PI_Notional1 ), XMLValue( record, PI_Notional2 ) );
	m_premium = XMLValue( record, PI_Premium );
	m_upfront = XMLValue( record, PI_Upfront );
	m_side = ProtectionParser::ParseEnum( XMLValue( record, PI_ProtectionSide ) );
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
	m_tsCurve_protection = Handle<YieldTermStructure>( ::build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_DiscountProtection ), ShiftOption::ShiftNothing ) )->GetCurveData() ) );
	m_tsCurve_premium = Handle<YieldTermStructure>( ::build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_DiscountPremium ), ShiftOption::ShiftNothing ) )->GetCurveData() ) );

	XMLValue fx1val = XMLValue( record, PI_FXSpot1 );
	XMLValue fx2val = XMLValue( record, PI_FXSpot2 );

	Real fx1, fx2;

	if( fx1val.GetType() == L"string" )
	{
		fx1 = CurveTable::instance().GetFXCurve( XMLValue( record, PI_FXSpot1 ), ShiftOption::ShiftNothing )->fwdValue.front();
	}
	else
	{
		fx1 = fx1val;
	}

	if( fx2val.GetType() == L"string" )
	{
		fx2 = CurveTable::instance().GetFXCurve( XMLValue( record, PI_FXSpot2 ), ShiftOption::ShiftNothing )->fwdValue.front();
	}
	else
	{
		fx2 = fx2val;
	}

	m_fxSpot = std::make_pair( fx1, fx2 );

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

void CDSParam::Calculate()
{
	CDSPricingResult res = CalcRes();

	GetResultObject()->InsertEndChild( TiXmlElement( "price" ) )->ToElement()->SetAttribute( "value", ::ToString( res.price ) );
	GetResultObject()->InsertEndChild( TiXmlElement( "fairSpread" ) )->ToElement()->SetAttribute( "value", ::ToString( res.fairSpread ) );
	GetResultObject()->InsertEndChild( TiXmlElement( "defaultLeg" ) )->ToElement()->SetAttribute( "value", ::ToString( res.defaultLeg ) );
	GetResultObject()->InsertEndChild( TiXmlElement( "coupontLeg" ) )->ToElement()->SetAttribute( "value", ::ToString( res.couponLeg ) );

	{
		TiXmlElement notional1( "notional1" );

		notional1.SetAttribute( "value", ::ToString( m_nominal.first ) );
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
		TiXmlElement notional2( "notional2" );

		notional2.SetDoubleAttribute( "value", m_nominal.second );
		notional2.SetAttribute( "type", "double" );

		GetResultObject()->InsertEndChild( notional2 );
	}

	{
		if( m_fxSpot.first != m_fxSpot.second )
		{
			m_fxSpot.first *= 1.01;
		}
		else
		{
			m_fxSpot.second *= 1.01;
			m_fxSpot.first *= 1.01;
		}

		CDSPricingResult resp1 = CalcRes();

		if( m_fxSpot.first != m_fxSpot.second )
		{
			m_fxSpot.first /= 1.01 * 1.01;
		}
		else
		{
			m_fxSpot.second /= 1.01 * 1.01;
			m_fxSpot.first /= 1.01 * 1.01;
		}

		CDSPricingResult resm1 = CalcRes();

		Real delta = ( resp1.price - resm1.price ) * .5;
		Real gamma = ( resp1.price + resm1.price - 2 * res.price );

		{
			TiXmlElement fxDelta( "fxDelta" );

			fxDelta.SetAttribute( "value", ::ToString( delta / ( m_fxSpot.first * 1.01 * 0.01 ) ) );
			fxDelta.SetAttribute( "type", "double" );

			GetResultObject()->InsertEndChild( fxDelta );
		}

		{
			TiXmlElement fxGamma( "fxGamma" );

			fxGamma.SetAttribute( "value", ::ToString( gamma / ( m_fxSpot.first * 1.01 * 0.01 ) ) );
			fxGamma.SetAttribute( "type", "double" );

			GetResultObject()->InsertEndChild( fxGamma );
		}
	}
}

QuantLib::CDSPricingResult CDSParam::CalcRes()
{
	CDSPricingResult res;
	switch( m_quoted_spreads.size() )
	{
	case 1:
		res = cds_singlename( m_todaysDate, m_side, m_nominal, m_upfront, m_premium, m_dayCounter, m_tsCurve_protection, m_tsCurve_premium, m_fxSpot, m_recoveryRate, m_tenors.front(), m_quoted_spreads.front(), m_effectiveDate, m_terminationDate, m_tenor, m_calendar, m_convention, m_terminationDateConvention, m_rule, m_endOfMonth, m_firstDate, m_nextToLastDate );
	default:
		res = cds_NtD( m_todaysDate, 1, m_side, m_nominal, m_upfront, m_premium, m_dayCounter, m_tsCurve_protection, m_tsCurve_premium, m_fxSpot, m_recoveryRate, m_correlation, m_tenors, m_quoted_spreads, m_effectiveDate, m_terminationDate, m_tenor, m_calendar, m_convention, m_terminationDateConvention, m_rule, m_endOfMonth, m_firstDate, m_nextToLastDate );
		break;
	}

	return res;
}
