#pragma once

#include "IProductParam.h"

class YieldCurveInfoWrapperProxy;

class FXDigitalParam : public IProductParam
{
public:
	FXDigitalParam() {};
	virtual void Calculate() override;
	virtual void FetchResult() override;
	virtual void SetDataImpl( TiXmlElement* record ) override;

private:
	Real m_result;

	Real m_notional;
	Date m_effectiveDate;
	Date m_terminationDate;

	Real m_basePrice;
	Real m_vol;

	Option::Type m_optionType;
	Real m_rebate;
	Real m_slope;
	Real m_strike;

	bool m_calcDelta;
	bool m_calcVega;
	bool m_calcGamma;
	bool m_calcXGamma;

	std::wstring m_underlyingCode;
	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_underlyingCurve;
};
