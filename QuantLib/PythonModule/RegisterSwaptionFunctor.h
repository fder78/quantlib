#pragma once

#include "IFunctor.h"

class RegisterSwaptionFunctor : public IFunctor
{
public:
	virtual void Run( const TiXmlElement* param_root ) const;
};
