#include "StdAfx.h"

#include "YieldCurveInfoWrapperProxy.h"
#include "YieldCurveInfoWrapper.h"

#include "CurveTable.h"

YieldCurveInfoWrapperProxy::YieldCurveInfoWrapperProxy( const std::wstring& curveName )
	: m_curveName( curveName )
	, m_yieldCurveWrapper( CurveTable::instance().GetYieldCurve( curveName, ShiftOption::ShiftNothing ) )
	, m_shiftOption( ShiftOption::ShiftNothing )
{
}

void YieldCurveInfoWrapperProxy::ShiftCurve( Real delta )
{
	m_shiftOption = ( delta == 0. ) ? ShiftOption::ShiftNothing : ShiftOption( ShiftOption::ST_ShiftAll, delta );
	m_yieldCurveWrapper = CurveTable::instance().GetYieldCurve( m_curveName, m_shiftOption );
}

void YieldCurveInfoWrapperProxy::ShiftCurve( const ShiftOption& so )
{
	m_shiftOption = so;
	m_yieldCurveWrapper = CurveTable::instance().GetYieldCurve( m_curveName, m_shiftOption );
}

void YieldCurveInfoWrapperProxy::ShiftCurvePerTenor( Real delta, int i )
{
	m_shiftOption = ( delta == 0. ) ? ShiftOption::ShiftNothing : ShiftOption( ShiftOption::ST_ShiftOne, delta, i );
	m_yieldCurveWrapper = CurveTable::instance().GetYieldCurve( m_curveName, m_shiftOption );
}

boost::shared_ptr<TiXmlElement> YieldCurveInfoWrapperProxy::ParseToXML( Period period ) const
{
	return m_yieldCurveWrapper->ParseToXML( period );
}
