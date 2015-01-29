#ifndef spread_index_hpp
#define spread_index_hpp

#include <ql/index.hpp>
#include <ql/time/calendar.hpp>
#include <ql/currency.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/period.hpp>
#include <ql/indexes/interestrateindex.hpp>
#include <ql/indexes/iborindex.hpp>
#include <ql/cashflows/floatingratecoupon.hpp>
#include <ql/time/schedule.hpp>
#include <ql/indexes/swapindex.hpp>

namespace QuantLib {

	class SpreadIndex : public InterestRateIndex {
	public:
		SpreadIndex(boost::shared_ptr<InterestRateIndex> index1,
			boost::shared_ptr<InterestRateIndex> index2, 
			Rate alpha = 1.0, 
			Rate beta = -1.0) : 
		InterestRateIndex(index1->familyName(), index1->tenor(), index1->fixingDays(), index1->currency(),
                        index1->fixingCalendar(), index1->dayCounter()),
		index1_(index1), index2_(index2), alpha_(alpha), beta_(beta) {}

		Handle<YieldTermStructure> forwardingTermStructure() const {
			if (boost::dynamic_pointer_cast<IborIndex>(index1_))
				return boost::dynamic_pointer_cast<IborIndex>(index1_)->forwardingTermStructure();
			else if (boost::dynamic_pointer_cast<SwapIndex>(index1_))
				return boost::dynamic_pointer_cast<SwapIndex>(index1_)->forwardingTermStructure();
			else
				QL_FAIL("Index Identification Fail");
		}
		
		Date maturityDate(const Date& valueDate) const {
			QL_FAIL("Not applicable"); }

		boost::shared_ptr<InterestRateIndex> index1() { return index1_;}
		boost::shared_ptr<InterestRateIndex> index2() { return index2_;}

		Rate alpha() {return alpha_;}
		Rate beta() {return beta_;}

	protected:
		Rate forecastFixing(const Date& fixingDate) const { QL_FAIL("Not applicable"); }

		boost::shared_ptr<InterestRateIndex> index1_;
		boost::shared_ptr<InterestRateIndex> index2_;
		Rate alpha_;
		Rate beta_;
	};

}

#endif

