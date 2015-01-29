#pragma once

#include "RAUsingProcessParam.h"

class RAMCParam : public RAUsingProcessParam
{
public:
	virtual void SetDataImpl( TiXmlElement* record ) override;

private:
	virtual ResultInfo DoCalculation() override;

	virtual void FetchResult() override;

protected:
	int m_numSimul;

	Real m_error;
};