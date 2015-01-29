#pragma once

#include "RADaishinParam.h"

class RAUsingProcessParam : public RADaishinParam
{
public:
	virtual void SetDataImpl( TiXmlElement* record ) override;

	virtual std::vector<std::pair<Date, Real> > GetCashFlow() const override;

protected:
	Date m_firstCallDate;
	Real m_pastAccrual;

	std::vector<Matrix> m_indexMatrix;
};