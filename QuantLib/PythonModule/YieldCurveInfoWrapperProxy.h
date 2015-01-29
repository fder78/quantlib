#pragma once

#include "ShiftOption.h"
class YieldCurveInfoWrapper;

// 해당 커브의 shift된 버젼을 들고 있다.
class YieldCurveInfoWrapperProxy
{
public:
	YieldCurveInfoWrapperProxy( const std::wstring& curveName );

	boost::shared_ptr<YieldCurveInfoWrapper> GetYieldCurveWrapper() const { return m_yieldCurveWrapper; }

	void ShiftCurve( const ShiftOption& so );
	void ShiftCurve( Real delta );
	void ShiftCurvePerTenor( Real delta, int i );

	ShiftOption GetShiftOption() const { return m_shiftOption; }

	std::wstring GetCurveName() const { return m_curveName; }

	boost::shared_ptr<TiXmlElement> ParseToXML( Period period ) const;

private:
	const std::wstring m_curveName;
	boost::shared_ptr<YieldCurveInfoWrapper> m_yieldCurveWrapper;

	ShiftOption m_shiftOption;
};
