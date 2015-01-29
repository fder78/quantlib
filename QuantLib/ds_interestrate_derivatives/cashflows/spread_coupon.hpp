
#ifndef spread_coupon_hpp
#define spread_coupon_hpp

#include <ql/cashflows/floatingratecoupon.hpp>
#include <ql/cashflows/digitalcoupon.hpp>
#include <ql/cashflows/capflooredcoupon.hpp>
#include <ql/time/schedule.hpp>

#include <ds_interestrate_derivatives/index/spread_index.hpp>

namespace QuantLib {

	class SpreadCoupon : public FloatingRateCoupon {
	public:
		SpreadCoupon(const Date& paymentDate,
			Real nominal,
			const Date& startDate,
			const Date& endDate,
			Natural fixingDays,
			const boost::shared_ptr<SpreadIndex>& index,
			Real gearing = 1.0,
			Spread spread = 0.0,
			const Date& refPeriodStart = Date(),
			const Date& refPeriodEnd = Date(),
			const DayCounter& dayCounter = DayCounter(),
			bool isInArrears = false);

		const boost::shared_ptr<SpreadIndex>& Index() const {
			return SpreadIndex_;
		}

	private:
		boost::shared_ptr<SpreadIndex> SpreadIndex_;
	};
	

	class CappedFlooredSpreadCoupon : public CappedFlooredCoupon {
	public:
		CappedFlooredSpreadCoupon(
			const Date& paymentDate,
			Real nominal,
			const Date& startDate,
			const Date& endDate,
			Natural fixingDays,
			const boost::shared_ptr<SpreadIndex>& index,
			Real gearing = 1.0,
			Spread spread= 0.0,
			const Rate cap = Null<Rate>(),
			const Rate floor = Null<Rate>(),
			const Date& refPeriodStart = Date(),
			const Date& refPeriodEnd = Date(),
			const DayCounter& dayCounter = DayCounter(),
			bool isInArrears = false)
			: CappedFlooredCoupon(boost::shared_ptr<FloatingRateCoupon>(new
			SpreadCoupon(paymentDate, nominal, startDate, endDate, fixingDays,
			index, gearing, spread, refPeriodStart, refPeriodEnd,
			dayCounter, isInArrears)), cap, floor) {}

	};


	class DigitalSpreadCoupon : public DigitalCoupon {
	public:
		DigitalSpreadCoupon(
			const boost::shared_ptr<SpreadCoupon>& underlying,
			Rate callStrike = Null<Rate>(),
			Position::Type callPosition = Position::Long,
			bool isCallATMIncluded = false,
			Rate callDigitalPayoff = Null<Rate>(),
			Rate putStrike = Null<Rate>(),
			Position::Type putPosition = Position::Long,
			bool isPutATMIncluded = false,
			Rate putDigitalPayoff = Null<Rate>(),
			const boost::shared_ptr<DigitalReplication>& replication =
			boost::shared_ptr<DigitalReplication>())
			: DigitalCoupon(underlying, callStrike, callPosition, isCallATMIncluded,
			callDigitalPayoff, putStrike, putPosition,
			isPutATMIncluded, putDigitalPayoff, replication) {}
	};

	class SpreadLeg {
	public:
		SpreadLeg(const Schedule& schedule,
			const boost::shared_ptr<SpreadIndex>& index);
		SpreadLeg& withNotionals(Real notional);
		SpreadLeg& withNotionals(const std::vector<Real>& notionals);
		SpreadLeg& withPaymentDayCounter(const DayCounter&);
		SpreadLeg& withPaymentAdjustment(BusinessDayConvention);
		SpreadLeg& withFixingDays(Natural fixingDays);
		SpreadLeg& withFixingDays(const std::vector<Natural>& fixingDays);
		SpreadLeg& withGearings(Real gearing);
		SpreadLeg& withGearings(const std::vector<Real>& gearings);
		SpreadLeg& withSpreads(Spread spread);
		SpreadLeg& withSpreads(const std::vector<Spread>& spreads);
		SpreadLeg& withCaps(Rate cap);
		SpreadLeg& withCaps(const std::vector<Rate>& caps);
		SpreadLeg& withFloors(Rate floor);
		SpreadLeg& withFloors(const std::vector<Rate>& floors);
		SpreadLeg& inArrears(bool flag = true);
		SpreadLeg& withZeroPayments(bool flag = true);
		operator Leg() const;
	private:
		Schedule schedule_;
		boost::shared_ptr<SpreadIndex> index_;
		std::vector<Real> notionals_;
		DayCounter paymentDayCounter_;
		BusinessDayConvention paymentAdjustment_;
		std::vector<Natural> fixingDays_;
		std::vector<Real> gearings_;
		std::vector<Spread> spreads_;
		std::vector<Rate> caps_, floors_;
		bool inArrears_, zeroPayments_;
	};


	//! helper class building a sequence of digital ibor-rate coupons
	class DigitalSpreadLeg {
	public:
		DigitalSpreadLeg(const Schedule& schedule,
			const boost::shared_ptr<SpreadIndex>& index);
		DigitalSpreadLeg& withNotionals(Real notional);
		DigitalSpreadLeg& withNotionals(const std::vector<Real>& notionals);
		DigitalSpreadLeg& withPaymentDayCounter(const DayCounter&);
		DigitalSpreadLeg& withPaymentAdjustment(BusinessDayConvention);
		DigitalSpreadLeg& withFixingDays(Natural fixingDays);
		DigitalSpreadLeg& withFixingDays(const std::vector<Natural>& fixingDays);
		DigitalSpreadLeg& withGearings(Real gearing);
		DigitalSpreadLeg& withGearings(const std::vector<Real>& gearings);
		DigitalSpreadLeg& withSpreads(Spread spread);
		DigitalSpreadLeg& withSpreads(const std::vector<Spread>& spreads);
		DigitalSpreadLeg& inArrears(bool flag = true);
		DigitalSpreadLeg& withCallStrikes(Rate strike);
		DigitalSpreadLeg& withCallStrikes(const std::vector<Rate>& strikes);
		DigitalSpreadLeg& withLongCallOption(Position::Type);
		DigitalSpreadLeg& withCallATM(bool flag = true);
		DigitalSpreadLeg& withCallPayoffs(Rate payoff);
		DigitalSpreadLeg& withCallPayoffs(const std::vector<Rate>& payoffs);
		DigitalSpreadLeg& withPutStrikes(Rate strike);
		DigitalSpreadLeg& withPutStrikes(const std::vector<Rate>& strikes);
		DigitalSpreadLeg& withLongPutOption(Position::Type);
		DigitalSpreadLeg& withPutATM(bool flag = true);
		DigitalSpreadLeg& withPutPayoffs(Rate payoff);
		DigitalSpreadLeg& withPutPayoffs(const std::vector<Rate>& payoffs);
		DigitalSpreadLeg& withReplication(
			const boost::shared_ptr<DigitalReplication>& replication =
			boost::shared_ptr<DigitalReplication>());
		operator Leg() const;
	private:
		Schedule schedule_;
		boost::shared_ptr<SpreadIndex> index_;
		std::vector<Real> notionals_;
		DayCounter paymentDayCounter_;
		BusinessDayConvention paymentAdjustment_;
		std::vector<Natural> fixingDays_;
		std::vector<Real> gearings_;
		std::vector<Spread> spreads_;
		bool inArrears_;
		std::vector<Rate> callStrikes_, callPayoffs_;
		Position::Type longCallOption_;
		bool callATM_;
		std::vector<Rate> putStrikes_, putPayoffs_;
		Position::Type longPutOption_;
		bool putATM_;
		boost::shared_ptr<DigitalReplication> replication_;
	};
}

#endif
