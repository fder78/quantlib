

#ifndef discretized_callable_bond_hpp
#define discretized_callable_bond_hpp

#include <ds_interestrate_derivatives/instruments/notes/callable_cpn_bond.hpp>
#include <ds_interestrate_derivatives/cashflows/range_accrual_coupon.hpp>
#include <ds_interestrate_derivatives/model/generalized_hullwhite.hpp>

#include <ql/discretizedasset.hpp>
#include <ql/cashflows/floatingratecoupon.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>

#include <ql/models/model.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <ql/experimental/shortrate/generalizedhullwhite.hpp>
#include <ql/math/distributions/normaldistribution.hpp>

namespace QuantLib {
	class DiscretizedCallableBond : public DiscretizedAsset {
	public:

		enum CpnType {Fixed, Floating, Digital, RangeAccrual, Nominal};

		DiscretizedCallableBond(const CallableCpnBond::arguments &args,
			const Date& referenceDate,
			const DayCounter& dayCounter,
			const boost::shared_ptr<ShortRateModel> model,
			boost::shared_ptr<YieldTermStructure> discCurve,
			Size pastAccruals);  

		void reset(Size size); 

		std::vector<Time> mandatoryTimes() const; 
		Rate cmsRate(Date resetDate, Rate r, boost::shared_ptr<InterestRateIndex> index) const;
		Rate iborRate(Date resetDate, Rate r,boost::shared_ptr<InterestRateIndex> index) const;
		Rate spreadRate(Date resetDate, Rate r,boost::shared_ptr<InterestRateIndex> index) const;
		Rate GetCpnDigital(boost::shared_ptr<FloatingRateCoupon> rtmp, Rate rate);
		Rate GetCpnFloating(boost::shared_ptr<FloatingRateCoupon> rtmp, Rate rate);

	protected:
		void preAdjustValuesImpl();
		void postAdjustValuesImpl();
		void applyCallability(Size i, Real ai);

	private: 
		CallableCpnBond::arguments arguments_;    // settlementDate, Leg cashflows, calendar
		const Date& referenceDate_;
		const DayCounter& dayCounter_;
		Exercise::Type exerciseType_;
		Size pastAccruals_;

		std::vector<Time> startTimes_;
		std::vector<Time> endTimes_;
		std::vector<Time> fixingTimes_;
		std::vector<Time> payTimes_;
		std::vector<Time> callabilityTimes_;

		Leg cashflows_;
		CallabilitySchedule callabilitySchedule_;
		std::vector<DiscretizedCallableBond::CpnType> couponType_;
		std::vector<bool> isInArrears_;

		boost::shared_ptr<ShortRateModel> model_;
		boost::shared_ptr<YieldTermStructure> discCurve_;

		CumulativeNormalDistribution f_;
	};
}

#endif