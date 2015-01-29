
#include <ql/cashflows/cashflowvectors.hpp>
#include <ql/cashflows/simplecashflow.hpp>
#include <ql/time/schedule.hpp>

#include <ds_interestrate_derivatives/instruments/notes/fixedrate_cpn_bond.hpp>

namespace QuantLib {

	FixedRateCpnBond::FixedRateCpnBond(Natural settlementDays,
		Real faceAmount,
		const Schedule& schedule,
		const std::vector<Rate>& coupons,
		const DayCounter& accrualDayCounter,
		BusinessDayConvention paymentConvention,
		Real redemption,
		const Date& issueDate,
		const CallabilitySchedule& callabilitySchedule, 
		const Exercise::Type& type)
		: CallableCpnBond(settlementDays, schedule.calendar(), issueDate,callabilitySchedule,type),
		frequency_(schedule.tenor().frequency()),
		dayCounter_(accrualDayCounter) {

			maturityDate_ = schedule.endDate();

			cashflows_ = FixedRateLeg(schedule)
				.withNotionals(faceAmount)
				.withCouponRates(coupons, accrualDayCounter)
				.withPaymentAdjustment(paymentConvention);

			addRedemptionsToCashflows(std::vector<Real>(1, redemption));

			QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
			QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");
	}

	FixedRateCpnBond::FixedRateCpnBond(Natural settlementDays,
		const Calendar& calendar,
		Real faceAmount,
		const Date& startDate,
		const Date& maturityDate,
		const Period& tenor,
		const std::vector<Rate>& coupons,
		const DayCounter& accrualDayCounter,
		BusinessDayConvention accrualConvention,
		BusinessDayConvention paymentConvention,
		Real redemption,
		const Date& issueDate,
		const Date& stubDate,
		DateGeneration::Rule rule,
		bool endOfMonth,
		const CallabilitySchedule& callabilitySchedule, const Exercise::Type& type)
		: CallableCpnBond(settlementDays, calendar, issueDate,callabilitySchedule,type) ,
		frequency_(tenor.frequency()), dayCounter_(accrualDayCounter) {

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

			Schedule schedule(startDate, maturityDate_, tenor,
				calendar_, accrualConvention, accrualConvention,
				rule, endOfMonth,
				firstDate, nextToLastDate);

			cashflows_ = FixedRateLeg(schedule)
				.withNotionals(faceAmount)
				.withCouponRates(coupons, accrualDayCounter)
				.withPaymentAdjustment(paymentConvention);

			addRedemptionsToCashflows(std::vector<Real>(1, redemption));

			QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
			QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");
	}

	FixedRateCpnBond::FixedRateCpnBond(Natural settlementDays,
		Real faceAmount,
		const Schedule& schedule,
		const std::vector<InterestRate>& coupons,
		BusinessDayConvention paymentConvention,
		Real redemption,
		const Date& issueDate,
		const CallabilitySchedule& callabilitySchedule, const Exercise::Type& type)
		: CallableCpnBond(settlementDays, schedule.calendar(), issueDate,callabilitySchedule,type) ,
		frequency_(schedule.tenor().frequency()),
		dayCounter_(coupons[0].dayCounter()) {

			maturityDate_ = schedule.endDate();

			cashflows_ = FixedRateLeg(schedule)
				.withNotionals(faceAmount)
				.withCouponRates(coupons)
				.withPaymentAdjustment(paymentConvention);

			addRedemptionsToCashflows(std::vector<Real>(1, redemption));

			QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
			QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");
	}

	void FixedRateCpnBond::setupArguments(PricingEngine::arguments* args) const{
		CallableCpnBond::setupArguments(args);
		FixedRateCpnBond::arguments* arguments = dynamic_cast<FixedRateCpnBond::arguments*>(args);
		if (arguments != 0){
		//QL_REQUIRE(arguments != 0, "wrong argument type");
		}
	}
	
	void FixedRateCpnBond::arguments::validate() const {
	}

}
