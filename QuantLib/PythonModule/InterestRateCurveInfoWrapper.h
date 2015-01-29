#pragma once

#include "YieldCurveInfoWrapper.h"

class InterestRateCurveInfoWrapper : public YieldCurveInfoWrapper
{
public:
	InterestRateCurveInfoWrapper( const std::wstring& curveName, boost::shared_ptr<YieldCurveData> curveData, const ShiftOption& opt );

	YieldCurveInfo* GetInfo() const;
	boost::shared_ptr<HullWhiteParameters> GetHWParam( Period tenor ) const;
	Real GetPastFixing() const { return m_yields[ 0 ]; }

	boost::shared_ptr<YieldCurveData> GetCurveData() const { return m_curveData; }

	virtual void ShiftCurve( Real delta ) override;
	virtual void ShiftCurvePerTenor( Real delta, int i ) override;

	virtual boost::shared_ptr<StochasticProcess1D> GetStochasticProcess( Period tenor ) const override;

	virtual Real GetSpotValue() const override;
	virtual int GetTenorCount( Period tenor ) const override;
	virtual int GetChunkCount( Period tenor ) const override;
	virtual std::vector<Size> GetVegaTenors( Period tenor ) const override;

	virtual boost::shared_ptr<TiXmlElement> ParseToXML( Period tenor ) const override;

	virtual void ParseCurveInfo( TiXmlElement& parent, Period remainingTime ) override;

	ShiftOption GetShiftOption() const;

private:
	boost::scoped_array<int> m_daysToMat;
	boost::scoped_array<double> m_yields;
	boost::shared_ptr<YieldCurveData> m_curveData;
	boost::shared_ptr<YieldCurveInfo> m_curveInfo;

	ShiftOption m_shiftOption;
	int m_vegaTenorCount;
};


// ¶Ë
class GeneralizedHullWhiteProcess : public HullWhiteProcess
{
public:
	GeneralizedHullWhiteProcess( const Handle<YieldTermStructure>& h, const HullWhiteParameters& hwParam, const std::wstring& curveName, ShiftOption so, Period tenor );

	const HullWhiteTimeDependentParameters& GetParam();

private:
	boost::shared_ptr<HullWhiteTimeDependentParameters> m_param;
	ShiftOption m_shiftOption;
	std::wstring m_curveName;
	Period m_tenor;
};
