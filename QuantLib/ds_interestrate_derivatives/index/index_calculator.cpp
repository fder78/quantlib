
#include <ds_interestrate_derivatives/index/index_calculator.hpp>

namespace QuantLib {

	IndexCalculator::IndexCalculator(
		IndexType type, 
		boost::shared_ptr<OneFactorAffineModel> model) :
	type_(type), model_(model) {}
	
	const Rate IndexCalculator::operator()(Time t, Period tenor, Real shortRate) const {
		QL_REQUIRE(type_== IborRate, "Index type is not Ibor Rate.");
		Time dt = years(tenor);
		Real df = model_->discountBond(t, t+dt, shortRate);
		Rate iborRate = 1/dt*(1/df-1.0);
		return iborRate;
	}

	const Rate IndexCalculator::operator()(Time t, Period length, Period tenor, Real shortRate) const {
		QL_REQUIRE(type_== SwapRate, "Index type is not Swap Rate.");
		Time len = years(length);
		Time ten = years(tenor);
		Size n = Size(len/ten+0.5);
		Real cmsRate = 0.0;
		for (Size i=0; i<n; ++i)
			cmsRate += ten * model_->discountBond(t, t + ten*(i+1), shortRate);
		cmsRate = (1.0 - model_->discountBond(t, t + len, shortRate)) / cmsRate;
		return cmsRate;
	}
}