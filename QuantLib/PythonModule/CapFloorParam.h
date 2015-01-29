#pragma once

#include "IProductParam.h"

class YieldCurveInfoWrapper;
class YieldCurveInfoWrapperProxy;


class CapFloorParam : public IProductParam
{

public:
	CapFloorParam();

	virtual void Calculate() override;


private:

	virtual void SetDataImpl( TiXmlElement* record ) override;
	void LoadScheduleInfo( const TiXmlElement* record );
	void LoadLegInfo( const TiXmlElement* record );
	virtual std::vector<Real> DoCalculation();
	void LoadValuationInfo( const TiXmlElement* record );
	void LoadCurveInfo( const TiXmlElement* record );
	void LoadCallPutInfo( const TiXmlElement* record );


	void ApplyCurveInfo();

	virtual Real GetBiasForProduct() const { return m_biasForScenario; }


protected:
	//boost::shared_ptr<VanillaSwap> m_Swap;
	Real m_biasForScenario;
	boost::shared_ptr<IborIndex> m_FloatingRate;
	boost::shared_ptr<YieldTermStructure> m_DCTermStructure;
	
	Real m_strike;
	boost::shared_ptr<Schedule> m_schedule;
	int m_fixingDays;
	BusinessDayConvention m_convention;
	Volatility	m_volatility;


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
	CapFloor::Type m_capFloor;
	bool m_calcDelta;
	bool m_calcGamma;
	bool m_calcVega;

	Date m_EvalDate;
	Real m_Nominal;
	std::string m_strCurrency;

	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_DCCurveProxy;
	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_RefCurveProxy;
	RelinkableHandle<YieldTermStructure> m_DCTermStructureHandle;
	boost::shared_ptr<IborIndex> m_lbortmp;

	//Calendar Calendar;
	BusinessDayConvention m_BDCpayment;
	Calendar m_Calendar;
	Period m_fixedTenor;
	Period m_floatingTenor;

	BusinessDayConvention m_BDCschedule;


	Rate m_spreadFloating;
	Rate m_fixedRate;
	BusinessDayConvention m_BDCcoupon;

	Rate m_pastFixingFloating;


	typedef std::map<std::wstring, boost::shared_ptr<YieldCurveInfoWrapperProxy> > YCurveMap;
	YCurveMap m_Ycurves;

	DayCounter m_DayCounter;


};