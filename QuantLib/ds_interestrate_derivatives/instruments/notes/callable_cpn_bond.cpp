
#include <ds_interestrate_derivatives/instruments/notes/callable_cpn_bond.hpp>

namespace QuantLib {
	
	CallableCpnBond::CallableCpnBond(
					Natural settlementDays,
                    const Calendar& calendar,
					const Date& issueDate,					
					const CallabilitySchedule& callabilitySchedule, 
					const Exercise::Type& type,
					const bool& average,
					const Real alpha,
					const Real fixingRate
					)
    : Bond(settlementDays, calendar, issueDate),
	   CallSchedule(callabilitySchedule, type), isAverage_(average), alpha_(alpha), fixingRate_(fixingRate)
	{
			//QL_ENSURE(!cashflows().empty(), "bond with no cashflows!");
	}


	void CallableCpnBond::setupArguments(PricingEngine::arguments* args) const{

		Bond::setupArguments(args);

		CallableCpnBond::arguments* arguments = dynamic_cast<CallableCpnBond::arguments*>(args);
		if (arguments != 0) {
		//QL_REQUIRE(arguments != 0, "wrong argument type");
			arguments->callabilitySchedule = callabilitySchedule_;
			arguments->exerciseType = exerciseType_;
			arguments->isAverage = isAverage_;
			arguments->alpha = alpha_;
			arguments->fixingRate = fixingRate_;
			arguments->inverseFixing = Null<Real>();
			arguments->inverseGearing = Null<Real>();
			arguments->cap  = Null<Real>();
			arguments->floor = Null<Real>();
		}
	}
	void CallableCpnBond::arguments::validate() const {
	}

	///convertibility
	ConvertibleCpnBond::ConvertibleCpnBond(
					 Natural settlementDays,
                     const Calendar& calendar,
					 boost::shared_ptr<InterestRateIndex> stockIndex,
					 const boost::shared_ptr<Exercise>& exercise,
					 Real initialStrike,
					 Real faceAmount,
                     const Date& issueDate,					 
                     const CallabilitySchedule& putCallSchedule,
					 const RefixRate refixRate,
					 const Exercise::Type& type,
					 bool isBW
					)
    : CallableCpnBond(settlementDays, calendar, issueDate, putCallSchedule, type), faceAmount_(faceAmount),
	exercise_(exercise), initialStrike_(initialStrike), refixRate_(refixRate), isBW_(isBW)
	{
		stockIndexes_.empty();
		stockIndexes_.push_back(stockIndex);
		currencyIndexes_.empty();
	}

	ConvertibleCpnBond::ConvertibleCpnBond(
					 Natural settlementDays,
                     const Calendar& calendar,
					 boost::shared_ptr<InterestRateIndex> stockIndex,
					 boost::shared_ptr<InterestRateIndex> currencyIndex,
					 const boost::shared_ptr<Exercise>& exercise,
					 Real initialStrike,
					 Real faceAmount,
                     const Date& issueDate,					 
                     const CallabilitySchedule& putCallSchedule,
					 const RefixRate refixRate,
					 const Exercise::Type& type,
					 bool isBW
					)
    : CallableCpnBond(settlementDays, calendar, issueDate, putCallSchedule, type), faceAmount_(faceAmount),
	exercise_(exercise), initialStrike_(initialStrike), refixRate_(refixRate), isBW_(isBW)
	{
		stockIndexes_.empty();
		stockIndexes_.push_back(stockIndex);
		currencyIndexes_.empty();
		currencyIndexes_.push_back(currencyIndex);
	}


	void ConvertibleCpnBond::setupArguments(PricingEngine::arguments* args) const{

		CallableCpnBond::setupArguments(args);

		ConvertibleCpnBond::arguments* arguments = dynamic_cast<ConvertibleCpnBond::arguments*>(args);
		if (arguments != 0) {
		//QL_REQUIRE(arguments != 0, "wrong argument type");
			arguments->exercise = exercise_;
			arguments->initialStrike = initialStrike_;
			arguments->stockIndexes=stockIndexes_;
			arguments->currencyIndexes=currencyIndexes_;
			arguments->refixRate=refixRate_;
			arguments->isBW=isBW_;
			arguments->faceAmount=faceAmount_;
		}
	}
}