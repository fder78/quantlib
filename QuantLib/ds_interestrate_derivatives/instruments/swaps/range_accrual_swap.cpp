
#include <ds_interestrate_derivatives/instruments/swaps/range_accrual_swap.hpp>

namespace QuantLib {
	
	void RangeAccrualSwap::setupArguments(PricingEngine::arguments* args) const{
		RangeAccrualNote::setupArguments(args);
		RangeAccrualSwap::arguments* arguments = dynamic_cast<RangeAccrualSwap::arguments*>(args);
		if (arguments != 0) {
			arguments->side = side_;
			arguments->floatingSchedule = floatingSchedule_;
			arguments->alpha = alpha_;
		}
	}

}