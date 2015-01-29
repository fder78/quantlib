#pragma once

#include <ql/handle.hpp>
#include <ql/patterns/lazyobject.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ds_interestrate_derivatives/model/generalized_hullwhite.hpp>
#include <ql/methods/finitedifferences/solvers/fdmbackwardsolver.hpp>
#include <ql/methods/finitedifferences/utilities/fdmdirichletboundary.hpp>


namespace QuantLib {

	class FdmMesher;
	class FdmInnerValueCalculator;
	class FdmSnapshotCondition;
	class FdmStepConditionComposite;
	class BicubicSpline;

	class FdmR2HWSolver : public LazyObject {
	public:
		FdmR2HWSolver(
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
			Size dampingSteps = 0,
			const FdmSchemeDesc& schemeDesc = FdmSchemeDesc::Hundsdorfer());

		Real valueAt(Real x, Rate r) const;
		Real thetaAt(Real x, Rate r) const;

		// First and second order derivative with respect to S_t. 
		// Please note that this is not the "model implied" delta or gamma.
		// E.g. see Fabio Mercurio, Massimo Morini 
		// "A Note on Hedging with Local and Stochastic Volatility Models",
		// http://papers.ssrn.com/sol3/papers.cfm?abstract_id=1294284  
		//Real deltaAt(Real x, Rate r, Real eps) const;
		//Real gammaAt(Real x, Rate r, Real eps) const;
		//Real deltaYAt(Real x, Rate r, Real eps) const;
		//Real gammaYAt(Real x, Rate r, Real eps) const;

	protected:
		void performCalculations() const;

	private:
		const boost::shared_ptr<Generalized_HullWhite> hw0_, hw1_;
		const Handle<YieldTermStructure> discTS_;
		const Real corrXR_;

		const boost::shared_ptr<FdmMesher> mesher_;
		const FdmBoundaryConditionSet bcSet_;
		const boost::shared_ptr<FdmSnapshotCondition> thetaCondition_;
		const boost::shared_ptr<FdmStepConditionComposite> condition_;
		const Time maturity_;
		const Size timeSteps_;
		const Size dampingSteps_;

		const FdmSchemeDesc schemeDesc_;

		std::vector<Real> r0_, r1_, initialValues_;
		mutable Matrix resultValues_;
		mutable boost::shared_ptr<BicubicSpline> interpolation_;
	};
}

