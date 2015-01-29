#include "StdAfx.h"

#include "RAKAPParam.h"
#include "PricingSetting.h"
#include "ProductIndex.h"
#include "XMLValue.h"
#include "CurveTable.h"
#include "ParamParseUtil.h"

#include "InterestRateCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "IRProductJob.h"
#include "RemoteXMLJob.h"
#include "PerProcessSingleton.h"
#include "CalculationProxy.h"
#include "RangeAccrualDLLSingleton.h"

#include "pricing_functions/hull_white_calibration.hpp"
#include "pricing_functions/single_hull_white_calibration.hpp"

RAKAPParam::RAKAPParam()
	: m_valuationInfo( new ValuationInfo() )
	, m_bondInfo( new RangeAccrualBondInfo() )
	, m_scheduleInfo( new ScheduleInfo() )
	, m_callPutInfo( new CallPutInfo() )
	, m_curveCorrInfo( new CurveCorrelationInfo() )
	, m_lowerTrigger1( new double[ 1 ] )
	, m_upperTrigger1( new double[ 1 ] )
	, m_lowerTrigger2( new double[ 1 ] )
	, m_upperTrigger2( new double[ 1 ] )
	, m_gearing( new double[ 1 ] )
	, m_spread( new double[ 1 ] )
{
}

void RAKAPParam::ApplyCurveInfo()
{
	m_hwParams.reset( new HwParams[ m_curves.size() ] );
	m_corrMat.reset( new boost::scoped_array<Real>[ m_curves.size() ] );
	m_corrMatData.reset( new Real*[ m_curves.size() ] );
	m_curveInfos.reset( new YieldCurveInfo*[ m_curves.size() ] );

	for( size_t i = 0; i < m_curves.size(); i++ )
	{
		m_corrMat[ i ].reset( new Real[ m_curves.size() ] );
	}

	int i = 0;
	for each( const CurveMap::value_type& v1 in m_curves )
	{
		int j = 0;
		for each( const CurveMap::value_type& v2 in m_curves )
		{
			m_corrMat[ i ][ j ] = CurveTable::instance().GetCorr( v1.first, v2.first );
			j++;
		}
		m_curveInfos[ i ] = v1.second->GetInfo();
		m_corrMatData[ i ] = m_corrMat[ i ].get();
		m_hwParams[ i ].meanReversion = v1.second->GetHWParam( GetRemainingTime() )->a;
		m_hwParams[ i ].volatility = v1.second->GetHWParam( GetRemainingTime() )->sigma;

		i++;
	}

	m_curveCorrInfo->curveNum = m_curves.size();
	m_curveCorrInfo->correlationMatrix = m_corrMatData.get();
	m_curveCorrInfo->hwParams = m_hwParams.get();
	m_curveCorrInfo->curve = m_curveInfos.get();
}


void RAKAPParam::SetDataImpl( TiXmlElement* record )
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

	m_isSwap = XMLValue( record, PI_Type ) == L"RASwapKAP" || XMLValue( record, PI_Type ) == L"RASwapCMS";
	m_evalDate = ::ToString( ::ToWString( PricingSetting::instance().GetEvaluationDate() ) ).c_str();

	LoadValuationInfo( record );
	LoadBondInfo( record );
	LoadReceiveScheduleInfo( record );	
	LoadCallPutInfo( record );

	LoadSwapInfo( record );
	LoadCurveCorrInfo( record );
}


void RAKAPParam::LoadBondInfo( const TiXmlElement* record )
{
	m_notional = m_bondInfo->faceAmount = XMLValue( record, PI_Notional );
	m_bondInfo->accrualDayCounter = XMLValue( record, PI_DayCounterAccrual );
	m_bondInfo->paymentConvention = XMLValue( record, PI_RAPaymentConvention );

	m_bondInfo->couponInfo.payIndex.index1 = XMLValue( record, PI_RAPaymentIndex );
	if( XMLValue( record, PI_RAPaymentIndex ) != L"Fixed" )
	{
		m_paymentCurveWrapper = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_RAPaymentIndex ), ShiftOption::ShiftNothing ) );
		m_bondInfo->couponInfo.payIndex.termStructure1 = m_paymentCurveWrapper->GetInfo();
		m_bondInfo->couponInfo.payIndex.pastFixing1 = m_paymentCurveWrapper->GetPastFixing();

		//	m_curves.insert( std::make_pair( m_paymentCurveWrapper->GetCurveName(), m_paymentCurveWrapper ) );
	}

	m_obs1Curve1Wrapper = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_RAOBS1Currency1 ), ShiftOption::ShiftNothing ) );
	m_bondInfo->couponInfo.obsIndex.index1 = XMLValue( record, PI_RAOBS1Index1 );
	m_bondInfo->couponInfo.obsIndex.termStructure1 = m_obs1Curve1Wrapper->GetInfo();
	m_bondInfo->couponInfo.obsIndex.pastFixing1 = m_obs1Curve1Wrapper->GetPastFixing();

	m_curves.insert( std::make_pair( m_obs1Curve1Wrapper->GetCurveName(), m_obs1Curve1Wrapper ) );


	m_bondInfo->couponInfo.obsIndex.isSpread = XMLValue( record, PI_RAOBS1Index2 ).GetNullableValue<std::wstring>() != L"";
	if( m_bondInfo->couponInfo.obsIndex.isSpread )
	{
		m_obs1Curve2Wrapper = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_RAOBS1Currency2 ), ShiftOption::ShiftNothing ) );
		m_bondInfo->couponInfo.obsIndex.index2 = XMLValue( record, PI_RAOBS1Index2 );
		m_bondInfo->couponInfo.obsIndex.termStructure2 = m_obs1Curve2Wrapper->GetInfo();
		m_bondInfo->couponInfo.obsIndex.pastFixing2 = m_obs1Curve2Wrapper->GetPastFixing();

		//		m_curves.insert( std::make_pair(m_obs1Curve2Wrapper->GetCurveName(), m_obs1Curve2Wrapper ) );

	}

	m_gearing[ 0 ] = 1.0;
	m_bondInfo->couponInfo.gearing = m_gearing.get();
	m_bondInfo->couponInfo.gearingNum = 1;

	m_spread[ 0 ] = XMLValue( record, PI_RARebate );
	m_bondInfo->couponInfo.spread = m_spread.get();
	m_bondInfo->couponInfo.spreadNum = 1;

	m_lowerTrigger1[ 0 ] = XMLValue( record, PI_RAOBS1LowerLevel );
	m_bondInfo->couponInfo.lowerTrigger = m_lowerTrigger1.get();
	m_bondInfo->couponInfo.lowerTriggerNum = 1;

	m_upperTrigger1[ 0 ] = XMLValue( record, PI_RAOBS1UpperLevel );
	m_bondInfo->couponInfo.upperTrigger = m_upperTrigger1.get();
	m_bondInfo->couponInfo.upperTriggerNum = 1;

	if(	XMLValue( record, PI_RAOBS2Index1 ).GetNullableValue<std::wstring>() != L"" )
	{
		m_bondInfo->aIndexInfo.hasIndex = true;

		m_obs2Curve1Wrapper = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_RAOBS2Currency1 ), ShiftOption::ShiftNothing ) );
		m_bondInfo->aIndexInfo.obsIndex.index1 = XMLValue( record, PI_RAOBS2Index1 );
		m_bondInfo->aIndexInfo.obsIndex.termStructure1 = m_obs2Curve1Wrapper->GetInfo();
		m_bondInfo->aIndexInfo.obsIndex.pastFixing1 = m_obs2Curve1Wrapper->GetPastFixing();

		m_curves.insert( std::make_pair( m_obs2Curve1Wrapper->GetCurveName(), m_obs2Curve1Wrapper ) );

		m_bondInfo->aIndexInfo.obsIndex.isSpread = XMLValue( record, PI_RAOBS2Index2 ).GetNullableValue<std::wstring>() != L"";
		if( m_bondInfo->aIndexInfo.obsIndex.isSpread )
		{
			m_obs2Curve2Wrapper = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_RAOBS2Currency2 ), ShiftOption::ShiftNothing ) );
			m_bondInfo->aIndexInfo.obsIndex.index2 = XMLValue( record, PI_RAOBS2Index2 );
			m_bondInfo->aIndexInfo.obsIndex.termStructure2 = m_obs2Curve2Wrapper->GetInfo();
			m_bondInfo->aIndexInfo.obsIndex.pastFixing2 = m_obs2Curve2Wrapper->GetPastFixing();

			m_curves.insert( std::make_pair( m_obs2Curve2Wrapper->GetCurveName(), m_obs2Curve2Wrapper ) );
		}

		m_lowerTrigger2[ 0 ] = XMLValue( record, PI_RAOBS2LowerLevel );
		m_bondInfo->aIndexInfo.lowerTrigger = m_lowerTrigger2.get();
		m_bondInfo->aIndexInfo.lowerTriggerNum = 1;

		m_upperTrigger2[ 0 ] = XMLValue( record, PI_RAOBS2UpperLevel );
		m_bondInfo->aIndexInfo.upperTrigger = m_upperTrigger2.get();
		m_bondInfo->aIndexInfo.upperTriggerNum = 1;
	}
	else
	{
		m_bondInfo->aIndexInfo.hasIndex = false;
	}

	m_bondInfo->nInRange = XMLValue( record, PI_RANInRange );
}

void RAKAPParam::LoadValuationInfo( const TiXmlElement* record )
{
	m_valuationInfo->valuationDate = m_evalDate.c_str();
	m_valuationInfo->nSamples = XMLValue( record, PI_RANumOfSimul );
	m_valuationInfo->setSeed = 41;
	m_valuationInfo->numNodes = 200;
	m_valuationInfo->stepsPerYear = 100;
}

void RAKAPParam::LoadReceiveScheduleInfo( const TiXmlElement* record )
{
	m_scheduleInfo->couponTenor = PaymentTenorParser::ParseEnum( XMLValue( record, PI_Tenor ) );
	m_effectiveDate = ::ConvertDateFormat( XMLValue( record, PI_EffectiveDate ) );
	m_terminationDate = ::ConvertDateFormat( XMLValue( record, PI_TerminationDate ) );
	m_scheduleInfo->effectiveDate = m_effectiveDate.c_str();
	m_scheduleInfo->terminationDate = m_terminationDate.c_str();
	m_scheduleInfo->calendar = XMLValue( record, PI_Calendar );
	m_scheduleInfo->bdConvention = XMLValue( record, PI_BDC );
	m_scheduleInfo->terminalConvention = XMLValue( record, PI_BDCTerminal );
	m_scheduleInfo->rule = XMLValue( record, PI_DayGenerationRule );
	m_scheduleInfo->endOfMonth = XMLValue( record, PI_EOM );

	m_firstDate = ::ConvertDateFormat( XMLValue( record, PI_FirstDate ) );
	m_nextToLastDate = ::ConvertDateFormat( XMLValue( record, PI_NextToLastdate ) );

	m_scheduleInfo->firstDate = m_firstDate.c_str();
	m_scheduleInfo->nextToLastDate = m_nextToLastDate.c_str();
}

void RAKAPParam::LoadCallPutInfo( const TiXmlElement* record )
{
	m_callPutInfo->callPut = XMLValue( record, PI_RACallPut );
	m_callPutInfo->value = XMLValue( record, PI_RACallValue );
	m_callStartDate = ::ConvertDateFormat( XMLValue( record, PI_RACallStartDate ) );
	m_callEndDate = ::ConvertDateFormat( XMLValue( record, PI_RACallEndDate ) );
	m_callPutInfo->startDate = m_callStartDate.c_str();
	m_callPutInfo->endDate = m_callEndDate.c_str();
	m_callPutInfo->tenor = PaymentTenorParser::ParseEnum( XMLValue( record, PI_RACallTenor ) );
}

void RAKAPParam::LoadCurveCorrInfo( const TiXmlElement* record )
{
	m_discountCurveInfo = CurveTable::instance().GetYieldCurveProxy( XMLValue( record, PI_RfRateCurve ) );
	m_curves.insert( std::make_pair( m_discountCurveInfo->GetCurveName(), boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_discountCurveInfo->GetYieldCurveWrapper() ) ) ); 

	m_hwParams.reset( new HwParams[ m_curves.size() ] );
	m_corrMat.reset( new boost::scoped_array<Real>[ m_curves.size() ] );
	m_corrMatData.reset( new Real*[ m_curves.size() ] );
	m_curveInfos.reset( new YieldCurveInfo*[ m_curves.size() ] );

	for( size_t i = 0; i < m_curves.size(); i++ )
	{
		m_corrMat[ i ].reset( new Real[ m_curves.size() ] );
	}

	int i = 0;
	for each( const CurveMap::value_type& v1 in m_curves )
	{
		int j = 0;
		for each( const CurveMap::value_type& v2 in m_curves )
		{
			m_corrMat[ i ][ j ] = CurveTable::instance().GetCorr( v1.first, v2.first );
			j++;
		}
		m_curveInfos[ i ] = v1.second->GetInfo();
		m_corrMatData[ i ] = m_corrMat[ i ].get();
		m_hwParams[ i ].meanReversion = v1.second->GetHWParam( GetRemainingTime() )->a;
		m_hwParams[ i ].volatility = v1.second->GetHWParam( GetRemainingTime() )->sigma;

		i++;
	}

	m_curveCorrInfo->curveNum = m_curves.size();
	m_curveCorrInfo->correlationMatrix = m_corrMatData.get();
	m_curveCorrInfo->hwParams = m_hwParams.get();
	m_curveCorrInfo->curve = m_curveInfos.get();
}

void RAKAPParam::LoadSwapInfo( const TiXmlElement* record )
{
	if( m_isSwap )
	{		
		m_swapInfo.reset( new RangeAccrualSwapInfo );

		m_swapInfo->swapType = XMLValue( record, PI_RASPayRec );
		m_swapInfo->hasNotionalExchange = XMLValue( record, PI_RASPayerNotionalExchange );

		m_payerRfInfo = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_RASPayerRfRateCurve ), ShiftOption::ShiftNothing ) );
		m_curves.insert( std::make_pair( m_payerRfInfo->GetCurveName(), m_payerRfInfo ) );

		m_callableBondInfo.reset( new CallableBondInfo );

		boost::shared_ptr<CallableBondInfo> cbInfo( m_callableBondInfo );
		cbInfo->accrualDayCounter = XMLValue( record, PI_RASPayerDayCounter ).GetValue<const char*>();
		cbInfo->faceAmount = XMLValue( record, PI_RASPayingNotional );

		cbInfo->paymentConvention = XMLValue( record, PI_RASPayerFixingBDC );
		cbInfo->type = XMLValue( record, PI_RASPayerPaymentType );

		if( XMLValue( record, PI_RASPayerPaymentType ).GetValue<std::wstring>() == L"Fixed" )
		{
			cbInfo->fixedCouponInfo.couponsNum = 1;
			m_payerFixedCoupon = XMLValue( record, PI_RASPayerPaymentSpread );
			cbInfo->fixedCouponInfo.coupons = &m_payerFixedCoupon;
		}
		else
		{
			m_payerFloatingCouponSpread = XMLValue( record, PI_RASPayerPaymentSpread );
			m_payerFloatingFloor = XMLValue( record, PI_RASPayerFloor );
			m_payerFloatingCap = XMLValue( record, PI_RASPayerCap );

			m_floatingCurve = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( XMLValue( record, PI_RASPayerPaymentCCy ), ShiftOption::ShiftNothing ) );

			cbInfo->floatingCouponInfo.index.index = XMLValue( record, PI_RASPayerPaymentIdx );
			cbInfo->floatingCouponInfo.index.pastFixing = m_floatingCurve->GetPastFixing();
			cbInfo->floatingCouponInfo.index.termStructure = m_floatingCurve->GetInfo();
			cbInfo->floatingCouponInfo.inArrears = XMLValue( record, PI_RASPayerInArrear );
			cbInfo->floatingCouponInfo.spreadNum = 1;
			cbInfo->floatingCouponInfo.gearingNum = 1;

			cbInfo->floatingCouponInfo.spread = &m_payerFloatingCouponSpread;

			cbInfo->floatingCouponInfo.gearing = m_gearing.get();

			if( m_payerFloatingCap > 0. )
			{
				cbInfo->floatingCouponInfo.capNum = 1;
				cbInfo->floatingCouponInfo.floorNum = 1;
			}
			else
			{
				cbInfo->floatingCouponInfo.capNum = 0;
				cbInfo->floatingCouponInfo.floorNum = 0;
			}

			cbInfo->floatingCouponInfo.cap = &m_payerFloatingCap;
			cbInfo->floatingCouponInfo.floor = &m_payerFloatingFloor;
		}

		m_payerScheduleInfo.reset( new ScheduleInfo() );

		m_payerScheduleInfo->couponTenor = PaymentTenorParser::ParseEnum( XMLValue( record, PI_Tenor ) );
		m_payerEffectiveDate = ::ConvertDateFormat( XMLValue( record, PI_RASPayerEffectiveDate ) );
		m_payerTerminationDate = ::ConvertDateFormat( XMLValue( record, PI_RASPayerTerminationDate ) );
		m_payerScheduleInfo->effectiveDate = m_payerEffectiveDate.c_str();
		m_payerScheduleInfo->terminationDate = m_payerTerminationDate.c_str();
		m_payerScheduleInfo->calendar = XMLValue( record, PI_RASPayerCalendar );
		m_payerScheduleInfo->bdConvention = XMLValue( record, PI_RASPayerBDC );
		m_payerScheduleInfo->terminalConvention = XMLValue( record, PI_RASPayerBDC_Terminal );
		m_payerScheduleInfo->rule = XMLValue( record, PI_RASPayerRule );
		m_payerScheduleInfo->endOfMonth = XMLValue( record, PI_RASPayerEOM );

		m_payerFirstDate = ::ConvertDateFormat( XMLValue( record, PI_RASPayerFirstDate ) );
		m_payerNextToLastDate = ::ConvertDateFormat( XMLValue( record, PI_RASPayerNextToLastDate ) );

		m_payerScheduleInfo->firstDate = m_payerFirstDate.c_str();
		m_payerScheduleInfo->nextToLastDate = m_payerNextToLastDate.c_str();
	}
}


ResultInfo RAKAPParam::DoCalculation()
{
	ApplyCurveInfo();

	ResultInfo res;
	if( m_isSwap )
	{		
		RangeAccrualDLLSingleton::instance().SwapFunc( &res, *m_valuationInfo, *m_bondInfo, *m_scheduleInfo, *m_callPutInfo, boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_discountCurveInfo->GetYieldCurveWrapper() )->GetInfo(), *m_curveCorrInfo, *m_swapInfo, *m_callableBondInfo, *m_payerScheduleInfo, m_payerRfInfo->GetInfo() );
	}
	else
	{
		RangeAccrualDLLSingleton::instance().NoteFunc( &res, *m_valuationInfo, *m_bondInfo, *m_scheduleInfo, *m_callPutInfo, boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_discountCurveInfo->GetYieldCurveWrapper() )->GetInfo(), *m_curveCorrInfo );
		res.npv *= -1.;
	}
	return res;
}

QuantLib::Period RAKAPParam::GetRemainingTime() const
{
	return static_cast<int>( ::ConvertToDate( ::ToWString( m_terminationDate ) ) - PricingSetting::instance().GetEvaluationDate() + 1. ) * Days;
}

std::vector<std::pair<Date, Real> > RAKAPParam::GetCashFlow() const 
{
	return std::vector<std::pair<Date, Real> >();
}
