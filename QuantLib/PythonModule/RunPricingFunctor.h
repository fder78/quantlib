#pragma once

#include "IFunctor.h"

class RunPricingFunctor : public IFunctor
{
public:
	virtual void Run( const TiXmlElement* param_root ) const;
};

