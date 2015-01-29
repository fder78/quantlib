#pragma once

#include "RAMCParam.h"

class RACMSParam : public RAMCParam
{
public:
	RACMSParam() { }

	virtual ResultInfo DoCalculation() override;
	virtual void SetDataImpl( TiXmlElement* record ) override;

private:
	Real m_fixingCMS;
};