
#pragma once

#include <ql/methods/finitedifferences/stepcondition.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmesher.hpp>
#include <ql/methods/finitedifferences/utilities/fdminnervaluecalculator.hpp>
#include <ql/math/matrix.hpp>

#include <ql/processes/blackscholesprocess.hpp>
#include <ds_interestrate_derivatives/model/generalized_hullwhite.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <ql/cashflow.hpp>
#include <ql/math/distributions/normaldistribution.hpp>
#include <ql/math/distributions/bivariatenormaldistribution.hpp>

namespace QuantLib {

	class Fdm_R2Dual_RA_StepCondition : public StepCondition<Array> {
	public:
		Fdm_R2Dual_RA_StepCondition(
			const boost::shared_ptr<FdmMesher> & mesher,
			const boost::shared_ptr<Generalized_HullWhite> hwProcess0,
			const boost::shared_ptr<Generalized_HullWhite> hwProcess1,	
			const Handle<YieldTermStructure>& discTS,
			const std::vector<Time> stoppingTimes,
			const Real rho,
			const Leg& cashflows,
			const std::vector<std::vector<Real > >& down,
			const std::vector<std::vector<Real > >& up,
			const Time firstT,
			const Real firstAccrual,
			const Time firstCall,
			const Real callPrice,
			const Real inverseAlpha = Null<Real>(),
			const Real inverseGearing = Null<Real>(),
			const Real inverseFixing = Null<Real>(),
			const Real cap = Null<Real>(),
			const Real floor = Null<Real>(),
			const Real alpha = Null<Real>(),
			const Real fixingRate = Null<Real>(),
			const std::vector<Matrix> m = std::vector<Matrix>(1,Matrix()),
			const std::vector<Matrix> tenor = std::vector<Matrix>(1,Matrix()));

		void applyTo(Array& a, Time) const;


	private:
		Array getCpn(Real, Real, Size i, Time t, Time firstT, Time deltaT, Real pastAccrual) const;

		const boost::shared_ptr<FdmMesher> mesher_;
		Array cpn_;
		Real rho_, alpha_, fixingRate_;
		Real inverseGearing_, cap_, floor_, inverseFixing_, inverseAlpha_;
		Real firstTime_, callPrice_, firstT_, pastAccrual_;
		//Real firstTime_, callPrice_;
		std::vector<std::vector<Real > > down_;
		std::vector<std::vector<Real > > up_;
		boost::shared_ptr<Generalized_HullWhite> hw0_;
		boost::shared_ptr<Generalized_HullWhite> hw1_;
		Leg cashflows_;
		std::vector<Time> stoppingTimes_;
		Handle<YieldTermStructure> discTS_;

		std::vector<Matrix> tenor_;
		std::vector<Matrix> m_;
		CumulativeNormalDistribution f_;
		BivariateCumulativeNormalDistributionWe04DP f2_;

	};
}

