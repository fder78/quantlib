#include "StdAfx.h"
#include "yield_curve_bootstrapping.hpp"
#include "xccyratehelper.hpp"
#include <iostream>


namespace QuantLib {

	std::vector<std::pair<Integer, Rate> > yield_curve_bootstrapping(Date evaluationDate,
		std::vector<DepoRateData> deposit,
		std::vector<FutRateData> futures,
		std::vector<FraRateData> fras,
		std::vector<SwapRateData> swaps,
		std::vector<BondRateData> bonds,
		std::vector<XCCySwapRateData> xccys
		) {

		Settings::instance().evaluationDate() = evaluationDate;

		std::vector<boost::shared_ptr<RateHelper> > depoFRASwapBondInstruments;

		for (Size i=0; i<deposit.size(); ++i){
			boost::shared_ptr<RateHelper> depoInst(new DepositRateHelper(
				Handle<Quote>(new SimpleQuote(deposit[i].depoRateQuote)), 
				deposit[i].maturity, 
				deposit[i].fixingDays,
				deposit[i].calendar,
				deposit[i].bdc,
				deposit[i].endOfMonth,
				deposit[i].dayCounter));
			depoFRASwapBondInstruments.push_back(depoInst);
		}

		for (Size i=0; i<futures.size(); ++i) {
			boost::shared_ptr<RateHelper> futInst(new FuturesRateHelper(
				Handle<Quote>(new SimpleQuote(futures[i].futPriceQuote)), 
				futures[i].imm,
				futures[i].matMonths,
				futures[i].calendar, 
				futures[i].bdc,	
				futures[i].endOfMonth, 
				futures[i].dayCounter));
			depoFRASwapBondInstruments.push_back(futInst);
		}

		for (Size i=0; i<fras.size(); ++i) {
			boost::shared_ptr<RateHelper> fraInst(new FraRateHelper(
				Handle<Quote>(new SimpleQuote(fras[i].fraRateQuote)), 
				fras[i].monthsToStart,
				fras[i].monthsToEnd,
				fras[i].fixingDays,
				fras[i].calendar, 
				fras[i].bdc,	
				fras[i].endOfMonth, 
				fras[i].dayCounter));
			depoFRASwapBondInstruments.push_back(fraInst);
		}

		for (Size i=0; i<swaps.size(); ++i) {
			boost::shared_ptr<RateHelper> swapInst(new SwapRateHelper(
				Handle<Quote>(new SimpleQuote(swaps[i].swapRateQuote)),
				swaps[i].maturity,
				swaps[i].calendar,
				swaps[i].fixedFreq, 
				swaps[i].bdc, 
				swaps[i].dayCounter,
				swaps[i].iborIndex));
			depoFRASwapBondInstruments.push_back(swapInst);
		}

		for (Size i=0; i<xccys.size(); ++i) {
			boost::shared_ptr<RateHelper> xccyInst(
				new XCCySwapRateHelper(
				  Handle<Quote>( new SimpleQuote( xccys[ i ].spread ) )
				, xccys[ i ].tenor
				, xccys[ i ].fxSpot
				, xccys[ i ].floatLegCurrency
				, xccys[ i ].floatLegCalendar
				, xccys[ i ].floatLegConvention
				, xccys[ i ].floatLegDayCounter
				, xccys[ i ].floatLegIborIndex

				//fixed leg
				, xccys[ i ].fixedLegCurrency
				, xccys[ i ].fixedLegCalendar
				, xccys[ i ].fixedLegConvention
				, xccys[ i ].fixedLegDayCounter
				, xccys[ i ].fixedLegIborIndex

				//term structures
				, xccys[ i ].floatLegDiscTermStructureHandle
				, xccys[ i ].fixedLegDiscTermStructureHandle
			) );
			depoFRASwapBondInstruments.push_back(xccyInst);
		}

		for (Size i=0; i<bonds.size(); ++i) {
			Schedule schedule(bonds[i].issueDate, bonds[i].maturity, Period( bonds[i].cpnFreq ), bonds[i].calendar,
				Unadjusted, Unadjusted, DateGeneration::Backward, false);
			boost::shared_ptr<FixedRateBondHelper> bondHelper(new FixedRateBondHelper(
				Handle<Quote>(new SimpleQuote(bonds[i].quote)),
				bonds[i].settle,
				100.0,
				schedule,
				std::vector<Rate>(1,bonds[i].cpn),
				bonds[i].dayCounter,
				Unadjusted,
				100.,
				bonds[i].issueDate));
			depoFRASwapBondInstruments.push_back(bondHelper);
		}

		DayCounter termStructureDayCounter = Actual365Fixed();
		double tolerance = 1.0e-10;
		boost::shared_ptr<YieldTermStructure> depoFRASwapTermStructure(new PiecewiseYieldCurve<ZeroYield, Linear>(
			evaluationDate, depoFRASwapBondInstruments,
			termStructureDayCounter,
			tolerance));
		
		RelinkableHandle<YieldTermStructure> discountingTermStructure;
		discountingTermStructure.linkTo(depoFRASwapTermStructure);

		std::vector<std::pair<Integer, Rate> > yieldCurve;
		for (Size i=0; i<depoFRASwapBondInstruments.size(); ++i) {
			std::pair<Integer, Rate> temp;
			temp.first = depoFRASwapBondInstruments[i]->latestDate().serialNumber() - evaluationDate.serialNumber();
			temp.second = discountingTermStructure->zeroRate(depoFRASwapBondInstruments[i]->latestDate(), Actual365Fixed(), Continuous).rate();
			yieldCurve.push_back(temp);
		}

		std::sort(yieldCurve.begin(), yieldCurve.end());
		
		return yieldCurve;

	}
}