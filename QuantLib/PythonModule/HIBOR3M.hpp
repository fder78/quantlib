/* HIBOR3M index */
#pragma once

#include <ql/indexes/iborindex.hpp>
#include <ql/time/calendars/hongkong.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/currencies/asia.hpp>

namespace QuantLib {

	class HIBOR3M : public IborIndex {
	public:
		HIBOR3M(const Handle<YieldTermStructure>& h =
			Handle<YieldTermStructure>())
			: IborIndex("HIBOR3M", Period(3,Months), 1, HKDCurrency(),
			HongKong(), ModifiedFollowing,
			false, Actual365Fixed(), h) {}
	};
}

