
#include <ds_interestrate_derivatives/instruments/swaps/power_spread_swap.hpp>

namespace QuantLib {


	void PowerSpreadSwap::setupArguments(PricingEngine::arguments* args) const{
		PowerSpreadNote::setupArguments(args);
		PowerSpreadSwap::arguments* arguments = dynamic_cast<PowerSpreadSwap::arguments*>(args);
		if (arguments != 0) {
			arguments->side = side_;
			arguments->floatingSchedule = floatingSchedule_;
			arguments->floatingcashflows = floatingcashflows_;
			arguments->alpha = alpha_;
		}
	}

}