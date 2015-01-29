#pragma once

#include "IFunctor.h"

class RemoveCurveFunctor	: public IFunctor
{
public:
	virtual void Run( const TiXmlElement* param_root ) const;
};	