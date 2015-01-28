#pragma once

#include "IProductParam.h"

namespace QuantLib
{
	struct CDSPricingResult;
}

class CDSParam : public IProductParam
{
public:
	virtual void SetDataImpl( const TiXmlElement* record ) override;

	Date GetTodaysDate() const { return m_todaysDate; }
	std::pair<Real, Real> GetNominal() const { return m_nominal; }

	DayCounter GetDayCounter() const { return m_dayCounter; }
	Handle<YieldTermStructure> GetRiskFreeRate() const { return m_tsCurve_protection; }
	Handle<YieldTermStructure> GetDiscountCurve() const { return m_tsCurve_premium; }

	Real GetRecoveryRate() const { return m_recoveryRate; }
	Real GetCorrelaion() const { return m_correlation; }

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
	CDSPricingResult CalcRes();

private:
	std::string m_book;
	Date m_todaysDate; //
	std::pair<Real, Real> m_nominal; //
	Protection::Side m_side;
	Rate m_premium; //
	Rate m_upfront;
	DayCounter m_dayCounter; //

	Handle<YieldTermStructure> m_tsCurve_protection; //
	Handle<YieldTermStructure> m_tsCurve_premium;	//
	
	std::pair<Real, Real> m_fxSpot;

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