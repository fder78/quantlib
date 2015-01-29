#include "StdAfx.h"

#include "RAMCParam.h"

#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "PricingSetting.h"
#include "XMLValue.h"
#include "CurveTable.h"
#include "yield_builder.hpp"
#include "ParamParseUtil.h"
#include "ProductIndex.h"

#include "BloombergCaller.h"

#include "pricing_functions/hull_white_calibration.hpp"

ResultInfo RAMCParam::DoCalculation()
{
	ResultInfo res;
	std::vector<Real> tmpRes;
	if( m_isSwap )
	{
		tmpRes = dual_rangeaccrual_swap( PricingSetting::instance().GetEvaluationDate()
																	 , m_notional, m_side, m_alpha, m_swapSchedule, m_accrualRate.front(), m_schedule, m_accrualDayCounter, m_bd, m_lowerBound1, m_upperBound1
																	 , m_firstCallDate, m_pastAccrual
																	 , m_pastFixing
																	 , m_obs1CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() )
																	 , m_obs2CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() )
																	 , boost::dynamic_pointer_cast<HullWhiteProcess>( m_discountCurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ) )
																	 , m_rho12, m_rho1disc, m_rho2disc, m_numSimul, 512 );
	}
	else
	{
		tmpRes = dual_rangeaccrual( PricingSetting::instance().GetEvaluationDate()
															, m_notional, m_accrualRate.front(), m_schedule, m_accrualDayCounter, m_bd, m_lowerBound1, m_upperBound1
															, m_firstCallDate, m_pastAccrual
															, m_obs1CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() )
															, m_obs2CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() )
															, boost::dynamic_pointer_cast<HullWhiteProcess>( m_discountCurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ) )
															, m_rho12, m_rho1disc, m_rho2disc, m_numSimul, 512 );
		tmpRes[ 0 ] *= -1.;
	}

	res.npv = tmpRes[ 0 ];
	m_error = tmpRes[ 1 ];

	return res;
}

void RAMCParam::SetDataImpl( TiXmlElement* record )
{
	__super::SetDataImpl( record );
	m_numSimul = XMLValue( record, PI_RANumOfSimul );

	m_lowerBound1.insert( m_lowerBound1.end(), m_lowerBound2.begin(), m_lowerBound2.end() );
	m_upperBound1.insert( m_lowerBound1.end(), m_upperBound2.begin(), m_upperBound2.end() );
}

void RAMCParam::FetchResult()
{
	__super::FetchResult();

	TiXmlElement error( "error" );
	error.SetAttribute( "type", "double" );
	error.SetAttribute( "value", ::ToString( ::ToWString( m_error / m_notional ) ) );
	GetResultObject()->InsertEndChild( error );
}

