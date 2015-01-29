
#pragma once

#include <ql/methods/finitedifferences/operators/ninepointlinearop.hpp>
#include <ql/methods/finitedifferences/operators/fdmblackscholesop.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearopcomposite.hpp>
#include <ds_interestrate_derivatives/model/generalized_hullwhite.hpp>

namespace QuantLib {

	class FdmMesher;
	class GeneralizedBlackScholesProcess;

	/***************************************************************/
	class FdmBSM1dHWEquityPart {
	public:
		FdmBSM1dHWEquityPart(
			const boost::shared_ptr<FdmMesher>& mesher,
			const boost::shared_ptr<GeneralizedBlackScholesProcess>& p,
			const Integer direction);

		void setTime(Time t1, Time t2);
		const TripleBandLinearOp& getMap() const;

	protected:
		const Array rates_;
		const Array x_;
		const FirstDerivativeOp  dxMap_;
		const TripleBandLinearOp dxxMap_;
		TripleBandLinearOp mapT_;

		const boost::shared_ptr<FdmMesher> mesher_;
		const boost::shared_ptr<GeneralizedBlackScholesProcess> p_;
	};

	/***************************************************************/
	class FdmHWRatesPart {
	public:
		FdmHWRatesPart(
			const boost::shared_ptr<FdmMesher>& mesher,
			const boost::shared_ptr<Generalized_HullWhite>& hw,
			const Integer direction);

		void setTime(Time t1, Time t2, Real);
		const TripleBandLinearOp& getMap() const;

	protected:
		const boost::shared_ptr<FdmMesher> mesher_;
		const Array rates_;
		const TripleBandLinearOp dzMap_, dzzMap_;
		TripleBandLinearOp mapT_;
		const boost::shared_ptr<Generalized_HullWhite> hw_;
		boost::function<Real (Time)> vol_, speed_;
	};


	/***************************************************************/

	class FdmR2dHWOp : public FdmLinearOpComposite {
	public:
		FdmR2dHWOp(
			const boost::shared_ptr<FdmMesher>& mesher,
			const boost::shared_ptr<Generalized_HullWhite>& hw0,
			const boost::shared_ptr<Generalized_HullWhite>& hw1,
			const Handle<YieldTermStructure>& discTS,
			Real corr,
			Time maturity);

		Size size() const;
		void setTime(Time t1, Time t2);    
		Disposable<Array> apply(const Array& x) const;
		Disposable<Array> apply_mixed(const Array& x) const;

		Disposable<Array> apply_direction(Size direction,const Array& x) const;

		Disposable<Array> solve_splitting(Size direction, const Array& x, Real s) const;
		Disposable<Array> preconditioner(const Array& r, Real s) const;

	private:
		const boost::shared_ptr<FdmMesher> mesher_;
		const boost::shared_ptr<Generalized_HullWhite> hw0_;
		const boost::shared_ptr<Generalized_HullWhite> hw1_;
		const Handle<YieldTermStructure> discTS_;
		const Array x_;

		FdmHWRatesPart opR0_;
		FdmHWRatesPart opR1_;
		NinePointLinearOp corrMap_;
		const NinePointLinearOp corrMapTemplate_;

		boost::function<Real (Time)> vol0_, vol1_;
	};
}

