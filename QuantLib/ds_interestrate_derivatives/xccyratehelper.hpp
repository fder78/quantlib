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

/*! \file xccyratehelpers.hpp
    \brief cross currency swap rate helpers
*/

#ifndef quantlib_xccyratehelpers_hpp
#define quantlib_xccyratehelpers_hpp

#include <ql/termstructures/bootstraphelper.hpp>
#include <ql/time/calendar.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/cashflows/iborcoupon.hpp>

namespace QuantLib {

    class Quote;

    typedef BootstrapHelper<YieldTermStructure> RateHelper;
    typedef RelativeDateBootstrapHelper<YieldTermStructure>
                                                        RelativeDateRateHelper;

 

    //! Rate helper for bootstrapping cross currency swaps
    // Checks: 
    //          - checked only the case of USD/EUR xccy swaps. 
    // ToDos: 
    //          - Forward feature must be completed
    //          - based on the previous point the generalisation to mtm xccy swaps is straightforward
    //
    class XCCySwapRateHelper : public RelativeDateRateHelper {
      public:
		enum Type { BootstrapFlatDiscCurve = -1, BootstrapSpreadDiscCurve = 1 };
        XCCySwapRateHelper( Handle<Quote> spread,
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
							Type discCurveBootstrapType = XCCySwapRateHelper::BootstrapSpreadDiscCurve);

        //! \name RateHelper interface
        //@{
        Real impliedQuote() const;
        void setTermStructure(YieldTermStructure*);
        //@}
        //! \name Visitability
        //@{
        void accept(AcyclicVisitor&);
        //@}
    protected:
        void initializeDates();

        //general parameters
        Period fwdStart_;
		Rate   fxSpot_;
		Period swapTerm_;
		Type discCurveBootstrapType_;

		//flat leg
		Currency flatLegCurrency_;
        Calendar flatLegCalendar_;
        BusinessDayConvention flatLegConvention_;
        DayCounter flatLegDayCount_;
        boost::shared_ptr<IborIndex> flatLegIborIndex_;
        Handle<YieldTermStructure> flatLegDiscTermStructureHandle_;

		//spread leg
		Currency sprdLegCurrency_;
        Calendar sprdLegCalendar_;
        BusinessDayConvention sprdLegConvention_;
        DayCounter sprdLegDayCount_;
        boost::shared_ptr<IborIndex> sprdLegIborIndex_;
        Handle<YieldTermStructure> sprdLegDiscTermStructureHandle_;
        

		//instrument
		boost::shared_ptr<IborLeg> flatLeg_;
        boost::shared_ptr<IborLeg> sprdLeg_;

		//curve going to be optimized
		RelinkableHandle<YieldTermStructure> flatLegDiscRelinkableHandle_;
        RelinkableHandle<YieldTermStructure> sprdLegDiscRelinkableHandle_;

     };

}

#endif
