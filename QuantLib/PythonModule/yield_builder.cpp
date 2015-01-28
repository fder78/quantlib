#include "StdAfx.h"

#include "yield_builder.hpp"

namespace QuantLib {

	boost::shared_ptr<YieldTermStructure> build_yield_curve(const Date& today,
		const Rate& forward,
		const DayCounter& dc) {
			return boost::shared_ptr<YieldTermStructure>(new
				FlatForward(today, forward, dc));
	}

	boost::shared_ptr<YieldTermStructure> build_yield_curve(const YieldCurveData& data) {
			return boost::shared_ptr<YieldTermStructure>(new
				InterpolatedZeroCurve<Linear>(data.dates, data.yields, data.dc));
	}

}