
#include <ds_interestrate_derivatives/instruments/notes/range_accrual_note.hpp>

namespace QuantLib {

	std::vector<std::vector<Real> > RangeAccrualNote::getTriggers(std::vector<Real> trigger, Size n) {
		std::vector<std::vector<Real> > triggers;
		for (Size i=0; i<n; ++i)
			triggers.push_back(std::vector<Real>(1, detail::get(trigger, i, Null<Rate>())));
		return triggers;
	}

	std::vector<std::vector<Real> > RangeAccrualNote::getTriggers(std::vector<Real> trigger1, std::vector<Real> trigger2, Size n) {
		std::vector<std::vector<Real> > triggers;
		for (Size i=0; i<n; ++i) {
			std::vector<Real> temp;
			temp.push_back(detail::get(trigger1, i, Null<Rate>()));
			if (trigger2.size()>0)
				temp.push_back(detail::get(trigger2, i, Null<Rate>()));
			triggers.push_back(temp);
		}
		return triggers;
	}


	RangeAccrualNote::RangeAccrualNote(
		Natural settlementDays,
		Real faceAmount,
		const Schedule& schedule,
		const boost::shared_ptr<InterestRateIndex>& index,
		const boost::shared_ptr<InterestRateIndex>& obsIndex,
		const DayCounter& paymentDayCounter,
		BusinessDayConvention paymentConvention,
		Natural fixingDays,
		const std::vector<Real>& gearings,
		const std::vector<Spread>& spreads,
		const std::vector<Rate> lowerTriggers,
		const std::vector<Rate> upperTriggers,
		const Period& obsTenor,
		BusinessDayConvention obsConvention,
		Real redemption,
		const Date& issueDate,
		const CallabilitySchedule& callabilitySchedule, const Exercise::Type& type,
		const Real alpha,
		const Real fixingRate)
		: CallableCpnBond(settlementDays, schedule.calendar(), issueDate,callabilitySchedule,type, false, alpha, fixingRate), 
		inverseGearing_(Null<Real>()), cap_(Null<Real>()), floor_(Null<Real>())
	{

		lowerTriggers_ = getTriggers(lowerTriggers, schedule.size()-1);
		upperTriggers_ = getTriggers(upperTriggers, schedule.size()-1);

		maturityDate_ = schedule.endDate();

		obsIndex_.push_back(obsIndex);
		cashflows_ = FloatingRangeAccrualLeg(schedule, index, obsIndex_)
			.withNotionals(faceAmount)
			.withPaymentDayCounter(paymentDayCounter)
			.withPaymentAdjustment(paymentConvention)
			.withFixingDays(fixingDays)
			.withGearings(gearings)
			.withSpreads(spreads)
			.withLowerTriggers(lowerTriggers_)
			.withUpperTriggers(upperTriggers_)
			.withObservationTenor(obsTenor)
			.withObservationConvention(obsConvention);

		for (Size i=0; i<schedule.size()-1; ++i) {
			if (detail::get(gearings, i, 1.0) == 0.0)
				cpnType_.push_back(Fixed);
			else if (detail::get(lowerTriggers, i, Null<Real>())==Null<Real>() &&
				detail::get(upperTriggers, i, Null<Real>())==Null<Real>())
				cpnType_.push_back(Floating);
			else
				cpnType_.push_back(RangeAccrual);
		}

		addRedemptionsToCashflows(std::vector<Real>(1, redemption));

		QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
		QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");

		registerWith(index);
	}


	RangeAccrualNote::RangeAccrualNote(
		Natural settlementDays,
		Real faceAmount,
		const Date& startDate,
		const Date& maturityDate,
		Frequency couponFrequency,
		const Calendar& calendar,
		const boost::shared_ptr<InterestRateIndex>& index,
		const boost::shared_ptr<InterestRateIndex>& obsIndex,
		const DayCounter& accrualDayCounter,
		BusinessDayConvention accrualConvention,
		BusinessDayConvention paymentConvention,
		Natural fixingDays,
		const std::vector<Real>& gearings,
		const std::vector<Spread>& spreads,
		const std::vector<Rate> lowerTriggers,
		const std::vector<Rate> upperTriggers,
		const Period& obsTenor,
		BusinessDayConvention obsConvention,
		Real redemption,
		const Date& issueDate,
		const Date& stubDate,
		DateGeneration::Rule rule,
		bool endOfMonth,
		const CallabilitySchedule& callabilitySchedule, const Exercise::Type& type,
		const Real alpha,
		const Real fixingRate)
		: CallableCpnBond(settlementDays, calendar, issueDate,callabilitySchedule,type, false, alpha, fixingRate),
		inverseGearing_(Null<Real>()), cap_(Null<Real>()), floor_(Null<Real>())
	{

			maturityDate_ = maturityDate;

			Date firstDate, nextToLastDate;
			switch (rule) {
			case DateGeneration::Backward:
				firstDate = Date();
				nextToLastDate = stubDate;
				break;
			case DateGeneration::Forward:
				firstDate = stubDate;
				nextToLastDate = Date();
				break;
			case DateGeneration::Zero:
			case DateGeneration::ThirdWednesday:
			case DateGeneration::Twentieth:
			case DateGeneration::TwentiethIMM:
				QL_FAIL("stub date (" << stubDate << ") not allowed with " <<
					rule << " DateGeneration::Rule");
			default:
				QL_FAIL("unknown DateGeneration::Rule (" << Integer(rule) << ")");
			}

			Schedule schedule(startDate, maturityDate_, Period(couponFrequency),
				calendar_, accrualConvention, accrualConvention,
				rule, endOfMonth,
				firstDate, nextToLastDate);			

			lowerTriggers_ = getTriggers(lowerTriggers, schedule.size()-1);
			upperTriggers_ = getTriggers(upperTriggers, schedule.size()-1);

			obsIndex_.push_back(obsIndex);
			cashflows_ = FloatingRangeAccrualLeg(schedule, index, obsIndex_)
				.withNotionals(faceAmount)
				.withPaymentDayCounter(accrualDayCounter)
				.withPaymentAdjustment(paymentConvention)
				.withFixingDays(fixingDays)
				.withGearings(gearings)
				.withSpreads(spreads)
				.withLowerTriggers(lowerTriggers_)
				.withUpperTriggers(upperTriggers_)
				.withObservationTenor(obsTenor)
				.withObservationConvention(obsConvention);
			addRedemptionsToCashflows(std::vector<Real>(1, redemption));

			for (Size i=0; i<schedule.size()-1; ++i) {
				if (detail::get(gearings, i, 1.0) == 0.0)
					cpnType_.push_back(Fixed);
				else if (detail::get(lowerTriggers, i, Null<Real>())==Null<Real>() &&
					detail::get(upperTriggers, i, Null<Real>())==Null<Real>())
					cpnType_.push_back(Floating);
				else
					cpnType_.push_back(RangeAccrual);
			}

			QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
			QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");

			registerWith(index);
	}




	RangeAccrualNote::RangeAccrualNote(
		Natural settlementDays,
		Real faceAmount,
		const Schedule& schedule,
		const boost::shared_ptr<InterestRateIndex>& index,
		const boost::shared_ptr<InterestRateIndex>& obsIndex1,
		const boost::shared_ptr<InterestRateIndex>& obsIndex2,
		const DayCounter& paymentDayCounter,
		BusinessDayConvention paymentConvention,
		Natural fixingDays,
		const std::vector<Real>& gearings,
		const std::vector<Spread>& spreads,
		const std::vector<Rate> lowerTriggers1,
		const std::vector<Rate> upperTriggers1, //Big
		const std::vector<Rate> lowerTriggers2,
		const std::vector<Rate> upperTriggers2, //Big
		const Period& obsTenor,
		BusinessDayConvention obsConvention,
		Real redemption,
		const Date& issueDate,
		const CallabilitySchedule& callabilitySchedule, const Exercise::Type& type,
		const Real alpha,
		const Real fixingRate,
		std::vector<Matrix> m,
		std::vector<Matrix> tenor)
		: CallableCpnBond(settlementDays, schedule.calendar(), issueDate,callabilitySchedule,type, false, alpha, fixingRate),
		inverseGearing_(Null<Real>()), cap_(Null<Real>()), floor_(Null<Real>()), m_(m), tenor_(tenor)
	{

		maturityDate_ = schedule.endDate();

		lowerTriggers_ = getTriggers(lowerTriggers1, lowerTriggers2, schedule.size()-1);
		upperTriggers_ = getTriggers(upperTriggers1, upperTriggers2, schedule.size()-1);

		m_.resize(0);
		for (Size i=0; i<schedule.size()-1; ++i)
			m_.push_back(detail::get(m, i, Matrix()));

		tenor_.resize(0);
		for (Size i=0; i<schedule.size()-1; ++i)
			tenor_.push_back(detail::get(tenor, i, Matrix()));

		obsIndex_.push_back(obsIndex1);
		obsIndex_.push_back(obsIndex2);
		cashflows_ = FloatingRangeAccrualLeg(schedule, index, obsIndex_)
			.withNotionals(faceAmount)
			.withPaymentDayCounter(paymentDayCounter)
			.withPaymentAdjustment(paymentConvention)
			.withFixingDays(fixingDays)
			.withGearings(gearings)
			.withSpreads(spreads)
			.withLowerTriggers(lowerTriggers_)
			.withUpperTriggers(upperTriggers_)
			.withObservationTenor(obsTenor)
			.withObservationConvention(obsConvention);

		addRedemptionsToCashflows(std::vector<Real>(1, redemption));

		for (Size i=0; i<schedule.size()-1; ++i) {
			if (detail::get(gearings, i, 1.0) == 0.0)
				cpnType_.push_back(Fixed);
			else if (detail::get(lowerTriggers1, i, Null<Real>())==Null<Real>() &&
				detail::get(upperTriggers1, i, Null<Real>())==Null<Real>())
				cpnType_.push_back(Floating);
			else
				cpnType_.push_back(RangeAccrual);
		}

		QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
		QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");

		registerWith(index);
	}


	RangeAccrualNote::RangeAccrualNote(
		Natural settlementDays,
		Real faceAmount,
		const Date& startDate,
		const Date& maturityDate,
		Frequency couponFrequency,
		const Calendar& calendar,
		const boost::shared_ptr<InterestRateIndex>& index,
		const boost::shared_ptr<InterestRateIndex>& obsIndex1,
		const boost::shared_ptr<InterestRateIndex>& obsIndex2,
		const DayCounter& accrualDayCounter,
		BusinessDayConvention accrualConvention,
		BusinessDayConvention paymentConvention,
		Natural fixingDays,
		const std::vector<Real>& gearings,
		const std::vector<Spread>& spreads,
		const std::vector<Rate> lowerTriggers1,
		const std::vector<Rate> upperTriggers1, //Big
		const std::vector<Rate> lowerTriggers2,
		const std::vector<Rate> upperTriggers2, //Big
		const Period& obsTenor,
		BusinessDayConvention obsConvention,
		Real redemption,
		const Date& issueDate,
		const Date& stubDate,
		DateGeneration::Rule rule,
		bool endOfMonth,
		const CallabilitySchedule& callabilitySchedule, const Exercise::Type& type,
		const Real alpha,
		const Real fixingRate)
		: CallableCpnBond(settlementDays, calendar, issueDate,callabilitySchedule,type, false, alpha, fixingRate),
		inverseGearing_(Null<Real>()), cap_(Null<Real>()), floor_(Null<Real>())
	{

		maturityDate_ = maturityDate;

		Date firstDate, nextToLastDate;
		switch (rule) {
		case DateGeneration::Backward:
			firstDate = Date();
			nextToLastDate = stubDate;
			break;
		case DateGeneration::Forward:
			firstDate = stubDate;
			nextToLastDate = Date();
			break;
		case DateGeneration::Zero:
		case DateGeneration::ThirdWednesday:
		case DateGeneration::Twentieth:
		case DateGeneration::TwentiethIMM:
			QL_FAIL("stub date (" << stubDate << ") not allowed with " <<
				rule << " DateGeneration::Rule");
		default:
			QL_FAIL("unknown DateGeneration::Rule (" << Integer(rule) << ")");
		}

		Schedule schedule(startDate, maturityDate_, Period(couponFrequency),
			calendar_, accrualConvention, accrualConvention,
			rule, endOfMonth,
			firstDate, nextToLastDate);

		lowerTriggers_ = getTriggers(lowerTriggers1, lowerTriggers2, schedule.size()-1);
		upperTriggers_ = getTriggers(upperTriggers1, upperTriggers2, schedule.size()-1);

		obsIndex_.push_back(obsIndex1);
		obsIndex_.push_back(obsIndex2);
		cashflows_ = FloatingRangeAccrualLeg(schedule, index, obsIndex_)
			.withNotionals(faceAmount)
			.withPaymentDayCounter(accrualDayCounter)
			.withPaymentAdjustment(paymentConvention)
			.withFixingDays(fixingDays)
			.withGearings(gearings)
			.withSpreads(spreads)
			.withLowerTriggers(lowerTriggers_)
			.withUpperTriggers(upperTriggers_)
			.withObservationTenor(obsTenor)
			.withObservationConvention(obsConvention);
		addRedemptionsToCashflows(std::vector<Real>(1, redemption));

		for (Size i=0; i<schedule.size()-1; ++i) {
			if (detail::get(gearings, i, 1.0) == 0.0)
				cpnType_.push_back(Fixed);
			else if (detail::get(lowerTriggers1, i, Null<Real>())==Null<Real>() &&
				detail::get(upperTriggers1, i, Null<Real>())==Null<Real>())
				cpnType_.push_back(Floating);
			else
				cpnType_.push_back(RangeAccrual);
		}

		QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
		QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");

		registerWith(index);
	}


	void RangeAccrualNote::setupArguments(PricingEngine::arguments* args) const{
		CallableCpnBond::setupArguments(args);			
		CallableCpnBond::arguments* argtemp = dynamic_cast<CallableCpnBond::arguments*>(args);
		if (argtemp != 0) {
			argtemp->cap = cap_;
			argtemp->floor = floor_;
			argtemp->inverseFixing = inverseFixing_;
			argtemp->inverseGearing = inverseGearing_;
			argtemp->inverseAlpha = inverseAlpha_;
		}

		RangeAccrualNote::arguments* arguments = dynamic_cast<RangeAccrualNote::arguments*>(args);
		if (arguments != 0) {
			//QL_REQUIRE(arguments != 0, "wrong argument type");
			arguments->lowerTriggers = lowerTriggers_;
			arguments->upperTriggers = upperTriggers_;
			arguments->cpnType = cpnType_;
			arguments->coeff = m_;
			arguments->tenor = tenor_;
		}
	}

	void RangeAccrualNote::arguments::validate() const {
	}
}