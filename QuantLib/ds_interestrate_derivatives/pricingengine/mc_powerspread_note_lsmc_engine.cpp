
#include <ds_interestrate_derivatives/pricingengine/mc_powerspread_note_lsmc_engine.hpp>

#include <ql/math/functional.hpp>
#include <ql/methods/montecarlo/lsmbasissystem.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/processes/hullwhiteprocess.hpp>
#include <boost/bind.hpp>

using boost::bind;

namespace QuantLib {

	PowerSpreadPathPricer::PowerSpreadPathPricer(
		Size assetNumber,
		const Real callValue,
		const Real pastFixing,
		const boost::shared_ptr<StochasticProcessArray> processes,
		const Leg& cashflows,
		const bool isAvg,
		Size polynomOrder,
		LsmBasisSystem::PolynomType polynomType)
		: IREarlyExercisePathPricer(assetNumber, callValue, processes, cashflows, polynomOrder, polynomType),
		pastFixing_(pastFixing), isAvg_(isAvg), 
		indexCalculator_(IndexCalculator::IborRate, boost::shared_ptr<OneFactorAffineModel>(new HullWhite(model_)))
	{}


	Real PowerSpreadPathPricer::cpnCalculate(const MultiPath& path, Size idx) const {

		if( boost::shared_ptr<CappedFlooredSpreadCoupon> cpn = 
			boost::dynamic_pointer_cast<CappedFlooredSpreadCoupon>(cashflows_[idx+cashflows_.size()-path.pathSize()]) )
		{
			Time t = path[0].timeGrid()[idx];
			Time deltaT = cpn->accrualPeriod();
			Time firstT = path[0].timeGrid()[idx+1] - path[0].timeGrid()[idx];
		
			Real cpnRate = 0.0, cpnValue = 0.0;
			Size n = 10;
		
			if (isAvg_) {
				Real mean1 = 0.0, mean2 = 0.0, dt = 0.0;
				for (Size i=0; i<n; ++i) {
					dt = firstT/n*(i+0.5);
					mean1 = indexCalculator_(t+dt, Period(Quarterly), x_[0]->expectation(t, path[1][idx], dt));
					mean2 = indexCalculator_(t+dt, Period(Quarterly), x_[1]->expectation(t, path[2][idx], dt));
					cpnRate += (mean1 - mean2)/n;
				}
	
				cpnRate = cpn->spread() + cpn->gearing() * (cpnRate);

				if (idx==0)
					cpnValue = pastFixing_ * (1.0 - firstT / deltaT) + cpnRate * firstT / deltaT;
				else
					cpnValue = cpnRate * firstT / deltaT;
			}
				else {
				if (idx==0)
					cpnValue = pastFixing_;
				else {
					cpnRate = cpn->spread() + cpn->gearing() * 
						(indexCalculator_(t, Period(Quarterly), path[1][idx]) 
						- indexCalculator_(t, Period(Quarterly), path[2][idx])); //수정 필요
					cpnValue = cpnRate * firstT / deltaT;
				}
			}

			Real upperBound = cpn->cap();
			Real lowerBound = cpn->floor();
			cpnValue = (cpnValue > upperBound) ? upperBound : ((cpnValue < lowerBound) ? lowerBound : cpnValue);
			Real cpnAmt = cpn->nominal() * deltaT * cpnValue;

			return cpnAmt;

		}
		else if(boost::shared_ptr<FixedRateCoupon> fixedcpn = 
			boost::dynamic_pointer_cast<FixedRateCoupon>(cashflows_[idx+cashflows_.size()-path.pathSize()]))
		{
			Real cpnAmt = fixedcpn->amount();

			return cpnAmt;

		}

	}

}
