
#ifndef range_accrual_swap_hpp
#define range_accrual_swap_hpp

#include <ds_interestrate_derivatives/instruments/notes/range_accrual_note.hpp>

namespace QuantLib {

	class Schedule;
	//class IborIndex;

	class RangeAccrualSwap : public RangeAccrualNote {
	public:
		enum CpnType {Fixed, Floating, RangeAccrual};
		enum Side {Payer, Receiver};

		/* Single Range*/
		RangeAccrualSwap(
			Natural settlementDays,
			Real faceAmount,
			Side side,
			const Schedule& floatingSchedule,	
			const Rate alpha,		
			const Schedule& fixedSchedule,			
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
			const Exercise::Type& type = Exercise::Bermudan) : 
		RangeAccrualNote(settlementDays, faceAmount, fixedSchedule, index, obsIndex, accrualDayCounter, paymentConvention, fixingDays, 
			gearings, spreads, lowerTriggers, upperTriggers, obsTenor, obsConvention, redemption, issueDate, callabilitySchedule, type),
			side_(side), floatingSchedule_(floatingSchedule), alpha_(alpha) {}


		/* Dual Range*/		
		RangeAccrualSwap(Natural settlementDays,
			Real faceAmount,
			Side side,
			const Schedule& floatingSchedule,
			const Rate alpha,
			const Schedule& fixedSchedule,
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
			const Exercise::Type& type = Exercise::Bermudan) : 
		RangeAccrualNote(settlementDays, faceAmount, fixedSchedule, index, obsIndex1, obsIndex2, accrualDayCounter, paymentConvention, fixingDays, 
			gearings, spreads, lowerTriggers1, upperTriggers1, lowerTriggers2, upperTriggers2, obsTenor, obsConvention, redemption, issueDate, callabilitySchedule, type),
			side_(side), floatingSchedule_(floatingSchedule), alpha_(alpha) {}

		class arguments;
		class engine;
		void setupArguments(PricingEngine::arguments*) const;


	private:
		Side side_;
		Schedule floatingSchedule_;
		Rate alpha_;


	};

	class RangeAccrualSwap::arguments : public virtual RangeAccrualNote::arguments {
	public:
		Side side;
		Schedule floatingSchedule;
		Rate alpha;
	};

	class RangeAccrualSwap::engine :	public GenericEngine<RangeAccrualSwap::arguments,
		RangeAccrualSwap::results> {};


}

#endif
