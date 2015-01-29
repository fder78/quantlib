
#include <ds_interestrate_derivatives/pricingengine/mc_powerspread_swap_lsmc_engine.hpp>

#include <ql/time/daycounters/all.hpp>
#include <ql/math/functional.hpp>
#include <ql/methods/montecarlo/lsmbasissystem.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/processes/hullwhiteprocess.hpp>
#include <boost/bind.hpp>

using boost::bind;

namespace QuantLib {

	PowerSpreadSwapPathPricer::PowerSpreadSwapPathPricer(
		Size assetNumber,
		const Real callValue,
		const Real pastFixing,
		const boost::shared_ptr<StochasticProcessArray> processes,
		const Leg& cashflows,
		PowerSpreadSwap::Side side,
		Schedule floatingSchedule,
		Rate alpha,
		Rate floatingFixingRate,
		Size polynomOrder,
		LsmBasisSystem::PolynomType polynomType)
		: PowerSpreadPathPricer(assetNumber, callValue, pastFixing, processes, cashflows, polynomOrder, polynomType),
		side_(side), alpha_(alpha), floatingSchedule_(floatingSchedule), floatingFixingRate_(floatingFixingRate) {}

	Real PowerSpreadSwapPathPricer::floatingCpnCalculate(const MultiPath& path, Size idx) const {
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

	Real PowerSpreadSwapPathPricer::cpnCalculate(const MultiPath& path, Size idx) const {
		Real temp = PowerSpreadPathPricer::cpnCalculate(path, idx);

		Integer signOfFixed;
		if (side_==PowerSpreadSwap::Payer)
			signOfFixed = -1;
		else
			signOfFixed = +1;

		Real floatingCpn = floatingCpnCalculate(path, idx);
		return signOfFixed * (temp - floatingCpn);
	}

}
