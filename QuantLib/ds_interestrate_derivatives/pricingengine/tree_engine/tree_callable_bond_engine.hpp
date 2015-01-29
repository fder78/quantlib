
#ifndef tree_callable_bond_engine_hpp
#define tree_callable_bond_engine_hpp

#include <ql/instruments/bonds/floatingratebond.hpp>
#include <ql/pricingengines/latticeshortratemodelengine.hpp>
#include <ds_interestrate_derivatives/instruments/notes/callable_cpn_bond.hpp>

namespace QuantLib {

	class HwTreeCallableBondEngine
		: public LatticeShortRateModelEngine<CallableCpnBond::arguments, CallableCpnBond::results> {
	public:
		HwTreeCallableBondEngine(
			const boost::shared_ptr<ShortRateModel>& model,
			Size timeSteps,
			const Handle<YieldTermStructure>& termStructure = Handle<YieldTermStructure>(),
			Size pastAccrual = 0);

		HwTreeCallableBondEngine(
			const boost::shared_ptr<ShortRateModel>& model,
			const TimeGrid& timeGrid,
			const Handle<YieldTermStructure>& termStructure = Handle<YieldTermStructure>(),
			Size pastAccrual = 0);

		void calculate() const;
	private:
		Handle<YieldTermStructure> termStructure_;
		Size pastAccruals_;
	};

}


#endif

