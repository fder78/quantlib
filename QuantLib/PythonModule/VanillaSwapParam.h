#pragma once

#include "IRProductParam.h"

class YieldCurveInfoWrapper;
class YieldCurveInfoWrapperProxy;

#include "EnumParser.h"
class VanillaSwapSideParser : public EnumParser<VanillaSwapSideParser, VanillaSwap::Type >
{
public:
	void BuildEnumMap();;
};


class VanillaSwapParam : public IRProductParam
{
public:
	VanillaSwapParam();

private:
	virtual void SetDataImpl( TiXmlElement* record ) override;
	void LoadScheduleInfo( const TiXmlElement* record );
	void LoadLegInfo( const TiXmlElement* record );
	virtual ResultInfo DoCalculation() override;

	boost::shared_ptr<VanillaSwap> CreateInstrument() const;

	void LoadValuationInfo( const TiXmlElement* record );
	void LoadCurveInfo( const TiXmlElement* record );
	void LoadCallPutInfo( const TiXmlElement* record );
	
	virtual Real GetBiasForProduct() const { return m_biasForScenario; }
	virtual Period GetRemainingTime() const;
	virtual std::vector<std::pair<Date, Real> > GetCashFlow() const;

protected:
	Real m_biasForScenario;

	//bool m_isSwap;
	Date m_tradeDate;
	Date m_effectiveDate;
	Date m_terminationDate;
	Date m_recentFixDate;

	bool m_isOnPaymentDay;
	Real m_payment;

	std::string m_strTraderID;
	std::string m_strCode;
	std::string m_strBook;
	std::string m_strBacktoBack;
	std::string m_strExtCalculator;
	std::string m_strCounterParty;
	VanillaSwap::Type m_side;

	Date m_EvalDate;
	std::string m_strCurrency;

	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_DCCurveProxy;
	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_RefCurveProxy;
	boost::shared_ptr<IborIndex> m_lbortmp;

	//Calendar Calendar;
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

	DayCounter m_FloatingDayCounter;
	DayCounter m_FixedDayCounter;
};
