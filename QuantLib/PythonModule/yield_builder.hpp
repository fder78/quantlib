#ifndef yield_builder_hpp
#define yield_builder_hpp

#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/yield/zerocurve.hpp>

namespace QuantLib {

	struct YieldCurveData {
		std::vector<Date> dates;
		std::vector<Rate> yields;
		DayCounter dc;
	};
	
	boost::shared_ptr<YieldTermStructure> build_yield_curve(const Date& today,
		const Rate& forward,
		const DayCounter& dc);


	boost::shared_ptr<YieldTermStructure> build_yield_curve(const YieldCurveData& data);

}

#endif