#ifndef callability_hpp
#define callability_hpp

#include <ql/instruments/callabilityschedule.hpp>
#include <ql/exercise.hpp>
#include <ql/methods/montecarlo/multipath.hpp>
#include <ql/indexes/interestrateindex.hpp>

namespace QuantLib {

	class CallSchedule {
	public:
		CallSchedule(const CallabilitySchedule& callabilitySchedule = CallabilitySchedule(),
			const Exercise::Type& type = Exercise::Bermudan) :
		  callabilitySchedule_(callabilitySchedule), exerciseType_(type) {}
	protected:
		const CallabilitySchedule& callabilitySchedule() {return callabilitySchedule_;}
		const Exercise::Type& exerciseType() {return exerciseType_;}
		CallabilitySchedule callabilitySchedule_;
		Exercise::Type exerciseType_;
	};

	class HybridCallability: public Callability {
	public:
		HybridCallability(
			const Price& price, 
			Type type, 
			const Date& date, 
			std::vector<Rate> strikes,
			std::vector<boost::shared_ptr<InterestRateIndex> > indexes=std::vector<boost::shared_ptr<InterestRateIndex> >(),
			bool isAND=false,
			bool isCall=true
			)
			: Callability(price, type, date), strikes_(strikes), indexes_(indexes), isAND_(isAND), isCall_(isCall)
		{

		}

		std::vector<boost::shared_ptr<InterestRateIndex> > indexes(){ return indexes_;}


		virtual bool isCallable(const MultiPath& multiPath, Size iTime, std::vector<Size> assetNumber=std::vector<Size>()){

			bool retBool(true);

			if(assetNumber.size()==0){
				for(Size i=0;i<multiPath.assetNumber();i++){
					retBool=retBool&&(multiPath[i][iTime]>=strikes_[i]);
				}
			}else{
				QL_REQUIRE(strikes_.size()==assetNumber.size(),"the size of assets to compare is not same as the size of strikes");

				for(Size i=0; i<assetNumber.size();i++){
					Size k=assetNumber[i];
					retBool=retBool&&(multiPath[k][iTime]>=strikes_[i]);
				}
			}		
			
			return retBool;
		}

		virtual bool isCallable(const std::vector<Rate> assets){

			bool retBool(true);

			bool isCallStriked=(isAND_)? true:false;
			bool isPutStriked =(isAND_)? true:false;
		

			for(Size i=0; i< assets.size();i++){	
				if(isAND_){
					isCallStriked=(assets[i]<strikes_[i])? false: isCallStriked;
					isPutStriked=(assets[i]>strikes_[i])? false: isPutStriked;
				}else{
					isCallStriked=(assets[i]>strikes_[i])? true: isCallStriked;
					isPutStriked=(assets[i]<strikes_[i])? true: isPutStriked;
				}
			}

			if(isCall_){
				return isCallStriked;
			}else{
				return isPutStriked;
			}

			
		}

		virtual bool isCallable(Real, Real, Real){
			return false;
		}
		
	protected:

		bool isCall_, isAND_, isCallStriked, isPutStriked; 
		std::vector<Rate> strikes_;
		std::vector<boost::shared_ptr<InterestRateIndex> > indexes_;
	};

	class RefixRate{
	public:
		RefixRate()
		{
			isRefixing_=false;
			refixSchedule_.resize(0);
		}

		RefixRate(const Real currentStrike)
			:currentStrike_(currentStrike)
		{
			isRefixing_=false;
			refixSchedule_.resize(0);
			
		}


		RefixRate(
			const Real nominal,
			const Real currentStrike,
			const Real floor,
			const std::vector<Date> refixDates,
			const Period refixingPeriod=Period(1, Months),
			const bool isMin=true
			)
			: nominal_(nominal), currentStrike_(currentStrike), floor_(floor), refixingPeriod_(refixingPeriod), isMin_(isMin)
		{
			QL_REQUIRE(refixDates.size()==2,"refixDates must have begin and end date");
			QL_REQUIRE(refixDates[1] > refixDates[0] , "refixDates must have begin and end date");

			QL_REQUIRE(currentStrike_>floor_,"current strike must greater than floor");

			isRefixing_=true;

			refixSchedule_.resize(0);

			for(Date date=refixDates[0]; date<=refixDates[1]; date=date+refixingPeriod){
				refixSchedule_.push_back(date);
			}
		}		

		bool isRefixing() const {
			return isRefixing_;
		}

		virtual std::vector<Real> refixingStrikes(TimeGrid timeGrid, Real iniStrike) const{
			std::vector<Real> retVals;
			retVals.resize(timeGrid.size());

			for(Size i=1;i<retVals.size();i++){
				retVals[i]=iniStrike;
			}
			return retVals;

		}

		virtual std::vector<Real> refixingStrikes(TimeGrid timeGrid, std::vector<Real> asset, DayCounter dayCounter) const{
			
			Real nominal=nominal_; 
			Period refixingPeriod=refixingPeriod_;

			std::vector<Real> retVals;
			retVals.resize(timeGrid.size());

			Time refixT=1.0/(Real)refixingPeriod.frequency();
			retVals[0]=currentStrike_;
			for(Size i=1;i<retVals.size();i++){
				retVals[i]=Null<Real>();
			}

			std::vector<Time> refixTime;			
			Date referenceDate=Settings::instance().evaluationDate();

			for(Size k=0; k <refixSchedule_.size(); k++){
				Time t=dayCounter.yearFraction(referenceDate,refixSchedule_[k]);
				if(t>0)
					refixTime.push_back(t);
			}

			Size KRX=0;

			if(!isRefixing_){
				for(Size iT=1;iT<refixTime.size(); iT){
					retVals[iT]=currentStrike_;
				}

				return retVals;
			}

			for(Size iT=1; iT < refixTime.size(); iT++){
				Time T=timeGrid.closestTime(std::max(refixTime[iT]-refixT,0.0));
				Size iTime=timeGrid.closestIndex(refixTime[iT]);
				Size iT0=timeGrid.closestIndex(T);

				for(Size h=KRX+1;h<iTime;h++){
					retVals[h]=retVals[h-1];
				}

				KRX=iTime;

				Real mean;

				if(iT0==iTime){
					mean=asset[iTime];
				}else{
					Real a(asset[iT0]), b(asset[iTime]);
					Real T1=timeGrid.at(iTime)-timeGrid.at(iT0);
			
					a=b-(refixT/T1)*(b-a);

					Real meanM =(a+b)/2.0;	//기간평균
					Real meanD=b;			//최근일

					if(isMin_){
						mean=std::min(meanM,meanD);
					}else{
						mean=std::max(meanM,meanD);
					}
				}
				retVals[KRX]=std::min(retVals[KRX-1],mean);
				retVals[KRX]=std::max(retVals[KRX], floor_);	
			}

			for(Size i=std::min(KRX+1,timeGrid.size());i<timeGrid.size();i++){
				retVals[i]=retVals[KRX];
			}
			
			return retVals;
		}

		Real nominal(){ return nominal_;}
		Real conversionRatio() { return nominal_/currentStrike_;}
		Real strike() {return currentStrike_;}
		Real floor(){ return floor_;}
		std::vector<Date> refixSchedule() { return refixSchedule_;}

	private:
		Real nominal_,currentStrike_, floor_;
		Period refixingPeriod_;
		bool isRefixing_, isMin_;		
		std::vector<Date> refixSchedule_;
	};

	class ConvertibleCallability: public Callability {
	public:
		ConvertibleCallability(
			const Price& price, 
			Type type, 
			const Date& date,
			const Rate& initialStrike,
			std::vector<boost::shared_ptr<InterestRateIndex> > indexes,
			const Real nominal=100
			)
			: Callability(price, type, date), indexes_(indexes), initialStrike_(initialStrike)
		{
			nominal_=nominal;
			isCall_=true;
		}	

		std::vector<boost::shared_ptr<InterestRateIndex> > indexes(){ return indexes_;}

	
		virtual bool isCallable(Real bond, Real stock, Real strike) //refixing
		{
			Real value(stock*nominal_/strike);

			if(isCall_){
				return (value>bond)? true: false;		
			}else{
				return (value>bond)? false: true;		
			}
		}

		virtual Real conversionValue(Real bond, Real stock, Real strike)
		{
			if(isCallable(bond, stock, strike)){
				return stock*(nominal_/strike);
			}else{
				return bond;
			}			
		}

		Real nominal() {return nominal_;}
		Real initialStrike() {return initialStrike_;}

	protected: 
		bool isCall_;
		Real initialStrike_, nominal_, assetLowerbound_; 
		std::vector<boost::shared_ptr<InterestRateIndex> > indexes_;
		RefixRate refixRate_;

	};
	
	class BWCallability: public ConvertibleCallability {
	public:
		BWCallability(
			const Price& price, 
			Type type, 
			const Date& date,
			const Rate& initialStrike,
			std::vector<boost::shared_ptr<InterestRateIndex> > indexes,
			const Real nominal=100
			)
			: ConvertibleCallability(price, type, date, initialStrike, indexes, nominal)
		{
			isCall_=true;
		}
	};

}

#endif