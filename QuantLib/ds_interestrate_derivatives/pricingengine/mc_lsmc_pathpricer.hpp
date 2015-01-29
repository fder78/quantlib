
#ifndef mc_lsmc_path_pricer_hpp
#define mc_lsmc_path_pricer_hpp

#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/math/functional.hpp>
#include <ql/math/linearleastsquaresregression.hpp>
#include <ql/methods/montecarlo/pathpricer.hpp>
#include <ql/methods/montecarlo/earlyexercisepathpricer.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace QuantLib {

	template <class PathType>
	class LSMCPathPricer : public PathPricer<PathType> {
	public:
		typedef typename EarlyExerciseTraits<PathType>::StateType StateType;

		LSMCPathPricer(
			const TimeGrid& times,
			const std::vector<bool>& isCallable,
			const boost::shared_ptr<EarlyExercisePathPricer<PathType> >& );

		virtual void calibrate() = 0;

	protected:
		bool  calibrationPhase_;
		const TimeGrid times_;
		const boost::shared_ptr<EarlyExercisePathPricer<PathType> > pathPricer_;
		const std::vector<bool> isCallable_;

		boost::scoped_array<Array> coeff_;
		mutable std::vector<PathType> paths_;
		const   std::vector<boost::function1<Real, StateType> > v_;
	};

	template <class PathType> inline LSMCPathPricer<PathType>::LSMCPathPricer(
		const TimeGrid& times,
		const std::vector<bool>& isCallable,
		const boost::shared_ptr<EarlyExercisePathPricer<PathType> >& pathPricer)
		: calibrationPhase_(true),
		pathPricer_(pathPricer),
		times_(times),
		isCallable_(isCallable),
		coeff_     (new Array[times.size()-1]),
		v_         (pathPricer_->basisSystem()) {}

}


#endif
