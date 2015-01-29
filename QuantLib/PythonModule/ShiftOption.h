#pragma once

class XMLIStream;
class XMLOStream;

class ShiftOption
{
public:
	enum E_YieldShiftType
	{
		ST_ShiftNothing,
		ST_ShiftAll,
		ST_ShiftOne,
		ST_ShiftAllPlusOne,
		ST_ShiftTwo,
		ST_ShiftChunk,
		ST_ShiftChunkTwo,
	};

	enum E_VolShiftType
	{
		VST_ShiftNothing,
		VST_ShiftAll,
		VST_ShiftOne,
		VST_ShiftAllPlusOne
	};

	ShiftOption( E_YieldShiftType type, Real delta, int tenorIdx = 0, Real shiftDelta = 0.,  E_VolShiftType vsType = VST_ShiftNothing, Real volDelta = 0., int volTenorIdx = 0, Real shiftVolDelta = 0., Real delta2 = 0, int tenorIdx2 = 0 );
	ShiftOption( Real delta );

	// 해당 차수의 테너를 가지고 델타를 계산한다.
	Real GetDelta( int tenorIdx, int chunkIdx ) const;

	// 해당 차수의 테너를 가지고 델타를 계산한다.
	Real GetVolDelta( int tenorIdx ) const;
	static const ShiftOption ShiftNothing;

private:
	E_YieldShiftType m_shiftType;
	int m_tenorIdx;
	int m_tenorIdx2;
	Real m_delta;
	Real m_delta2;
	Real m_shiftDelta;


	E_VolShiftType m_volShiftType;
	int m_volTenorIdx;
	Real m_volDelta;
	Real m_shiftVolDelta;

	friend bool operator < ( const ShiftOption& lhs, const ShiftOption& rhs );

	friend XMLOStream& operator << ( XMLOStream& out, const ShiftOption& val );
	friend XMLIStream& operator >> ( XMLIStream& in, ShiftOption& val );
};

inline bool operator < ( const ShiftOption& lhs, const ShiftOption& rhs )
{
	return memcmp( reinterpret_cast<const void*>( &lhs ), reinterpret_cast<const void*>( &rhs ), sizeof( ShiftOption ) ) < 0;
}
