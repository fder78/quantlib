#include <ds_interestrate_derivatives/cashflows/range_accrual_coupon.hpp>
#include <ql/cashflows/couponpricer.hpp>

namespace QuantLib{

	RangeAccrualCoupon::RangeAccrualCoupon(
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
		std::vector<Real> upperTrigger)
		: FloatingRateCoupon(paymentDate, nominal, startDate, endDate,
		fixingDays, index, gearing, spread,
		refPeriodStart, refPeriodEnd, dayCounter),
		obsIndex_(obsIndex),
		observationsSchedule_(observationsSchedule),
		lowerTrigger_(lowerTrigger),
		upperTrigger_(upperTrigger){

			QL_REQUIRE(lowerTrigger.size()==upperTrigger.size(), "Numbers of lower and upper triggers are not equal");
			for (Size i=0; i<lowerTrigger.size(); ++i) {
				if (lowerTrigger[i]!=Null<Real>() && upperTrigger[i]!=Null<Real>())
					QL_REQUIRE(lowerTrigger[i]<upperTrigger[i],  "lowerTrigger>=upperTrigger");
			}

			observationDates_ = observationsSchedule_->dates();
			observationDates_.pop_back();                       //remove end date
			observationsNo_ = observationDates_.size();
			
			Handle<YieldTermStructure> rateCurve;
			if (boost::dynamic_pointer_cast<IborIndex>(index))
				rateCurve = boost::dynamic_pointer_cast<IborIndex>(index)->forwardingTermStructure();
			else if (boost::dynamic_pointer_cast<SwapIndex>(index))
				rateCurve = boost::dynamic_pointer_cast<SwapIndex>(index)->forwardingTermStructure();
			else if (boost::dynamic_pointer_cast<SpreadIndex>(index))
				rateCurve = boost::dynamic_pointer_cast<SpreadIndex>(index)->forwardingTermStructure();
			else
				QL_FAIL("Index Identification Fail");


			Date referenceDate = rateCurve->referenceDate();

			startTime_ = dayCounter.yearFraction(referenceDate, startDate);
			endTime_ = dayCounter.yearFraction(referenceDate, endDate);
			for(Size i=0;i<observationsNo_;i++) {
				observationTimes_.push_back(
					dayCounter.yearFraction(referenceDate, observationDates_[i]));
			}
	}

	FloatingRangeAccrualLeg::FloatingRangeAccrualLeg(
		const Schedule& schedule,
		const boost::shared_ptr<InterestRateIndex>& index,
		const std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex)
		: schedule_(schedule), index_(index), obsIndex_(obsIndex),
		paymentAdjustment_(Following),
		observationConvention_(ModifiedFollowing) {}

	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withNotionals(Real notional) {
		notionals_ = std::vector<Real>(1,notional);
		return *this;
	}
	
	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withNotionals(
		const std::vector<Real>& notionals) {
			notionals_ = notionals;
			return *this;
	}
	
	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withPaymentDayCounter(
		const DayCounter& dayCounter) {
			paymentDayCounter_ = dayCounter;
			return *this;
	}

	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withPaymentAdjustment(
		BusinessDayConvention convention) {
			paymentAdjustment_ = convention;
			return *this;
	}


	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withFixingDays(Natural fixingDays) {
		fixingDays_ = std::vector<Natural>(1,fixingDays);
		return *this;
	}


	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withFixingDays(
		const std::vector<Natural>& fixingDays) {
			fixingDays_ = fixingDays;
			return *this;
	}


	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withGearings(Real gearing) {
		gearings_ = std::vector<Real>(1,gearing);
		return *this;
	}


	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withGearings(
		const std::vector<Real>& gearings) {
			gearings_ = gearings;
			return *this;
	}


	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withSpreads(Spread spread) {
		spreads_ = std::vector<Spread>(1,spread);
		return *this;
	}


	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withSpreads(
		const std::vector<Spread>& spreads) {
			spreads_ = spreads;
			return *this;
	}


	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withLowerTriggers(std::vector<Real> trigger) {
		lowerTriggers_.resize(0);
		lowerTriggers_.push_back(trigger);
		return *this;
	}


	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withLowerTriggers(
		const std::vector<std::vector<Rate> > triggers) {
			lowerTriggers_ = triggers;
			return *this;
	}


	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withUpperTriggers(std::vector<Real> trigger) {
		upperTriggers_.resize(0);
		upperTriggers_.push_back(trigger);
		return *this;
	}

	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withUpperTriggers(
		const std::vector<std::vector<Rate> > triggers) {
			upperTriggers_ = triggers;
			return *this;
	}

	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withObservationTenor(
		const Period& tenor) {
			observationTenor_ = tenor;
			return *this;
	}

	FloatingRangeAccrualLeg& FloatingRangeAccrualLeg::withObservationConvention(
		BusinessDayConvention convention) {
			observationConvention_ = convention;
			return *this;
	}

	FloatingRangeAccrualLeg::operator Leg() const {

		QL_REQUIRE(!notionals_.empty(), "no notional given");

		Size n = schedule_.size()-1;
		QL_REQUIRE(notionals_.size() <= n,
			"too many nominals (" << notionals_.size() <<
			"), only " << n << " required");
		QL_REQUIRE(fixingDays_.size() <= n,
			"too many fixingDays (" << fixingDays_.size() <<
			"), only " << n << " required");
		QL_REQUIRE(gearings_.size()<=n,
			"too many gearings (" << gearings_.size() <<
			"), only " << n << " required");
		QL_REQUIRE(spreads_.size()<=n,
			"too many spreads (" << spreads_.size() <<
			"), only " << n << " required");
		QL_REQUIRE(lowerTriggers_.size()<=n,
			"too many lowerTriggers (" << lowerTriggers_.size() <<
			"), only " << n << " required");
		QL_REQUIRE(upperTriggers_.size()<=n,
			"too many upperTriggers (" << upperTriggers_.size() <<
			"), only " << n << " required");

		Leg leg; leg.reserve(n);

		// the following is not always correct
		Calendar calendar = schedule_.calendar();

		Date refStart, start, refEnd, end;
		Date paymentDate;
		std::vector<boost::shared_ptr<Schedule> > observationsSchedules;

		for (Size i=0; i<n; ++i) {
			refStart = start = schedule_.date(i);
			refEnd   =   end = schedule_.date(i+1);
			paymentDate = calendar.adjust(end, paymentAdjustment_);
			if (i==0   && !schedule_.isRegular(i+1)) {
				BusinessDayConvention bdc = schedule_.businessDayConvention();
				refStart = calendar.adjust(end - schedule_.tenor(), bdc);
			}
			if (i==n-1 && !schedule_.isRegular(i+1)) {
				BusinessDayConvention bdc = schedule_.businessDayConvention();
				refEnd = calendar.adjust(start + schedule_.tenor(), bdc);
			}
			if (detail::get(gearings_, i, 1.0) == 0.0) { // Ibor coupon
				leg.push_back(boost::shared_ptr<CashFlow>(new
					FixedRateCoupon(paymentDate,
					detail::get(notionals_, i, Null<Real>()),
					detail::get(spreads_, i, 0.0),
					paymentDayCounter_,
					start, end, refStart, refEnd)));
			} else { // floating coupon
				observationsSchedules.push_back(
					boost::shared_ptr<Schedule>(new
					Schedule(start, end,
					observationTenor_, calendar,
					observationConvention_,
					observationConvention_,
					DateGeneration::Forward, false)));

				leg.push_back(boost::shared_ptr<CashFlow>(new
					RangeAccrualCoupon(
						paymentDate,
						detail::get(notionals_, i, Null<Real>()),
						index_,
						obsIndex_,
						start, end,
						detail::get(fixingDays_, i, 2),
						paymentDayCounter_,
						detail::get(gearings_, i, 1.0),
						detail::get(spreads_, i, 0.0),
						refStart, refEnd,
						observationsSchedules.back(),
						detail::get(lowerTriggers_, i, std::vector<Rate>()),
						detail::get(upperTriggers_, i, std::vector<Rate>())
						)));
			}
		}
		//setCouponPricer(leg, boost::shared_ptr<FloatingRateCouponPricer>(new BlackIborCouponPricer));
		return leg;
	}
}
