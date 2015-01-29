#pragma once

#include "IRProductParam.h"

class YieldCurveInfoWrapper;
class YieldCurveInfoWrapperProxy;

class FixedRateBondParam : public IRProductParam
{

public:
	FixedRateBondParam();

	enum Type { Buy = 1, Sell = -1 };


private:
	virtual void SetDataImpl( TiXmlElement* record ) override;
	void LoadScheduleInfo( const TiXmlElement* record );
	void LoadLegInfo( const TiXmlElement* record );
	virtual ResultInfo DoCalculation() override;
	void LoadValuationInfo( const TiXmlElement* record );
	void LoadCurveInfo( const TiXmlElement* record );

	virtual Real GetBiasForProduct() const override { return m_biasForScenario; }
	virtual Period GetRemainingTime() const override;
	virtual std::vector<std::pair<Date, Real> > GetCashFlow() const override;

protected:
	Real m_biasForScenario;
	boost::shared_ptr<YieldTermStructure> m_DCTermStructure;

	//bool m_isSwap;
	Date m_tradeDate;
	Date m_effectiveDate;
	Date m_terminationDate;
	Date m_recentFixDate;
	Date m_issueDate;

	std::string m_strTraderID;
	std::string m_strCode;
	std::string m_strBook;
	std::string m_strBacktoBack;
	std::string m_strExtCalculator;
	std::string m_strCounterParty;
	Type m_side;

	std::string m_strCurrency;

	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_DCCurveProxy;
	RelinkableHandle<YieldTermStructure> m_DCTermStructureHandle;

	//Calendar Calendar;
	Calendar m_Calendar;
	Period m_fixedTenor;

	BusinessDayConvention m_BDCschedule;
	
	boost::shared_ptr<Schedule> m_schedule;
	Rate m_fixedRate;

	DayCounter m_FixedDayCounter;
};