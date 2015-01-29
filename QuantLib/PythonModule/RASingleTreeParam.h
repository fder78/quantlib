#pragma once

#include "RADaishinParam.h"

class YieldCurveInfoWrapper;
class YieldCurveInfoWrapperProxy;

class RASingleTreeParam : public RADaishinParam
{
public:
	RASingleTreeParam();
	
	virtual void SetDataImpl( TiXmlElement* record ) override;
	virtual std::vector<std::pair<Date, Real> > GetCashFlow() const override;

private:
	virtual ResultInfo DoCalculation() override;
	
	void LoadCallScheduleInfo( const TiXmlElement* record );
	void LoadAccrualInfo( const TiXmlElement* record );

protected:
	Schedule m_callSchedule;
	std::vector<Real> m_callValues;

	Size m_pastAccrualCnt;
	std::vector<Size> m_volDataIdx;
};
