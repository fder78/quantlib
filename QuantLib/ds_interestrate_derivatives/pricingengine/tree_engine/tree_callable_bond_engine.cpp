
#include <ds_interestrate_derivatives/pricingengine/tree_engine/tree_callable_bond_engine.hpp>
#include <ds_interestrate_derivatives/pricingengine/tree_engine/discretized_callable_bond.hpp>

#include <ql/models/shortrate/onefactormodel.hpp>
#include <ql/cashflows/coupon.hpp>

namespace QuantLib {

    HwTreeCallableBondEngine::HwTreeCallableBondEngine(
                               const boost::shared_ptr<ShortRateModel>& model,
                               Size timeSteps,
                               const Handle<YieldTermStructure>& termStructure,
							   Size pastAccruals)
    : LatticeShortRateModelEngine<CallableCpnBond::arguments, CallableCpnBond::results >(model, timeSteps),
      termStructure_(termStructure), pastAccruals_(pastAccruals) {
        registerWith(termStructure_);
    }

    HwTreeCallableBondEngine::HwTreeCallableBondEngine(
                               const boost::shared_ptr<ShortRateModel>& model,
                               const TimeGrid& timeGrid,
                               const Handle<YieldTermStructure>& termStructure,
							   Size pastAccruals)
    : LatticeShortRateModelEngine<CallableCpnBond::arguments, CallableCpnBond::results>(model, timeGrid),
      termStructure_(termStructure), pastAccruals_(pastAccruals) {
        registerWith(termStructure_);
    }

    void HwTreeCallableBondEngine::calculate() const {

        QL_REQUIRE(!model_.empty(), "no model specified");

        Date referenceDate;
        DayCounter dayCounter;

		boost::shared_ptr<TermStructureConsistentModel> tsmodel = boost::dynamic_pointer_cast<TermStructureConsistentModel>(*model_);
		if (tsmodel) {
			referenceDate = tsmodel->termStructure()->referenceDate();
			dayCounter = tsmodel->termStructure()->dayCounter();
		} else {
			referenceDate = termStructure_->referenceDate();
			dayCounter = termStructure_->dayCounter();
		}

		DiscretizedCallableBond bond(arguments_, referenceDate, dayCounter, *model_, termStructure_.currentLink(), pastAccruals_);
		std::vector<Time> times = bond.mandatoryTimes();
		Time matTime = *(std::max_element(times.begin(), times.end()));

		std::vector<boost::shared_ptr<CashFlow> > temp = arguments_.cashflows;

		boost::shared_ptr<Lattice> lattice;
		if (lattice_) {
			lattice = lattice_;
		} else {
			TimeGrid timeGrid(times.begin(), times.end(), timeSteps_);
			lattice = model_->tree(timeGrid);
		}

		bond.initialize(lattice, matTime);
		bond.rollback(0.0);

		results_.value = bond.presentValue();

    }

}


