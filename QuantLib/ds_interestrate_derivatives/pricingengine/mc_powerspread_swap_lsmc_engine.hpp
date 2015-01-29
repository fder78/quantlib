
#ifndef mc_powerspread_swap_engine_lsmc_hpp
#define mc_powerspread_swap_engine_lsmc_hpp

#include <ds_interestrate_derivatives/instruments/swaps/power_spread_swap.hpp>
#include <ds_interestrate_derivatives/pricingengine/mc_powerspread_note_lsmc_engine.hpp>


namespace QuantLib {

	//RANGE ACCRUAL PATH PRICER
	class PowerSpreadSwapPathPricer : public PowerSpreadPathPricer  {
	public:
		PowerSpreadSwapPathPricer(Size assetNumber,
			const Real callValue,
			const Real pastFixing,
			const boost::shared_ptr<StochasticProcessArray> proecesses,
			const Leg& cashflows,
			PowerSpreadSwap::Side side,
			Schedule floatingSchedule,
			Rate alpha,
			Rate floatingFixingRate,
			Size polynomOrder = 2,
			LsmBasisSystem::PolynomType polynomType = LsmBasisSystem::Monomial);

		Real floatingCpnCalculate(const MultiPath& path, Size t) const;
		virtual Real cpnCalculate(const MultiPath& path, Size t) const;

	protected:
		PowerSpreadSwap::Side side_;
		Schedule floatingSchedule_;
		Rate alpha_;
		Rate floatingFixingRate_;

	};


	//RANGE ACCRUAL LSMC �� ����
	template <class RNG = PseudoRandom, class S = Statistics>
	class MC_PowerSpread_Swap_Engine_LSMC : public MC_LSMC_Engine<PowerSpreadSwap::engine, MultiVariate, RNG> {

	public:
		// ������
		MC_PowerSpread_Swap_Engine_LSMC(
			const boost::shared_ptr<StochasticProcessArray>& processes,
			const Real pastFixing,
			const Real floatingFixingRate,
			BigNatural seed,
			Size requiredSamples,
			Size nCalibrationSamples,
			bool antitheticVariate = false,
			bool controlVariate = false,
			bool brownianBridge = false)
			: MC_LSMC_Engine<PowerSpreadSwap::engine,MultiVariate,RNG>(
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
			nCalibrationSamples), floatingFixingRate_(floatingFixingRate), pastFixing_(pastFixing) {}

	protected:
		virtual boost::shared_ptr<LSMCPathPricer<MultiPath> > lsmPathPricer() const;
		virtual TimeGrid timeGrid() const;

	private:
		const Rate pastFixing_;
		const Rate floatingFixingRate_;

	};


	template <class RNG, class S>
	inline TimeGrid MC_PowerSpread_Swap_Engine_LSMC<RNG, S>::timeGrid() const {
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
	inline boost::shared_ptr<LSMCPathPricer<MultiPath> >	MC_PowerSpread_Swap_Engine_LSMC<RNG, S>::lsmPathPricer() const {

		boost::shared_ptr<StochasticProcessArray> processArray = boost::dynamic_pointer_cast<StochasticProcessArray>(this->process_);
		QL_REQUIRE(processArray && processArray->size()>0, "Stochastic process array required");

		Date firstCallDate = arguments_.callabilitySchedule[0]->date();
		Real callValue = 0.0;

		boost::shared_ptr<PowerSpreadPathPricer> earlyExercisePathPricer;
		
		earlyExercisePathPricer = boost::shared_ptr<PowerSpreadPathPricer>(new PowerSpreadSwapPathPricer(
			processArray->size(), 
			callValue, pastFixing_,
			boost::dynamic_pointer_cast<StochasticProcessArray>(process_),
			arguments_.cashflows,
			arguments_.side,
			arguments_.floatingSchedule,
			arguments_.alpha,
			floatingFixingRate_
			));

		TimeGrid times = this->timeGrid();
		std::vector<bool> isCallable(times.size(), false);
		//���� �������� ù��° call ��¥���� ũ�� call�������� true�� ����
		for (Size i=0; i<times.size()-1; ++i) {
			if (this->arguments_.cashflows[i]->date() >= firstCallDate)
				isCallable[i+1] = true;
		}

		return boost::shared_ptr<IR_LSMC_PathPricer<MultiPath, PowerSpreadPathPricer> > (
			new IR_LSMC_PathPricer<MultiPath, PowerSpreadPathPricer>(times, isCallable, earlyExercisePathPricer));
	}

}
#endif
