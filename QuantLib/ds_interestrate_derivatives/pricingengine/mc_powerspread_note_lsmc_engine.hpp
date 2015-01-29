
#ifndef mc_powerspread_engine_lsmc_hpp
#define mc_powerspread_engine_lsmc_hpp

#include <ds_interestrate_derivatives/instruments/notes/power_spread_note.hpp>
#include <ds_interestrate_derivatives/pricingengine/mc_ir_lsmc_pathpricer.hpp>
#include <ql/math/distributions/bivariatenormaldistribution.hpp>

#include <ds_interestrate_derivatives/pricingengine/mc_ir_early_exercise_path_pricer.hpp>
#include <ds_interestrate_derivatives/index/index_calculator.hpp>

namespace QuantLib {

	//RANGE ACCRUAL PATH PRICER
	class PowerSpreadPathPricer : public IREarlyExercisePathPricer  {
	public:
		PowerSpreadPathPricer(Size assetNumber,
			const Real callValue,
			const Real pastFixing,
			const boost::shared_ptr<StochasticProcessArray> proecesses,
			const Leg& cashflows,
			const bool isAvg,
			Size polynomOrder = 2,
			LsmBasisSystem::PolynomType polynomType = LsmBasisSystem::Monomial);

		virtual Real cpnCalculate(const MultiPath& path, Size t) const;

	protected:
		const Real pastFixing_;
		const bool isAvg_;
		IndexCalculator indexCalculator_;
	};

	//RANGE ACCRUAL LSMC 평가 엔진
	template <class RNG = PseudoRandom, class S = Statistics>
	class MC_PowerSpread_Engine_LSMC : public MC_LSMC_Engine<PowerSpreadNote::engine, MultiVariate, RNG> {

	public:
		// 생성자
		MC_PowerSpread_Engine_LSMC(
			const boost::shared_ptr<StochasticProcessArray>& processes,
			const Real pastFixing,
			BigNatural seed,
			Size requiredSamples,
			Size nCalibrationSamples,
			bool antitheticVariate = false,
			bool controlVariate = false,
			bool brownianBridge = false)
			: MC_LSMC_Engine<PowerSpreadNote::engine,MultiVariate,RNG>(
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
			pastFixing_(pastFixing) {}

	protected:
		virtual boost::shared_ptr<LSMCPathPricer<MultiPath> > lsmPathPricer() const;
		virtual TimeGrid timeGrid() const;

	private:
		const Real pastFixing_;
	};


	template <class RNG, class S>
	inline TimeGrid MC_PowerSpread_Engine_LSMC<RNG, S>::timeGrid() const {
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
	inline boost::shared_ptr<LSMCPathPricer<MultiPath> >
		MC_PowerSpread_Engine_LSMC<RNG, S>::lsmPathPricer() const {

		boost::shared_ptr<StochasticProcessArray> processArray = boost::dynamic_pointer_cast<StochasticProcessArray>(this->process_);
		QL_REQUIRE(processArray && processArray->size()>0, "Stochastic process array required");

		Date firstCallDate = arguments_.callabilitySchedule[0]->date();
		Real callValue = arguments_.callabilitySchedule[0]->price().amount();

		boost::shared_ptr<PowerSpreadPathPricer> earlyExercisePathPricer;
		
		earlyExercisePathPricer = boost::shared_ptr<PowerSpreadPathPricer>(new PowerSpreadPathPricer(
			processArray->size(), 
			callValue, pastFixing_,
			boost::dynamic_pointer_cast<StochasticProcessArray>(process_),
			arguments_.cashflows,
			arguments_.isAvg));

		TimeGrid times = this->timeGrid();
		std::vector<bool> isCallable(times.size(), false);
		//쿠폰 지급일이 첫번째 call 날짜보다 크면 call가능일을 true로 변경
		for (Size i=0; i<times.size()-1; ++i) {
			if (this->arguments_.cashflows[i]->date() >= firstCallDate)
				isCallable[i+1] = true;
		}

		return boost::shared_ptr<IR_LSMC_PathPricer<MultiPath, PowerSpreadPathPricer> > (
			new IR_LSMC_PathPricer<MultiPath, PowerSpreadPathPricer>(times, isCallable, earlyExercisePathPricer));
	}

}
#endif
