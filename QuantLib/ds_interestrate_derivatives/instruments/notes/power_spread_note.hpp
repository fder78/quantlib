
#ifndef power_spread_note_hpp
#define power_spread_note_hpp

#include <ql/instruments/bond.hpp>
#include <ql/time/dategenerationrule.hpp>

#include <ds_interestrate_derivatives/cashflows/spread_coupon.hpp>
#include <ds_interestrate_derivatives/instruments/notes/callable_cpn_bond.hpp>

namespace QuantLib {

	class Schedule;
	//class IborIndex;

	class PowerSpreadNote : public CallableCpnBond {
	public:

		PowerSpreadNote(Natural settlementDays,
			Real faceAmount,
			const Schedule& schedule,
			const boost::shared_ptr<SpreadIndex>& index,
			const DayCounter& accrualDayCounter,
			BusinessDayConvention paymentConvention	= Following,
			Natural fixingDays = Null<Natural>(),
			const std::vector<Real>& gearings = std::vector<Real>(1, 1.0),
			const std::vector<Spread>& spreads = std::vector<Spread>(1, 0.0),
			const std::vector<Rate> caps = std::vector<Rate>(),
			const std::vector<Rate> floors = std::vector<Rate>(), //Big
			const bool isAvg = false,
			Real redemption = 100.0,
			const Date& issueDate = Date(),
			const CallabilitySchedule& callabilitySchedule = CallabilitySchedule(),
			const Exercise::Type& type = Exercise::Bermudan);

		class arguments;
		class engine;
		void setupArguments(PricingEngine::arguments*) const;

	private:
		const bool isAvg_;

	};

	class PowerSpreadNote::arguments : public virtual CallableCpnBond::arguments {
	public:
		void validate() const;
		bool isAvg;
	};

	class PowerSpreadNote::engine :	public GenericEngine<PowerSpreadNote::arguments,
		PowerSpreadNote::results> {};

}

#endif
