
#include <ds_interestrate_derivatives/pricingengine/tree_engine/discretized_callable_bond.hpp>
#include <ds_interestrate_derivatives/index/spread_index.hpp>

#include <ql/cashflows/coupon.hpp>
#include <ql/cashflows/simplecashflow.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/cashflows/floatingratecoupon.hpp>
#include <ql/cashflows/digitalcoupon.hpp>
#include <ql/cashflows/capflooredcoupon.hpp>

#include <ql/time/daycounter.hpp>
#include <ql/instruments/vanillaswap.hpp>
#include <ql/indexes/interestrateindex.hpp>
#include <ql/indexes/swapindex.hpp>
#include <ql/indexes/iborindex.hpp>

#include <ql/time/daycounter.hpp>
#include <ql/time/period.hpp>
#include <ql/index.hpp>
#include <ql/processes/hullwhiteprocess.hpp>
#include <ql/experimental/shortrate/generalizedhullwhite.hpp>

namespace QuantLib {

	DiscretizedCallableBond::DiscretizedCallableBond(
		const CallableCpnBond::arguments& args,
		const Date& referenceDate,
		const DayCounter& dayCounter,
		const boost::shared_ptr<ShortRateModel> model,
		boost::shared_ptr<YieldTermStructure> discCurve,
		Size pastAccruals)
		: arguments_(args),
		referenceDate_(referenceDate),
		dayCounter_(dayCounter), model_(model), discCurve_(discCurve),
		pastAccruals_(pastAccruals) {

			cashflows_ = arguments_.cashflows; 
			callabilitySchedule_ = arguments_.callabilitySchedule;
			exerciseType_ = arguments_.exerciseType;

			payTimes_.resize(cashflows_.size()-1);
			startTimes_.resize(cashflows_.size()-1);
			fixingTimes_.resize(cashflows_.size()-1);
			endTimes_.resize(cashflows_.size()-1);

			couponType_.resize(cashflows_.size()-1);
			isInArrears_.resize(cashflows_.size()-1);
			callabilityTimes_.resize(callabilitySchedule_.size());

			std::vector<Date> payDate;
			payDate.resize(cashflows_.size()-1);

			for(Size i=0; i<payTimes_.size();i++){
				payTimes_[i] = dayCounter_.yearFraction(referenceDate_, Date(cashflows_[i]->date().serialNumber(), 0));
				payDate[i] = cashflows_[i]->date();

				boost::shared_ptr<Coupon> coupon 
					= boost::dynamic_pointer_cast<Coupon>(cashflows_[i]);
				startTimes_[i] = dayCounter_.yearFraction(referenceDate_, Date(coupon->accrualStartDate().serialNumber(), 0));
				endTimes_[i] = dayCounter_.yearFraction(referenceDate_, Date(coupon->accrualEndDate().serialNumber(), 0));

				if(boost::dynamic_pointer_cast<FixedRateCoupon>(coupon) ){
					couponType_[i] = Fixed;
					isInArrears_[i]=false;
				}
				else if(boost::dynamic_pointer_cast<DigitalCoupon>(coupon) ){
					boost::shared_ptr<DigitalCoupon> dgtCpn = boost::dynamic_pointer_cast<DigitalCoupon>(coupon);
					couponType_[i] = Digital;
					isInArrears_[i] = dgtCpn->isInArrears();
					fixingTimes_[i] = dayCounter_.yearFraction(referenceDate_, Date(dgtCpn->fixingDate().serialNumber(), 0));

				}
				else if(boost::dynamic_pointer_cast<RangeAccrualCoupon>(coupon) ){
					boost::shared_ptr<RangeAccrualCoupon> rngCpn = boost::dynamic_pointer_cast<RangeAccrualCoupon>(coupon);
					couponType_[i] = RangeAccrual;
					isInArrears_[i] = false;
					fixingTimes_[i] = startTimes_[i];
				}
				else if(boost::dynamic_pointer_cast<FloatingRateCoupon>(coupon) ){
					boost::shared_ptr<FloatingRateCoupon> flCpn = boost::dynamic_pointer_cast<FloatingRateCoupon>(coupon);
					couponType_[i] = Floating;
					isInArrears_[i] = flCpn->isInArrears();
					fixingTimes_[i] = dayCounter_.yearFraction(referenceDate_, Date(flCpn->fixingDate().serialNumber(), 0));
				}
				else 
					QL_FAIL("Undefined Coupon Type");
			}

			for (Size i=0; i<callabilityTimes_.size(); i++)
				callabilityTimes_[i] = dayCounter_.yearFraction(referenceDate_, Date(callabilitySchedule_[i]->date().serialNumber(), 0));
	}


	std::vector<Time> DiscretizedCallableBond::mandatoryTimes() const {

		std::vector<Time> times, temp;
		temp.insert(temp.end(), startTimes_.begin(), startTimes_.end());
		temp.insert(temp.end(), endTimes_.begin(), endTimes_.end());
		temp.insert(temp.end(), payTimes_.begin(), payTimes_.end());
		//temp.insert(temp.end(), fixingTimes_.begin(), fixingTimes_.end());
		temp.insert(temp.end(), callabilityTimes_.begin(), callabilityTimes_.end());
		for (Size i=0; i<temp.size(); ++i)
			if (temp[i]>=0.0) times.push_back(temp[i]);
		return times;

	}


	void DiscretizedCallableBond::reset(Size size) {
		Real nominal_=cashflows_.back()->amount();
		if (arguments_.alpha!=Null<Real>())
			nominal_ = 0.0;
		values_ = Array(size, nominal_);
	}

	void DiscretizedCallableBond::preAdjustValuesImpl() {
		TimeGrid grid = method()->timeGrid();
		Time t0 = grid[grid.index(time())];
		Time t1 = grid[grid.index(time())+1];

		boost::shared_ptr<Generalized_HullWhite> hw = boost::dynamic_pointer_cast<Generalized_HullWhite>(model_);
		Handle<YieldTermStructure> ts = hw->termStructure();		
		Real creditDiscount = discCurve_->discount(t1) / discCurve_->discount(t0) * ts->discount(t0) / ts->discount(t1);
		values_ *= creditDiscount;

		for (Size i=0; i<payTimes_.size(); i++) {

			Time t = payTimes_[i];
			Time fix = fixingTimes_[i];

			if(fix>0.0 && t>0.0 && isOnTime(startTimes_[i])){

				DiscretizedDiscountBond bond;
				bond.initialize(method(), payTimes_[i]);
				bond.rollback(time_);

				boost::shared_ptr<FloatingRateCoupon> rtmp = boost::dynamic_pointer_cast<FloatingRateCoupon> (cashflows_[i]);
				boost::shared_ptr<InterestRateIndex> index_ = rtmp->index();

				Time tau = rtmp->accrualPeriod();
				Real nominal_ = rtmp->nominal();

				Date resetDate = isInArrears_[i]? rtmp->accrualEndDate(): rtmp->accrualStartDate();

				boost::shared_ptr<Generalized_HullWhite> hwmodel_ = boost::dynamic_pointer_cast<Generalized_HullWhite>(model_);

				std::vector<Rate> lowerTrigger_;
				std::vector<Rate> upperTrigger_;
				boost::shared_ptr<HullWhiteProcess> process;

				if(couponType_[i] == RangeAccrual){

					boost::shared_ptr<RangeAccrualCoupon> raCpn
						= boost::dynamic_pointer_cast<RangeAccrualCoupon>(rtmp);

					lowerTrigger_ = raCpn->lowerTrigger();  
					upperTrigger_ = raCpn->upperTrigger();

					Handle<YieldTermStructure> curve = hwmodel_->termStructure();
					Array parameters = hwmodel_->params();
					//Real a0 = parameters[0], sigma = parameters[1];
					Real a0 = hwmodel_->speed()(t),  sigma = hwmodel_->vol()(t);
					process.reset(new HullWhiteProcess(curve, a0, sigma));

					std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex_ = raCpn->obsIndex();

					for (Size m =0; m <obsIndex_.size(); m++){
						lowerTrigger_[m] = (lowerTrigger_[m] == Null<Rate>())? QL_MIN_REAL: lowerTrigger_[m];
						upperTrigger_[m] = (upperTrigger_[m] == Null<Rate>())? QL_MAX_REAL: upperTrigger_[m];
					}
				}

				/*short rate range 계산*/
				Rate cpnRate;
				Size k = grid.index(time());

				for (Size j=0; j<values_.size(); j++) {
					Real x = (this->method()->grid(t))[j];
					boost::shared_ptr<OneFactorModel::ShortRateTree> tree = 
						boost::dynamic_pointer_cast<OneFactorModel::ShortRateTree>(this->method());
					Real u = tree->underlying(k, j);
					Real shrt = hwmodel_->dynamics()->shortRate(time_, u);
					
					//Real shrt1 = - std::log(tree->discount(k,j)) / tree->timeGrid().dt(k);
					
					if (boost::dynamic_pointer_cast<SwapIndex>(index_)) 
						cpnRate = cmsRate(resetDate, shrt, index_); //cms rate
					else if (boost::dynamic_pointer_cast<IborIndex>(index_))
						cpnRate = iborRate(resetDate, shrt, index_); //ibor rate, CD rate
					else if (boost::dynamic_pointer_cast<SpreadIndex>(index_)){ //resetDate, fixingDate 모두 필요???
						cpnRate =  spreadRate(resetDate, shrt, index_);
						//cpnRate =  spreadRate(resetDate, shrt, index_);
					}
					else 
						QL_FAIL("Index Identification Fail");

					if(couponType_[i]== Floating)
						cpnRate = GetCpnFloating(rtmp,cpnRate);

					else if (couponType_[i]== Digital)
						cpnRate = GetCpnDigital(rtmp,cpnRate);

					else if(couponType_[i]== RangeAccrual){

						boost::shared_ptr<RangeAccrualCoupon> raCpn	= boost::dynamic_pointer_cast<RangeAccrualCoupon>(rtmp);

						Time dT= dayCounter_.yearFraction(resetDate,raCpn->observationDates().back());
						Real shrtMean = hwmodel_->expectation(time_, time_+dT, shrt);
						std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex_ = raCpn->obsIndex();
						Size obsIndexSize = obsIndex_.size();

						Size obsNum = raCpn->observationsNo();

						Date lastObsDate = raCpn->observationDates()[obsNum-1];
						std::vector<Rate> obsRate, obsRateExp, dIndexdr;
						obsRate.resize(obsIndexSize);
						obsRateExp.resize(obsIndexSize);
						dIndexdr.resize(obsIndexSize);

						Real delta = 0.001;
						//obs Index range 계산//
						for (Size m =0; m <obsIndexSize; m++){
							if(boost::dynamic_pointer_cast<SwapIndex>(obsIndex_[m])){
								obsRate[m] = cmsRate(resetDate, shrt, obsIndex_[m]);
								obsRateExp[m] = cmsRate(lastObsDate, shrtMean, obsIndex_[m]);
								dIndexdr[m] = std::abs((cmsRate(resetDate, shrt + delta, obsIndex_[m]) - cmsRate(resetDate, shrt - delta, obsIndex_[m]))/(2*delta));
							}
							else if(boost::dynamic_pointer_cast<IborIndex>(obsIndex_[m])){
								obsRate[m] = iborRate(resetDate, shrt, obsIndex_[m]);
								obsRateExp[m] = iborRate(lastObsDate,shrtMean, obsIndex_[m]);
								dIndexdr[m] = std::abs((iborRate(resetDate, shrt + delta, obsIndex_[m])-iborRate(resetDate, shrt - delta, obsIndex_[m]))/(2*delta));
							}
							else if(boost::dynamic_pointer_cast<SpreadIndex>(obsIndex_[m])){
								obsRate[m] = spreadRate(resetDate, shrt, obsIndex_[m]);
								obsRateExp[m] = spreadRate(lastObsDate,shrtMean, obsIndex_[m]);
								dIndexdr[m] = std::abs((spreadRate(resetDate, shrt + delta, obsIndex_[m])-spreadRate(resetDate, shrt - delta, obsIndex_[m]))/(2*delta));
							}
						}

						Real num=0;
						std::vector<Real> du, dd, mean, stdDev;

						du.resize(obsIndexSize);
						dd.resize(obsIndexSize);
						mean.resize(obsIndexSize);
						stdDev.resize(obsIndexSize);

						for (Size k=0; k<obsNum; k++) {
							Time dt = dayCounter_.yearFraction(resetDate, raCpn->observationDates()[k]);
							Date obsDate = raCpn->observationDates()[k];
							Real shrtStd = hwmodel_->stdDeviation(time_, time_+dt, shrt); ////fix????????????


							for (Size m =0; m <obsIndexSize; m++){
								mean[m] = obsRate[m] + (obsRateExp[m] -obsRate[m]) * dt/dT;
								stdDev[m] = shrtStd * dIndexdr[m] /* std::sqrt(dt/dT)*/;
								du[m] = (upperTrigger_[m]-mean[m])/stdDev[m];  //trigger가 null이면 infinite로 나와서 upProb 계산 안됨
								dd[m] = (lowerTrigger_[m]-mean[m])/stdDev[m];
							}

							Real upProb=0.0, downProb=0.0;
							if(obsIndexSize==2) {
								upProb = f_(std::min(du[0],du[1]));
								downProb = f_(std::max(dd[0],dd[1]));
							}
							else{
								upProb = f_(du[0]);
								downProb = f_(dd[0]);
							}
							num += upProb - downProb;
						}
						cpnRate = (raCpn->gearing() * cpnRate + raCpn->spread()) * num/obsNum ;
						
						if (arguments_.inverseGearing!=Null<Real>()) {
							Real rate = (1.0 / bond.values()[j] -1) / tau;
							cpnRate = cpnRate + arguments_.inverseGearing * rate;
							cpnRate = (cpnRate>arguments_.cap) ? arguments_.cap : (cpnRate<arguments_.floor) ? arguments_.floor : cpnRate;
						}
					}

					Real floating = 0.0;
					if (arguments_.alpha!=Null<Real>())
						floating = (1.0 / bond.values()[j] -1) / tau + arguments_.alpha;

					values_[j] += nominal_ * (cpnRate - floating) * tau * bond.values()[j];

				}
			}			
		}

		switch(exerciseType_){
		case Exercise::American:
			if(time_>=callabilityTimes_[0]) {
				Size i;
				for (i=0; i<cashflows_.size()-1; ++i) {
					if (time_<payTimes_[i])
						break;
				}
				Real t1 = payTimes_[i];
				Real t0 = (i>0) ? payTimes_[i-1] : 0.0;
				Real ai = cashflows_[i]->amount() * (time_-t0)/(t1-t0);
				applyCallability(0, ai);
			}
			break;

		case Exercise::Bermudan:
		case Exercise::European:
			for (Size i=0; i<callabilityTimes_.size(); i++) {
				Time t = callabilityTimes_[i];
				if (t > 0.0 && isOnTime(t)) {
					applyCallability(i, 0.0);
				}
			}
			break;
		}

	}

	void DiscretizedCallableBond::postAdjustValuesImpl() { 

		//Real addAmount(0.0);

		for (Size i=0; i<payTimes_.size(); i++) {
			if( couponType_[i] != Fixed /*&& !isInArrears_[i]*/){
				Time t = payTimes_[i];
				Time start = startTimes_[i];
				Time fix = fixingTimes_[i];

				if (t>0.0 && start<=0.0 && time_==0){

					DiscretizedDiscountBond bond;
					bond.initialize(method(), t);
					bond.rollback(0);

					boost::shared_ptr<FloatingRateCoupon> rtmp 
						= boost::dynamic_pointer_cast<FloatingRateCoupon>(cashflows_[i]);

					Real tau = rtmp->accrualPeriod();
					Real nominal_ = rtmp->nominal();
					//Rate r = rtmp->indexFixing();
					Rate r = 0.03;

					if(couponType_[i]== Floating)
						r = GetCpnFloating(rtmp,r);

					else if (couponType_[i]== Digital)
						r = GetCpnDigital(rtmp,r);

					else if(couponType_[i]== RangeAccrual){
						boost::shared_ptr<Generalized_HullWhite> hwmodel_ = 
							boost::dynamic_pointer_cast<Generalized_HullWhite>(model_);

						boost::shared_ptr<RangeAccrualCoupon> raCpn 
							= boost::dynamic_pointer_cast<RangeAccrualCoupon>(rtmp);

						std::vector<Real> lowerTrigger_ = raCpn->lowerTrigger();  
						std::vector<Real> upperTrigger_ = raCpn->upperTrigger();

						std::vector<boost::shared_ptr<InterestRateIndex> > obsIndex_ = raCpn->obsIndex();
						Size obsIndexSize = obsIndex_.size();

						for (Size m =0; m <obsIndexSize; m++){
							lowerTrigger_[m] = (lowerTrigger_[m] == Null<Rate>())? QL_MIN_REAL: lowerTrigger_[m];
							upperTrigger_[m] = (upperTrigger_[m] == Null<Rate>())? QL_MAX_REAL: upperTrigger_[m];
						}

						Size obsNum = raCpn->observationsNo();
						Real num =0;
						
						Handle<YieldTermStructure> curve = hwmodel_->termStructure();
						Array parameters = hwmodel_->params();
						//Real a0 = parameters[0], sigma = parameters[1];
						Real a0 = hwmodel_->speed()(t),  sigma = hwmodel_->vol()(t);
						boost::shared_ptr<HullWhiteProcess> process(new HullWhiteProcess(curve, a0, sigma));
						
						Real x = (this->method()->grid(0))[0];
						boost::shared_ptr<OneFactorModel::ShortRateTree> tree = 
							boost::dynamic_pointer_cast<OneFactorModel::ShortRateTree>(this->method());

						Real r0 = hwmodel_->dynamics()->shortRate(0,x); 
						//Real r0 = - std::log(tree->discount(0,0)) / tree->timeGrid().dt(0);

						Size pastDays=0;

						std::vector<Date> obsDate = raCpn->observationDates();

						Time dT = dayCounter_.yearFraction(referenceDate_,raCpn->observationDates().back());
						if(dT>0){

							Real shrtMean = hwmodel_->expectation(0, dT, r0);
							Date lastObsDate = raCpn->observationDates()[obsNum-1];
							std::vector<Rate> obsRate, obsRateExp, dIndexdr;
							obsRate.resize(obsIndexSize);
							obsRateExp.resize(obsIndexSize);
							dIndexdr.resize(obsIndexSize);

							Real delta = 0.001;

							for (Size m =0; m <obsIndexSize; m++){
								if(boost::dynamic_pointer_cast<SwapIndex>(obsIndex_[m])){
									obsRate[m] = cmsRate(referenceDate_, r0, obsIndex_[m]);
									obsRateExp[m] = cmsRate(lastObsDate, shrtMean, obsIndex_[m]);
									dIndexdr[m] = std::abs((cmsRate(referenceDate_, r0 + delta, obsIndex_[m]) - cmsRate(referenceDate_, r0 - delta, obsIndex_[m]))/(2*delta));
								}
								else if(boost::dynamic_pointer_cast<IborIndex>(obsIndex_[m])){
									obsRate[m] = iborRate(referenceDate_, r0, obsIndex_[m]);
									obsRateExp[m] = iborRate(lastObsDate,shrtMean,obsIndex_[m]);
									dIndexdr[m] = std::abs((iborRate(referenceDate_, r0 + delta, obsIndex_[m])-iborRate(referenceDate_, r0 - delta, obsIndex_[m]))/(2*delta));
								}
								else if(boost::dynamic_pointer_cast<SpreadIndex>(obsIndex_[m])){
									obsRate[m] = spreadRate(referenceDate_, r0, obsIndex_[m]);
									obsRateExp[m] = spreadRate(lastObsDate,shrtMean, obsIndex_[m]);
									dIndexdr[m] = std::abs((spreadRate(referenceDate_, r0 + delta,obsIndex_[m])-spreadRate(referenceDate_, r0 - delta, obsIndex_[m]))/(2*delta));
								}
							}

							std::vector<Real> du, dd, mean, stdDev;

							du.resize(obsIndexSize);
							dd.resize(obsIndexSize);
							mean.resize(obsIndexSize);
							stdDev.resize(obsIndexSize);

							for(Size k=0; k<obsNum; k++){
								Time obsTime = dayCounter_.yearFraction(referenceDate_, raCpn->observationDates()[k]);

								if (obsTime>0){
									Time dt = obsTime; 
									Real shrtStd = hwmodel_->stdDeviation(0, dt, r0);

									//range 계산//
									for (Size m =0; m <obsIndex_.size(); m++){
										mean[m] = obsRate[m] + (obsRateExp[m] -obsRate[m]) * dt/dT;
										stdDev[m] = shrtStd * dIndexdr[m] /*std::sqrt(dt/dT)*/;
										du[m] = (upperTrigger_[m]-mean[m])/stdDev[m];
										dd[m] = (lowerTrigger_[m]-mean[m])/stdDev[m];
									}
									Real upProb=0.0, downProb=0.0;
									if(obsIndexSize==2) {
										upProb = f_(std::min(du[0],du[1]));
										downProb = f_(std::max(dd[0],dd[1]));
									}
									else{
										upProb = f_(du[0]);
										downProb = f_(dd[0]);
									}
									num += upProb - downProb;
								}
								else {
									pastDays++;
								}
							}

							//QL_REQUIRE(pastAccruals_ <= pastDays, "Past Counting is larger than number of observation days passed!" );
						}
						num = num + pastAccruals_;
						Real fixed = raCpn->spread();
						r = (raCpn->gearing() * r + fixed) *num/obsNum;
						if (arguments_.inverseGearing!=Null<Real>()) {
							r = r + arguments_.inverseGearing * arguments_.inverseFixing;
							r = (r>arguments_.cap) ? arguments_.cap : (r<arguments_.floor) ? arguments_.floor : r;
						}
					}
					
					Real floating = 0.0;
					if (arguments_.alpha!=Null<Real>())
						floating = arguments_.fixingRate + arguments_.alpha;

					values_ += (r-floating) * tau * nominal_ * bond.values()[0];

				}
			}
		}
	}


	Rate DiscretizedCallableBond::iborRate(Date resetDate, Rate r, boost::shared_ptr<InterestRateIndex> index) const {

		boost::shared_ptr<Generalized_HullWhite> hwmodel_ = 
			boost::dynamic_pointer_cast<Generalized_HullWhite>(model_); 

		Time resetTime= dayCounter_.yearFraction(referenceDate_, resetDate);

		Time tenor = (index->dayCounter()).yearFraction(resetDate, resetDate+index->tenor());	
		Time tenor1 = dayCounter_.yearFraction(resetDate, resetDate+index->tenor());

		Real disc = hwmodel_->discountBond(resetTime, resetTime+tenor1, r);

		return (1/disc-1)/tenor1;	
	}

	Rate DiscretizedCallableBond::cmsRate(Date resetDate, Rate r, boost::shared_ptr<InterestRateIndex> index) const { //index를 보내줘야 함.

		boost::shared_ptr<HullWhite> hwmodel_ = 
			boost::dynamic_pointer_cast<HullWhite>(model_);

		boost::shared_ptr<SwapIndex> swapindex = boost::dynamic_pointer_cast<SwapIndex>(index);

		Time tau = (index->dayCounter()).yearFraction(resetDate, resetDate + swapindex->fixedLegTenor());
		Time Tenor =(index->dayCounter()).yearFraction(resetDate, resetDate + swapindex->tenor());

		Size size = (Size)(Tenor/tau + 0.4999);
		Time resetTime= dayCounter_.yearFraction(referenceDate_, resetDate); 

		Real denominator = 0;

		for (Size i=1;i<=size;i++){
			denominator += tau*hwmodel_->discountBond(resetTime,resetTime+i*tau,r);
		}

		Real numerator = 1 - hwmodel_->discountBond(resetTime,resetTime+size*tau,r);

		Rate cmsrate = numerator/denominator;
		return cmsrate;
	}

	Rate DiscretizedCallableBond::spreadRate(Date resetDate, Rate r, boost::shared_ptr<InterestRateIndex> index) const {

		boost::shared_ptr<SpreadIndex> spreadIndex = boost::dynamic_pointer_cast<SpreadIndex>(index);

		std::vector<boost::shared_ptr<InterestRateIndex> > spIndex;
		spIndex.push_back(spreadIndex->index1());
		spIndex.push_back(spreadIndex->index2());

		std::vector<Rate> rate;

		for(Size k=0; k<spIndex.size(); k++) {
			Rate rtemp;
			if (boost::dynamic_pointer_cast<SwapIndex>(spIndex[k]))
				rtemp = cmsRate(resetDate, r, spIndex[k]); 
			else if (boost::dynamic_pointer_cast<IborIndex>(spIndex[k]))
				rtemp = iborRate(resetDate, r, spIndex[k]);
			else QL_FAIL("Index Identification Fail");
			rate.push_back(rtemp);
		}
		return rate[0] - rate[1];
	}


	void DiscretizedCallableBond::applyCallability(Size i, Real ai) {
		switch (callabilitySchedule_[i]->type() ) {
		case Callability::Call:
			for (Size j=0; j<values_.size(); j++) {
				values_[j] =
					std::min(callabilitySchedule_[i]->price().amount() + ai, values_[j]);
			}
			break;
		case Callability::Put:
			for (Size j=0; j<values_.size(); j++) {
				values_[j] = 
					std::max(values_[j], callabilitySchedule_[i]->price().amount() + ai);
			}
			break;
		default:
			QL_FAIL("unknown callability type");
		}
	}

	Rate DiscretizedCallableBond::GetCpnFloating(boost::shared_ptr<FloatingRateCoupon> rtmp,Rate rate){

		Real gearing = rtmp->gearing();
		Real spread = rtmp->spread();
		Real cap, floor;

		boost::shared_ptr<CappedFlooredCoupon> cpnCFTemp
			= boost::dynamic_pointer_cast<CappedFlooredCoupon>(rtmp);
		if (cpnCFTemp){
			cap = cpnCFTemp->cap();
			floor = cpnCFTemp->floor();
		}
		else{
			cap = floor = Null<Rate>();
		}
		cap = (cap==Null<Rate>()) ? QL_MAX_REAL : cap;
		floor = (floor==Null<Rate>()) ? QL_MIN_REAL : floor;

		return std::max(std::min(gearing * rate + spread, cap), floor);
	}

	Rate DiscretizedCallableBond::GetCpnDigital(boost::shared_ptr<FloatingRateCoupon> rtmp,Rate rate){

		Real gearing = rtmp->gearing();
		Real spread = rtmp->spread();
		Real Kc, Kp;

		boost::shared_ptr<DigitalCoupon> (dgtCpn)
			= boost::dynamic_pointer_cast<DigitalCoupon>(rtmp);
		if (dgtCpn){
			Kc = dgtCpn->callStrike();
			Kp = dgtCpn->putStrike();
		}
		else{
			Kc = Kp = Null<Rate>(); //Null일 경우 floating 과 같지 않음!!
		}

		Rate indexA=0.0, indexB=0.0;

		Real ccsi= dgtCpn->isLongCall()? 1 : -1 ;
		Real pcsi= dgtCpn->isLongPut() ? 1 : -1 ;

		if(dgtCpn->hasCall()){
			if(rate > dgtCpn->callStrike()) {
				if(dgtCpn->callDigitalPayoff() != Null<Rate>())
					indexA = ccsi * dgtCpn->callDigitalPayoff();
				else 
					indexA =ccsi * rate ;
			}
		}

		if(dgtCpn->hasPut()) {
			if(dgtCpn->putStrike()> rate) {
				if(dgtCpn->putDigitalPayoff() != Null<Rate>())
					indexB = pcsi * dgtCpn->putDigitalPayoff();
				else 
					indexB = pcsi * rate ;
			}
		}
		return gearing *(indexA + indexB) + spread;

	}
}