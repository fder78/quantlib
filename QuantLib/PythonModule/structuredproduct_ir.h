#pragma once
// 다음 ifdef 블록은 DLL에서 내보내기하는 작업을 쉽게 해 주는 매크로를 만드는 
// 표준 방식입니다. 이 DLL에 들어 있는 파일은 모두 명령줄에 정의된 _EXPORTS 기호로
// 컴파일되며, 동일한 DLL을 사용하는 다른 프로젝트에서는 이 기호를 정의할 수 없습니다.
// 이렇게 하면 소스 파일에 이 파일이 들어 있는 다른 모든 프로젝트에서는 
// STRUCTUREDPRODUCT_IR_API 함수를 DLL에서 가져오는 것으로 보고, 이 DLL은
// 이 DLL은 해당 매크로로 정의된 기호가 내보내지는 것으로 봅니다.
#ifdef STATIC
	#define STRUCTUREDPRODUCT_IR_API
#else
	#ifdef STRUCTUREDPRODUCT_IR_EXPORTS
		#define STRUCTUREDPRODUCT_IR_API __declspec(dllexport)
	#else
		#define STRUCTUREDPRODUCT_IR_API __declspec(dllimport)
	#endif
#endif

extern "C"{

	/* YYYY-MM-DD YYYYMMDD YYYY/MM/DD */
	typedef const char* DATE_T;

	/* IRINDEX_T
	CD91 | USD Libor | EUR Libor | KRW CMS | USD CMS | EUR CMS | FIXED */
	typedef const char* IRINDEX_T;

	/* DAYCOUNTER_T
	Actual/360 | Actual/365(Fixed) | Actual/Actual(ISDA) | Actual/Actual(ISMA) | Actual/Actual(Euro) | 30/360 | 30E/360 | SimpleDayCounter */
	typedef const char* DAYCOUNTER_T;

	/* BDCONVENTION_T
	Following | ModifiedFollowing | Preceding | ModifiedPreceding | Unadjusted */
	typedef const char* BDCONVENTION_T;

	/* GENRULE_T
	Backward | Forward */
	typedef const char* GENRULE_T;

	/* CALENDAR_T
	Seoul | NewYork | London | Tokyo*/
	typedef const char* CALENDAR_T;

	/* CALLPUT_T
	Call | Put |	 */
	typedef const char* CALLPUT_T;

	/* CPNTYPE_T
	FixedCpn | FloatingCpn | SpreadCpn */
	typedef const char* CPNTYPE_T;

	struct STRUCTUREDPRODUCT_IR_API ResultInfo{
		double npv;
		//double cleanPrice;
		//double dirtyPrice;
		//double accruedCoupon;
		double duration;
		double convexity;
		double error;
	};

	/*struct STRUCTUREDPRODUCT_IR_API AddHolidayInfo{
		const char* calendar;
		int holidayNum;
		DATE_T* holidays;
	};*/

	struct STRUCTUREDPRODUCT_IR_API ValuationInfo{
		DATE_T valuationDate;
		//bool giveSettings;
		int nSamples;
		int numNodes;
		int stepsPerYear;
		int setSeed;
	};

	//int addHolidayInfoNum;
	//const AddHolidayInfo* addHolidayInfos;

	struct STRUCTUREDPRODUCT_IR_API FixedBondInfo
	{		
		double faceAmount;				
		DAYCOUNTER_T accrualDayCounter;		
		BDCONVENTION_T paymentConvention;
		const double *coupons;
		int	couponsNum;
	};

	struct STRUCTUREDPRODUCT_IR_API BondInfo
	{
		double faceAmount;
		DAYCOUNTER_T accrualDayCounter;
		BDCONVENTION_T paymentConvention;
	};

	
	struct STRUCTUREDPRODUCT_IR_API YieldCurveInfo
	{
		const int *day;
		const double *rate;
		int size;
	};

	struct STRUCTUREDPRODUCT_IR_API HwParams{
		double meanReversion;
		double volatility;
	};

	struct STRUCTUREDPRODUCT_IR_API CurveCorrelationInfo
	{
		int curveNum;
		YieldCurveInfo** curve;
		double** correlationMatrix;
		HwParams* hwParams;
	};

	/* CALLABLEBOND_T
	Fixed | Floating | FloatingAverage */
	typedef const char* CALLABLEBOND_T;

	struct STRUCTUREDPRODUCT_IR_API IborIndexInfo{
		IRINDEX_T index;
		double pastFixing;
		const YieldCurveInfo* termStructure;
	};

	struct STRUCTUREDPRODUCT_IR_API IndexInfo{
		bool isSpread;
		IRINDEX_T index1;
		double pastFixing1;
		const YieldCurveInfo* termStructure1;
		IRINDEX_T index2;
		double pastFixing2;
		const YieldCurveInfo* termStructure2;
	};

	struct STRUCTUREDPRODUCT_IR_API FloatingCouponInfo{
		IborIndexInfo index;
		bool inArrears;
		int capNum;
		int floorNum;
		int gearingNum;
		int spreadNum;
		const double *cap;
		const double *floor;
		const double *gearing;
		const double *spread;
	};

	struct STRUCTUREDPRODUCT_IR_API FixedCouponInfo{
		int couponsNum;
		const double *coupons;
	};

	struct STRUCTUREDPRODUCT_IR_API CallableBondInfo{
		double faceAmount;
		DAYCOUNTER_T accrualDayCounter;	
		BDCONVENTION_T paymentConvention;
		CALLABLEBOND_T type;
		// fixed only information
		FixedCouponInfo fixedCouponInfo;
		// floating only information
		FloatingCouponInfo floatingCouponInfo;
	};

	struct STRUCTUREDPRODUCT_IR_API ScheduleInfo{
		DATE_T effectiveDate;
		DATE_T terminationDate;
		int couponTenor; // tenor = 0 to american, tenor = -1 to once
		CALENDAR_T  calendar;
		BDCONVENTION_T bdConvention;
		BDCONVENTION_T terminalConvention;
		GENRULE_T rule;
		bool endOfMonth;
		DATE_T firstDate;
		DATE_T nextToLastDate;
	};

	struct STRUCTUREDPRODUCT_IR_API CallPutInfo{
		CALLPUT_T callPut;
		double value;
		DATE_T startDate;
		DATE_T endDate;
		int tenor; // tenor = 0 to american, tenor = -1 to once
	};
	
	struct STRUCTUREDPRODUCT_IR_API RangeAccrualCouponInfo{
		IndexInfo payIndex;
		IndexInfo obsIndex;
		int gearingNum;
		int spreadNum;
		int lowerTriggerNum;
		int upperTriggerNum;
		const double *gearing;
		const double *spread;
		const double *lowerTrigger;
		const double *upperTrigger;
	};

	struct STRUCTUREDPRODUCT_IR_API AdditionalIndexInfo{
		bool hasIndex;
		IndexInfo obsIndex;
		int lowerTriggerNum;
		int upperTriggerNum;
		const double *lowerTrigger;
		const double *upperTrigger;
	};


	struct STRUCTUREDPRODUCT_IR_API SpreadCouponInfo{
		IndexInfo index;
		bool inArrears;
		int capNum;
		int floorNum;
		int gearingNum;
		int spreadNum;
		double *cap;
		double *floor;
		double *gearing;
		double *spread;
	};


	struct STRUCTUREDPRODUCT_IR_API SpreadCpnBondInfo{
		double faceAmount;
		DAYCOUNTER_T accrualDayCounter;	
		BDCONVENTION_T paymentConvention;
		SpreadCouponInfo couponInfo;
		bool isAverage; // 변경된 부분
	};


	STRUCTUREDPRODUCT_IR_API int ir_SpreadCpnBond
		(ResultInfo *result,
		const ValuationInfo valuationInfo,
		const SpreadCpnBondInfo bondInfo,
		const ScheduleInfo scheduleInfo,
		const CallPutInfo callPutInfo,
		const YieldCurveInfo *discountingTS,
		const CurveCorrelationInfo curveCorrelationInfo);

	typedef int (*spreadcpnbondptr)
		(ResultInfo *result,
		const ValuationInfo valuationInfo,
		const SpreadCpnBondInfo bondInfo,
		const ScheduleInfo scheduleInfo,
		const CallPutInfo callPutInfo,
		const YieldCurveInfo *discountingTS,
		const CurveCorrelationInfo curveCorrelationInfo);

	struct STRUCTUREDPRODUCT_IR_API RangeAccrualBondInfo{
		double faceAmount;
		DAYCOUNTER_T accrualDayCounter;	
		BDCONVENTION_T paymentConvention;
		RangeAccrualCouponInfo couponInfo;
		AdditionalIndexInfo aIndexInfo;
		int nInRange;
	};
	
	STRUCTUREDPRODUCT_IR_API int ir_RangeAccrualBond
		(ResultInfo *result,
		const ValuationInfo valuationInfo,
		const RangeAccrualBondInfo bondInfo,
		const ScheduleInfo scheduleInfo,
		const CallPutInfo callPutInfo,
		const YieldCurveInfo *discountingTS,
		const CurveCorrelationInfo curveCorrelationInfo);

	typedef int (*rangeaccrualbondptr)
		(ResultInfo *result,
		const ValuationInfo valuationInfo,
		const RangeAccrualBondInfo bondInfo,
		const ScheduleInfo scheduleInfo,
		const CallPutInfo callPutInfo,
		const YieldCurveInfo *discountingTS,
		const CurveCorrelationInfo curveCorrelationInfo);


	STRUCTUREDPRODUCT_IR_API int addHoliday
		(CALENDAR_T calendar,
		int holidayNum,
		DATE_T* holidays);

	typedef int (*addholidayptr)
		(CALENDAR_T calendar,
		int holidayNum,
		DATE_T* holidays);

	/*SWAP_T
	Payer | Receiver */
	typedef const char* SWAP_T;

	struct STRUCTUREDPRODUCT_IR_API SwapInfo{
		SWAP_T swapType;
		bool hasNotionalExchange;
		//bool isAverage;
	};

	typedef SwapInfo RangeAccrualSwapInfo;

	STRUCTUREDPRODUCT_IR_API int ir_RangeAccrualSwap
		//Valuation
		(ResultInfo *result,
		const ValuationInfo valuationInfo,
		const RangeAccrualSwapInfo rangeAccrualSwapInfo,
		//Callable Bond Part(Fixed/Floating/FloatingAverage)
		const CallableBondInfo payerBondInfo,
		const ScheduleInfo payerScheduleInfo,
		const YieldCurveInfo *payerDiscountingTS,
		//Range Accrual Bond Part
		const RangeAccrualBondInfo receiverBondInfo,
		const ScheduleInfo receiverScheduleInfo,
		const YieldCurveInfo *receiverDiscountingTS,
		//Hull White parameter, Callability, Curve Correlation
		const CallPutInfo callPutInfo,
		const CurveCorrelationInfo curveCorrelationInfo);

	typedef int (*rangeaccrualswapptr)
		//Valuation
		(ResultInfo *result,
		const ValuationInfo valuationInfo,
		const RangeAccrualSwapInfo rangeAccrualSwapInfo,
		//Callable Bond Part(Fixed/Floating/FloatingAverage)
		const CallableBondInfo payerBondInfo,
		const ScheduleInfo payerScheduleInfo,
		const YieldCurveInfo *payerDiscountingTS,
		//Range Accrual Bond Part
		const RangeAccrualBondInfo receiverBondInfo,
		const ScheduleInfo receiverScheduleInfo,
		const YieldCurveInfo *receiverDiscountingTS,
		//Hull White parameter, Callability, Curve Correlation
		const CallPutInfo callPutInfo,
		const CurveCorrelationInfo curveCorrelationInfo);

}