
#include <ds_interestrate_derivatives/pricingengine/mc_rangeaccrual_swap_lsmc_engine.hpp>

#include <ql/time/daycounters/all.hpp>
#include <ql/math/functional.hpp>
#include <ql/methods/montecarlo/lsmbasissystem.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/processes/hullwhiteprocess.hpp>
#include <boost/bind.hpp>

using boost::bind;

namespace QuantLib {

	RangeAccrualSwapPathPricer::RangeAccrualSwapPathPricer(
		Size assetNumber,
		const Real callValue,
		const Real pastAccrual,
		const boost::shared_ptr<StochasticProcessArray> processes,
		const Leg& cashflows,
		RangeAccrualSwap::Side side,
		Schedule floatingSchedule,
		Rate alpha,
		Rate pastFixing,
		Size polynomOrder,
		LsmBasisSystem::PolynomType polynomType)
		: RangeAccrualPathPricer(assetNumber, callValue, pastAccrual, processes, cashflows, polynomOrder, polynomType),
		side_(side), alpha_(alpha), floatingSchedule_(floatingSchedule), pastFixing_(pastFixing) {}

	Real RangeAccrualSwapPathPricer::floatingCpnCalculate(const MultiPath& path, Size idx) const {
		//floating 지급일이 now-next 사이에 들어있는 cf의 현재가치를 계산
		Time now = path[0].timeGrid()[idx];
		Time next = path[0].timeGrid()[idx+1];
		Real cpn = 0.0;

		//now-next사이 첫번째 쿠폰 지급 시점과 now이전 직전 쿠폰지급시점
		Time firstCpnTime = QL_MIN_REAL, lastCpnTime = QL_MIN_REAL;

		for (Size i=0; i<floatingSchedule_.size(); ++i) {
			//t: cf의 지급일
			Time t = ActualActual().yearFraction(Settings::instance().evaluationDate(), floatingSchedule_[i]);			
			if (t>next)
				break;
			if (t>now) {
				//t1: 직전 cf의 지급일
				Time t1 = ActualActual().yearFraction(Settings::instance().evaluationDate(), floatingSchedule_[i-1]);
				cpn += (t-t1) * alpha_ * model_.discountBond(now, t, path[0][idx]);
				if (firstCpnTime < 0) {
					firstCpnTime = t;
					lastCpnTime = t1;
				}
			}
		}
		cpn += ((firstCpnTime-lastCpnTime) * pastFixing_ + 1) * model_.discountBond(now, firstCpnTime, path[0][idx]) 
			- model_.discountBond(now, next, path[0][idx]);
		cpn /= model_.discountBond(now, next, path[0][idx]);
		Real notional = cashflows_.back()->amount();
		return notional * cpn;
	}

	Real RangeAccrualSwapPathPricer::cpnCalculate(const MultiPath& path, Size idx) const {
		Real temp = RangeAccrualPathPricer::cpnCalculate(path, idx);

		Integer signOfFixed;
		if (side_==RangeAccrualSwap::Payer)
			signOfFixed = -1;
		else
			signOfFixed = +1;

		Real floatingCpn = floatingCpnCalculate(path, idx);
		return signOfFixed * (temp - floatingCpn);
	}


	Real RangeAccrualSwapPathPricerMinusCMS::cpnCalculate(const MultiPath& path, Size idx) const {

		Real temp = RangeAccrualSwapPathPricer::cpnCalculate(path, idx);
		Real cmsRate = 0.0;
		Time now = path[0].timeGrid()[idx];
		Time next = path[0].timeGrid()[idx+1];

		if (idx==0){
			cmsRate = fixingCMS_;
			boost::shared_ptr<RangeAccrualCoupon> cpn = 
				boost::dynamic_pointer_cast<RangeAccrualCoupon>(cashflows_[idx+cashflows_.size()-path.pathSize()]);
			now = ActualActual().yearFraction(Settings::instance().evaluationDate(), cpn->accrualStartDate());
		} 
		else {
			/*CMS 1년 금리계산*/
			for (Size i=0; i<4; ++i)  //CMS 1Y RATE
				cmsRate += 0.25 * model_.discountBond(now, now+0.25*(i+1), path[0][idx]);
			cmsRate = (1.0-model_.discountBond(now, now+1.0, path[0][idx])) / cmsRate;
		}

		Real notionalAmt = cashflows_.back()->amount();

		Integer signOfFixed;
		if (side_==RangeAccrualSwap::Payer)
			signOfFixed = -1;
		else
			signOfFixed = +1;
		temp -= signOfFixed * ((next-now)*cmsRate*notionalAmt);
		return temp;

	}

}
