
#ifndef range_accrual_note_hpp
#define range_accrual_note_hpp

#include <ql/instruments/bond.hpp>
#include <ql/time/dategenerationrule.hpp>
#include <ql/math/matrix.hpp>

#include <ds_interestrate_derivatives/cashflows/range_accrual_coupon.hpp>
#include <ds_interestrate_derivatives/instruments/notes/callable_cpn_bond.hpp>

namespace QuantLib {

	class Schedule;
	//class IborIndex;

	class RangeAccrualNote : public CallableCpnBond {
	public:
		enum CpnType {Fixed, Floating, RangeAccrual};

		/* Single Range*/
		RangeAccrualNote(Natural settlementDays,
			Real faceAmount,
			const Schedule& schedule,
			const boost::shared_ptr<InterestRateIndex>& index,
			const boost::shared_ptr<InterestRateIndex>& obsIndex,
			const DayCounter& accrualDayCounter,
			BusinessDayConvention paymentConvention	= Following,
			Natural fixingDays = Null<Natural>(),
			const std::vector<Real>& gearings = std::vector<Real>(1, 1.0),
			const std::vector<Spread>& spreads = std::vector<Spread>(1, 0.0),
			const std::vector<Rate> lowerTriggers = std::vector<Rate>(),
			const std::vector<Rate> upperTriggers = std::vector<Rate>(), //Big
			const Period& obsTenor = Period(1,Days),
			BusinessDayConvention obsConvention	= Unadjusted,
			Real redemption = 100.0,
			const Date& issueDate = Date(),
			const CallabilitySchedule& callabilitySchedule = CallabilitySchedule(),
			const Exercise::Type& type = Exercise::Bermudan,
			const Real alpha = Null<Real>(),
			const Real fixingRate = Null<Real>());

		RangeAccrualNote(Natural settlementDays,
			Real faceAmount,
			const Date& startDate,
			const Date& maturityDate,
			Frequency couponFrequency,
			const Calendar& calendar,
			const boost::shared_ptr<InterestRateIndex>& index,
			const boost::shared_ptr<InterestRateIndex>& obsIndex,
			const DayCounter& accrualDayCounter,
			BusinessDayConvention accrualConvention = Following,
			BusinessDayConvention paymentConvention = Following,
			Natural fixingDays = Null<Natural>(),
			const std::vector<Real>& gearings = std::vector<Real>(1, 1.0),
			const std::vector<Spread>& spreads = std::vector<Spread>(1, 0.0),
			const std::vector<Rate> lowerTriggers = std::vector<Rate>(),
			const std::vector<Rate> upperTriggers = std::vector<Rate>(), //Big
			const Period& obsTenor = Period(1,Days),
			BusinessDayConvention obsConvention = Unadjusted,
			Real redemption = 100.0,
			const Date& issueDate = Date(),
			const Date& stubDate = Date(),
			DateGeneration::Rule rule = DateGeneration::Backward,
			bool endOfMonth = false,
			const CallabilitySchedule& callabilitySchedule = CallabilitySchedule(),
			const Exercise::Type& type = Exercise::Bermudan,
			const Real alpha = Null<Real>(),
			const Real fixingRate = Null<Real>());

		/* Dual Range*/		
		RangeAccrualNote(Natural settlementDays,
			Real faceAmount,
			const Schedule& schedule,
			const boost::shared_ptr<InterestRateIndex>& index,
			const boost::shared_ptr<InterestRateIndex>& obsIndex1,
			const boost::shared_ptr<InterestRateIndex>& obsIndex2,
			const DayCounter& accrualDayCounter,
			BusinessDayConvention paymentConvention	= Following,
			Natural fixingDays = Null<Natural>(),
			const std::vector<Real>& gearings = std::vector<Real>(1, 1.0),
			const std::vector<Spread>& spreads = std::vector<Spread>(1, 0.0),
			const std::vector<Rate> lowerTriggers1 = std::vector<Rate>(),
			const std::vector<Rate> upperTriggers1 = std::vector<Rate>(), //Big
			const std::vector<Rate> lowerTriggers2 = std::vector<Rate>(),
			const std::vector<Rate> upperTriggers2 = std::vector<Rate>(), //Big
			const Period& obsTenor = Period(1,Days),
			BusinessDayConvention obsConvention	= Unadjusted,
			Real redemption = 100.0,
			const Date& issueDate = Date(),
			const CallabilitySchedule& callabilitySchedule = CallabilitySchedule(),
			const Exercise::Type& type = Exercise::Bermudan,
			const Real alpha = Null<Real>(),
			const Real fixingRate = Null<Real>(),
			const std::vector<Matrix> m = std::vector<Matrix>(1,Matrix()),
			const std::vector<Matrix> tenor = std::vector<Matrix>(1,Matrix()));

		RangeAccrualNote(Natural settlementDays,
			Real faceAmount,
			const Date& startDate,
			const Date& maturityDate,
			Frequency couponFrequency,
			const Calendar& calendar,
			const boost::shared_ptr<InterestRateIndex>& index,
			const boost::shared_ptr<InterestRateIndex>& obsIndex1,
			const boost::shared_ptr<InterestRateIndex>& obsIndex2,
			const DayCounter& accrualDayCounter,
			BusinessDayConvention accrualConvention = Following,
			BusinessDayConvention paymentConvention = Following,
			Natural fixingDays = Null<Natural>(),
			const std::vector<Real>& gearings = std::vector<Real>(1, 1.0),
			const std::vector<Spread>& spreads = std::vector<Spread>(1, 0.0),
			const std::vector<Rate> lowerTriggers1 = std::vector<Rate>(),
			const std::vector<Rate> upperTriggers1 = std::vector<Rate>(), //Big
			const std::vector<Rate> lowerTriggers2 = std::vector<Rate>(),
			const std::vector<Rate> upperTriggers2 = std::vector<Rate>(), //Big
			const Period& obsTenor = Period(1,Days),
			BusinessDayConvention obsConvention = Unadjusted,
			Real redemption = 100.0,
			const Date& issueDate = Date(),
			const Date& stubDate = Date(),
			DateGeneration::Rule rule = DateGeneration::Backward,
			bool endOfMonth = false,
			const CallabilitySchedule& callabilitySchedule = CallabilitySchedule(),
			const Exercise::Type& type = Exercise::Bermudan,
			const Real alpha = Null<Real>(),
			const Real fixingRate = Null<Real>());

		class arguments;
		class engine;
		void setupArguments(PricingEngine::arguments*) const;

		void setSwap(const Real alpha, const Real fixingRate) {
			alpha_ = alpha;
			fixingRate_ = fixingRate;
		}
		void setInverseFloater(const Real inverseAlpha,
			const Real inverseGearing, const Real cap, const Real floor, const Real inverseFixing) {
			inverseAlpha_ = inverseAlpha;
			inverseGearing_ = inverseGearing;
			inverseFixing_ = inverseFixing;
			cap_ = cap;
			floor_ = floor;
		}

	private:	
		std::vector<std::vector<Real> > getTriggers(std::vector<Real> trigger, Size n);
		std::vector<std::vector<Real> > getTriggers(std::vector<Real> trigger1, std::vector<Real> trigger2, Size n);

		std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex_;
		std::vector<std::vector<Real> > lowerTriggers_;
		std::vector<std::vector<Real> > upperTriggers_;

		std::vector<CpnType> cpnType_;
		std::vector<Matrix> m_, tenor_;
		Real inverseAlpha_, inverseGearing_, cap_, floor_, inverseFixing_;
	};

	class RangeAccrualNote::arguments : public virtual CallableCpnBond::arguments {
	public:
		std::vector<CpnType> cpnType;
		std::vector<std::vector<Real> > lowerTriggers, upperTriggers;
		std::vector<Matrix> coeff;
		std::vector<Matrix> tenor;
		//Real inverseGearing, cap, floor, inverseFixing;
		void validate() const;
	};

	class RangeAccrualNote::engine : public GenericEngine<RangeAccrualNote::arguments, RangeAccrualNote::results> {};


}

#endif
