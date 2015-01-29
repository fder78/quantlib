#pragma once

#include "IProductParam.h"
#include <ds_interestrate_derivatives/instruments/swaps/power_spread_swap.hpp>

class YieldCurveInfoWrapper;
class InterestRateCurveInfoWrapper;
class YieldCurveInfoWrapperProxy;

class RemoteXMLJob;
class IRProductJob;

class PowerSpreadParam : public IProductParam
{


public:
	PowerSpreadParam();

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

	virtual Period GetRemainingTime() const;

protected:
	Real m_biasForScenario;

	bool m_isSwap;

	bool m_calcDelta;
	bool m_calcGamma;
	bool m_calcVega;
	bool m_calcXGamma;

	Date m_effectiveDate;
	Date m_terminationDate;


	std::string m_strTraderID;
	std::string m_strCode;
	std::string m_strBook;
	std::string m_strBacktoBack;
	std::string m_strExtCalculator;
	std::string m_strCounterParty;

	


	Date m_EvalDate;
	Real m_Nominal;
	PowerSpreadSwap::Side m_side;
	Rate m_spreadFloating;
	Real m_gearingFloating;

	boost::shared_ptr<Schedule> m_scheduleFloating;
	boost::shared_ptr<Schedule> m_scheduleStructured;
	DayCounter m_DayCounterStructured;
	BusinessDayConvention m_BDC;
	std::vector<Real> m_gearings;
	std::vector<Real> m_spreads;
	std::vector<Real> m_caps;
	std::vector<Real> m_floors;
	bool m_isAvg;
	Real m_pastFixingStructured;
	Rate m_pastFixingFloating;


	Date m_callStartDate;
	Date m_callEndDate;
	Period m_callTenor;
	//Callability::Type m_callPut;
	//Real callValue;


	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_DCCurveProxy;
	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_RefCurveLongProxy;
	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_RefCurveShortProxy;
	typedef std::map<std::wstring, boost::shared_ptr<InterestRateCurveInfoWrapper> > CurveMap;
	CurveMap m_curves;

	typedef std::map<std::wstring, boost::shared_ptr<YieldCurveInfoWrapperProxy> > YCurveMap;
	YCurveMap m_Ycurves;

	Real m_rhoLongShort;
	Real m_rhoLongDisc;
	Real m_rhoShortDisc;


	int m_numSimulation;
	int m_numCalibration;


	DayCounter m_DayCounterFloating;
	std::string m_strCurrency;


};