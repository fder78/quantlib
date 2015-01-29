#pragma once

#include "IFunctor.h"

class RegisterCapVolFunctor : public IFunctor
{
	virtual void Run( const TiXmlElement* param_root ) const;
};