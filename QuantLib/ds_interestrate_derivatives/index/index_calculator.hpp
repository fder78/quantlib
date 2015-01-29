#ifndef index_calculator_hpp
#define index_calculator_hpp

#include <ql/models/shortrate/onefactormodel.hpp>

namespace QuantLib {

	class IndexCalculator {
		
	public:
		enum IndexType {IborRate, SwapRate};
		IndexCalculator(IndexType type, boost::shared_ptr<OneFactorAffineModel> model);
		const Rate operator()(Time t, Period tenor, Real shortRate) const;
		const Rate operator()(Time t, Period length, Period tenor, Real shortRate) const;

	private:
		IndexType type_;
		boost::shared_ptr<OneFactorAffineModel> model_;
		
	};

}

#endif

