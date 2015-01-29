#include "StdAfx.h"

#include "RAUsingProcessParam.h"

#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "PricingSetting.h"
#include "XMLValue.h"
#include "CurveTable.h"
#include "yield_builder.hpp"
#include "ParamParseUtil.h"
#include "ProductIndex.h"

#include "BloombergCaller.h"

void RAUsingProcessParam::SetDataImpl( TiXmlElement* record )
{
	__super::SetDataImpl( record );

	if( XMLValue( record, PI_RAIndexMatrix ).GetType() == L"string" )
	{
		std::vector<std::wstring> vec = ::Split( XMLValue( record, PI_RAIndexMatrix ).GetValue<std::wstring>(), boost::is_any_of( L"/" ) );
		for each( const std::wstring& v in vec )
		{
			std::vector<Real> mat = ::SplitStrToRealVector( v, boost::is_any_of( L"," ) );
			m_indexMatrix.push_back( Matrix( 2, 2, 0. ) );

			std::copy( mat.begin(), mat.end(), m_indexMatrix.back().begin() );
		}
	}
	else
	{
		m_indexMatrix.push_back( Matrix( 2, 2, 0. ) );
		m_indexMatrix.front()[ 0 ][ 0 ] = 1.;
		m_indexMatrix.front()[ 1 ][ 1 ] = 1.;
	}

	m_firstCallDate = XMLValue( record, PI_RACallStartDate );

	Schedule::const_iterator iter = m_schedule.lower_bound( PricingSetting::instance().GetEvaluationDate() );
	Date termStartDate = m_schedule.dates().front();
	Date termEndDate = m_schedule.dates().back();
	if( iter != m_schedule.begin() )
	{
		termStartDate = *( iter - 1 );
	}
	if( iter != m_schedule.end() )
	{
		termEndDate = *iter;
	}

	size_t termIdx = std::distance( m_schedule.begin(), iter );

	// TODO: 2 index ±¸Çö
	std::vector<Date> inRangeDates;
	inRangeDates = ::CalcNInRange( PricingSetting::instance().GetEvaluationDate(), m_obs1Ticker, m_schedule, m_upperBound1, m_lowerBound1 );
	if( termStartDate < termEndDate )
	{
		m_pastAccrual = static_cast<Real>( inRangeDates.size() ) / static_cast<Real>( m_accrualDayCounter.dayCount( termStartDate, termEndDate ) );
	}
	else
	{
		m_pastAccrual = 0.;
	}	
}

std::vector<std::pair<Date, Real> > RAUsingProcessParam::GetCashFlow() const 
{
	Date evalDate( PricingSetting::instance().GetEvaluationDate().serialNumber() );
	std::vector<Date>::const_iterator iter = std::lower_bound( m_schedule.dates().begin(), m_schedule.dates().end(), evalDate );
	if( iter != m_schedule.dates().end() && *iter == evalDate )
	{
		std::vector<Date> inRangeDates( ::CalcNInRange( PricingSetting::instance().GetEvaluationDate() - 1, m_obs1Ticker, m_schedule, m_upperBound1, m_lowerBound1 ) );
		Size cpnIdx = std::distance( m_schedule.dates().begin(), iter );
		Real coupon = m_accrualRate[ std::min( cpnIdx, m_accrualRate.size() - 1 ) ];
		std::vector<std::pair<Date, Real> > result;
		result.push_back( std::make_pair( evalDate, coupon * static_cast<Real>( inRangeDates.size() ) / static_cast<Real>( m_accrualDayCounter.dayCount( *( iter - 1 ), *iter ) ) * m_notional / static_cast<Real>( m_schedule.tenor().frequency() ) ) );
	}
	return std::vector<std::pair<Date, Real> >();
}
