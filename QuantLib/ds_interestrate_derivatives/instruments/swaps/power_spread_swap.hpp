
#ifndef power_spread_swap_hpp
#define power_spread_swap_hpp

#include <ds_interestrate_derivatives/instruments/notes/power_spread_note.hpp>

namespace QuantLib {

	class Schedule;

	class PowerSpreadSwap : public PowerSpreadNote {
	public:
		enum CpnType {Fixed, Floating, RangeAccrual};
		enum Side {Payer, Receiver};

		PowerSpreadSwap(Natural settlementDays,
			Real faceAmount,
			Side side,
			const Schedule& floatingSchedule,
			Leg floatingcashflows,
			const Rate alpha,
			const Schedule& fixedSchedule,		
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
			const Exercise::Type& type = Exercise::Bermudan) :
		PowerSpreadNote(settlementDays, faceAmount, fixedSchedule, index, accrualDayCounter, paymentConvention, fixingDays, 
			gearings, spreads, caps, floors, isAvg, redemption, issueDate, callabilitySchedule, type),
			side_(side), floatingSchedule_(floatingSchedule), alpha_(alpha),
			floatingcashflows_(floatingcashflows) {}

		class arguments;
		class engine;
		void setupArguments(PricingEngine::arguments*) const;

	private:
		Side side_;
		Schedule floatingSchedule_;
		Rate alpha_;
		Leg floatingcashflows_;

	};

	class PowerSpreadSwap::arguments : public virtual PowerSpreadNote::arguments {
	public:
		Side side;
		Schedule floatingSchedule;
		Real floatingGearing;
		Leg floatingcashflows;
		Rate alpha;
	};

	class PowerSpreadSwap::engine :	public GenericEngine<PowerSpreadSwap::arguments,
		PowerSpreadSwap::results> {};


}

#endif
