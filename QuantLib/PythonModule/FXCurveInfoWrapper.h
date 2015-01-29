#pragma once

#include "YieldCurveInfoWrapper.h"

class InterestRateCurveInfoWrapper;

class FXCurveInfoWrapper : public YieldCurveInfoWrapper
{
public:
	FXCurveInfoWrapper( Real spot, const std::wstring& curveName, boost::shared_ptr<InterestRateCurveInfoWrapper> irs, boost::shared_ptr<InterestRateCurveInfoWrapper> crs, Real fxVol, boost::shared_ptr<FXCurveData> fxCurveData );

	virtual void ShiftCurve( Real delta ) override { }
	virtual void ShiftCurvePerTenor( Real delta, int i ) override { }

	virtual boost::shared_ptr<StochasticProcess1D> GetStochasticProcess( Period tenor ) const override;

	virtual Real GetSpotValue() const override { return m_spot; }
	virtual int GetTenorCount( Period tenor ) const override { return m_tenorCount; }
	virtual int GetChunkCount( Period tenor ) const override;
	virtual std::vector<Size> GetVegaTenors( Period tenor ) const override { return std::vector<Size>( 1, 1 ); }

	virtual boost::shared_ptr<TiXmlElement> ParseToXML( Period tenor ) const override;
	virtual void ParseCurveInfo( TiXmlElement& parent, Period remainingTime ) override;

private:
	Real m_spot;

	boost::shared_ptr<YieldTermStructure> m_irs;
	boost::shared_ptr<YieldTermStructure> m_crs;

	boost::shared_ptr<InterestRateCurveInfoWrapper> m_irsWrapper;
	boost::shared_ptr<InterestRateCurveInfoWrapper> m_crsWrapper;

	boost::shared_ptr<FXCurveData> m_curveData;
	Real m_vol;

	int m_tenorCount;
};