#pragma once

#include "PerProcessSingleton.h"
#include "structuredproduct_ir.h"
#include "IProductParam.h"

struct ValuationInfo;


class RangeAccrualDLLSingleton : public PerProcessSingleton<RangeAccrualDLLSingleton>
{
public:
	RangeAccrualDLLSingleton();
	int NoteFunc( ResultInfo* result,	const ValuationInfo& valuationInfo, const RangeAccrualBondInfo& bondInfo, const ScheduleInfo& scheduleInfo, const CallPutInfo& callPutInfo, const YieldCurveInfo* discountingTS, const CurveCorrelationInfo& curveCorrelationInfo);
	int SwapFunc( ResultInfo* result,	const ValuationInfo& valuationInfo, const RangeAccrualBondInfo& bondInfo, const ScheduleInfo& scheduleInfo, const CallPutInfo& callPutInfo, const YieldCurveInfo* discountingTS, const CurveCorrelationInfo& curveCorrelationInfo, const RangeAccrualSwapInfo& swapInfo, const CallableBondInfo& payerBondInfo, const ScheduleInfo& payerSchedule, const YieldCurveInfo* payerDiscountingTS );

private:
	void AddHoliday()
	{
		// TODO: 도시별 휴일 입력
	}

private:
	HMODULE m_module;

	rangeaccrualbondptr m_rangeAccrualBondFunc;
	rangeaccrualswapptr m_rangeAccrualSwapFunc;
	addholidayptr m_addHolidayFunc;
};
