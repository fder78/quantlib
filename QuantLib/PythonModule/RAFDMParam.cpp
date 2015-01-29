#include "StdAfx.h"

#include "RAFDMParam.h"

#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "PricingSetting.h"
#include "XMLValue.h"
#include "CurveTable.h"
#include "yield_builder.hpp"
#include "ParamParseUtil.h"
#include "ProductIndex.h"

#include "InterestRateCurveInfoWrapper.h"

#include "BloombergCaller.h"

#include "pricing_functions/hull_white_calibration.hpp"


ResultInfo RAFDMParam::DoCalculation()
{
	ResultInfo res;
	std::vector<Real> tmpRes;

	HullWhiteTimeDependentParameters hp1 = boost::dynamic_pointer_cast<GeneralizedHullWhiteProcess>( m_obs1CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ) )->GetParam();
	HullWhiteTimeDependentParameters hp2 = boost::dynamic_pointer_cast<GeneralizedHullWhiteProcess>( m_obs2CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ) )->GetParam();
	if( m_isSwap )
	{
		tmpRes = dual_rangeaccrual_fdm( PricingSetting::instance().GetEvaluationDate()
			, m_notional, m_accrualRate, m_gearing, m_schedule, m_accrualDayCounter, m_bd, m_lowerBound1, m_upperBound1, m_lowerBound2, m_upperBound2
			, m_firstCallDate, m_pastAccrual
			, build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_obs1CurveInfo->GetYieldCurveWrapper() )->GetCurveData() )
			, hp1
			, m_obs1FXVol, m_obs1FXCorr
			, build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_obs2CurveInfo->GetYieldCurveWrapper() )->GetCurveData() )
			, hp2
			, m_obs2FXVol, m_obs2FXCorr
			, Handle<YieldTermStructure>( build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_discountCurveInfo->GetYieldCurveWrapper() )->GetCurveData() ) )
			, m_rho12, m_tGrid, m_rGrid
			, m_invAlpha
			, m_invGearing, m_invFixing, m_invCap, m_invFloor
			, m_alpha, m_pastFixing, m_indexMatrix );
	}
	else
	{		
		tmpRes = dual_rangeaccrual_fdm( PricingSetting::instance().GetEvaluationDate()
			, m_notional, m_accrualRate, m_gearing, m_schedule, m_accrualDayCounter, m_bd, m_lowerBound1, m_upperBound1, m_lowerBound2, m_upperBound2
			, m_firstCallDate, m_pastAccrual
			, build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_obs1CurveInfo->GetYieldCurveWrapper() )->GetCurveData() )
			, hp1
			, m_obs1FXVol, m_obs1FXCorr
			, build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_obs2CurveInfo->GetYieldCurveWrapper() )->GetCurveData() )
			, hp2
			, m_obs2FXVol, m_obs2FXCorr
			, Handle<YieldTermStructure>( build_yield_curve( *boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( m_discountCurveInfo->GetYieldCurveWrapper() )->GetCurveData() ) )
			, m_rho12, m_tGrid, m_rGrid
			, m_invAlpha, m_invGearing, m_invFixing, m_invCap, m_invFloor, Null<Real>(), Null<Real>(), m_indexMatrix );
	}

	tmpRes[ 0 ] *= -1.;
	res.npv = tmpRes[ 0 ];
	return res;
}

void RAFDMParam::SetDataImpl( TiXmlElement* record )
{
	__super::SetDataImpl( record );
	std::vector<std::wstring> p = ::Split<std::wstring>( XMLStrValue( record, PI_RANumOfSimul ), boost::is_any_of( L"/" ) );

	m_tGrid = boost::lexical_cast<int>( p[ 0 ] );
	m_rGrid = boost::lexical_cast<int>( p[ 1 ] );

	m_obs1FXVol = XMLValue( record, PI_RAOBS1FXVol );
	m_obs1FXCorr = XMLValue( record, PI_RAOBS1FXCorr );
	m_obs2FXVol = XMLValue( record, PI_RAOBS2FXVol );
	m_obs2FXCorr = XMLValue( record, PI_RAOBS2FXCorr );
}
