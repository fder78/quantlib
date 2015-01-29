#include "StdAfx.h"

#include "FXDigitalParam.h"
#include "StringUtil.h"
#include "ProductIndex.h"
#include "XMLValue.h"

#include "CurveTable.h"
#include "EnumParser.h"
#include "PricingSetting.h"
#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"

#include "BloombergCaller.h"

class OptionTypeParser : public EnumParser<OptionTypeParser, Option::Type>
{
public:
	void BuildEnumMap()
	{
		AddEnum( L"Put", Option::Put );
		AddEnum( L"Call", Option::Call );
	}
};

inline std::wstring ExtractCurrencyCode( const std::wstring& src )
{
	return ::Split( src, boost::is_any_of( L"_" ) )[ 0 ];
}

void FXDigitalParam::Calculate()
{
	Date evalDate = PricingSetting::instance().GetEvaluationDate();
	Settings::instance().evaluationDate() = evalDate;

	std::wstring blpCode = ::ExtractCurrencyCode( m_underlyingCode ) + L"KRW Curncy";
	Real s = ::Blph<xmlrpc_c::value_double>( blpCode, L"last price", SouthKorea().advance( evalDate, -1 * Days, Preceding ) );

	/*Market Data*/
	boost::shared_ptr<Quote> spot( new SimpleQuote( s / m_basePrice * 100. ) );
	
	boost::shared_ptr<GeneralizedBlackScholesProcess> bsmProcess( boost::dynamic_pointer_cast<GeneralizedBlackScholesProcess>( m_underlyingCurve->GetYieldCurveWrapper()->GetStochasticProcess( 1 * Years ) ) );
	
	/*Product Spec*/
	boost::shared_ptr<Exercise> europeanExercise( new EuropeanExercise( m_terminationDate ) );

	boost::shared_ptr<StrikedTypePayoff> payoff;
	if( m_rebate > 0 )
	{
		payoff = boost::shared_ptr<StrikedTypePayoff>( new CashOrNothingPayoff( m_optionType, m_strike * 100., m_rebate * 100. ) );
	}
	else
	{
		payoff = boost::shared_ptr<StrikedTypePayoff>( new AssetOrNothingPayoff( m_optionType, m_strike * 100. ) );
	}

	VanillaOption europeanOption(payoff, europeanExercise);

	/*Pricing Engine*/
	europeanOption.setPricingEngine(
		boost::shared_ptr<PricingEngine>(
		new AnalyticEuropeanEngine(bsmProcess)));

	/*°á°ú: price & Greeks*/
	m_result = europeanOption.NPV();
}

void FXDigitalParam::FetchResult()
{
	TiXmlElement npvNode( "npv" );
	npvNode.SetAttribute( "type", "double" );
	npvNode.SetAttribute( "value", ::ToString( ::ToWString(  m_result * m_notional / 100. ) ) );
	GetResultObject()->InsertEndChild( npvNode );

	TiXmlElement unitPrice( "unitPrice" );
	unitPrice.SetAttribute( "type", "double" );
	unitPrice.SetAttribute( "value", ::ToString( ::ToWString( m_result * 100. ) ) );
	GetResultObject()->InsertEndChild( unitPrice );

	TiXmlElement notional( "notional" );
	notional.SetAttribute( "type", "double" );
	notional.SetAttribute( "value", ::ToString( ::ToWString( m_notional ) ) );
	GetResultObject()->InsertEndChild( notional );

	TiXmlElement error( "error" );
	error.SetAttribute( "type", "double" );
	error.SetAttribute( "value", ::ToString( ::ToWString( m_result / m_notional ) ) );
	GetResultObject()->InsertEndChild( error );

	TiXmlElement evalTime( "evalTime" );
	evalTime.SetAttribute( "type", "string" );
	evalTime.SetAttribute( "value", ::ToString( ::ToWString( Date::todaysDate() ) + L" " + ::ToWString( Date::todaysDate().second() ) ) );
	GetResultObject()->InsertEndChild( evalTime );
}

void FXDigitalParam::SetDataImpl( TiXmlElement* record )
{
	m_notional = XMLValue( record, PI_Notional );
	m_effectiveDate = XMLValue( record, PI_EffectiveDate );
	m_terminationDate = XMLValue( record, PI_TerminationDate );
	
	m_rebate = XMLValue( record, PI_Rebate );
	m_slope = XMLValue( record, PI_Slope );
	m_strike = XMLValue( record, PI_Strike );
	m_optionType = OptionTypeParser::ParseEnum( XMLValue( record, PI_CallPut ) );

	m_underlyingCode = XMLStrValue( record, PI_FXDigitalUnderlyingCode );
	m_basePrice = XMLValue( record, PI_FXDigitalUnderlyingBasePrice );
	m_vol = XMLValue( record, PI_FXDigitalUnderlyingVol ).GetNullableValue<Real>();

	if( m_vol > 0. )
	{
		CurveTable::instance().SetVol( m_underlyingCode, m_vol );
	}

	m_underlyingCurve = CurveTable::instance().GetYieldCurveProxy( m_underlyingCode );

	m_calcDelta = XMLValue( record, PI_CalcDelta ) == L"Y";
	m_calcVega = XMLValue( record, PI_CalcVega ) == L"Y";
	m_calcXGamma = XMLValue( record, PI_CalcXGamma ) == L"Y";
}
