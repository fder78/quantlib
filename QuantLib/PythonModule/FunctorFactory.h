#pragma once

#include "PerProcessSingleton.h"

class IFunctor;

class FunctorFactory : public PerProcessSingleton<FunctorFactory>
{
public:
	FunctorFactory();

	boost::shared_ptr<IFunctor> GetFunctor( const std::string& name );

private:
	void RegisterFunctor( const std::string& name, boost::shared_ptr<IFunctor> functor );
	void BuildFunctor();

private:
	typedef std::map<std::string, boost::shared_ptr<IFunctor> > FunctorMap;
	FunctorMap m_functors;
};
