
#ifndef range_accrual_coupon_hpp
#define range_accrual_coupon_hpp

#include <ds_interestrate_derivatives/index/spread_index.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/utilities/vectors.hpp>
#include <vector>

namespace QuantLib {

	class RangeAccrualCoupon : public FloatingRateCoupon {
	public:
		RangeAccrualCoupon(
			const Date& paymentDate,
			Real nominal,
			const boost::shared_ptr<InterestRateIndex>& index,
			const std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex,
			const Date& startDate,
			const Date& endDate,
			Natural fixingDays,
			const DayCounter& dayCounter,
			Real gearing,
			Rate spread,
			const Date& refPeriodStart,
			const Date& refPeriodEnd,
			const boost::shared_ptr<Schedule>&  observationsSchedule,
			std::vector<Real> lowerTrigger,
			std::vector<Real> upperTrigger);

		Real startTime() const {return startTime_; }
		Real endTime() const {return endTime_; }
		std::vector<Real> lowerTrigger() const {return lowerTrigger_; }
		std::vector<Real> upperTrigger() const {return upperTrigger_; }
		Size observationsNo() const {return observationsNo_; }
		Rate rate() const { return spread(); }

		const std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex() const {
			return obsIndex_;
		}
		const std::vector<Date>& observationDates() const {
			return observationDates_;
		}
		const std::vector<Real>& observationTimes() const {
			return observationTimes_;
		}
		const boost::shared_ptr<Schedule> observationsSchedule() const {
			return observationsSchedule_;
		}

	private:
		Real startTime_;                               // S
		Real endTime_;                                 // T

		const boost::shared_ptr<Schedule> observationsSchedule_;
		std::vector<Date> observationDates_;
		std::vector<Real> observationTimes_;
		Size observationsNo_;

		std::vector<Real> lowerTrigger_;  //Dual range의 경우 2개가 들어올수 있음
		std::vector<Real> upperTrigger_;
		
		const std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex_;
	};


	//! helper class building a sequence of range-accrual floating-rate coupons
	class FloatingRangeAccrualLeg {
	public:
		FloatingRangeAccrualLeg(const Schedule& schedule,
			const boost::shared_ptr<InterestRateIndex>& index,
			const std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex);
		FloatingRangeAccrualLeg& withNotionals(Real notional);
		FloatingRangeAccrualLeg& withNotionals(const std::vector<Real>& notionals);
		FloatingRangeAccrualLeg& withPaymentDayCounter(const DayCounter&);
		FloatingRangeAccrualLeg& withPaymentAdjustment(BusinessDayConvention);
		FloatingRangeAccrualLeg& withFixingDays(Natural fixingDays);
		FloatingRangeAccrualLeg& withFixingDays(const std::vector<Natural>& fixingDays);
		FloatingRangeAccrualLeg& withGearings(Real gearing);
		FloatingRangeAccrualLeg& withGearings(const std::vector<Real>& gearings);
		FloatingRangeAccrualLeg& withSpreads(Spread spread);
		FloatingRangeAccrualLeg& withSpreads(const std::vector<Spread>& spreads);
		FloatingRangeAccrualLeg& withLowerTriggers(std::vector<Real> trigger);
		FloatingRangeAccrualLeg& withLowerTriggers(const std::vector<std::vector<Rate> > triggers);
		FloatingRangeAccrualLeg& withUpperTriggers(std::vector<Real> trigger);
		FloatingRangeAccrualLeg& withUpperTriggers(const std::vector<std::vector<Rate> > triggers);
		FloatingRangeAccrualLeg& withObservationTenor(const Period&);
		FloatingRangeAccrualLeg& withObservationConvention(BusinessDayConvention);
		operator Leg() const;
	private:
		Schedule schedule_;
		boost::shared_ptr<InterestRateIndex> index_;
		std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex_;
		std::vector<Real> notionals_;
		DayCounter paymentDayCounter_;
		BusinessDayConvention paymentAdjustment_;
		std::vector<Natural> fixingDays_;
		std::vector<Real> gearings_;
		std::vector<Spread> spreads_;
		std::vector<std::vector<Rate> > lowerTriggers_, upperTriggers_;
		Period observationTenor_;
		BusinessDayConvention observationConvention_;
	};
	
}


#endif
