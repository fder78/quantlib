
#include "StdAfx.h"
#ifndef yield_curve_bootstrapping_hpp
#define yield_curve_bootstrapping_hpp

namespace QuantLib {

	struct DepoRateData {
		Rate depoRateQuote;
		Period maturity;
		Integer fixingDays;
		Calendar calendar;
		BusinessDayConvention bdc;
		bool endOfMonth;
		DayCounter dayCounter;
	};

	struct FutRateData {
		Real futPriceQuote;
		Date imm;
		Integer matMonths;
		Calendar calendar;
		BusinessDayConvention bdc;
		bool endOfMonth;
		DayCounter dayCounter;
	};

	struct FraRateData {
		Rate fraRateQuote;
		Natural monthsToStart;
		Natural monthsToEnd;
		Natural fixingDays;
		Calendar calendar;
		BusinessDayConvention bdc;
		bool endOfMonth;
		DayCounter dayCounter;
	};

	struct SwapRateData {
		Rate swapRateQuote;
		Period maturity;
		Calendar calendar;
		Frequency fixedFreq;
		BusinessDayConvention bdc;
		DayCounter dayCounter;
		boost::shared_ptr<IborIndex> iborIndex;
	};

	struct BondRateData
	{
		Rate quote;
		Rate cpn;
		Date issueDate;
		Date maturity;
		Natural settle;
		Frequency cpnFreq;
		Calendar calendar;
		DayCounter dayCounter;
	};

	struct XCCySwapRateData
	{
			Rate spread;
			Period tenor;

			//floating leg
			Rate fxSpot;
			Currency floatLegCurrency;
			Calendar floatLegCalendar;
			BusinessDayConvention floatLegConvention;
			DayCounter floatLegDayCounter;
			boost::shared_ptr<IborIndex> floatLegIborIndex;

			//fixed leg
			Currency fixedLegCurrency;
			Calendar fixedLegCalendar;
			BusinessDayConvention fixedLegConvention;
			DayCounter fixedLegDayCounter;
			boost::shared_ptr<IborIndex> fixedLegIborIndex;
			
			//term structures
			Handle<YieldTermStructure> floatLegDiscTermStructureHandle;
			Handle<YieldTermStructure> fixedLegDiscTermStructureHandle;
	};

	std::vector<std::pair<Integer, Rate> > yield_curve_bootstrapping(Date evaluationDate,
		std::vector<DepoRateData> deposit,
		std::vector<FutRateData> futures,
		std::vector<FraRateData> fras,
		std::vector<SwapRateData> swaps,
		std::vector<BondRateData> bonds
		//,std::vector<XCCySwapRateData> xccys
		);

}

#endif