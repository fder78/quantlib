#pragma once

#include "pricing_functions/dual_rangeaccrual_swap.hpp"
#include "IRProductParam.h"

class RADaishinParam : public IRProductParam
{
public:
	RADaishinParam();

	virtual void SetDataImpl( TiXmlElement* record ) override;
	virtual Real GetBiasForProduct() const override { return m_biasForScenario; }

	virtual Period GetRemainingTime() const override;

protected:
	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_obs1CurveInfo;
	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_obs2CurveInfo;

	std::wstring m_obs1Ticker;
	std::wstring m_obs2Ticker;

	Schedule m_schedule;

	DayCounter m_accrualDayCounter;

	std::vector<Real> m_accrualRate;

	std::vector<Real> m_lowerBound1;
	std::vector<Real> m_upperBound1;

	std::vector<Real> m_lowerBound2;
	std::vector<Real> m_upperBound2;

	BusinessDayConvention m_bd;

	Real m_rho12;
	Real m_rho1disc;
	Real m_rho2disc;

	RangeAccrualSwap::Side m_side;
	Rate m_alpha;
	Schedule m_swapSchedule;
	Real m_pastFixing;

	std::vector<Real> m_gearing;

	Real m_invAlpha;
	Real m_invGearing;
	Real m_invFixing;
	Real m_invCap;
	Real m_invFloor;
};

std::vector<Date> CalcNInRange( Date evalDate, const std::wstring& code, const Schedule& schedule, const std::vector<Real>& upper, const std::vector<Real>& lower );