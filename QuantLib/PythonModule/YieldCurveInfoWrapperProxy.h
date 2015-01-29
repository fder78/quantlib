#pragma once

#include "ShiftOption.h"
class YieldCurveInfoWrapper;

// �ش� Ŀ���� shift�� ������ ��� �ִ�.
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
