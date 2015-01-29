#pragma once

#include "IRProductParam.h"
#include "pricing_functions/cap_floor_swaption.hpp"

class YieldCurveInfoWrapper;
class YieldCurveInfoWrapperProxy;


class SwaptionParam : public IRProductParam
{
public:
	SwaptionParam();

private:
	virtual void SetDataImpl( TiXmlElement* record ) override;

	void LoadScheduleInfo( const TiXmlElement* record );
	void LoadLegInfo( const TiXmlElement* record );
	virtual ResultInfo DoCalculation() override;

	void LoadValuationInfo( const TiXmlElement* record );
	void LoadCurveInfo( const TiXmlElement* record );
	void LoadCallPutInfo( const TiXmlElement* record );
	
	void ApplyCurveInfo();

	virtual Real GetBiasForProduct() const { return m_biasForScenario; }

	virtual Period GetRemainingTime() const override;
	virtual std::vector<std::pair<Date, Real> > GetCashFlow() const override;

private:
	Real GetVolatility() const;

protected:

	Settlement::Type m_settlementType;
	Real m_strike;
	Date m_exerciseDate;
	int m_fixingDays;

	Real m_biasForScenario;
	boost::shared_ptr<IborIndex> m_lbortmp;
	boost::shared_ptr<YieldTermStructure> m_DCTermStructure;

	//bool m_isSwap;
	Date m_tradeDate;
	Date m_effectiveDate;
	Date m_terminationDate;
	Date m_recentFixDate;


	std::string m_strTraderID;
	std::string m_strCode;
	std::string m_strBook;
	std::string m_strBacktoBack;
	std::string m_strExtCalculator;
	std::string m_strCounterParty;
	VanillaSwap::Type m_side;

	Date m_evalDate;
	std::string m_strCurrency;

	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_discountCurveProxy;

	//Calendar Calendar;
	BusinessDayConvention m_BDCpayment;
	Calendar m_Calendar;
	Period m_fixedTenor;
	Period m_floatingTenor;

	BusinessDayConvention m_BDCschedule;
	
	boost::shared_ptr<Schedule> m_scheduleFloating;
	boost::shared_ptr<Schedule> m_scheduleFixed;
	Rate m_spreadFloating;
	Rate m_fixedRate;
	BusinessDayConvention m_BDCcoupon;

	Rate m_pastFixingFloating;

	DayCounter m_floatingDayCounter;
	DayCounter m_fixedDayCounter;
};