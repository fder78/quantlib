
#include <ds_interestrate_derivatives/pricingengine/fdm_engine/fdm_ra_engine_ghw.hpp>
#include <ds_interestrate_derivatives/pricingengine/fdm_engine/fdm_ra_stepcondition_ghw.hpp>
#include <ds_interestrate_derivatives/pricingengine/fdm_engine/fdm_ghw_solver.hpp>

#include <ql/methods/finitedifferences/stepconditions/fdmstepconditioncomposite.hpp>
#include <ql/methods/finitedifferences/stepconditions/fdmbermudanstepcondition.hpp>
#include <ql/methods/finitedifferences/meshers/uniform1dmesher.hpp>
#include <ql/methods/finitedifferences/meshers/fdmblackscholesmesher.hpp>
#include <ql/experimental/finitedifferences/fdmhullwhitemesher.hpp>
#include <ql/methods/finitedifferences/utilities//fdminnervaluecalculator.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearoplayout.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmeshercomposite.hpp>

namespace QuantLib {

	Fdm_R2_Dual_RA_Engine::Fdm_R2_Dual_RA_Engine(
		const boost::shared_ptr<Generalized_HullWhite>& hw0,
		const boost::shared_ptr<Generalized_HullWhite>& hw1,
		const Handle<YieldTermStructure>& discTS,
		Real corrXR,
		Real pastAccrual,
		Size tGrid, 
		Size xGrid, 
		Size rGrid,
		Size dampingSteps,
		bool controlVariate,
		const FdmSchemeDesc& schemeDesc)
		: hw0_(hw0), hw1_(hw1), corrXR_(corrXR), discTS_(discTS),
		tGrid_(tGrid), 
		xGrid_(xGrid),
		rGrid_(rGrid),
		dampingSteps_(dampingSteps),
		schemeDesc_(schemeDesc),
		controlVariate_(controlVariate),
		pastAccrual_(pastAccrual) {
	}

	void Fdm_R2_Dual_RA_Engine::calculate() const {  

		// 1. Layout  ////////////////////////////////////////////////////////////////////////////////////////////
		std::vector<Size> dim;
		dim.push_back(xGrid_);
		dim.push_back(rGrid_);
		const boost::shared_ptr<FdmLinearOpLayout> layout(new FdmLinearOpLayout(dim));

		// 2. Mesher ////////////////////////////////////////////////////////////////////////////////////////////
		const Time maturity = discTS_->timeFromReference(arguments_.cashflows.back()->date());
		const boost::shared_ptr<Fdm1dMesher> shortRateMesher0(new FdmHullWhiteMesher(xGrid_, hw0_, maturity));
		const boost::shared_ptr<Fdm1dMesher> shortRateMesher1(new FdmHullWhiteMesher(rGrid_, hw1_, maturity));

		std::vector<boost::shared_ptr<Fdm1dMesher> > meshers;
		meshers.push_back(shortRateMesher0);
		meshers.push_back(shortRateMesher1);
		boost::shared_ptr<FdmMesher> mesher(new FdmMesherComposite(layout, meshers));

		// 3. Step conditions  ////////////////////////////////////////////////////////////////////////////////////////////
		std::list<boost::shared_ptr<StepCondition<Array> > > stepConditions;
		std::list<std::vector<Time> > stoppingTimes;

		std::vector<Time> tempTimes;
		for (Size i=0; i<arguments_.cashflows.size()-2; ++i) {
			Time t = discTS_->timeFromReference(arguments_.cashflows[i]->date());
			if (t>0 && discTS_->referenceDate().serialNumber() != arguments_.cashflows[i]->date().serialNumber() )
				tempTimes.push_back(t);
		}		
		Time firstT = (tempTimes.empty()) ? discTS_->timeFromReference(arguments_.cashflows.back()->date()) : tempTimes.front();
		Time firstCallDate = discTS_->timeFromReference(arguments_.callabilitySchedule[0]->date());

		stepConditions.push_back(boost::shared_ptr<StepCondition<Array> >(
			new Fdm_R2Dual_RA_StepCondition(mesher, hw0_, hw1_,
			discTS_,
			tempTimes,
			corrXR_, 
			arguments_.cashflows, 
			arguments_.lowerTriggers,
			arguments_.upperTriggers,
			firstT,
			pastAccrual_, 
			firstCallDate,
			arguments_.callabilitySchedule[0]->price().amount(),
			arguments_.inverseAlpha, arguments_.inverseGearing, arguments_.inverseFixing, arguments_.cap, arguments_.floor,			
			arguments_.alpha,
			arguments_.fixingRate,
			arguments_.coeff,
			arguments_.tenor)));


		stoppingTimes.push_back(tempTimes);
		boost::shared_ptr<FdmStepConditionComposite> conditions(
			new FdmStepConditionComposite(stoppingTimes, stepConditions));
		// 4. Boundary conditions  ////////////////////////////////////////////////////////////////////////////////////////////
		FdmBoundaryConditionSet boundaries;

		// 5. Solver  ////////////////////////////////////////////////////////////////////////////////////////////
		Real callValue = (arguments_.alpha==Null<Real>()) ? arguments_.cashflows.back()->amount() : 0.;
		boost::shared_ptr<Payoff> payoff(new CashOrNothingPayoff(Option::Call, QL_MIN_REAL, callValue));
		boost::shared_ptr<FdmInnerValueCalculator> calculator(new FdmLogInnerValue(payoff, mesher, 0));
		solver_ = boost::shared_ptr<FdmR2HWSolver>(
			new FdmR2HWSolver(
			hw0_,	hw1_, discTS_,
			corrXR_,
			mesher, boundaries, conditions,
			calculator,
			maturity, tGrid_, dampingSteps_,
			schemeDesc_));

		// 6. Results  ////////////////////////////////////////////////////////////////////////////////////////////
		FetchResult( 1. );
	}

	void Fdm_R2_Dual_RA_Engine::FetchResult( Real sLevel ) const
	{
		const Real r0 = hw0_->x0();
		const Rate r1 = hw1_->x0();

		results_.value = solver_->valueAt(r0, r1);
		Real delta1 = solver_->valueAt(r0+0.0001, r1) - solver_->valueAt(r0-0.0001, r1);
		Real delta0 = solver_->valueAt(r0, r1+0.0001) - solver_->valueAt(r0, r1-0.0001);

		Real gamma1 = solver_->valueAt(r0+0.0001, r1) - 2*solver_->valueAt(r0, r1) + solver_->valueAt(r0-0.0001, r1);
		Real gamma0 = solver_->valueAt(r0, r1+0.0001) - 2*solver_->valueAt(r0, r1) + solver_->valueAt(r0, r1-0.0001);

		results_.additionalResults["Price"] = results_.value;
		results_.additionalResults["Delta0"] = delta0 / 2;
		results_.additionalResults["Delta1"] = delta1 / 2;
		results_.additionalResults["Gamma0"] = gamma0;
		results_.additionalResults["Gamma1"] = gamma1;
		results_.additionalResults["Theta"] = solver_->thetaAt(r0, r1);
	}

}
