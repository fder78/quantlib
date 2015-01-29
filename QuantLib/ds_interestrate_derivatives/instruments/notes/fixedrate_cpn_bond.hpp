
#ifndef fixedRateCpnBond_hpp
#define fixedRateCpnBond_hpp

#include <ql/instruments/bond.hpp>
#include <ql/time/dategenerationrule.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/interestrate.hpp>
#include <ds_interestrate_derivatives/instruments/notes/callable_cpn_bond.hpp>

namespace QuantLib {

	class Schedule;

	//! fixed-rate bond
	/*! /ingroup instruments

	/test calculations are tested by checking results against
	cached values.
	*/
	class FixedRateCpnBond : public CallableCpnBond  {
	public:
		FixedRateCpnBond(Natural settlementDays,
			Real faceAmount,
			const Schedule& schedule,
			const std::vector<Rate>& coupons,
			const DayCounter& accrualDayCounter,
			BusinessDayConvention paymentConvention = Following,
			Real redemption = 100.0,
			const Date& issueDate = Date(),
			const CallabilitySchedule& callabilitySchedule = CallabilitySchedule(),
			const Exercise::Type& type = Exercise::Bermudan);
		FixedRateCpnBond(Natural settlementDays,
			const Calendar& calendar,
			Real faceAmount,
			const Date& startDate,
			const Date& maturityDate,
			const Period& tenor,
			const std::vector<Rate>& coupons,
			const DayCounter& accrualDayCounter,
			BusinessDayConvention accrualConvention = Following,
			BusinessDayConvention paymentConvention = Following,
			Real redemption = 100.0,
			const Date& issueDate = Date(),
			const Date& stubDate = Date(),
			DateGeneration::Rule rule = DateGeneration::Backward,
			bool endOfMonth = false,
			const CallabilitySchedule& callabilitySchedule = CallabilitySchedule(),
			const Exercise::Type& type = Exercise::Bermudan);
		FixedRateCpnBond(Natural settlementDays,
			Real faceAmount,
			const Schedule& schedule,
			const std::vector<InterestRate>& coupons,
			BusinessDayConvention paymentConvention = Following,
			Real redemption = 100.0,
			const Date& issueDate = Date(),
			const CallabilitySchedule& callabilitySchedule = CallabilitySchedule(),
			const Exercise::Type& type = Exercise::Bermudan);
		Frequency frequency() const { return frequency_; }
		const DayCounter& dayCounter() const { return dayCounter_; }

		class arguments;
		class engine;
		void setupArguments(PricingEngine::arguments*) const;

	protected:
		Frequency frequency_;
		DayCounter dayCounter_;
	};

	class FixedRateCpnBond::arguments : public virtual CallableCpnBond::arguments {
	public:
		void validate() const;
	};
	
	class FixedRateCpnBond::engine :	public GenericEngine<FixedRateCpnBond::arguments,
		FixedRateCpnBond::results> {};

}

#endif
