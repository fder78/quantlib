
#ifndef mc_rangeaccrual_engine_lsmc_hpp
#define mc_rangeaccrual_engine_lsmc_hpp

#include <ds_interestrate_derivatives/instruments/notes/range_accrual_note.hpp>
#include <ds_interestrate_derivatives/pricingengine/mc_ir_lsmc_pathpricer.hpp>
#include <ql/math/distributions/bivariatenormaldistribution.hpp>

#include <ds_interestrate_derivatives/pricingengine/mc_ir_early_exercise_path_pricer.hpp>

namespace QuantLib {

	//RANGE ACCRUAL PATH PRICER
	class RangeAccrualPathPricer : public IREarlyExercisePathPricer  {
	public:
		RangeAccrualPathPricer(Size assetNumber,
			const Real callValue,
			const Real pastAccrual,
			const boost::shared_ptr<StochasticProcessArray> proecesses,
			const Leg& cashflows,
			Size polynomOrder = 2,
			LsmBasisSystem::PolynomType polynomType = LsmBasisSystem::Monomial);

		virtual Real cpnCalculate(const MultiPath& path, Size t) const;

	protected:

		const Real pastAccrual_;
		BivariateCumulativeNormalDistributionWe04DP f2_;
		CumulativeNormalDistribution f_;
		std::vector<bool> isGBM_;		

	};

	class RangeAccrualPathPricerMinusCMS : public RangeAccrualPathPricer {
	public:
		RangeAccrualPathPricerMinusCMS(Size assetNumber,
			const Real callValue,
			const Real pastAccrual,
			const Rate fixingCMS,
			const boost::shared_ptr<StochasticProcessArray> proecesses,
			const Leg& cashflows,
			Size polynomOrder = 2,
			LsmBasisSystem::PolynomType polynomType = LsmBasisSystem::Monomial) :
		RangeAccrualPathPricer(assetNumber, callValue, pastAccrual, proecesses, cashflows, polynomOrder, polynomType),
		fixingCMS_(fixingCMS) {}

		virtual Real cpnCalculate(const MultiPath& path, Size t) const;

	private:
		const Rate fixingCMS_;

	};



	//RANGE ACCRUAL LSMC �� ����
	template <class RNG = PseudoRandom, class S = Statistics>
	class MC_RangeAccrual_Engine_LSMC : public MC_LSMC_Engine<RangeAccrualNote::engine, MultiVariate, RNG> {

	public:
		// ������
		MC_RangeAccrual_Engine_LSMC(
			const boost::shared_ptr<StochasticProcessArray>& processes,
			const Real pastAccrual,
			BigNatural seed,
			Size requiredSamples,
			Size nCalibrationSamples,
			bool antitheticVariate = false,
			bool controlVariate = false,
			bool brownianBridge = false,
			Size productCode = 0,
			Rate pastFixing = 0.0)
			: MC_LSMC_Engine<RangeAccrualNote::engine,MultiVariate,RNG>(
			processes,
			Null<Size>(),
			Null<Size>(),
			brownianBridge,
			antitheticVariate,
			controlVariate,
			requiredSamples,
			Null<Real>(),
			Null<Size>(),
			seed,
			nCalibrationSamples), 
			pastAccrual_(pastAccrual), productCode_(productCode), pastFixing_(pastFixing) {}

	protected:
		virtual boost::shared_ptr<LSMCPathPricer<MultiPath> > lsmPathPricer() const;
		virtual TimeGrid timeGrid() const;

	private:
		const Real pastAccrual_;
		const Size productCode_;
		const Rate pastFixing_;

	};


	template <class RNG, class S>
	inline TimeGrid MC_RangeAccrual_Engine_LSMC<RNG, S>::timeGrid() const {
		boost::shared_ptr<StochasticProcessArray> processes(boost::dynamic_pointer_cast<StochasticProcessArray>(process_));
		std::vector<Real> times;
		for (Size i=0; i<this->arguments_.cashflows.size()-1; ++i){
			Time t = ActualActual().yearFraction(Settings::instance().evaluationDate(), this->arguments_.cashflows[i]->date());
			if (t>0.0)
				times.push_back(t);
		}
		return TimeGrid(times.begin(), times.end());
	}


	template <class RNG, class S>
	inline boost::shared_ptr<LSMCPathPricer<MultiPath> >	MC_RangeAccrual_Engine_LSMC<RNG, S>::lsmPathPricer() const {

			boost::shared_ptr<StochasticProcessArray> processArray = boost::dynamic_pointer_cast<StochasticProcessArray>(this->process_);
			QL_REQUIRE(processArray && processArray->size()>0, "Stochastic process array required");

			Date firstCallDate = arguments_.callabilitySchedule[0]->date();
			Real callValue = arguments_.callabilitySchedule[0]->price().amount();

			boost::shared_ptr<RangeAccrualPathPricer> earlyExercisePathPricer;
			if (productCode_==1) 
				earlyExercisePathPricer = boost::shared_ptr<RangeAccrualPathPricer>(new RangeAccrualPathPricerMinusCMS(
				processArray->size(), 
				callValue, pastAccrual_, pastFixing_,
				boost::dynamic_pointer_cast<StochasticProcessArray>(process_),
				arguments_.cashflows));
			else
				earlyExercisePathPricer = boost::shared_ptr<RangeAccrualPathPricer>(new RangeAccrualPathPricer(
				processArray->size(), 
				callValue, pastAccrual_,
				boost::dynamic_pointer_cast<StochasticProcessArray>(process_),
				arguments_.cashflows));

			TimeGrid times = this->timeGrid();
			std::vector<bool> isCallable(times.size(), false);
			int diff = this->arguments_.cashflows.size() - times.size();
			//���� �������� ù��° call ��¥���� ũ�� call�������� true�� ����
			for (Size i=0; i<times.size()-1; ++i) {
				if (this->arguments_.cashflows[i + diff]->date() >= firstCallDate)
					isCallable[i+1] = true;
			}

			return boost::shared_ptr<IR_LSMC_PathPricer<MultiPath, RangeAccrualPathPricer> > (
				new IR_LSMC_PathPricer<MultiPath, RangeAccrualPathPricer>(times, isCallable, earlyExercisePathPricer));
	}

}
#endif
