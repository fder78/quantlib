#pragma once

class TiXmlElement;

class IFunctor
{
public:
	IFunctor() { }
	virtual ~IFunctor() { }

	virtual void Run( const TiXmlElement* param_root ) const = 0;
};
