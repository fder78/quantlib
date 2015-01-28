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
 
    
    
    class XCCySwapRateHelper : public RelativeDateRateHelper {


      public:
		enum Type { BootstrapfloatDiscCurve = -1, BootstrapFixedDiscCurve = 1 };


        XCCySwapRateHelper( const Handle<Quote>& spread,
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
							const Handle<YieldTermStructure>& floatLegDiscTermStructureHandle
							= Handle<YieldTermStructure>(),				
							const Handle<YieldTermStructure>& fixedLegDiscTermStructureHandle
							= Handle<YieldTermStructure>(),
							//general
							Type discCurveBootstrapType = XCCySwapRateHelper::BootstrapFixedDiscCurve,
							const Period& fwdStart = 0*Days,
							const Real& fxFwd=0);

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
		Period swapTerm_;
		Type discCurveBootstrapType_;
		Rate  fxSpot_;
		Real  fxFwd_;


		//float leg
		Currency floatLegCurrency_;
        Calendar floatLegCalendar_;
        BusinessDayConvention floatLegConvention_;
        DayCounter floatLegDayCount_;
        boost::shared_ptr<IborIndex> floatLegIborIndex_;
        Handle<YieldTermStructure> floatLegDiscTermStructureHandle_;

		//spread leg
		Currency fixedLegCurrency_;
        Calendar fixedLegCalendar_;
        BusinessDayConvention fixedLegConvention_;
        DayCounter fixedLegDayCount_;
        boost::shared_ptr<IborIndex> fixedLegIborIndex_;
        Handle<YieldTermStructure> fixedLegDiscTermStructureHandle_;
        

		//instrument
		boost::shared_ptr<IborLeg> floatLeg_;
        boost::shared_ptr<IborLeg> fixedLeg_;
	

		//curve going to be optimized
		RelinkableHandle<YieldTermStructure> floatLegDiscRelinkableHandle_;
        RelinkableHandle<YieldTermStructure> fixedLegDiscRelinkableHandle_;

     };

}

#endif
