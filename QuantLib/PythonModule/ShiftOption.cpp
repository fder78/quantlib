#include "StdAfx.h"

#include "ShiftOption.h"

#include "XMLStream.h"

const ShiftOption ShiftOption::ShiftNothing( ShiftOption::ST_ShiftNothing, 0., 0, 0., ShiftOption::VST_ShiftNothing, 0., 0, 0. );

ShiftOption::ShiftOption( E_YieldShiftType type, Real delta, int tenorIdx /*= 0*/, Real shiftDelta /*= 0.*/, E_VolShiftType vsType /*= VST_ShiftNothing*/, Real volDelta /*= 0.*/, int volTenorIdx /*= 0*/, Real shiftVolDelta /*= 0.*/, Real delta2 /*= 0*/, int tenorIdx2 /*= 0 */ ) : m_delta( delta )
	, m_shiftType( type )
	, m_tenorIdx( tenorIdx )
	, m_tenorIdx2( tenorIdx2 )
	, m_delta2( delta2 )
	, m_shiftDelta( shiftDelta )
	, m_volShiftType( vsType )
	, m_volDelta( volDelta )
	, m_volTenorIdx( volTenorIdx )
	, m_shiftVolDelta( shiftVolDelta )
{
}

ShiftOption::ShiftOption( Real delta ) : m_delta( delta )
	, m_shiftType( ST_ShiftAll )
	, m_tenorIdx( 0 )
{
}

QuantLib::Real ShiftOption::GetDelta( int tenorIdx, int chunkIdx ) const
{
	switch( m_shiftType )
	{
	case ST_ShiftNothing:
		return 0.;
	case ST_ShiftAll:
		return m_delta;
	case ST_ShiftOne:
		return ( tenorIdx == m_tenorIdx ) ? m_delta : 0.;
	case ST_ShiftTwo:
		return ( ( tenorIdx == m_tenorIdx ) ? m_delta : 0. ) + ( ( tenorIdx == m_tenorIdx2 ) ? m_delta2 : 0. );
	case ST_ShiftAllPlusOne:
		return ( tenorIdx == m_tenorIdx ) ? m_delta + m_shiftDelta : m_shiftDelta;
	case ST_ShiftChunk:
		return ( chunkIdx == m_tenorIdx ) ? m_delta : 0.;
	case ST_ShiftChunkTwo:
		return ( ( chunkIdx == m_tenorIdx ) ? m_delta : 0. ) + ( ( chunkIdx == m_tenorIdx2 ) ? m_delta2 : 0. );
	}

	return 0.;
}

QuantLib::Real ShiftOption::GetVolDelta( int tenorIdx ) const
{
	switch( m_volShiftType )
	{
	case VST_ShiftNothing:
		return 0.;
	case VST_ShiftAll:
		return m_volDelta;
	case VST_ShiftOne:
		return ( tenorIdx >= m_volTenorIdx ) ? m_volDelta : 0.;
	case VST_ShiftAllPlusOne:
		return ( tenorIdx >= m_volTenorIdx ) ? m_volDelta + m_shiftVolDelta : m_shiftVolDelta;

	}

	return 0.;
}


XMLOStream& operator << ( XMLOStream& out, const ShiftOption& val )
{
	out << static_cast<int>( val.m_shiftType ) << val.m_tenorIdx << val.m_tenorIdx2 << val.m_delta << val.m_delta2 << val.m_shiftDelta;
	out << static_cast<int>( val.m_volShiftType ) << val.m_volTenorIdx << val.m_volDelta << val.m_shiftVolDelta;

	return out;
}

XMLIStream& operator >> ( XMLIStream& in, ShiftOption& val )
{
	int shiftType;
	int volShiftType;
	in >> shiftType >> val.m_tenorIdx >> val.m_tenorIdx2 >> val.m_delta >> val.m_delta2 >> val.m_shiftDelta;
	in >> volShiftType >> val.m_volTenorIdx >> val.m_volDelta >> val.m_shiftVolDelta;
	val.m_shiftType = static_cast<ShiftOption::E_YieldShiftType>( shiftType );
	val.m_volShiftType = static_cast<ShiftOption::E_VolShiftType>( volShiftType );

	return in;
}