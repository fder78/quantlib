
#ifndef mc_lsmc_engine_hpp
#define mc_lsmc_engine_hpp

#include <ql/pricingengines/mcsimulation.hpp>
#include <ds_interestrate_derivatives/pricingengine/mc_lsmc_pathpricer.hpp>

namespace QuantLib {

	//LSMC 엔진의 기본형
	template <class GenericEngine, template <class> class MC, class RNG, class S = Statistics>
	class MC_LSMC_Engine : public GenericEngine, public McSimulation<MC,RNG,S> {

	public:
		typedef typename MC<RNG>::path_type path_type;
		typedef typename McSimulation<MC,RNG,S>::stats_type stats_type;
		typedef typename McSimulation<MC,RNG,S>::path_pricer_type path_pricer_type;
		typedef typename McSimulation<MC,RNG,S>::path_generator_type path_generator_type;

		MC_LSMC_Engine(
			const boost::shared_ptr<StochasticProcess>& process,
			Size timeSteps,
			Size timeStepsPerYear,
			bool brownianBridge,
			bool antitheticVariate,
			bool controlVariate,
			Size requiredSamples,
			Real requiredTolerance,
			Size maxSamples,
			BigNatural seed,
			Size nCalibrationSamples = Null<Size>());

		void calculate() const;

	protected:
		virtual boost::shared_ptr<LSMCPathPricer<path_type> > lsmPathPricer() const = 0;
		virtual TimeGrid timeGrid() const = 0;
		virtual boost::shared_ptr<path_pricer_type> pathPricer() const;  //PATH PRICER 단순 리턴
		virtual boost::shared_ptr<path_generator_type> pathGenerator() const;  //공통으로 쓰임

		boost::shared_ptr<StochasticProcess> process_;
		const Size timeSteps_;
		const Size timeStepsPerYear_;
		const bool brownianBridge_;
		const Size requiredSamples_;
		const Real requiredTolerance_;
		const Size maxSamples_;
		const Size seed_;
		const Size nCalibrationSamples_;

		mutable boost::shared_ptr<LSMCPathPricer<path_type> > pathPricer_;
	};

	//생성자
	template <class GenericEngine, template <class> class MC, class RNG, class S>
	inline MC_LSMC_Engine<GenericEngine,MC,RNG,S>::MC_LSMC_Engine(
		const boost::shared_ptr<StochasticProcess>& process,
		Size timeSteps,
		Size timeStepsPerYear,
		bool brownianBridge,
		bool antitheticVariate,
		bool controlVariate,
		Size requiredSamples,
		Real requiredTolerance,
		Size maxSamples,
		BigNatural seed,
		Size nCalibrationSamples)
		: McSimulation<MC,RNG,S> (antitheticVariate, controlVariate),
		process_            (process),
		timeSteps_          (timeSteps),
		timeStepsPerYear_   (timeStepsPerYear),
		brownianBridge_     (brownianBridge),
		requiredSamples_    (requiredSamples),
		requiredTolerance_  (requiredTolerance),
		maxSamples_         (maxSamples),
		seed_               (seed),
		nCalibrationSamples_( (nCalibrationSamples == Null<Size>()) ? 2048 : nCalibrationSamples) {
			//QL_REQUIRE(timeSteps != Null<Size>() || timeStepsPerYear != Null<Size>(), "no time steps provided");
			QL_REQUIRE(timeSteps == Null<Size>() || timeStepsPerYear == Null<Size>(), "both time steps and time steps per year were provided");
			QL_REQUIRE(timeSteps != 0, "timeSteps must be positive, " << timeSteps <<" not allowed");
			QL_REQUIRE(timeStepsPerYear != 0, "timeStepsPerYear must be positive, " << timeStepsPerYear <<" not allowed");
			this->registerWith(process_);
	}

	//PATH PRICER 리턴
	template <class GenericEngine, template <class> class MC, class RNG, class S>
	inline boost::shared_ptr<typename MC_LSMC_Engine<GenericEngine,MC,RNG,S>::path_pricer_type>
		MC_LSMC_Engine<GenericEngine,MC,RNG,S>::pathPricer() const {
			QL_REQUIRE(pathPricer_, "path pricer unknown");
			return pathPricer_;
	}

	//CALCULATE 계산 실행
	template <class GenericEngine, template <class> class MC, class RNG, class S>
	inline void MC_LSMC_Engine<GenericEngine,MC,RNG,S>::calculate() const {
		pathPricer_ = this->lsmPathPricer();
		this->mcModel_ = boost::shared_ptr<MonteCarloModel<MC,RNG,S> >(
			new MonteCarloModel<MC,RNG,S>(pathGenerator(), pathPricer_, stats_type(), this->antitheticVariate_));

		this->mcModel_->addSamples(nCalibrationSamples_);
		this->pathPricer_->calibrate();

		McSimulation<MC,RNG,S>::calculate(requiredTolerance_, requiredSamples_, maxSamples_);
		this->results_.value = this->mcModel_->sampleAccumulator().mean();
		if (RNG::allowsErrorEstimate) {
			this->results_.errorEstimate = this->mcModel_->sampleAccumulator().errorEstimate();
		}
	}

	//PATH GENERATOR (공통)
	template <class GenericEngine, template <class> class MC, class RNG, class S>
	inline boost::shared_ptr<typename MC_LSMC_Engine<GenericEngine,MC,RNG,S>::path_generator_type>
		MC_LSMC_Engine<GenericEngine,MC,RNG,S>::pathGenerator() const {

			Size dimensions = process_->factors();
			TimeGrid grid = this->timeGrid();
			typename RNG::rsg_type generator = RNG::make_sequence_generator(dimensions*(grid.size()-1),seed_);
			return boost::shared_ptr<path_generator_type>(new path_generator_type(process_, grid, generator, brownianBridge_));
	}

}


#endif
