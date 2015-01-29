
#include <ds_interestrate_derivatives/instruments/notes/power_spread_note.hpp>

namespace QuantLib {

	PowerSpreadNote::PowerSpreadNote(
		Natural settlementDays,
		Real faceAmount,
		const Schedule& schedule,
		const boost::shared_ptr<SpreadIndex>& index,
		const DayCounter& accrualDayCounter,
		BusinessDayConvention paymentConvention,
		Natural fixingDays,
		const std::vector<Real>& gearings,
		const std::vector<Spread>& spreads,
		const std::vector<Rate> caps,
		const std::vector<Rate> floors, //Big
		const bool isAvg,
		Real redemption,
		const Date& issueDate,
		const CallabilitySchedule& callabilitySchedule,
		const Exercise::Type& type)
		: CallableCpnBond(settlementDays, schedule.calendar(), issueDate, callabilitySchedule, type), isAvg_(isAvg)

	{

		maturityDate_ = schedule.endDate();

		cashflows_ = SpreadLeg(schedule, index)
			.withNotionals(faceAmount)
			.withPaymentDayCounter(accrualDayCounter)
			.withPaymentAdjustment(paymentConvention)
			.withFixingDays(fixingDays)
			.withGearings(gearings)
			.withSpreads(spreads)
			.withCaps(caps)
			.withFloors(floors);

		addRedemptionsToCashflows(std::vector<Real>(1, redemption));

		QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
		QL_ENSURE(redemptions_.size() == 1, "multiple redemptions created");

		registerWith(index);
	}




	void PowerSpreadNote::setupArguments(PricingEngine::arguments* args) const{
		CallableCpnBond::setupArguments(args);
		PowerSpreadNote::arguments* arguments = dynamic_cast<PowerSpreadNote::arguments*>(args);
		if (arguments != 0) {
			//QL_REQUIRE(arguments != 0, "wrong argument type");
			arguments->isAvg = isAvg_;
		}
	}

	void PowerSpreadNote::arguments::validate() const {
	}
}