#ifndef mc_ir_early_exercise_path_pricer_hpp
#define mc_ir_early_exercise_path_pricer_hpp

#include <ds_interestrate_derivatives/pricingengine/mc_lsmc_engine.hpp>

#include <ql/time/daycounters/actualactual.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <ql/processes/hullwhiteprocess.hpp>
#include <ql/processes/stochasticprocessarray.hpp>
#include <ql/methods/montecarlo/lsmbasissystem.hpp>
#include <ql/methods/montecarlo/earlyexercisepathpricer.hpp>
#include <boost/function.hpp>


namespace QuantLib {

	//RANGE ACCRUAL PATH PRICER
	class IREarlyExercisePathPricer : public EarlyExercisePathPricer<MultiPath>  {
	public:
		IREarlyExercisePathPricer(
			const Size assetNumber,
			const Real callValue,
			const boost::shared_ptr<StochasticProcessArray> processes,
			const Leg& cashflows,
			Size polynomOrder = 2,
			LsmBasisSystem::PolynomType polynomType = LsmBasisSystem::Monomial) :
		assetNumber_(assetNumber),
		scalingValue_(1.0),
			callValue_(callValue),
			v_           (LsmBasisSystem::multiPathBasisSystem(assetNumber_, polynomOrder, polynomType)),
			processes_(processes), cashflows_(cashflows),
			discProcess_(boost::dynamic_pointer_cast<HullWhiteProcess>(processes->process(0))),
			model_(Handle<YieldTermStructure>(discProcess_->yieldTermStructure()), discProcess_->a(), discProcess_->sigma())
		{
			QL_REQUIRE(   polynomType == LsmBasisSystem::Monomial
				|| polynomType == LsmBasisSystem::Laguerre
				|| polynomType == LsmBasisSystem::Hermite
				|| polynomType == LsmBasisSystem::Hyperbolic
				|| polynomType == LsmBasisSystem::Chebyshev2nd,
				"insufficient polynom type");

			v_.push_back(boost::bind(&IREarlyExercisePathPricer::payoff, this, _1));
			x_.resize(processes_->size()-1);
			for (Size i=1; i<processes_->size(); ++i)
				x_[i-1] = processes_->process(i);
		}
		
		virtual Real cpnCalculate(const MultiPath& path, Size idx) const = 0;
		
		virtual Real operator()(const MultiPath& path, Size idx) const {
				return this->payoff(this->state(path, idx));
		}	
		
		Array state(const MultiPath& path, Size idx) const {
			QL_REQUIRE(path.assetNumber() == assetNumber_, "invalid multipath");

			Array tmp(assetNumber_);
			for (Size i=0; i<assetNumber_; ++i) {
				tmp[i] = path[i][idx]*scalingValue_;
			}
			return tmp;
		}

		Real discFactor(Time t0, Time t1, Rate x) const {
			return model_.discountBond(t0, t1, x);
		}

		std::vector<boost::function1<Real, Array> > basisSystem() const {
			return v_;
		}

	protected:
		Real payoff(const Array& state) const {
			return callValue_;
		}

		const Size assetNumber_;
		const Real callValue_;
		const boost::shared_ptr<StochasticProcessArray> processes_;
		const Leg cashflows_;
		Real scalingValue_;
		std::vector<boost::function1<Real, Array> > v_;

		boost::shared_ptr<HullWhiteProcess> discProcess_;
		HullWhite model_;
		std::vector<boost::shared_ptr<StochasticProcess1D> > x_;
	};
}

#endif