
#include <ql/cashflows/couponpricer.hpp>
#include <ql/cashflows/capflooredcoupon.hpp>
#include <ql/cashflows/cashflowvectors.hpp>
#include <ql/indexes/interestrateindex.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>


#include <ds_interestrate_derivatives/cashflows/spread_coupon.hpp>


namespace QuantLib {

	SpreadCoupon::SpreadCoupon(const Date& paymentDate,
		Real nominal,
		const Date& startDate,
		const Date& endDate,
		Natural fixingDays,
		const boost::shared_ptr<SpreadIndex>& SpreadIndex,
		Real gearing,
		Spread spread,
		const Date& refPeriodStart,
		const Date& refPeriodEnd,
		const DayCounter& dayCounter,
		bool isInArrears)
		: FloatingRateCoupon(paymentDate, nominal, startDate, endDate,
		fixingDays, SpreadIndex, gearing, spread,
		refPeriodStart, refPeriodEnd,
		dayCounter, isInArrears),
		SpreadIndex_(SpreadIndex) {}

	SpreadLeg::SpreadLeg(const Schedule& schedule,
		const boost::shared_ptr<SpreadIndex>& index)
		: schedule_(schedule), index_(index),
		paymentAdjustment_(Following),
		inArrears_(false), zeroPayments_(false) {}

	SpreadLeg& SpreadLeg::withNotionals(Real notional) {
		notionals_ = std::vector<Real>(1,notional);
		return *this;
	}

	SpreadLeg& SpreadLeg::withNotionals(const std::vector<Real>& notionals) {
		notionals_ = notionals;
		return *this;
	}

	SpreadLeg& SpreadLeg::withPaymentDayCounter(const DayCounter& dayCounter) {
		paymentDayCounter_ = dayCounter;
		return *this;
	}

	SpreadLeg& SpreadLeg::withPaymentAdjustment(BusinessDayConvention convention) {
		paymentAdjustment_ = convention;
		return *this;
	}

	SpreadLeg& SpreadLeg::withFixingDays(Natural fixingDays) {
		fixingDays_ = std::vector<Natural>(1,fixingDays);
		return *this;
	}

	SpreadLeg& SpreadLeg::withFixingDays(const std::vector<Natural>& fixingDays) {
		fixingDays_ = fixingDays;
		return *this;
	}

	SpreadLeg& SpreadLeg::withGearings(Real gearing) {
		gearings_ = std::vector<Real>(1,gearing);
		return *this;
	}

	SpreadLeg& SpreadLeg::withGearings(const std::vector<Real>& gearings) {
		gearings_ = gearings;
		return *this;
	}

	SpreadLeg& SpreadLeg::withSpreads(Spread spread) {
		spreads_ = std::vector<Spread>(1,spread);
		return *this;
	}

	SpreadLeg& SpreadLeg::withSpreads(const std::vector<Spread>& spreads) {
		spreads_ = spreads;
		return *this;
	}

	SpreadLeg& SpreadLeg::withCaps(Rate cap) {
		caps_ = std::vector<Rate>(1,cap);
		return *this;
	}

	SpreadLeg& SpreadLeg::withCaps(const std::vector<Rate>& caps) {
		caps_ = caps;
		return *this;
	}

	SpreadLeg& SpreadLeg::withFloors(Rate floor) {
		floors_ = std::vector<Rate>(1,floor);
		return *this;
	}

	SpreadLeg& SpreadLeg::withFloors(const std::vector<Rate>& floors) {
		floors_ = floors;
		return *this;
	}

	SpreadLeg& SpreadLeg::inArrears(bool flag) {
		inArrears_ = flag;
		return *this;
	}

	SpreadLeg& SpreadLeg::withZeroPayments(bool flag) {
		zeroPayments_ = flag;
		return *this;
	}

	SpreadLeg::operator Leg() const {

		Leg cashflows =
			FloatingLeg<SpreadIndex, SpreadCoupon, CappedFlooredSpreadCoupon>(
			schedule_, notionals_, index_, paymentDayCounter_,
			paymentAdjustment_, fixingDays_, gearings_, spreads_,
			caps_, floors_, inArrears_, zeroPayments_);

		return cashflows;
	}




	DigitalSpreadLeg::DigitalSpreadLeg(const Schedule& schedule,
		const boost::shared_ptr<SpreadIndex>& index)
		: schedule_(schedule), index_(index),
		paymentAdjustment_(Following), inArrears_(false),
		longCallOption_(Position::Long), callATM_(false),
		longPutOption_(Position::Long), putATM_(false) {}

	DigitalSpreadLeg& DigitalSpreadLeg::withNotionals(Real notional) {
		notionals_ = std::vector<Real>(1,notional);
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withNotionals(
		const std::vector<Real>& notionals) {
			notionals_ = notionals;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withPaymentDayCounter(
		const DayCounter& dayCounter) {
			paymentDayCounter_ = dayCounter;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withPaymentAdjustment(
		BusinessDayConvention convention) {
			paymentAdjustment_ = convention;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withFixingDays(Natural fixingDays) {
		fixingDays_ = std::vector<Natural>(1,fixingDays);
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withFixingDays(
		const std::vector<Natural>& fixingDays) {
			fixingDays_ = fixingDays;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withGearings(Real gearing) {
		gearings_ = std::vector<Real>(1,gearing);
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withGearings(
		const std::vector<Real>& gearings) {
			gearings_ = gearings;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withSpreads(Spread spread) {
		spreads_ = std::vector<Spread>(1,spread);
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withSpreads(
		const std::vector<Spread>& spreads) {
			spreads_ = spreads;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::inArrears(bool flag) {
		inArrears_ = flag;
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withCallStrikes(Rate strike) {
		callStrikes_ = std::vector<Rate>(1,strike);
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withCallStrikes(
		const std::vector<Rate>& strikes) {
			callStrikes_ = strikes;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withLongCallOption(Position::Type type) {
		longCallOption_ = type;
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withCallATM(bool flag) {
		callATM_ = flag;
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withCallPayoffs(Rate payoff) {
		callPayoffs_ = std::vector<Rate>(1,payoff);
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withCallPayoffs(
		const std::vector<Rate>& payoffs) {
			callPayoffs_ = payoffs;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withPutStrikes(Rate strike) {
		putStrikes_ = std::vector<Rate>(1,strike);
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withPutStrikes(
		const std::vector<Rate>& strikes) {
			putStrikes_ = strikes;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withLongPutOption(Position::Type type) {
		longPutOption_ = type;
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withPutATM(bool flag) {
		putATM_ = flag;
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withPutPayoffs(Rate payoff) {
		putPayoffs_ = std::vector<Rate>(1,payoff);
		return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withPutPayoffs(
		const std::vector<Rate>& payoffs) {
			putPayoffs_ = payoffs;
			return *this;
	}

	DigitalSpreadLeg& DigitalSpreadLeg::withReplication(
		const boost::shared_ptr<DigitalReplication>& replication) {
			replication_ = replication;
			return *this;
	}

	DigitalSpreadLeg::operator Leg() const {

		return FloatingDigitalLeg<SpreadIndex, SpreadCoupon, DigitalSpreadCoupon>(
			schedule_, notionals_, index_, paymentDayCounter_,
			paymentAdjustment_, fixingDays_,
			gearings_, spreads_, inArrears_,
			callStrikes_, longCallOption_,
			callATM_, callPayoffs_,
			putStrikes_, longPutOption_,
			putATM_, putPayoffs_,
			replication_);
	}


}
