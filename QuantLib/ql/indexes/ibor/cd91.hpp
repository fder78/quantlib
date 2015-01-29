/* CD91 rate index */

#ifndef cd91_hpp
#define cd91_hpp

#include <ql/indexes/iborindex.hpp>
#include <ql/time/calendars/southkorea.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/currencies/asia.hpp>

namespace QuantLib {

	class CD91 : public IborIndex {
	public:
		CD91(const Handle<YieldTermStructure>& h =
			Handle<YieldTermStructure>())
			: IborIndex("CD91", Period(91,Days), 1, KRWCurrency(),
			SouthKorea(SouthKorea::Settlement), ModifiedFollowing,
			false, Actual365Fixed(), h) {}
	};
}

#endif