
#ifndef cross_currency_swap_hpp
#define cross_currency_swap_hpp

#include <ql/instruments/vanillaswap.hpp>

namespace QuantLib {

    class CrossCCYSwap : public VanillaSwap {
      public:
        class arguments;
        class results;
        class engine;

		CrossCCYSwap(Type type,
			Real nominal_dom,
			Real nominal_frn,
			const Schedule& fixedSchedule,
			Rate fixedRate,
			const DayCounter& fixedDayCount,
			const Schedule& floatSchedule,
			const boost::shared_ptr<IborIndex>& iborIndex,
			Spread spread,
			const DayCounter& floatingDayCount,
			boost::optional<BusinessDayConvention> paymentConvention = boost::none) :
		nominalFRN(nominal_frn),
			VanillaSwap(type, nominal_dom, fixedSchedule, fixedRate, fixedDayCount, floatSchedule, iborIndex, spread, floatingDayCount, paymentConvention)
		{}		

        void setupArguments(PricingEngine::arguments* args) const;
        void fetchResults(const PricingEngine::results*) const;

      private:
        Real nominalFRN;

    };
	
    //! %Arguments for simple swap calculation
    class CrossCCYSwap::arguments : public VanillaSwap::arguments {
      public:
        arguments() : type(Receiver),
                      nominal(Null<Real>()) {}
        Real nominalFRN;
        void validate() const;
    };

    class CrossCCYSwap::engine : public GenericEngine<CrossCCYSwap::arguments,
                                                     CrossCCYSwap::results> {};


}

#endif
