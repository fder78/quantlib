
#ifndef CallableCpnBond_hpp
#define CallableCpnBond_hpp

#include <ql/instruments/bond.hpp>
#include <ql/time/dategenerationrule.hpp>
#include <ql/cashflows/iborcoupon.hpp>
#include <ql/cashflows/simplecashflow.hpp>
#include <ql/time/schedule.hpp>
#include <ql/indexes/swapindex.hpp>
#include <ql/indexes/iborindex.hpp>

#include <ds_interestrate_derivatives/instruments/callability.hpp>
#include <ql/exercise.hpp>

namespace QuantLib {

	class CallableCpnBond : public Bond, CallSchedule {
	public:

		CallableCpnBond(
					 Natural settlementDays,
                     const Calendar& calendar,
                     const Date& issueDate = Date(),
                     const CallabilitySchedule& putCallSchedule = CallabilitySchedule(),
					 const Exercise::Type& type = Exercise::Bermudan,
					 const bool& average=false,
					 const Real alpha=Null<Real>(),
					 const Real fixingRate = Null<Real>()
					 );
		
		class arguments;
		class engine;
		void setupArguments(PricingEngine::arguments*) const;
		bool isAverage_;
		Real alpha_, fixingRate_;
	};

	class CallableCpnBond::arguments : public virtual Bond::arguments {
	public:
		CallabilitySchedule callabilitySchedule;
		Exercise::Type exerciseType;
		bool isAverage;
		Real alpha, fixingRate;
		Real inverseAlpha, inverseGearing, inverseFixing, cap, floor;
		void validate() const;
	};
	
	class CallableCpnBond::engine :	public GenericEngine<CallableCpnBond::arguments,
		CallableCpnBond::results> {};

	//// ConvertibleCpnBond Ãß°¡
	class ConvertibleCpnBond : public CallableCpnBond {
	public:
		ConvertibleCpnBond(
					 Natural settlementDays,
                     const Calendar& calendar,
					 boost::shared_ptr<InterestRateIndex> stock,
					 const boost::shared_ptr<Exercise>&,
					 Real initialStrike,
					 Real faceAmount,
                     const Date& issueDate = Date(),					 
                     const CallabilitySchedule& putCallSchedule = CallabilitySchedule(),
					 const RefixRate refixRate=RefixRate(),
					 const Exercise::Type& type = Exercise::Bermudan,
					 bool isBW=false
			);
		ConvertibleCpnBond(
					 Natural settlementDays,
                     const Calendar& calendar,
					 boost::shared_ptr<InterestRateIndex> stockIndex,
					 boost::shared_ptr<InterestRateIndex> currencyIndex,
					 const boost::shared_ptr<Exercise>&,
					 Real initialStrike,
					 Real faceAmount,
                     const Date& issueDate = Date(),					 
                     const CallabilitySchedule& putCallSchedule = CallabilitySchedule(),
					 const RefixRate refixRate=RefixRate(),
					 const Exercise::Type& type = Exercise::Bermudan,
					 bool isBW=false
			);
		class arguments;
		class engine;
		void setupArguments(PricingEngine::arguments*) const;
		boost::shared_ptr<Exercise> exercise_;
		Real initialStrike_;
		std::vector<boost::shared_ptr<InterestRateIndex> > stockIndexes_;
		std::vector<boost::shared_ptr<InterestRateIndex> > currencyIndexes_;
		RefixRate refixRate_;
		bool isBW_;
		Real faceAmount_;
	};

	class ConvertibleCpnBond::arguments : public virtual CallableCpnBond::arguments {
	public:					 
		boost::shared_ptr<Exercise> exercise;		
		Real initialStrike;
		std::vector<boost::shared_ptr<InterestRateIndex> > stockIndexes;
		std::vector<boost::shared_ptr<InterestRateIndex> > currencyIndexes;
		RefixRate refixRate;
		bool isBW;
		Real faceAmount;
	};
	
	class ConvertibleCpnBond::engine :	public GenericEngine<ConvertibleCpnBond::arguments,
		ConvertibleCpnBond::results> {};
}
	
#endif

