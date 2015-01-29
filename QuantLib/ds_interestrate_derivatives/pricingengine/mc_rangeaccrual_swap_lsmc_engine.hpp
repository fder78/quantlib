
#ifndef mc_rangeaccrual_swap_engine_lsmc_hpp
#define mc_rangeaccrual_swap_engine_lsmc_hpp

#include <ds_interestrate_derivatives/instruments/swaps/range_accrual_swap.hpp>
#include <ds_interestrate_derivatives/pricingengine/mc_rangeaccrual_note_lsmc_engine.hpp>


namespace QuantLib {
	
	//RANGE ACCRUAL PATH PRICER
	class RangeAccrualSwapPathPricer : public RangeAccrualPathPricer  {
	public:
		RangeAccrualSwapPathPricer(Size assetNumber,
			const Real callValue,
			const Real pastAccrual,
			const boost::shared_ptr<StochasticProcessArray> proecesses,
			const Leg& cashflows,
			RangeAccrualSwap::Side side,
			Schedule floatingSchedule,
			Rate alpha,
			Rate pastFixing,
			Size polynomOrder = 2,
			LsmBasisSystem::PolynomType polynomType = LsmBasisSystem::Monomial);

		Real floatingCpnCalculate(const MultiPath& path, Size t) const;
		virtual Real cpnCalculate(const MultiPath& path, Size t) const;

	protected:
		RangeAccrualSwap::Side side_;
		Schedule floatingSchedule_;
		Rate alpha_;
		Rate pastFixing_;

	};

	class RangeAccrualSwapPathPricerMinusCMS : public RangeAccrualSwapPathPricer {
	public:
		RangeAccrualSwapPathPricerMinusCMS(Size assetNumber,
			const Real callValue,
			const Real pastAccrual,
			const Rate fixingCMS,
			const boost::shared_ptr<StochasticProcessArray> proecesses,
			const Leg& cashflows,
			RangeAccrualSwap::Side side,
			Schedule floatingSchedule,
			Rate alpha,
			Rate pastFixing,
			Size polynomOrder = 2,
			LsmBasisSystem::PolynomType polynomType = LsmBasisSystem::Monomial) :
		RangeAccrualSwapPathPricer(assetNumber, callValue, pastAccrual, proecesses, cashflows, 
			side, floatingSchedule, alpha, pastFixing, polynomOrder, polynomType), fixingCMS_(fixingCMS) {}

		virtual Real cpnCalculate(const MultiPath& path, Size t) const;

	private:
		const Rate fixingCMS_;

	};


	
	//RANGE ACCRUAL LSMC 평가 엔진
	template <class RNG = PseudoRandom, class S = Statistics>
	class MC_RangeAccrual_Swap_Engine_LSMC : public MC_LSMC_Engine<RangeAccrualSwap::engine, MultiVariate, RNG> {

	public:
		// 생성자
		MC_RangeAccrual_Swap_Engine_LSMC(
			const boost::shared_ptr<StochasticProcessArray>& processes,
			const Real pastAccrual,
			const Real floatingFixingRate,
			BigNatural seed,
			Size requiredSamples,
			Size nCalibrationSamples,
			bool antitheticVariate = false,
			bool controlVariate = false,
			bool brownianBridge = false,
			Size productCode = 0,
			Rate pastFixing = 0.0)
			: MC_LSMC_Engine<RangeAccrualSwap::engine,MultiVariate,RNG>(
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
			nCalibrationSamples), floatingFixingRate_(floatingFixingRate),
			pastAccrual_(pastAccrual), productCode_(productCode), pastFixing_(pastFixing) {}

	protected:
		virtual boost::shared_ptr<LSMCPathPricer<MultiPath> > lsmPathPricer() const;
		virtual TimeGrid timeGrid() const;

	private:
		const Real pastAccrual_;
		const Size productCode_;
		const Rate pastFixing_;
		const Rate floatingFixingRate_;

	};


	template <class RNG, class S>
	inline TimeGrid MC_RangeAccrual_Swap_Engine_LSMC<RNG, S>::timeGrid() const {
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
	inline boost::shared_ptr<LSMCPathPricer<MultiPath> >	MC_RangeAccrual_Swap_Engine_LSMC<RNG, S>::lsmPathPricer() const {

		boost::shared_ptr<StochasticProcessArray> processArray = boost::dynamic_pointer_cast<StochasticProcessArray>(this->process_);
		QL_REQUIRE(processArray && processArray->size()>0, "Stochastic process array required");

		Date firstCallDate = arguments_.callabilitySchedule[0]->date();
		Real callValue = 0.0;

		boost::shared_ptr<RangeAccrualPathPricer> earlyExercisePathPricer;
		if (productCode_==1) 
			earlyExercisePathPricer = boost::shared_ptr<RangeAccrualPathPricer>(new RangeAccrualSwapPathPricerMinusCMS(
			processArray->size(), 
			callValue, pastAccrual_, pastFixing_, 
			boost::dynamic_pointer_cast<StochasticProcessArray>(process_),
			arguments_.cashflows,
			arguments_.side,
			arguments_.floatingSchedule,
			arguments_.alpha,
			floatingFixingRate_
			));
		else
			earlyExercisePathPricer = boost::shared_ptr<RangeAccrualPathPricer>(new RangeAccrualSwapPathPricer(
			processArray->size(), 
			callValue, pastAccrual_,
			boost::dynamic_pointer_cast<StochasticProcessArray>(process_),
			arguments_.cashflows,
			arguments_.side,
			arguments_.floatingSchedule,
			arguments_.alpha,
			floatingFixingRate_
			));

		TimeGrid times = this->timeGrid();
		std::vector<bool> isCallable(times.size(), false);
		//쿠폰 지급일이 첫번째 call 날짜보다 크면 call가능일을 true로 변경
		for (Size i=0; i<times.size()-1; ++i) {
			if (this->arguments_.cashflows[i]->date() >= firstCallDate)
				isCallable[i+1] = true;
		}

		return boost::shared_ptr<IR_LSMC_PathPricer<MultiPath, RangeAccrualPathPricer> > (
			new IR_LSMC_PathPricer<MultiPath, RangeAccrualPathPricer>(times, isCallable, earlyExercisePathPricer));
	}

}
#endif
