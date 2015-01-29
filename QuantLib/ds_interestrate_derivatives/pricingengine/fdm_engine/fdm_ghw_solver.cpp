
#include <ds_interestrate_derivatives/pricingengine/fdm_engine/fdm_ghw_solver.hpp>
#include <ds_interestrate_derivatives/pricingengine/fdm_engine/fdm_ghw_op.hpp>
#include <ql/methods/finitedifferences/operators/fdm2dblackscholesop.hpp>

#include <ql/math/interpolations/cubicinterpolation.hpp>
#include <ql/math/interpolations/bicubicsplineinterpolation.hpp>
#include <ql/methods/finitedifferences/finitedifferencemodel.hpp>

#include <ql/methods/finitedifferences/stepconditions/fdmstepconditioncomposite.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmesher.hpp>
#include <ql/methods/finitedifferences/stepconditions/fdmsnapshotcondition.hpp>
#include <ql/methods/finitedifferences/utilities/fdmquantohelper.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearoplayout.hpp>
#include <ql/methods/finitedifferences/utilities/fdminnervaluecalculator.hpp>


namespace QuantLib {

	FdmR2HWSolver::FdmR2HWSolver(
		const boost::shared_ptr<Generalized_HullWhite>& hw0,
		const boost::shared_ptr<Generalized_HullWhite>& hw1,
		const Handle<YieldTermStructure>& discTS,
		Real corrXR,
		const boost::shared_ptr<FdmMesher>& mesher,
		const FdmBoundaryConditionSet& bcSet,
		const boost::shared_ptr<FdmStepConditionComposite> & condition,
		const boost::shared_ptr<FdmInnerValueCalculator>& calculator,
		Time maturity,
		Size timeSteps,
		Size dampingSteps,
		const FdmSchemeDesc& schemeDesc)
		: hw0_(hw0), hw1_(hw1), discTS_(discTS),
		corrXR_(corrXR),
		mesher_(mesher),
		bcSet_(bcSet),
		thetaCondition_(new FdmSnapshotCondition(0.99*std::min(1.0/365.0, condition->stoppingTimes().empty() ? maturity : condition->stoppingTimes().front()))),
		condition_(FdmStepConditionComposite::joinConditions(thetaCondition_, condition)), 
		maturity_(maturity),
		timeSteps_(timeSteps),
		dampingSteps_(dampingSteps),
		schemeDesc_(schemeDesc),
		initialValues_(mesher->layout()->size()),
		resultValues_(mesher->layout()->dim()[1], mesher->layout()->dim()[0]) {

			r0_.reserve(mesher->layout()->dim()[0]);
			r1_.reserve(mesher->layout()->dim()[1]);

			const boost::shared_ptr<FdmLinearOpLayout> layout = mesher->layout();
			const FdmLinearOpIterator endIter = layout->end();
			for (FdmLinearOpIterator iter = layout->begin(); iter != endIter; ++iter) {
					initialValues_[iter.index()] = calculator->innerValue(iter,0);

					if (!iter.coordinates()[1]) {
						r0_.push_back(mesher->location(iter, 0));
					}
					if (!iter.coordinates()[0]) {
						r1_.push_back(mesher->location(iter, 1));
					}
			}
	}

	void FdmR2HWSolver::performCalculations() const {

		boost::shared_ptr<FdmLinearOpComposite> map(new FdmR2dHWOp(
			mesher_, 
			hw0_,
			hw1_,
			discTS_,
			corrXR_,
			QL_MIN_REAL
			));

		Array rhs(initialValues_.size());
		std::copy(initialValues_.begin(), initialValues_.end(), rhs.begin());

		FdmBackwardSolver(map, bcSet_, condition_, schemeDesc_)
			.rollback(rhs, maturity_, 0.0, timeSteps_, dampingSteps_);

		std::copy(rhs.begin(), rhs.end(), resultValues_.begin());
		interpolation_ = boost::shared_ptr<BicubicSpline> (
			new BicubicSpline(r0_.begin(), r0_.end(),			
			r1_.begin(), r1_.end(), resultValues_));

	}

	Real FdmR2HWSolver::valueAt(Real r0, Rate r1) const {

		calculate();
		return interpolation_->operator()(r0, r1);
	}


	Real FdmR2HWSolver::thetaAt(Real r0, Rate r1) const {
		QL_REQUIRE(condition_->stoppingTimes().front() > 0.0,
			"stopping time at zero-> can't calculate theta");

		calculate();
		Matrix thetaValues(resultValues_.rows(), resultValues_.columns());

		const Array& rhs = thetaCondition_->getValues();
		std::copy(rhs.begin(), rhs.end(), thetaValues.begin());

		if( interpolation_->isInRange( r0, r1 ) )
		{
			return 0.;
		}

		return (BicubicSpline(r0_.begin(), r0_.end(), r1_.begin(), r1_.end(), thetaValues)(r0, r1) - valueAt(r0, r1))
			/ thetaCondition_->getTime();
	}
}
