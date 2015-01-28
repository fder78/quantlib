/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
  Copyright (C) 2012 Andre Miemiec
  Copyright (C) 2012 Mehdi Bouassab
                    

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
#include "StdAfx.h"
#include <ql/quote.hpp>
#include <ql/currency.hpp>
#include <ql/cashflows/cashflows.hpp>

#include "xccyratehelper.hpp"

using boost::shared_ptr;

namespace QuantLib {

    namespace {
        void no_deletion(YieldTermStructure*) {}
    }


		XCCySwapRateHelper::XCCySwapRateHelper( const Handle<Quote>& spread,
		const Period& tenor,
		//floating leg
		Rate   fxSpot, 
		const Currency& floatLegCurrency,
		const Calendar& floatLegCalendar,
		const BusinessDayConvention& floatLegConvention,
		const DayCounter& floatLegDayCount,
		const boost::shared_ptr<IborIndex>& floatLegIborIndex,

		//fixed leg
		const Currency& fixedLegCurrency,
		const Calendar& fixedLegCalendar,
		const BusinessDayConvention& fixedLegConvention,
		const DayCounter& fixedLegDayCount,
		const boost::shared_ptr<IborIndex>& fixedLegIborIndex,
		//term structures
		const Handle<YieldTermStructure>& floatLegDiscTermStructureHandle,				
		const Handle<YieldTermStructure>& fixedLegDiscTermStructureHandle,
		//general
		Type discCurveBootstrapType,
		const Period& fwdStart,
		const Real& fxFwd)
	  : RelativeDateRateHelper(spread),swapTerm_(tenor),fwdStart_(fwdStart),fxSpot_(fxSpot), fxFwd_(fxFwd),
      discCurveBootstrapType_(discCurveBootstrapType),
	  floatLegCalendar_(floatLegCalendar),floatLegConvention_(floatLegConvention),
	  floatLegDayCount_(floatLegDayCount),
	  floatLegDiscTermStructureHandle_(floatLegDiscTermStructureHandle),
	  floatLegCurrency_(floatLegCurrency),
	  fixedLegCalendar_(fixedLegCalendar),fixedLegConvention_(fixedLegConvention),
	  fixedLegDayCount_(fixedLegDayCount),
	  fixedLegDiscTermStructureHandle_(fixedLegDiscTermStructureHandle),
	  fixedLegCurrency_(fixedLegCurrency)
	{

		fixedLegIborIndex_ = fixedLegIborIndex->clone(fixedLegDiscRelinkableHandle_);
		fixedLegIborIndex_->unregisterWith(fixedLegDiscRelinkableHandle_);
		

		floatLegIborIndex_ = floatLegIborIndex->clone(floatLegDiscRelinkableHandle_);
		floatLegIborIndex_->unregisterWith(floatLegDiscRelinkableHandle_);
		
	
        //take fixing into account
        registerWith(floatLegIborIndex_);
		registerWith(floatLegDiscTermStructureHandle_);

        registerWith(fixedLegIborIndex_);
		registerWith(fixedLegDiscTermStructureHandle_);

        initializeDates();
    }

    void XCCySwapRateHelper::initializeDates() {

		Date today = Settings::instance().evaluationDate();
		Date SpotDate= floatLegCalendar_.advance(today,floatLegIborIndex_->fixingDays(),Days,floatLegConvention_,false);
        Date StartDate = SpotDate+fwdStart_;

        Date EndDate = StartDate + swapTerm_;
 
		Schedule floatLegSchedule(StartDate,
			                      EndDate,
								  floatLegIborIndex_->tenor(),
								  floatLegCalendar_,
								  floatLegConvention_,
								  floatLegConvention_,
								  DateGeneration::Backward,
								  false);

		//For a forward start xccy, notionals in the two currencies must be equivalent at the first time they are exchanged
		Real floatNotional = fwdStart_!=Period() ? fxFwd_ :  fxSpot_;

		IborLeg floatLeg  = IborLeg(floatLegSchedule,floatLegIborIndex_)
							.withNotionals(floatNotional)
							.withPaymentDayCounter(floatLegDayCount_)
							.withPaymentAdjustment(floatLegConvention_)
							.withFixingDays(floatLegIborIndex_->fixingDays() )
							.withGearings(1.0);
							

		floatLeg_ = boost::shared_ptr<IborLeg>( new IborLeg(floatLeg));

		Schedule sprdLegSchedule( StartDate,
			                      EndDate,
								  fixedLegIborIndex_->tenor(),
								  fixedLegCalendar_,
								  fixedLegConvention_,
								  fixedLegConvention_,
								  DateGeneration::Backward,
								  false);



		IborLeg sprdLeg  = IborLeg(sprdLegSchedule,fixedLegIborIndex_)
							.withNotionals(1.0)
							.withPaymentDayCounter(fixedLegDayCount_)
							.withPaymentAdjustment(fixedLegConvention_)
							.withFixingDays(fixedLegIborIndex_->fixingDays())
							.withGearings(0.0)
							.withSpreads(this->quote()->value());		

		fixedLeg_ = boost::shared_ptr<IborLeg>( new IborLeg(sprdLeg));

        earliestDate_ = StartDate;
        latestDate_   = floatLegCalendar_.adjust(EndDate,floatLegConvention_);

    }

    void XCCySwapRateHelper::setTermStructure(YieldTermStructure* t) {
        // do not set the relinkable handle as an observer -
        // force recalculation when needed
        bool observer = false;

        shared_ptr<YieldTermStructure> temp(t, no_deletion);

		if(discCurveBootstrapType_ == XCCySwapRateHelper::BootstrapFixedDiscCurve){

           floatLegDiscRelinkableHandle_.linkTo(*floatLegDiscTermStructureHandle_, observer);
           fixedLegDiscRelinkableHandle_.linkTo(temp, observer);
		   

		} else {

           floatLegDiscRelinkableHandle_.linkTo(temp, observer);
           fixedLegDiscRelinkableHandle_.linkTo(*fixedLegDiscTermStructureHandle_, observer);

		}

        RelativeDateRateHelper::setTermStructure(t);
    }

    Real XCCySwapRateHelper::impliedQuote() const {

		Date today = Settings::instance().evaluationDate();
		Date float_SpotDate= floatLegCalendar_.advance(today,floatLegIborIndex_->fixingDays(),Days,floatLegConvention_,false);
		Date fixed_SpotDate= fixedLegCalendar_.advance(today,fixedLegIborIndex_->fixingDays(),Days,fixedLegConvention_,false);


        Date strtDate = CashFlows::startDate(*floatLeg_);
        Date termDate = CashFlows::maturityDate(*floatLeg_);

		
		//Setting fixedLegIborIndex_ and floatLegIborIndex_ term structure

		QL_REQUIRE(!(floatLegIborIndex_->forwardingTermStructure().empty()), "floatLegIborIndex_ TermStructure is empty");
		QL_REQUIRE(!(fixedLegIborIndex_->forwardingTermStructure().empty()), "fixedLegIborIndex_ TermStructure is empty");
		
		Real floatNotional = fwdStart_!=Period() ? fxFwd_ :  fxSpot_;

	
		Real npvfloatLegNotionalAtEnd = floatNotional*
			(*floatLegDiscRelinkableHandle_)->discount(termDate)/
			(*floatLegDiscRelinkableHandle_)->discount(float_SpotDate);

		Real npvfloatLeg = CashFlows::npv(*floatLeg_, **floatLegDiscRelinkableHandle_, true, float_SpotDate, float_SpotDate);


		Real npvSprdLegNotionalAtEnd = (*fixedLegDiscRelinkableHandle_)->discount(termDate)/
			(*fixedLegDiscRelinkableHandle_)->discount(fixed_SpotDate);

        Real npvSprdLeg  = CashFlows::npv(*fixedLeg_, **fixedLegDiscRelinkableHandle_, true, fixed_SpotDate, fixed_SpotDate);

        Real npvSprdLegAnnuity  = CashFlows::bps(*fixedLeg_, **fixedLegDiscRelinkableHandle_, true, fixed_SpotDate, fixed_SpotDate);

		Real npvSprdLegWithoutSpread  = npvSprdLeg-10000*npvSprdLegAnnuity*(*this).quote()->value();



        Real implicitQuote  = (npvfloatLeg+npvfloatLegNotionalAtEnd);
		     implicitQuote /= fxSpot_;
             implicitQuote -= npvSprdLegNotionalAtEnd;
             implicitQuote -= npvSprdLegWithoutSpread;
             implicitQuote /= npvSprdLegAnnuity;
             implicitQuote /= 10000;

		return implicitQuote;
		 
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
