
#include <ds_interestrate_derivatives/pricingengine/fdm_engine/fdm_ghw_op.hpp>

#include <ql/math/functional.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/methods/finitedifferences/meshers/fdmmesher.hpp>
#include <ql/methods/finitedifferences/operators/secondderivativeop.hpp>
#include <ql/methods/finitedifferences/operators/secondordermixedderivativeop.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearopiterator.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearoplayout.hpp>

namespace QuantLib {

	/******************************************************************************/

	FdmBSM1dHWEquityPart::FdmBSM1dHWEquityPart(
		const boost::shared_ptr<FdmMesher>& mesher,
		const boost::shared_ptr<GeneralizedBlackScholesProcess>& p,
		const Integer direction)
		: rates_(mesher->locations(1)),
		x_(mesher->locations(0)),
		p_(p),
		dxMap_ (FirstDerivativeOp(direction, mesher)),
		dxxMap_(SecondDerivativeOp(direction, mesher)),
		mapT_   (direction, mesher),
		mesher_ (mesher) {
	}

	void FdmBSM1dHWEquityPart::setTime(Time t1, Time t2) {
		const Rate q = p_->dividendYield()->forwardRate(t1, t2, Continuous).rate();
		//******변동성 고정///////////////////////////////////////////////////////////////
		const boost::shared_ptr<FdmLinearOpLayout> layout=mesher_->layout();
		const FdmLinearOpIterator endIter = layout->end();

		Array variance(layout->size());
		for (FdmLinearOpIterator iter = layout->begin(); iter!=endIter; ++iter) {
			const Size i = iter.index();
			variance[i] = square<Real>()(p_->localVolatility()->localVol(0.5*(t1+t2), std::exp(x_[i]), true));
		}
		//Real variance = p_->blackVolatility()->blackVol(0.0, p_->x0());
		//variance *= variance;

		mapT_.axpyb(rates_ - 0.5*variance - q, 
			dxMap_, dxxMap_.mult(0.5*variance), Array());
	}

	const TripleBandLinearOp& FdmBSM1dHWEquityPart::getMap() const {
		return mapT_;
	}

	/******************************************************************************/

	FdmHWRatesPart::FdmHWRatesPart(
		const boost::shared_ptr<FdmMesher>& mesher,
		const boost::shared_ptr<Generalized_HullWhite>& hw,
		const Integer direction)
		: rates_(mesher->locations(direction)), mesher_(mesher),
		dzMap_(FirstDerivativeOp(direction, mesher)),
		dzzMap_(SecondDerivativeOp(direction, mesher)),
		mapT_(direction, mesher),
		hw_(hw), vol_(hw_->vol()), speed_(hw_->speed()) {
	}

	void FdmHWRatesPart::setTime(Time t1, Time t2, Real rateDiff) {
		const Time dt = t2-t1;
		const Array drift = (rates_*(std::exp(-speed_(t1)*dt)-1.0) + hw_->expectation(t1, t2, 0.0))/dt - vol_(t1) * hw_->fxVol() * hw_->fxCorr();		
		mapT_.axpyb(drift, dzMap_, 
			dzzMap_.mult(0.5*vol_(t1)*vol_(t1)*Array(mesher_->layout()->size(), 1.)).add(-0.5 * (mesher_->locations(0) + rateDiff)),
			Array());

	}

	const TripleBandLinearOp& FdmHWRatesPart::getMap() const {
		return mapT_;
	}

	/******************************************************************************/

	FdmR2dHWOp::FdmR2dHWOp(
		const boost::shared_ptr<FdmMesher>& mesher,
		const boost::shared_ptr<Generalized_HullWhite>& hw0,
		const boost::shared_ptr<Generalized_HullWhite>& hw1,
		const Handle<YieldTermStructure>& discTS,
		Real corr,
		Time maturity)
		: mesher_(mesher), hw0_(hw0), hw1_(hw1), discTS_(discTS),
		opR0_(mesher, hw0_, 0), opR1_(mesher, hw1_, 1),
		corrMap_(0, 1, mesher),
		corrMapTemplate_(SecondOrderMixedDerivativeOp(0, 1, mesher).mult(Array(mesher->layout()->size(), corr))),
		vol0_(hw0_->vol()), vol1_(hw1_->vol()) {}

	Size FdmR2dHWOp::size() const {
		return 2;
	}

	void FdmR2dHWOp::setTime(Time t1, Time t2) {
		Real rateDiff = discTS_->forwardRate(t1, t2, Continuous) - hw0_->termStructure()->forwardRate(t1, t2, Continuous);
		opR0_.setTime(t1, t2, rateDiff);
		opR1_.setTime(t1, t2, rateDiff);
		corrMap_ = corrMapTemplate_.mult(Array(mesher_->layout()->size(), vol0_(t1) * vol1_(t1)));
	}

	Disposable<Array> FdmR2dHWOp::apply(const Array& x) const {
		return opR0_.getMap().apply(x) + opR1_.getMap().apply(x) + corrMap_.apply(x);
	}

	Disposable<Array> FdmR2dHWOp::apply_mixed(const Array& x) const {
		return corrMap_.apply(x);
	}

	Disposable<Array> FdmR2dHWOp::apply_direction(Size direction, const Array& x) const {
			if (direction == 0) {
				return opR0_.getMap().apply(x);
			}
			else if (direction == 1) {
				return opR1_.getMap().apply(x);
			}
			else
				QL_FAIL("direction is too large");
	}

	Disposable<Array> FdmR2dHWOp::solve_splitting(Size direction, const Array& x, Real s) const {
			if (direction == 0) {
				return opR0_.getMap().solve_splitting(x, s, 1.0);
			}
			else if (direction == 1) {
				return opR1_.getMap().solve_splitting(x, s, 1.0);
			}
			else
				QL_FAIL("direction is too large");
	}

	Disposable<Array> FdmR2dHWOp::preconditioner(const Array& r, Real dt) const {
			return solve_splitting(0, r, dt);
	}
}
