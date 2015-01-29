#pragma once

#include "IFunctor.h"

class RegisterYTMFunctor : public IFunctor
{
public:
	virtual void Run( const TiXmlElement* param_root ) const;
};
