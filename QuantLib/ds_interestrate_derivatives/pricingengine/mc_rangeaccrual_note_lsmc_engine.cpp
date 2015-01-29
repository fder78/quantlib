
#include <ds_interestrate_derivatives/pricingengine/mc_rangeaccrual_note_lsmc_engine.hpp>

#include <ql/math/functional.hpp>
#include <ql/methods/montecarlo/lsmbasissystem.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/processes/hullwhiteprocess.hpp>
#include <boost/bind.hpp>

using boost::bind;

namespace QuantLib {

	RangeAccrualPathPricer::RangeAccrualPathPricer(
		Size assetNumber,
		const Real callValue,
		const Real pastAccrual,
		const boost::shared_ptr<StochasticProcessArray> processes,
		const Leg& cashflows,
		Size polynomOrder,
		LsmBasisSystem::PolynomType polynomType)
		: IREarlyExercisePathPricer(assetNumber, callValue, processes, cashflows, polynomOrder, polynomType),
		pastAccrual_(pastAccrual), f2_(processes->correlation()[0][1]), f_(), isGBM_(processes->size(), false)
	{
		for (Size i=1; i<processes_->size(); ++i) {
			if (boost::dynamic_pointer_cast<GeneralizedBlackScholesProcess>(processes_->process(i)))
				isGBM_[i] = true;
		}
	}

	Real RangeAccrualPathPricer::cpnCalculate(const MultiPath& path, Size idx) const {

		Size assetNum = path.assetNumber()-1;
		std::vector<Real> s(assetNum, 0.0);
		std::vector<Real> up(assetNum, 0.0), down(assetNum, 0.0);
		std::vector<Real> u(assetNum, 0.0), d(assetNum, 0.0);

		boost::shared_ptr<RangeAccrualCoupon> cpn = 
			boost::dynamic_pointer_cast<RangeAccrualCoupon>(cashflows_[idx+cashflows_.size()-path.pathSize()]);

		for (Size i=0; i<assetNum; ++i) {
			s[i] = path[i+1][idx];
			up[i] = cpn->upperTrigger()[i];
			if (up[i]==Null<Real>()) 
				up[i] = 10e12;
			down[i] = cpn->lowerTrigger()[i];
			if (down[i]==Null<Real>()) 
				down[i] = -10e12;
			if (isGBM_[i+1]) {
				up[i] = std::log(up[i]);
				down[i] = std::log((down[i]>0.0) ? down[i] : QL_MIN_POSITIVE_REAL);
			}
		}

		Time t = path[0].timeGrid()[idx];
		Time deltaT = cpn->accrualPeriod();
		Time firstT = path[0].timeGrid()[idx+1] - path[0].timeGrid()[idx];

		Size n = 12; //temp////////////////////////////////////////////////////////////
		Real nN=0.0, dt=0.0;
		Real mean_s, std_s;
		for (Size i=0; i<n; ++i) {
			dt = firstT/n*(i+0.5);
			for (Size j=0; j<assetNum; ++j) {
				mean_s = x_[j]->expectation(t,s[j],dt);
				std_s = x_[j]->stdDeviation(t,s[j],dt);
				u[j] = (up[j]-mean_s)/std_s;
				d[j] = (down[j]-mean_s)/std_s;
			}

			nN += (f2_(u[0],u[1])-f2_(u[0],d[1])-f2_(d[0],u[1])+f2_(d[0],d[1]))/n;
		}

		Real cpnValue = 0.0;
		if (idx==0)
			cpnValue = pastAccrual_ + nN * firstT / deltaT;
		else
			cpnValue = nN * firstT / deltaT;
		return cpn->amount() * cpnValue;
	}


	Real RangeAccrualPathPricerMinusCMS::cpnCalculate(const MultiPath& path, Size idx) const {
		Real temp = RangeAccrualPathPricer::cpnCalculate(path, idx);
		/*CMS 1년 금리계산*/
		Real cmsRate = 0.0;
		Time now = path[0].timeGrid()[idx];
		Time next = path[0].timeGrid()[idx+1];

		if (idx==0) {
			cmsRate = fixingCMS_;
			boost::shared_ptr<RangeAccrualCoupon> cpn = 
				boost::dynamic_pointer_cast<RangeAccrualCoupon>(cashflows_[idx+cashflows_.size()-path.pathSize()]);
			now = ActualActual().yearFraction(Settings::instance().evaluationDate(), cpn->accrualStartDate());
		}
		else {
			for (Size i=0; i<4; ++i)
				cmsRate += 0.25 * model_.discountBond(now, now+0.25*(i+1), path[0][idx]);
			cmsRate = (1.0-model_.discountBond(now, now+1.0, path[0][idx])) / cmsRate;
		}
		Real notionalAmt = cashflows_.back()->amount();
		return (temp - (next-now)*cmsRate*notionalAmt);
		//return (temp - (next-now)*cmsRate*notionalAmt < 0.0) ? 0.0 : (temp - (next-now)*cmsRate*notionalAmt);

	}

}
