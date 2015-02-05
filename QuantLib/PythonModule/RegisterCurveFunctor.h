#pragma once

#include "IFunctor.h"

class RegisterCurveFunctor : public IFunctor
{
public:
	virtual void Run( const TiXmlElement* param_root ) const;
};


class BuildCurveFunctor : public IFunctor
{
public:
	virtual void Run( const TiXmlElement* param_root ) const;
};
