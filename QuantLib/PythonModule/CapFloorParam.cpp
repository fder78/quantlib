#include "StdAfx.h"

#include "CapFloorParam.h"

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

#include "pricing_functions/cap_floor_swaption.hpp"


class CapFloorSideParser : public EnumParser<CapFloorSideParser, CapFloor::Type >
{
public:
	void BuildEnumMap();
};

void CapFloorSideParser::BuildEnumMap()
{
	AddEnum( L"Cap", CapFloor::Cap );
	AddEnum( L"Floor", CapFloor::Floor );
	AddEnum( L"Collar", CapFloor::Collar );

}



void CapFloorParam::LoadScheduleInfo( const TiXmlElement* record )
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

	m_schedule.reset( new Schedule(m_effectiveDate, m_terminationDate, tTenor, m_Calendar, m_BDCschedule, tTerminationBDC, tRule, tEOM));//, tFirstDate, tNextToLastDate ));
	m_BDCpayment = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDCSwapPayment ) );

	m_fixingDays = XMLValue( record, PI_FixingDays );
}


void CapFloorParam::LoadLegInfo( const TiXmlElement* record)
{
	m_strike = XMLValue( record, PI_Strike );
	m_volatility = XMLValue( record, PI_Volatility );

//	m_DayCounter = DayCounterParser::ParseEnum( XMLValue( record, PI_FixedDayCounter ) );
	m_BDCcoupon = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDC ) );
}


void CapFloorParam::LoadCurveInfo( const TiXmlElement* record )
{

	m_DCCurveProxy = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_DiscountRateCurve ) );
	m_Ycurves.insert( std::make_pair( m_DCCurveProxy->GetYieldCurveWrapper()->GetCurveName(), m_DCCurveProxy ) );

	m_RefCurveProxy = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_RefCurve ) );
	m_Ycurves.insert(std::make_pair(m_RefCurveProxy->GetYieldCurveWrapper()->GetCurveName(), m_RefCurveProxy));


}	

void CapFloorParam::SetDataImpl( TiXmlElement* record )
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

	m_capFloor = CapFloorSideParser::ParseEnum( XMLValue( record, PI_CapFloor ).GetValue<std::wstring>() );

	m_EvalDate = XMLValue( record, PI_EvalDate );
	m_Nominal = XMLValue( record, PI_Notional );
	m_strCurrency = XMLValue( record, PI_Currency ).GetValue<const char*>();

	m_effectiveDate = XMLValue( record, PI_EffectiveDate ) ;
	m_terminationDate = XMLValue( record, PI_TerminationDate ) ;
	m_calcDelta = XMLValue( record, PI_CalcDelta ) == L"Y";
	m_calcVega = XMLValue( record, PI_CalcVega ) == L"Y";

	LoadScheduleInfo( record );
	LoadLegInfo( record );
	LoadCurveInfo( record );



	m_DCTermStructure = build_yield_curve( 
		*boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_DCCurveProxy->GetYieldCurveWrapper() )->GetCurveData() );

	boost::shared_ptr<PricingEngine> swapEngine(new DiscountingSwapEngine(m_DCTermStructureHandle));
	m_DCTermStructureHandle.linkTo(m_DCTermStructure);

	m_lbortmp = IborIndexParser::ParseEnum(XMLValue( record, PI_FloatingRate ) );
	m_FloatingRate = m_lbortmp->clone( Handle<YieldTermStructure>( m_DCTermStructure ) );

	m_FloatingRate->clearFixings();
	//m_FloatingRate->addFixing( m_recentFixDate, m_pastFixingFloating );


}

void CapFloorParam::Calculate()
{

	/*
	double deltastep, dDuration;
	std::wstring strTmp;

	std::vector<Real> price;

	m_DCCurveProxy->ShiftCurve( GetBiasForProduct() );

	price = DoCalculation();


	TiXmlElement npvNode( "npv" );
 	npvNode.SetAttribute( "type", "double" );
	npvNode.SetAttribute( "value", ::ToString( ::ToWString(  price[0] ) ) );
	GetResultObject()->InsertEndChild( npvNode );

	if( !PricingSetting::instance().CalcGreek() )
	{
		return;
	}
	
	//Key Rate Duration Calculation
	std::vector<Real> resshiftp[2], respd, resmd;
	std::vector<Real> resshiftm[2];

	//std::vector<double> keyExposures;
	double keyExposures;
	
	double dbFlag;
	
	dbFlag = m_DCCurveProxy->GetYieldCurveWrapper()->GetSpotValue();

	if( dbFlag < 0.005 )
		deltastep = 0.0002;
	else if( dbFlag < 0.1 )
		deltastep = 0.001;
	else
		deltastep = dbFlag * 0.00001;


	m_DCCurveProxy->ShiftCurve( deltastep );
	ApplyCurveInfo();
	resshiftp[ 0 ] = DoCalculation();

	m_DCCurveProxy->ShiftCurve( -deltastep );
	ApplyCurveInfo();
	resshiftm[ 0 ] = DoCalculation();

	dDuration  = -1. * ( resshiftp[0][0] - resshiftm[0][0] ) / 2 / deltastep / 10000.;

	m_DCCurveProxy->ShiftCurve( 0. );
	ApplyCurveInfo();


	TiXmlElement *kExpNodeTitle = new TiXmlElement("KeyExposure" );
	GetResultObject()->LinkEndChild( kExpNodeTitle );


	for each(const YCurveMap::value_type& v in m_Ycurves)
	{
		boost::shared_ptr<YieldCurveInfoWrapper> curveWrapper = v.second->GetYieldCurveWrapper();

		TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString(curveWrapper->GetCurveName() ) );
		kExpNodeTitle->LinkEndChild( kExpNodeType );

		dbFlag = curveWrapper->GetSpotValue();

		if( dbFlag < 0.005 )
			deltastep = 0.0002;
		else if( dbFlag < 0.1 )
			deltastep = 0.001;
		else
			deltastep = dbFlag * 0.00001;


		for(int i = 0; i< curveWrapper->GetTenorCount() - 1; i++)
		{
			v.second->ShiftCurvePerTenor( deltastep, i );

			ApplyCurveInfo();
			respd = DoCalculation();

			v.second->ShiftCurvePerTenor( -deltastep, i );
			ApplyCurveInfo();
			resmd = DoCalculation();

			keyExposures = -1. * ( respd[0] - resmd[0] ) / 2 / deltastep / 10000.;

			v.second->ShiftCurvePerTenor( 0., i );
			ApplyCurveInfo();

			std::ostringstream buf;
			buf << boost::format( "kexp%d" ) % i;

			TiXmlElement kExpNode( buf.str() );
			kExpNode.SetAttribute( "type", "double" );
			kExpNode.SetAttribute( "value", ::ToString( ::ToWString( keyExposures ) ) );
			kExpNodeType->InsertEndChild( kExpNode );
		}
	}



	TiXmlElement expNode( "exp1bp" );
	expNode.SetAttribute( "type", "double" );
	expNode.SetAttribute( "value", ::ToString( ::ToWString( dDuration ) ) );
	GetResultObject()->InsertEndChild( expNode );

	TiXmlElement curveName( "CurveName" );
	curveName.SetAttribute( "type", "string" );
	curveName.SetAttribute( "value", ::ToString( m_DCCurveProxy->GetYieldCurveWrapper()->GetCurveName() ) );
	GetResultObject()->InsertEndChild( curveName );

	
	TiXmlElement convNode( "convexity" );
	convNode.SetAttribute( "type", "double" );
	convNode.SetAttribute( "value", ::ToString( ::ToWString( dConvexity ) ) );
	GetResultObject()->InsertEndChild( convNode );

	TiXmlElement unitPrice( "unitPrice" );
	unitPrice.SetAttribute( "type", "double" );
	unitPrice.SetAttribute( "value", ::ToString( ::ToWString( price[0] / m_Nominal * 10000. ) ) );
	GetResultObject()->InsertEndChild( unitPrice );

	TiXmlElement notional( "notional" );
	notional.SetAttribute( "type", "double" );
	notional.SetAttribute( "value", ::ToString( ::ToWString( m_Nominal ) ) );
	GetResultObject()->InsertEndChild( notional );

	TiXmlElement error( "error" );
	error.SetAttribute( "type", "double" );
	error.SetAttribute( "value", ::ToString( ::ToWString( price[1] ) ) );
	GetResultObject()->InsertEndChild( error );

	TiXmlElement evalTime( "evalTime" );
	evalTime.SetAttribute( "type", "string" );
	evalTime.SetAttribute( "value", ::ToString( ::ToWString( Date::todaysDate() ) + L" " + ::ToWString( Date::todaysDate().second() ) ) );
	GetResultObject()->InsertEndChild( evalTime );


	TiXmlElement MarketDataDate( "MarketDataDate" );
	evalTime.SetAttribute( "type", "string" );
	evalTime.SetAttribute( "value", ::ToString( ::ToWString( m_EvalDate ) ) );
	GetResultObject()->InsertEndChild( MarketDataDate );
	
	m_DCCurveProxy->ShiftCurve( 0 );
*/


	double deltastep, dDuration, dGamma;
	std::wstring strTmp;
	Real shiftBias = GetBiasForProduct();

	std::vector<Real> price;



	m_DCCurveProxy->ShiftCurve( shiftBias );

	price = DoCalculation();



	TiXmlElement npvNode( "npv" );
 	npvNode.SetAttribute( "type", "double" );
	npvNode.SetAttribute( "value", ::ToString( ::ToWString(  price[0] ) ) );
	GetResultObject()->InsertEndChild( npvNode );

	if( !PricingSetting::instance().CalcGreek() )
	{
		return;
	}
	
	//Key Rate Duration Calculation
	std::vector<Real> resDCShift[5];
	//std::vector<double> keyExposures;
	
	double dbFlag;
	
	dbFlag = m_DCCurveProxy->GetYieldCurveWrapper()->GetSpotValue();

/*	if( dbFlag < 0.005 )
		deltastep = 0.0002;
	else if( dbFlag < 0.1 )
		deltastep = 0.001;
	else
		deltastep = dbFlag * 0.00001;
		*/
	deltastep = 0.0001;

	m_DCCurveProxy->ShiftCurve( -2*deltastep +shiftBias);
	ApplyCurveInfo();
	resDCShift[ 0 ] = DoCalculation();

	m_DCCurveProxy->ShiftCurve( -deltastep +shiftBias);
	ApplyCurveInfo();
	resDCShift[ 1 ] = DoCalculation();

	m_DCCurveProxy->ShiftCurve( shiftBias );
	ApplyCurveInfo();
	resDCShift[ 2 ] = DoCalculation();

	m_DCCurveProxy->ShiftCurve( deltastep +shiftBias);
	ApplyCurveInfo();
	resDCShift[ 3 ] = DoCalculation();
	m_DCCurveProxy->ShiftCurve( 2*deltastep +shiftBias);
	ApplyCurveInfo();
	resDCShift[ 4 ] = DoCalculation();

	dDuration  = ( resDCShift[3][0] - resDCShift[1][0] ) / 2 / deltastep / 10000.;
	dGamma = ( resDCShift[0][0] + resDCShift[4][0] - 2*resDCShift[2][0]) / 4 / deltastep / deltastep / 10000. / 10000.;

	m_DCCurveProxy->ShiftCurve( 0 );
	ApplyCurveInfo();


	TiXmlElement *kExpNodeTitle = new TiXmlElement("KeyExposure" );
	GetResultObject()->LinkEndChild( kExpNodeTitle );

	Period length = static_cast<int>( Actual365Fixed().dayCount( PricingSetting::instance().GetEvaluationDate(), m_terminationDate ) ) * Days;
	
	for each(const YCurveMap::value_type& v in m_Ycurves)
	{
		boost::shared_ptr<YieldCurveInfoWrapper> curveWrapperTmp = v.second->GetYieldCurveWrapper();
		deltastep = 0.0001;

		if(m_calcDelta)
		{
			std::vector<Real> resNPVbyCurve, resDeltaShift[2];
			TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString(curveWrapperTmp->GetCurveName() ) );
			kExpNodeTitle->LinkEndChild( kExpNodeType );


			dbFlag = curveWrapperTmp->GetSpotValue();
			/*
			if( dbFlag < 0.005 )
				deltastep = 0.0002;
			else if( dbFlag < 0.1 )
				deltastep = 0.001;
			else
				deltastep = dbFlag * 0.00001;
			*/

			v.second->ShiftCurve(shiftBias );
			ApplyCurveInfo();
			resNPVbyCurve = DoCalculation();

			std::ostringstream bufNPVbyCurve;
			bufNPVbyCurve << boost::format( "NPVbyCurve" );

			TiXmlElement NPVbyCurveNode( bufNPVbyCurve.str() );
			NPVbyCurveNode.SetAttribute( "type", "double" );
			NPVbyCurveNode.SetAttribute( "value", ::ToString( ::ToWString( resNPVbyCurve ) ) );
			kExpNodeType->InsertEndChild( NPVbyCurveNode );



			v.second->ShiftCurve( -deltastep + shiftBias );
			ApplyCurveInfo();
			resDeltaShift[0] = DoCalculation();

			v.second->ShiftCurve( deltastep + shiftBias );
			ApplyCurveInfo();
			resDeltaShift[1] = DoCalculation();

			std::ostringstream bufShift;
			bufShift << boost::format( "kexpShift" );

			double shiftDelta = ( resDeltaShift[1][0] - resDeltaShift[0][0] ) / 2 / deltastep / 10000.;
			TiXmlElement kExpShiftDeltaNode( bufShift.str() );
			kExpShiftDeltaNode.SetAttribute( "type", "double" );
			kExpShiftDeltaNode.SetAttribute( "value", ::ToString( ::ToWString( shiftDelta ) ) );
			kExpNodeType->InsertEndChild( kExpShiftDeltaNode );

			v.second->ShiftCurve( 0 );
			ApplyCurveInfo();
			for(int i = 0; i < curveWrapperTmp->GetTenorCount( length ); i++)
			{
				std::vector<Real> resDaltaKey[2];


				//v.second->ShiftCurvePerTenor( -deltastep, i );
				v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAllPlusOne, -1*deltastep, i, shiftBias, ShiftOption::VST_ShiftNothing, 0., 0,0.) );
				ApplyCurveInfo();
				resDaltaKey[0] = DoCalculation();

				//v.second->ShiftCurvePerTenor( deltastep, i );
				v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAllPlusOne, deltastep, i, shiftBias, ShiftOption::VST_ShiftNothing, 0., 0,0.) );
				ApplyCurveInfo();
				resDaltaKey[1] = DoCalculation();

				double keyDelta = ( resDaltaKey[1][0] - resDaltaKey[0][0] ) / 2 / deltastep / 10000.;
				std::ostringstream buf;
				buf << boost::format( "kexp%d" ) % i;
				TiXmlElement kExpNode( buf.str() );
				kExpNode.SetAttribute( "type", "double" );
				kExpNode.SetAttribute( "value", ::ToString( ::ToWString( keyDelta ) ) );
				kExpNodeType->InsertEndChild( kExpNode );
			}
			v.second->ShiftCurve( 0 );
			ApplyCurveInfo();
		}

		if( ::ToWString(curveWrapperTmp->GetCurveName()) == L"KRW" && m_calcGamma)
		{
			std::vector<Real> resGammaShift[2];

			v.second->ShiftCurve( -2*deltastep + shiftBias );
			ApplyCurveInfo();
			resGammaShift[0] = DoCalculation();

			v.second->ShiftCurve(  shiftBias );
			ApplyCurveInfo();
			resGammaShift[1] = DoCalculation();

			v.second->ShiftCurve( 2*deltastep + shiftBias );
			ApplyCurveInfo();
			resGammaShift[2] = DoCalculation();



			std::ostringstream bufGammaNodeType;
			bufGammaNodeType << boost::format( "%sGamma" ) % ::ToString(curveWrapperTmp->GetCurveName() );
			TiXmlElement *kGammaNodeType = new TiXmlElement( bufGammaNodeType.str() );
			kExpNodeTitle->LinkEndChild( kGammaNodeType );

			double shiftGamma = ( resGammaShift[0][0] + resGammaShift[2][0] - 2*resGammaShift[1][0]) / 4 / deltastep / deltastep / 10000. / 10000.;

			std::ostringstream bufShift;
			bufShift << boost::format( "kexpShift" );
			TiXmlElement kExpShiftGammaNode( bufShift.str() );
			kExpShiftGammaNode.SetAttribute( "type", "double" );
			kExpShiftGammaNode.SetAttribute( "value", ::ToString( ::ToWString( shiftGamma ) ) );
			kGammaNodeType->InsertEndChild( kExpShiftGammaNode );

			v.second->ShiftCurve( 0 );
			ApplyCurveInfo();

			for(int i = 0; i <= curveWrapperTmp->GetTenorCount( length ); i++)
			{
				std::vector<Real> resGammaKey[3];

				//v.second->ShiftCurvePerTenor( -2*deltastep, i );
				v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAllPlusOne, -2*deltastep, i, shiftBias, ShiftOption::VST_ShiftNothing, 0., 0,0.) );
				ApplyCurveInfo();
				resGammaKey[0] = DoCalculation();
				
				v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAll, shiftBias, 0, 0., ShiftOption::VST_ShiftNothing, 0., 0,0.) );
				ApplyCurveInfo();
				resGammaKey[1] = DoCalculation();

				//v.second->ShiftCurvePerTenor( 2*deltastep, i );
				v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAllPlusOne, 2*deltastep, i, shiftBias, ShiftOption::VST_ShiftNothing, 0., 0,0.) );
				ApplyCurveInfo();
				resGammaKey[2] = DoCalculation();



				double keyGamma = ( resGammaKey[0][0] + resGammaKey[2][0] - 2*resGammaKey[1][0]) / 4 / deltastep / deltastep / 10000. / 10000.;
				std::ostringstream bufGamma;
				bufGamma << boost::format( "kexp%d" ) % i;
				TiXmlElement kGammaNode( bufGamma.str() );
				kGammaNode.SetAttribute( "type", "double" );
				kGammaNode.SetAttribute( "value", ::ToString( ::ToWString( keyGamma ) ) );
				kGammaNodeType->InsertEndChild( kGammaNode );

			}
			v.second->ShiftCurve( 0 );
			ApplyCurveInfo();
		}


		//std::wstring strCurveName = ::ToWString(curveWrapperTmp->GetCurveName());
		if( ::ToWString(curveWrapperTmp->GetCurveName()) == L"KRW" && m_calcVega)
		{
			std::ostringstream bufVegaNodeType;
			bufVegaNodeType << boost::format( "%sVega" ) % ::ToString(curveWrapperTmp->GetCurveName() );
			TiXmlElement *kVegaNodeType = new TiXmlElement( bufVegaNodeType.str() );
			kExpNodeTitle->LinkEndChild( kVegaNodeType );

			std::vector<Real> resEachVShiftp,resEachVShiftNothing;
			std::vector<Real> resEachVShiftm;
			Real resEachShiftVega;
			v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftNothing, 0., 0, 0., ShiftOption::VST_ShiftNothing, 0., 0,0.) );
			ApplyCurveInfo();
			resEachVShiftNothing = DoCalculation();


			m_volatility += 0.01;
			ApplyCurveInfo();
			resEachVShiftp = DoCalculation();

			
			m_volatility -= 0.02;
			ApplyCurveInfo();
			resEachVShiftm = DoCalculation();

			resEachShiftVega  = ( resEachVShiftp[0] - resEachVShiftm[0] ) / 2 / 0.01 / 100.;

			std::ostringstream bufShift;
			bufShift << boost::format( "kexpShift" );

			TiXmlElement kShiftVegaNode( bufShift.str() );
			kShiftVegaNode.SetAttribute( "type", "double" );
			kShiftVegaNode.SetAttribute( "value", ::ToString( ::ToWString( resEachShiftVega ) ) );
			kVegaNodeType->InsertEndChild( kShiftVegaNode );

			m_volatility += 0.01;
			v.second->ShiftCurve( 0 );
			ApplyCurveInfo();
			
		}



	}



	TiXmlElement expNode( "exp1bp" );
	expNode.SetAttribute( "type", "double" );
	expNode.SetAttribute( "value", ::ToString( ::ToWString( dDuration ) ) );
	GetResultObject()->InsertEndChild( expNode );

	TiXmlElement gammaNode( "gamma1bp" );
	gammaNode.SetAttribute( "type", "double" );
	gammaNode.SetAttribute( "value", ::ToString( ::ToWString( dGamma ) ) );
	GetResultObject()->InsertEndChild( gammaNode );

	TiXmlElement curveName( "CurveName" );
	curveName.SetAttribute( "type", "string" );
	curveName.SetAttribute( "value", ::ToString( m_DCCurveProxy->GetYieldCurveWrapper()->GetCurveName() ) );
	GetResultObject()->InsertEndChild( curveName );

	TiXmlElement unitPrice( "unitPrice" );
	unitPrice.SetAttribute( "type", "double" );
	unitPrice.SetAttribute( "value", ::ToString( ::ToWString( price[0] / m_Nominal * 10000. ) ) );
	GetResultObject()->InsertEndChild( unitPrice );

	TiXmlElement notional( "notional" );
	notional.SetAttribute( "type", "double" );
	notional.SetAttribute( "value", ::ToString( ::ToWString( m_Nominal ) ) );
	GetResultObject()->InsertEndChild( notional );

	TiXmlElement evalTime( "evalTime" );
	evalTime.SetAttribute( "type", "string" );
	evalTime.SetAttribute( "value", ::ToString( ::ToWString( Date::todaysDate() ) + L" " + ::ToWString( Date::todaysDate().second() ) ) );
	GetResultObject()->InsertEndChild( evalTime );
	m_DCCurveProxy->ShiftCurve( 0 );


}


std::vector<Real> CapFloorParam::DoCalculation()
{

	return cap_floor(m_EvalDate,
		m_capFloor,
		m_strike,//
		m_Nominal,
		*m_schedule,
		m_fixingDays,
		m_BDCpayment,
		m_FloatingRate,
		m_DCTermStructure,
		m_volatility);

}



CapFloorParam::CapFloorParam()
{

}

void CapFloorParam::ApplyCurveInfo()
{
	m_DCTermStructure = build_yield_curve( 
		*boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(m_DCCurveProxy->GetYieldCurveWrapper() )->GetCurveData() );
	boost::shared_ptr<PricingEngine> swapEngine(new DiscountingSwapEngine(m_DCTermStructureHandle));
	m_DCTermStructureHandle.linkTo(m_DCTermStructure);

	m_FloatingRate = m_lbortmp->clone( Handle<YieldTermStructure>( m_DCTermStructure ) );

	m_FloatingRate->clearFixings();
	//m_FloatingRate->addFixing( m_recentFixDate, m_pastFixingFloating );


}