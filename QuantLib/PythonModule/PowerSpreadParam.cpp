#include "StdAfx.h"

#include "PowerSpreadParam.h"

#include "PricingSetting.h"
#include "ProductIndex.h"
#include "XMLValue.h"
#include "CurveTable.h"
#include "ParamParseUtil.h"

#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "BloombergCaller.h"

#include "pricing_functions/single_hull_white_calibration.hpp"

#include "pricing_functions/daishin_power_spread_note.hpp"
#include "pricing_functions/daishin_power_spread_swap.hpp"
#include <ds_interestrate_derivatives/instruments/swaps/power_spread_swap.hpp>
//#include "dual_rangeaccrual.hpp"

#include "yield_builder.hpp"

#include "PerProcessSingleton.h"
#include "StringUtil.h"

#include "EnumParser.h"



namespace dll{
	extern HMODULE hLib;
}

class PowerSpreadSideParser : public EnumParser<PowerSpreadSideParser, PowerSpreadSwap::Side >
{
public:
	void BuildEnumMap();
};

void PowerSpreadSideParser::BuildEnumMap()
{
	AddEnum( L"Payer", PowerSpreadSwap::Payer );
	AddEnum( L"Receiver", PowerSpreadSwap::Receiver );
}

void PowerSpreadParam::LoadScheduleInfo( const TiXmlElement* record )
{
	Period tTenorStructured = TenorParser::ParseEnum( XMLValue( record, PI_Tenor ) );
	std::vector<std::wstring> calendars = Split( XMLValue( record, PI_Calendar ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );

	Calendar tCalendar;
	switch( calendars.size() )
	{
	case 1:
		tCalendar = CalendarParser::ParseEnum( XMLValue( record, PI_Calendar ) );
		break;
	case 2:
		tCalendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ) );
		break;
	case 3:
		tCalendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ) );
		break;
	case 4:
		tCalendar = JointCalendar( CalendarParser::ParseEnum( calendars[ 0 ] ), CalendarParser::ParseEnum( calendars[ 1 ] ), CalendarParser::ParseEnum( calendars[ 2 ] ), CalendarParser::ParseEnum( calendars[ 3 ] ) );
		break;
	}	

	
	BusinessDayConvention tTerminationBDC = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDCTerminal ) );
	DateGeneration::Rule tRule = DateGenerationRuleParser::ParseEnum( XMLValue( record, PI_DayGenerationRule ) );
	bool tEOM = XMLValue( record, PI_EOM );
	Date tFirstDate = XMLValue( record, PI_FirstDate );
	Date tNextToLastDate = XMLValue( record, PI_NextToLastdate );

	m_scheduleStructured.reset( new Schedule(m_effectiveDate, m_terminationDate, tTenorStructured, tCalendar, m_BDC, tTerminationBDC, tRule, tEOM, tFirstDate, tNextToLastDate ));


	if(m_isSwap == true)
	{
		Period tTenorFloating = TenorParser::ParseEnum( XMLValue( record, PI_FloatingTenor ) );
		m_scheduleFloating.reset( new Schedule(m_effectiveDate, m_terminationDate, tTenorFloating, tCalendar, m_BDC, tTerminationBDC, tRule, tEOM, tFirstDate, tNextToLastDate ));
	}
}


void PowerSpreadParam::LoadLegInfo( const TiXmlElement* record)
{
	
	if( m_isSwap )
	{
		m_side = PowerSpreadSideParser::ParseEnum( XMLValue( record, PI_PayRec ).GetValue<std::wstring>() );
		m_spreadFloating = XMLValue( record, PI_FloatingSpread );
		m_gearingFloating = XMLValue( record, PI_FloatingGearing );

		m_DayCounterFloating = DayCounterParser::ParseEnum( XMLValue( record, PI_FloatingDayCounter ) );

		Date recentFixDate = m_scheduleFloating->dates().front();
		Date today( PricingSetting::instance().GetEvaluationDate().serialNumber() );
		for( std::vector<Date>::const_reverse_iterator iter = m_scheduleFloating->dates().rbegin(); iter != m_scheduleFloating->dates().rend(); ++iter )
		{
			if( *iter <= today )
			{
				recentFixDate = *iter;
				break;
			}
		}
		m_pastFixingFloating = ::Blph<xmlrpc_c::value_double>( L"KWCDC Curncy", L"last price", recentFixDate ) / 100.;
	}

	m_DayCounterStructured = DayCounterParser::ParseEnum( XMLValue( record, PI_DayCounter ) );

	if(XMLValue(record,PI_Gearings).GetType() == L"string")
		m_gearings = SplitStrToRealVector( XMLValue( record, PI_Gearings ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	else
		m_gearings.push_back(XMLValue(record,PI_Gearings));
	
	if(XMLValue(record,PI_Spreads).GetType() == L"string")
		m_spreads = SplitStrToRealVector( XMLValue( record, PI_Spreads ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	else
		m_spreads.push_back(XMLValue(record,PI_Spreads));

	if(XMLValue(record,PI_Caps).GetType() == L"string")
		m_caps = SplitStrToRealVector( XMLValue( record, PI_Caps ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	else
		m_caps.push_back(XMLValue(record,PI_Caps));

	if(XMLValue(record,PI_Floors).GetType() == L"string")
		m_floors = SplitStrToRealVector( XMLValue( record, PI_Floors ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
	else
		m_floors.push_back(XMLValue(record,PI_Floors));

	m_isAvg = XMLValue( record, PI_isAvg );


	//Blphvec( const std::wstring& code, const std::wstring& field, const Date& startDate, const Date& ed, const std::wstring& periodUnit, int numData, const std::wstring& activeDateOption )
	m_pastFixingStructured = 0.0001;
	

}


void PowerSpreadParam::LoadCallPutInfo( const TiXmlElement* record )
{
	m_callStartDate = XMLValue( record, PI_CallStartDate ) ;
	m_callEndDate = XMLValue( record, PI_CallEndDate ) ;
	m_callTenor = TenorParser::ParseEnum( XMLValue( record, PI_CallTenor ) );
	//m_callPut = ;
	//callValue = ;
}


void PowerSpreadParam::LoadCurveInfo( const TiXmlElement* record )
{

	m_DCCurveProxy = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_DiscountRateCurve ) );
	m_Ycurves.insert( std::make_pair( m_DCCurveProxy->GetYieldCurveWrapper()->GetCurveName(), m_DCCurveProxy ) );

	m_RefCurveLongProxy = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_RefCurveLong ) );
	m_Ycurves.insert(std::make_pair(m_RefCurveLongProxy->GetYieldCurveWrapper()->GetCurveName(), m_RefCurveLongProxy));

	m_RefCurveShortProxy = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_RefCurveShort ) );
	m_Ycurves.insert(std::make_pair(m_RefCurveShortProxy->GetYieldCurveWrapper()->GetCurveName(), m_RefCurveShortProxy));


	m_rhoLongDisc = CurveTable::instance().GetCorr( m_DCCurveProxy->GetYieldCurveWrapper()->GetCurveName(), m_RefCurveLongProxy->GetYieldCurveWrapper()->GetCurveName() );
	m_rhoShortDisc = CurveTable::instance().GetCorr( m_DCCurveProxy->GetYieldCurveWrapper()->GetCurveName(), m_RefCurveShortProxy->GetYieldCurveWrapper()->GetCurveName() );
	m_rhoLongShort = CurveTable::instance().GetCorr( m_RefCurveLongProxy->GetYieldCurveWrapper()->GetCurveName(), m_RefCurveShortProxy->GetYieldCurveWrapper()->GetCurveName() );



}



void PowerSpreadParam::LoadValuationInfo( const TiXmlElement* record )
{
	m_numSimulation = XMLValue( record, PI_NumOfSimul );
	m_numCalibration = XMLValue( record, PI_NumOfCal );
}


void PowerSpreadParam::SetDataImpl( TiXmlElement* record )
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
	m_isSwap = XMLValue( record, PI_Type ) == L"PSSwap";
	m_strTraderID = XMLValue(record, PI_TraderID).GetValue<const char*>();
	m_strCode = XMLValue(record, PI_Code).GetValue<const char*>();
	m_strBook = XMLValue(record, PI_Book).GetValue<const char*>();

	m_strBacktoBack = XMLValue(record,PI_BacktoBack).GetValue<const char*>();
	m_strExtCalculator = XMLValue(record,PI_ExtCalculator).GetValue<const char*>();
	m_strCounterParty = XMLValue(record,PI_CounterParty).GetValue<const char*>();

	m_EvalDate = XMLValue( record, PI_EvalDate );
	m_Nominal = XMLValue( record, PI_Notional );
	m_strCurrency = XMLValue( record, PI_Currency ).GetValue<const char*>();

	m_effectiveDate = XMLValue( record, PI_EffectiveDate ) ;
	m_terminationDate = XMLValue( record, PI_TerminationDate ) ;

	m_BDC = BusinessDayConventionParser::ParseEnum( XMLValue( record, PI_BDC ) );


	m_calcDelta = XMLValue( record, PI_CalcDelta ) == L"Y";
	m_calcGamma = XMLValue( record, PI_CalcGamma ) == L"Y";
	m_calcVega = XMLValue( record, PI_CalcVega ) == L"Y";
	m_calcXGamma = XMLValue( record, PI_CalcXGamma ) == L"Y";


	LoadScheduleInfo( record );
	LoadLegInfo( record );
	LoadCallPutInfo( record );
	LoadCurveInfo( record );
	LoadValuationInfo( record );


}

void PowerSpreadParam::Calculate()
{

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

			v.second->ShiftCurve(shiftBias );
			ApplyCurveInfo();
			resNPVbyCurve = DoCalculation();

			std::ostringstream bufNPVbyCurve;
			bufNPVbyCurve << boost::format( "NPVbyCurve" );

			TiXmlElement NPVbyCurveNode( bufNPVbyCurve.str() );
			NPVbyCurveNode.SetAttribute( "type", "double" );
			NPVbyCurveNode.SetAttribute( "value", ::ToString( ::ToWString( resNPVbyCurve[0] ) ) );
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

			for(int i = 0; i <= curveWrapperTmp->GetTenorCount( GetRemainingTime() ); i++)
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

			for(int i = 0; i< curveWrapperTmp->GetTenorCount( GetRemainingTime() ); i++)
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


			v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAll, shiftBias, 0, 0., ShiftOption::VST_ShiftAll, 0.01) );
			ApplyCurveInfo();
			resEachVShiftp = DoCalculation();

			v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAll, shiftBias, 0, 0., ShiftOption::VST_ShiftAll, -0.01) );
			ApplyCurveInfo();
			resEachVShiftm = DoCalculation();

			resEachShiftVega  = ( resEachVShiftp[0] - resEachVShiftm[0] ) / 2 / 0.01 / 100.;

			std::ostringstream bufShift;
			bufShift << boost::format( "kexpShift" );

			TiXmlElement kShiftVegaNode( bufShift.str() );
			kShiftVegaNode.SetAttribute( "type", "double" );
			kShiftVegaNode.SetAttribute( "value", ::ToString( ::ToWString( resEachShiftVega ) ) );
			kVegaNodeType->InsertEndChild( kShiftVegaNode );

			v.second->ShiftCurve( 0 );
			ApplyCurveInfo();

			std::vector<Size> vegaIdx = curveWrapperTmp->GetVegaTenors( GetRemainingTime() );

			for each( Size idx in vegaIdx )
			{
				std::vector<Real> respd, resmd;

				v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAll, shiftBias, 0, 0., ShiftOption::VST_ShiftOne, 0.01, idx ) );
				ApplyCurveInfo();
				respd = DoCalculation();

				v.second->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAll, shiftBias, 0, 0., ShiftOption::VST_ShiftOne, -0.01, idx ) );
				ApplyCurveInfo();
				resmd = DoCalculation();


				double keyExposures = ( respd[0] - resmd[0] ) / 2 / 0.01 / 100.;


				std::ostringstream bufVega;
				bufVega << boost::format( "kexp%d" ) % idx;

				TiXmlElement vegaExpNode( bufVega.str() );
				vegaExpNode.SetAttribute( "type", "double" );
				vegaExpNode.SetAttribute( "value", ::ToString( ::ToWString( keyExposures ) ) );
				kVegaNodeType->InsertEndChild( vegaExpNode );


				//				m_resInfoForKeyVegaExp[ v.second->GetCurveName() ].push_back( keyExposures );

				v.second->ShiftCurve( 0 );
				ApplyCurveInfo();
			}
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



QuantLib::Period PowerSpreadParam::GetRemainingTime() const 
{
	return static_cast<int>( m_scheduleStructured->endDate() - PricingSetting::instance().GetEvaluationDate() + 1. ) * Days;
}

std::vector<Real> PowerSpreadParam::DoCalculation()
{
	std::vector<Real> price;

	if (m_isSwap == true)
	{
		price = power_spread_swap(m_EvalDate, 
			m_Nominal, 
			m_side, 
			m_spreadFloating, 
			*m_scheduleFloating, 
			*m_scheduleStructured, 
			m_DayCounterStructured,	
			m_BDC, m_gearings, m_spreads, m_caps, m_floors, m_isAvg, m_callStartDate,
			m_pastFixingStructured, 
			m_pastFixingFloating, 
			m_gearingFloating,
			m_DayCounterFloating,
			m_RefCurveLongProxy->GetYieldCurveWrapper()->GetStochasticProcess(GetRemainingTime()), 
			m_RefCurveShortProxy->GetYieldCurveWrapper()->GetStochasticProcess(GetRemainingTime()),	
			boost::dynamic_pointer_cast<HullWhiteProcess>( m_DCCurveProxy->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ) ), 
			m_rhoLongShort, m_rhoLongDisc, m_rhoShortDisc, m_numSimulation, m_numCalibration
 		);
		



	}
	else
	{
		price = power_spread_note(m_EvalDate, m_Nominal, *m_scheduleStructured, m_DayCounterStructured,	m_BDC, m_gearings, m_spreads, 
			m_caps, m_floors, m_isAvg, m_callStartDate, m_pastFixingStructured, 
			m_RefCurveLongProxy->GetYieldCurveWrapper()->GetStochasticProcess(GetRemainingTime()), 
			m_RefCurveShortProxy->GetYieldCurveWrapper()->GetStochasticProcess(GetRemainingTime()),	
			boost::dynamic_pointer_cast<HullWhiteProcess>( m_DCCurveProxy->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ) ), 
			m_rhoLongShort, m_rhoLongDisc, m_rhoShortDisc, m_numSimulation, m_numCalibration
		);

		if (m_side == PowerSpreadSwap::Payer)
		{
			price[0] *= -1.;
		}

	}


	return price;
}

PowerSpreadParam::PowerSpreadParam()
{

}
void PowerSpreadParam::ApplyCurveInfo()
{

}
