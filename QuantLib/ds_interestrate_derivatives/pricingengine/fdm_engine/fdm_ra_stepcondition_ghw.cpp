
#include <ds_interestrate_derivatives/pricingengine/fdm_engine/fdm_ra_stepcondition_ghw.hpp>

#include <ql/math/distributions/normaldistribution.hpp>
#include <ql/math/distributions/bivariatenormaldistribution.hpp>
#include <ds_interestrate_derivatives/cashflows/range_accrual_coupon.hpp>
#include <ql/methods/finitedifferences/operators/fdmlinearoplayout.hpp>

namespace QuantLib {

	Fdm_R2Dual_RA_StepCondition::Fdm_R2Dual_RA_StepCondition(
		const boost::shared_ptr<FdmMesher> & mesher,
		const boost::shared_ptr<Generalized_HullWhite> hw0,
		const boost::shared_ptr<Generalized_HullWhite> hw1,	
		const Handle<YieldTermStructure>& discTS,
		const std::vector<Time> stoppingTimes,
		const Real rho,
		const Leg& cashflows,
		const std::vector<std::vector<Real > >& down,
		const std::vector<std::vector<Real > >& up,
		const Time firstT,
		const Real firstAccrual,
		const Time firstCall,
		const Real callPrice,
		const Real inverseAlpha,
		const Real inverseGearing,
		const Real inverseFixing,
		const Real cap,
		const Real floor,
		const Real alpha,
		const Real fixingRate,
		const std::vector<Matrix> m,
		const std::vector<Matrix> tenor)
		: mesher_(mesher),
		cpn_(mesher->layout()->size(), 0.0), f2_(rho), discTS_(discTS), inverseGearing_(inverseGearing), cap_(cap), floor_(floor), inverseFixing_(inverseFixing),
		firstTime_(firstCall), firstT_(firstT), pastAccrual_(firstAccrual), callPrice_(callPrice), up_(up), down_(down), rho_(rho), inverseAlpha_(inverseAlpha),
		hw0_(hw0), hw1_(hw1), cashflows_(cashflows), stoppingTimes_(stoppingTimes), alpha_(alpha), fixingRate_(fixingRate), m_(m), tenor_(tenor) {}

	void Fdm_R2Dual_RA_StepCondition::applyTo(Array& a, Time t) const {

		std::vector<Time>::const_iterator iter
			= std::find(stoppingTimes_.begin(), stoppingTimes_.end(), t);

		if ((iter != stoppingTimes_.end() || t<=1.0/365.0) && t>0.0) { 

			Size j = 0;
			for (j=0; j<stoppingTimes_.size(); ++j){
				if (t==stoppingTimes_[j])
					break;
			}

			Size i = j + cashflows_.size() - stoppingTimes_.size() -1;

			boost::shared_ptr<RangeAccrualCoupon> cpn;
			if (j>=stoppingTimes_.size())
				i = cashflows_.size() - stoppingTimes_.size() - 2;
			//	cpn = boost::dynamic_pointer_cast<FixedRateCoupon>(cashflows_[cashflows_.size() - stoppingTimes_.size() -2]);
			//else

			cpn = boost::dynamic_pointer_cast<RangeAccrualCoupon>(cashflows_[i]);
			Real deltaT = cpn->accrualPeriod();
			Real coupon = cpn->spread();
			Real gearing = cpn->gearing();

			if (t<=1.0/365.0)
				a += getCpn(gearing, coupon, i, t, firstT_, deltaT, pastAccrual_);
			else
				a += getCpn(gearing, coupon, i, t, deltaT, deltaT, 0.0);

			boost::shared_ptr<FdmLinearOpLayout> layout = mesher_->layout();
			const FdmLinearOpIterator endIter = layout->end();
			for (FdmLinearOpIterator iter = layout->begin(); iter != endIter; ++iter) {
				if (t>=firstTime_) {
					if (callPrice_ < a[iter.index()]) //call
					//Real s1 = std::exp(mesher_->location(iter, 0)); //stock
					//if (s1<=up_[i][0] && s1>=down_[i][0])
						a[iter.index()] = callPrice_;
				}
			}
		}
	}


	Array Fdm_R2Dual_RA_StepCondition::getCpn(Real gearing, Real coupon, Size i, Time t, Time firstT, Time deltaT, Real pastAccrual) const {

		boost::shared_ptr<FdmLinearOpLayout> layout = mesher_->layout();
		const FdmLinearOpIterator endIter = layout->end();
		Array a(layout->size(),0.0);
		Real a11, a12, a21, a22;
		
		Size numObs = m_[i].rows();//up_[i].size();
		a11 = m_[i][0][0]; a12 = m_[i][0][1];

		a21 = 0; a22 = 0;
		bool isRegular = false;

		Real upR0 = up_[i][0];
		Real dnR0 = down_[i][0];
		Real upR1 = QL_MAX_REAL, dnR1 = -1.0;

		if (numObs==2){
			upR1 = up_[i][1];
			dnR1 = down_[i][1];
			a21 = m_[i][1][0]; 
			a22 = m_[i][1][1];
			if (a11==1.0 && a12==0.0 && a21==0.0 && a22==1.0)
				isRegular = true;
			if (a21==0.0 && a22==0.0)
				numObs=1;
		}

		Real mean_s0, mean_s1, std_s0, std_s1, ub0, ub1, lb0, lb1, u0, u1, d0, d1;

		//인덱스 range -> short rate range로 변경
		Real A0 = hw0_->pA(t, t+deltaT);
		Real B0 = hw0_->pB(t, t+deltaT);
		Real A1 = hw1_->pA(t, t+deltaT);
		Real B1 = hw1_->pB(t, t+deltaT);
		ub0 = std::log((upR0 * deltaT + 1) * A0) / B0;
		lb0 = std::log((dnR0 * deltaT + 1) * A0) / B0;
		ub1 = std::log((upR1 * deltaT + 1) * A1) / B1;
		lb1 = std::log((dnR1 * deltaT + 1) * A1) / B1;

		for (FdmLinearOpIterator iter = layout->begin(); iter != endIter; ++iter) {

			Real r0 = mesher_->location(iter, 0); 
			Real r1 = mesher_->location(iter, 1); 

			Real nN=0.0, dt=0.0;

			Real mean_s0T = hw0_->expectation(t, t+firstT, r0);
			Real mean_s1T = hw1_->expectation(t, t+firstT, r1);

			Real std_s0T = hw0_->stdDeviation(t, t+firstT, r0);
			Real std_s1T = hw1_->stdDeviation(t, t+firstT, r1);
			
			Integer n = 8;
			if (isRegular) {
				u0 = (ub0-mean_s0T)/std_s0T;
				d0 = (lb0-mean_s0T)/std_s0T;
				u1 = (ub1-mean_s1T)/std_s1T;
				d1 = (lb1-mean_s1T)/std_s1T;
				Real p = (f2_(u1,u0)-f2_(u1,d0)-f2_(d1,u0)+f2_(d1,d0));
				Integer n = (p>0.01 && p<0.99) ? ((p>0.1 && p<0.9) ? 20 : 5) : 1;
			}			

			for (Integer i=0; i<n; ++i) {
				dt = firstT/n*(i+0.5);
				mean_s0 = r0 + dt*(mean_s0T - r0)/firstT;
				mean_s1 = r1 + dt*(mean_s1T - r1)/firstT;
				std_s0 = std_s0T * std::sqrt(dt/firstT);
				std_s1 = std_s1T * std::sqrt(dt/firstT);

				if (numObs==2 && isRegular) {
					u0 = (ub0-mean_s0)/std_s0;
					d0 = (lb0-mean_s0)/std_s0;
					u1 = (ub1-mean_s1)/std_s1;
					d1 = (lb1-mean_s1)/std_s1;
					nN += (f2_(u1,u0)-f2_(u1,d0)-f2_(d1,u0)+f2_(d1,d0))/n;
				}
				else if (numObs==2) {
					// 수정 필요
					Real m0 = a11*mean_s0 + a12*mean_s1;
					Real m1 = a21*mean_s0 + a22*mean_s1;
					Real s0 = std::sqrt( a11*a11*std_s0*std_s0 + a12*a12*std_s1*std_s1 +2.*rho_*a11*a12*std_s0*std_s1 );
					Real s1 = std::sqrt( a21*a21*std_s0*std_s0 + a22*a22*std_s1*std_s1 +2.*rho_*a21*a22*std_s0*std_s1 );
					Real cov = a11*a21*std_s0*std_s0 + a12*a22*std_s1*std_s1 + rho_*(a11*a22 + a21*a12)*std_s0*std_s1 ;
					Real rhoX = cov / s0 / s1;
					if (rhoX>=1.0)
						rhoX = 1.0 - QL_MIN_POSITIVE_REAL;
					if (rhoX<=-1.0)
						rhoX = -1.0 + QL_MIN_POSITIVE_REAL;
					BivariateCumulativeNormalDistributionWe04DP f2(rhoX);

					u0 = (ub0-m0)/s0;
					d0 = (lb0-m0)/s0;
					u1 = (ub1-m1)/s1;
					d1 = (lb1-m1)/s1;
					nN += (f2(u1,u0)-f2(u1,d0)-f2(d1,u0)+f2(d1,d0))/n;

				} 
				else { //spread single range
					Real ti = t+dt;
					Real A0 = hw0_->pA(ti, ti+deltaT);
					Real B0 = hw0_->pB(ti, ti+deltaT);
					Real P0 = A0 * std::exp(-B0*r0);
					Real d0 = 1.0/deltaT*(B0/A0*std::exp(B0*r0));
					mean_s0 = 1.0/deltaT*(1.0/A0 * std::exp(B0*r0) - 1.0);					

					if (tenor_[i][0][0]>0) {
						Real annuity = 0.0, dannuity = 0.0;
						for (Size k=0; k < (Size)tenor_[i][0][0] / deltaT; ++k) {
							A0 = hw0_->pA(ti, ti+(k+1)*deltaT);	
							B0 = hw0_->pB(ti, ti+(k+1)*deltaT);
							P0 = A0 * std::exp(-B0*r0);
							d0 = -B0 * P0;
							annuity += P0;
							dannuity += d0;
						}
						mean_s0 = (1.0 - P0) / (deltaT * annuity);
						d0 = A0*B0*std::exp(-B0*r0) / (deltaT * annuity) - (1.0 - P0) / deltaT / annuity / annuity * dannuity;
					}
					
					Real A1 = hw1_->pA(ti, ti+deltaT);	
					Real B1 = hw1_->pB(ti, ti+deltaT);
					Real P1 = A1 * std::exp(-B1*r1);
					Real d1 = 1.0/deltaT*(B1/A1*std::exp(B1*r1));
					mean_s1 = 1.0/deltaT*(1.0/A1 * std::exp(B1*r1) - 1.0);

					if (tenor_[i][0][1]>0) {
						Real annuity = 0.0, dannuity = 0.0;
						for (Size k=0; k < (Size)tenor_[i][0][1] / deltaT; ++k) {
							A1 = hw1_->pA(ti, ti+(k+1)*deltaT);	
							B1 = hw1_->pB(ti, ti+(k+1)*deltaT);
							P1 = A1 * std::exp(-B1*r1);
							d1 = -B1 * P1;
							annuity += P1;
							dannuity += d1;
						}
						mean_s1 = (1.0 - P1) / (deltaT * annuity);
						d1 = A1*B1*std::exp(-B1*r1) / (deltaT * annuity) - (1.0 - P1) / deltaT / annuity / annuity * dannuity;
					}

					Real m = a11*mean_s0 + a12*mean_s1;
					Real s = std::sqrt( a11*a11*d0*d1*std_s0*std_s0 + a12*a12*d0*d1*std_s1*std_s1 +2.*rho_*a11*a12*d0*d1*std_s0*std_s1 );
					s = (s<=0.0) ? QL_MIN_POSITIVE_REAL : s;
					u0 = (upR0 - m)/s;
					d0 = (dnR0 - m)/s;
					nN += (f_(u0)-f_(d0))/n;
				}
			}

			a[iter.index()] = (pastAccrual + nN * firstT / deltaT);
			Real discFactor = hw0_->discountBond(t, t+firstT, r0);
			Real rate;
			if (deltaT == firstT)
				rate = 1/deltaT * (1/discFactor-1);
			else
				rate = inverseFixing_;

			a[iter.index()] *= (coupon + gearing * rate) * deltaT * cashflows_.back()->amount();

			if (inverseGearing_!=Null<Real>() && inverseAlpha_!=Null<Real>()) {
				Real R = (coupon + gearing * rate);
				Real temp  = (R * (pastAccrual + nN * firstT / deltaT)  + inverseGearing_ * rate + inverseAlpha_);
				temp = (temp < floor_) ? floor_ : ((temp > cap_) ? cap_ : temp);
				a[iter.index()] = temp * deltaT * cashflows_.back()->amount();
			}

			if (alpha_!=Null<Real>()) {
				Real rate;
				if (deltaT == firstT)
					rate = 1/deltaT * (1/discFactor-1) + alpha_;
				else
					rate = fixingRate_ + alpha_;
				a[iter.index()]  -= deltaT * rate * cashflows_.back()->amount();
			}

			a[iter.index()] *= discFactor;
		}

		return a;

	}
}
