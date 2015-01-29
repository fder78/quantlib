
#ifndef mc_s2r_ra_lsmc_path_pricer_hpp
#define mc_s2r_ra_lsmc_path_pricer_hpp

#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/math/functional.hpp>
#include <ql/math/linearleastsquaresregression.hpp>
#include <ql/methods/montecarlo/pathpricer.hpp>
#include <ql/methods/montecarlo/earlyexercisepathpricer.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <ds_interestrate_derivatives/pricingengine/mc_lsmc_pathpricer.hpp>


namespace QuantLib {

	template <class PathType, class PathPricerType>
	class IR_LSMC_PathPricer : public LSMCPathPricer<PathType> {
	public:

		IR_LSMC_PathPricer(
			const TimeGrid& times,
			const std::vector<bool>& isCallable,
			const boost::shared_ptr<EarlyExercisePathPricer<PathType> >& );

		Real operator()(const PathType& path) const;
		virtual void calibrate();
	};

	template <class PathType, class PathPricerType> inline
		IR_LSMC_PathPricer<PathType, PathPricerType>::IR_LSMC_PathPricer(
		const TimeGrid& times,
		const std::vector<bool>& isCallable,
		const boost::shared_ptr<EarlyExercisePathPricer<PathType> >& pathPricer)
		: LSMCPathPricer<PathType>(times, isCallable, pathPricer) {}

	template <class PathType, class PathPricerType> inline
		Real IR_LSMC_PathPricer<PathType, PathPricerType>::operator()	(const PathType& path) const {

			if (calibrationPhase_) {
				// store paths for the calibration
				paths_.push_back(path);
				// result doesn't matter
				return 0.0;
			}

			const Size len = EarlyExerciseTraits<PathType>::pathLength(path);
			Real price = (*pathPricer_)(path, len-1); //발행 원금 리턴

			
			// 수정
			Real shortRate = 0.0, dt = 0.0, discFactor = 0.0;
			Real cpnAmt = 0.0;
			for (Size i=len-2; i>0; --i) {
				shortRate = path[0][i];				
				dt = times_[i+1] - times_[i]; //path[2].time(i+1) - path[2].time(i);

				cpnAmt = boost::dynamic_pointer_cast<PathPricerType>(pathPricer_)->cpnCalculate(path, i);
				price += cpnAmt; 
				discFactor = boost::dynamic_pointer_cast<PathPricerType>(pathPricer_)->discFactor(times_[i], times_[i+1], shortRate);
				price *= discFactor;
				const Real exercise = (*pathPricer_)(path, i); //발행 원금 리턴

				if (exercise < price && isCallable_[i]) { // ITM이라면 행사가격이 미래 가격보다 작아야 함
					const StateType regValue = pathPricer_->state(path, i);

					Real continuationValue = 0.0;
					for (Size l=0; l<v_.size(); ++l) {
						continuationValue += coeff_[i][l] * v_[l](regValue);
					}

					if (continuationValue > exercise) {
						price = exercise;
					}
				}
			}
			
			//첫번째 쿠폰 계산
			shortRate = path[0][0];
			dt = times_[1] - times_[0];

			cpnAmt = boost::dynamic_pointer_cast<PathPricerType>(pathPricer_)->cpnCalculate(path, 0);
			price += cpnAmt;
			discFactor = boost::dynamic_pointer_cast<PathPricerType>(pathPricer_)->discFactor(times_[0], times_[1], shortRate);
			price *= discFactor;
			return price;
	}

	template <class PathType, class PathPricerType> inline
		void IR_LSMC_PathPricer<PathType, PathPricerType>::calibrate() {
			const Size n = paths_.size();
			Array prices(n), exercise(n);
			const Size len = EarlyExerciseTraits<PathType>::pathLength(paths_[0]);

			for (Size i=0; i<n; ++i)
				prices[i] = (*pathPricer_)(paths_[i], len-1); //terminal payoff

			std::vector<Real>      y;
			std::vector<StateType> x;
			Real shortRate = 0.0, dt = 0.0, discFactor;
			Real cpnAmt = 0.0;

			for (Size i=len-2; i>0; --i) {
				y.clear();
				x.clear();

				//roll back step
				for (Size j=0; j<n; ++j) {
					exercise[j]=(*pathPricer_)(paths_[j], i);
					shortRate = paths_[j][0][i];
					dt = times_[i+1] - times_[i];

					cpnAmt = boost::dynamic_pointer_cast<PathPricerType>(pathPricer_)->cpnCalculate(paths_[j], i);
					discFactor = boost::dynamic_pointer_cast<PathPricerType>(pathPricer_)->discFactor(times_[i], times_[i+1], shortRate);
					prices[j] += cpnAmt; 
					prices[j] *= discFactor;

					//if (exercise[j] < prices[j]) {
						x.push_back(pathPricer_->state(paths_[j], i));
						y.push_back(prices[j]);
					//}
				}

				if (v_.size() >  x.size() || !isCallable_[i]) {
					// Call되지 않음
					coeff_[i] = Array(v_.size(), 0.0);
				}
				else {
					coeff_[i] = LinearLeastSquaresRegression<StateType>(x, y, v_).coefficients();
				}

				for (Size j=0, k=0; j<n; ++j) {

					if (exercise[j] < prices[j]) {
						Real continuationValue = 0.0;
						for (Size l=0; l<v_.size(); ++l) {
							continuationValue += coeff_[i][l] * v_[l](x[k]);
						}
						if (continuationValue > exercise[j]) {
							prices[j] = exercise[j];
						}
						++k;
					}
				}
			}

			// remove calibration paths and release memory
			std::vector<PathType> empty;
			paths_.swap(empty);
			// entering the calculation phase
			calibrationPhase_ = false;
	}
}


#endif
