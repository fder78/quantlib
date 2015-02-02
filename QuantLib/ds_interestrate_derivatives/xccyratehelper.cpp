/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2012 Andre Miemiec

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include "xccyratehelper.hpp"
#include <ql/quote.hpp>
#include <ql/currency.hpp>
#include <ql/cashflows/cashflows.hpp>

using boost::shared_ptr;

namespace QuantLib {

    namespace {
        void no_deletion(YieldTermStructure*) {}
    }


	XCCySwapRateHelper::XCCySwapRateHelper( Handle<Quote> spread,
		                                    Period swapTerm,
		                                    //flat leg
					                        Currency flatLegCurrency,
											Calendar flatLegCalendar,
											BusinessDayConvention flatLegConvention,
											DayCounter flatLegDayCount,
											boost::shared_ptr<IborIndex> flatLegIborIndex,
											Handle<YieldTermStructure> flatLegDiscTermStructureHandle,
											//spread leg
											Currency spreadLegCurrency,
											Calendar spreadLegCalendar,
											BusinessDayConvention spreadLegConvention,
											DayCounter spreadLegDayCount,
											boost::shared_ptr<IborIndex> spreadLegIborIndex,
											Handle<YieldTermStructure> spreadLegDiscTermStructureHandle,
											//general
											Rate   fxSpot,
											Period fwdStart,
											Type discCurveBootstrapType)
	  : RelativeDateRateHelper(spread),swapTerm_(swapTerm),fwdStart_(fwdStart),fxSpot_(fxSpot),
      discCurveBootstrapType_(discCurveBootstrapType),
	  flatLegCalendar_(flatLegCalendar),flatLegConvention_(flatLegConvention),
	  flatLegDayCount_(flatLegDayCount),flatLegIborIndex_(flatLegIborIndex),
	  flatLegDiscTermStructureHandle_(flatLegDiscTermStructureHandle),
	  flatLegCurrency_(flatLegCurrency),
	  sprdLegCalendar_(spreadLegCalendar),sprdLegConvention_(spreadLegConvention),
	  sprdLegDayCount_(spreadLegDayCount),sprdLegIborIndex_(spreadLegIborIndex),
	  sprdLegDiscTermStructureHandle_(spreadLegDiscTermStructureHandle),
	  sprdLegCurrency_(spreadLegCurrency)
	{
		QL_REQUIRE( (flatLegCurrency != spreadLegCurrency),"Currencies should differ!");

        // take fixing into account
        registerWith(flatLegIborIndex_);
		registerWith(flatLegDiscTermStructureHandle_);

        registerWith(sprdLegIborIndex_);
		registerWith(sprdLegDiscTermStructureHandle_);




        initializeDates();
    }

    void XCCySwapRateHelper::initializeDates() {

		Date today = Settings::instance().evaluationDate();

        Date effectiveDate = flatLegCalendar_.advance(today,flatLegIborIndex_->fixingDays(),Days,flatLegConvention_,false);

        Date terminationDate = effectiveDate + swapTerm_;

		Schedule flatLegSchedule(effectiveDate,
			                      terminationDate,
								  flatLegIborIndex_->tenor(),
								  flatLegCalendar_,
								  flatLegConvention_,
								  flatLegConvention_,
								  DateGeneration::Backward,
								  false);

		IborLeg flatLeg  = IborLeg(flatLegSchedule,flatLegIborIndex_)
							.withNotionals(1000000*fxSpot_)
							.withPaymentDayCounter(flatLegDayCount_)
							.withPaymentAdjustment(flatLegConvention_)
							.withFixingDays(flatLegIborIndex_->fixingDays() )
							.withGearings(1.0);

		flatLeg_ = boost::shared_ptr<IborLeg>( new IborLeg(flatLeg));

		Schedule sprdLegSchedule(effectiveDate,
			                      terminationDate,
								  sprdLegIborIndex_->tenor(),
								  sprdLegCalendar_,
								  sprdLegConvention_,
								  sprdLegConvention_,
								  DateGeneration::Backward,
								  false);

		IborLeg sprdLeg  = IborLeg(sprdLegSchedule,sprdLegIborIndex_)
							.withNotionals(1000000)
							.withPaymentDayCounter(sprdLegDayCount_)
							.withPaymentAdjustment(sprdLegConvention_)
							.withFixingDays(sprdLegIborIndex_->fixingDays())
							.withGearings(1.0)
							.withSpreads(this->quote()->value());		

		sprdLeg_ = boost::shared_ptr<IborLeg>( new IborLeg(sprdLeg));

        earliestDate_ = effectiveDate;
        latestDate_   = flatLegCalendar_.adjust(terminationDate,flatLegConvention_);

    }

    void XCCySwapRateHelper::setTermStructure(YieldTermStructure* t) {
        // do not set the relinkable handle as an observer -
        // force recalculation when needed
        bool observer = false;

        shared_ptr<YieldTermStructure> temp(t, no_deletion);

		if(discCurveBootstrapType_ == XCCySwapRateHelper::BootstrapSpreadDiscCurve){
           flatLegDiscRelinkableHandle_.linkTo(*flatLegDiscTermStructureHandle_, observer);
           sprdLegDiscRelinkableHandle_.linkTo(temp, observer);
		} else {
           flatLegDiscRelinkableHandle_.linkTo(temp, observer);
           sprdLegDiscRelinkableHandle_.linkTo(*sprdLegDiscTermStructureHandle_, observer);
		}

        RelativeDateRateHelper::setTermStructure(t);
    }


    Real XCCySwapRateHelper::impliedQuote() const {

		Date today = Settings::instance().evaluationDate();

        Date strtDate = CashFlows::startDate(*flatLeg_);
        Date termDate = CashFlows::maturityDate(*flatLeg_);


		Real npvFlatLegNotionalAtEnd = 1000000*fxSpot_*
			                           (*flatLegDiscRelinkableHandle_)->discount(termDate)/
									   (*flatLegDiscRelinkableHandle_)->discount(strtDate);

        Real npvFlatLeg = CashFlows::npv(*flatLeg_, **flatLegDiscRelinkableHandle_, true, strtDate, strtDate);

		Real npvSprdLegNotionalAtEnd = 1000000*
			                           (*sprdLegDiscRelinkableHandle_)->discount(termDate)/
									   (*sprdLegDiscRelinkableHandle_)->discount(strtDate);

		Real npvSprdLeg  = CashFlows::npv(*sprdLeg_, **sprdLegDiscRelinkableHandle_, true, strtDate, strtDate);


        Real npvSprdLegAnnuity  = CashFlows::bps(*sprdLeg_, **sprdLegDiscRelinkableHandle_, true, strtDate, strtDate);

		Real npvSprdLegWithoutSpread  = npvSprdLeg-10000*npvSprdLegAnnuity*(*this).quote()->value();



        Real impicitQuote  = (npvFlatLeg+npvFlatLegNotionalAtEnd);
		     impicitQuote /= fxSpot_;
             impicitQuote -= npvSprdLegNotionalAtEnd;
             impicitQuote -= npvSprdLegWithoutSpread;
             impicitQuote /= npvSprdLegAnnuity;
             impicitQuote /= 10000;

		return impicitQuote;
		 
    }

    void XCCySwapRateHelper::accept(AcyclicVisitor& v) {
        Visitor<XCCySwapRateHelper>* v1 =
            dynamic_cast<Visitor<XCCySwapRateHelper>*>(&v);
        if (v1 != 0)
            v1->visit(*this);
        else
            RateHelper::accept(v);
    }

}
