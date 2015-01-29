#include "StdAfx.h"
#include "RangeAccrualDLLSingleton.h"


RangeAccrualDLLSingleton::RangeAccrualDLLSingleton()
{
	m_module = ::LoadLibrary( L"structuredproduct_ir.dll" );
	m_rangeAccrualBondFunc = (rangeaccrualbondptr)GetProcAddress( m_module, "ir_RangeAccrualBond" );
	m_rangeAccrualSwapFunc = (rangeaccrualswapptr)GetProcAddress( m_module, "ir_RangeAccrualSwap" );
	m_addHolidayFunc = (addholidayptr)GetProcAddress( m_module, "addHoliday" );

	AddHoliday();
}

int RangeAccrualDLLSingleton::NoteFunc( ResultInfo* result, const ValuationInfo& valuationInfo, const RangeAccrualBondInfo& bondInfo, const ScheduleInfo& scheduleInfo, const CallPutInfo& callPutInfo, const YieldCurveInfo* discountingTS, const CurveCorrelationInfo& curveCorrelationInfo )
{
	return (*m_rangeAccrualBondFunc)( result, valuationInfo, bondInfo, scheduleInfo, callPutInfo, discountingTS, curveCorrelationInfo );
}

int RangeAccrualDLLSingleton::SwapFunc( ResultInfo* result, const ValuationInfo& valuationInfo, const RangeAccrualBondInfo& bondInfo, const ScheduleInfo& scheduleInfo, const CallPutInfo& callPutInfo, const YieldCurveInfo* discountingTS, const CurveCorrelationInfo& curveCorrelationInfo, const RangeAccrualSwapInfo& swapInfo, const CallableBondInfo& payerBondInfo, const ScheduleInfo& payerSchedule, const YieldCurveInfo* payerDiscountingTS )
{
	return (*m_rangeAccrualSwapFunc)( result, valuationInfo, swapInfo, payerBondInfo, payerSchedule, payerDiscountingTS, bondInfo, scheduleInfo, discountingTS, callPutInfo, curveCorrelationInfo );
}

