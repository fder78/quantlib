#include "StdAfx.h"

#include "RACMSParam.h"
#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "PricingSetting.h"
#include "yield_builder.hpp"
#include "BloombergCaller.h"
#include "ProductIndex.h"
#include "StringUtil.h"
#include "XMLValue.h"

#include "pricing_functions/hull_white_calibration.hpp"


ResultInfo RACMSParam::DoCalculation()
{
	ResultInfo res;
	std::vector<Real> tmpRes;
	if( m_isSwap )
	{
		tmpRes = dual_rangeaccrual_swap_minusCMS( PricingSetting::instance().GetEvaluationDate(), m_notional, m_side, m_alpha, m_swapSchedule, m_accrualRate.front(), m_schedule, m_accrualDayCounter, m_bd, m_lowerBound1, m_upperBound1, m_firstCallDate, m_pastAccrual, m_pastFixing, m_pastFixing, m_obs1CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ), m_obs2CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ), boost::dynamic_pointer_cast<HullWhiteProcess>( m_discountCurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ) ), m_rho12, m_rho1disc, m_rho2disc, m_numSimul, 512 );
	}
	else
	{
		tmpRes = dual_rangeaccrual_minusCMS( PricingSetting::instance().GetEvaluationDate(), m_notional, m_accrualRate.front(), m_schedule, m_accrualDayCounter, m_bd, m_lowerBound1, m_upperBound1, m_firstCallDate, m_pastAccrual, m_fixingCMS, m_obs1CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ), m_obs2CurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ),  boost::dynamic_pointer_cast<HullWhiteProcess>( m_discountCurveInfo->GetYieldCurveWrapper()->GetStochasticProcess( GetRemainingTime() ) ), m_rho12, m_rho1disc, m_rho2disc, m_numSimul, 512 );
		tmpRes[ 0 ] *= -1.;
	}

	res.npv = tmpRes[ 0 ];
	res.error = tmpRes[ 1 ];

	return res;
}


void RACMSParam::SetDataImpl( TiXmlElement* record )
{
	__super::SetDataImpl( record );
	m_fixingCMS = 0.0345;

	Date recentFixDate;
	Date today( PricingSetting::instance().GetEvaluationDate().serialNumber() );
	for( std::vector<Date>::const_reverse_iterator iter = m_swapSchedule.dates().rbegin(); iter != m_swapSchedule.dates().rend(); ++iter )
	{
		if( *iter <= PricingSetting::instance().GetEvaluationDate() )
		{
		recentFixDate = *iter;
		break;
		}
	}

	if( recentFixDate.serialNumber() > 0 )
	{		
		if( record->FirstChildElement( PI_RASPastFixedCMS ) == NULL )
		{
			m_fixingCMS = ::Blph<xmlrpc_c::value_double>( L"KWSWO3 Curncy", L"last price", recentFixDate ) / 100.;
			TiXmlElement pastCMSFixing( PI_RASPastFixedCMS );
			pastCMSFixing.SetAttribute( "type", "double" );
			pastCMSFixing.SetAttribute( "value", ::ToString( m_fixingCMS ) );

			record->InsertEndChild( pastCMSFixing );
		}
		else
		{
			m_fixingCMS = XMLValue( record, PI_RASPastFixedCMS );
		}		
	}
}

