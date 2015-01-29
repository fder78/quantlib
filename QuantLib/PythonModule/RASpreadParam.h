#pragma once

#include "RAFDMParam.h"

class RASpreadParam : public RAFDMParam
{
public:
	RASpreadParam() { }

	virtual void SetDataImpl( TiXmlElement* record ) override;
};