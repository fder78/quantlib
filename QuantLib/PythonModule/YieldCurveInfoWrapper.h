#pragma once

#include "ShiftOption.h"

struct YieldCurveInfo;
class ShiftOption;

namespace QuantLib
{
	struct YieldCurveData;
	struct HullWhiteParameters;
	struct HullWhiteTimeDependentParameters;
}

struct FXCurveData;

class YieldCurveInfoWrapper
{
public:
	YieldCurveInfoWrapper( const std::wstring& curveName )
		: m_curveName( curveName )
	{	}

	std::wstring GetCurveName() const { return m_curveName; }

	virtual boost::shared_ptr<StochasticProcess1D> GetStochasticProcess( Period tenor ) const = 0;

	virtual void ShiftCurve( Real delta ) = 0;
	virtual void ShiftCurvePerTenor( Real delta, int i ) = 0;
	
	virtual Real GetSpotValue() const = 0;
	virtual int GetTenorCount( Period tenor ) const = 0;
	virtual int GetChunkCount( Period tenor ) const = 0;
	virtual std::vector<Size> GetVegaTenors( Period tenor ) const = 0;

	virtual boost::shared_ptr<TiXmlElement> ParseToXML( Period tenor ) const = 0;

	virtual void ParseCurveInfo( TiXmlElement& parent, Period remainingTime ) = 0;

private:
	std::wstring m_curveName;
};
