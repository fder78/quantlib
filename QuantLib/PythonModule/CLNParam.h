#pragma once

#include "IProductParam.h"

namespace QuantLib
{
	struct CLNPricingResult;
}

class CLNParam : public IProductParam
{
public:
	virtual void SetDataImpl( TiXmlElement* record ) override;

	Date GetTodaysDate() const { return m_todaysDate; }
	std::pair<Real, Real> GetNominal() const { return m_nominal; }
	Rate GetCouponRate() const { return m_cpnRate; }

	DayCounter GetDayCounter() const { return m_dayCounter; }
	Handle<YieldTermStructure> GetRiskFreeRate() const { return m_tsCurve_rf; }
	Handle<YieldTermStructure> GetDiscountCurve() const { return m_tsCurve_disc; }

	std::vector<Date> GetFXFWDDate() const { return m_date_fxFwd; }
	std::vector<Real> GetFXFWDValue() const { return m_value_fxFwd; }
	Real GetFXVol() const { return m_fxVol; }

	Real GetRecoveryRate() const { return m_recoveryRate; }

	std::vector<std::vector<Period> > GetTenors() const { return m_tenors; }
	std::vector<std::vector<Real> > GetQuotedSpreads() const { return m_quoted_spreads; }

	Date GetEffectiveDate() const { return m_effectiveDate; }
	Date GetTerminationDate() const { return m_terminationDate; }

	Period GetTenor() const { return m_tenor; }
	Calendar GetCalendar() const { return m_calendar; }

	BusinessDayConvention GetConvention() const { return m_convention; }
	BusinessDayConvention GetTerminationDateContvention() const { return m_terminationDateConvention; }
	DateGeneration::Rule GetDateGenerationRule() const { return m_rule; }

	bool GetEOM() const { return m_endOfMonth; }
	Date GetFirstDate() const { return m_firstDate; }
	Date GetNextToLastDate() const { return m_nextToLastDate; }

	virtual void Calculate() override;

private:
	CLNPricingResult CalcRes();

private:
	std::string m_book;
	Date m_todaysDate; //
	std::pair<Real, Real> m_nominal; //
	Rate m_cpnRate; //
	DayCounter m_dayCounter; //

	Handle<YieldTermStructure> m_tsCurve_rf; //
	Handle<YieldTermStructure> m_tsCurve_disc;	//
	std::vector<Date> m_date_fxFwd; //
	std::vector<Real> m_value_fxFwd; //
	Real m_fxVol; //

	Real m_recoveryRate; //
	Real m_correlation; //
	std::vector<std::vector<Period> > m_tenors; //
	std::vector<std::vector<Real> > m_quoted_spreads; //

	Date m_effectiveDate; //
	Date m_terminationDate; //
	Period m_tenor; //
	Calendar m_calendar; //
	BusinessDayConvention m_convention; //
	BusinessDayConvention m_terminationDateConvention; //
	DateGeneration::Rule m_rule; //
	bool m_endOfMonth; //
	Date m_firstDate; //
	Date m_nextToLastDate; //
};