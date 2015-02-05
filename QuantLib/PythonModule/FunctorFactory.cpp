#include "StdAfx.h"

#include "FunctorFactory.h"

#include "RunPricingFunctor.h"
#include "RegisterYTMFunctor.h"
#include "RegisterSwaptionFunctor.h"
#include "RegisterCapVolFunctor.h"
#include "RemoveCurveFunctor.h"
#include "RegisterCurveFunctor.h"

boost::shared_ptr<IFunctor> FunctorFactory::GetFunctor( const std::string& name )
{
	FunctorMap::const_iterator iter = m_functors.find( name );
	if( iter == m_functors.end() )
	{
		QL_ASSERT( false, "등록되지 않은 함수(" + name + "을/를 호출했습니다!" );
	}

	return iter->second;
}

void FunctorFactory::RegisterFunctor( const std::string& name, boost::shared_ptr<IFunctor> functor )
{
	m_functors.insert( std::make_pair( name, functor ) );
}

void FunctorFactory::BuildFunctor()
{
	RegisterFunctor( "RunPricing", boost::shared_ptr<IFunctor>( new RunPricingFunctor() ) );
	RegisterFunctor( "RegisterYTM", boost::shared_ptr<IFunctor>( new RegisterYTMFunctor() ) );
	RegisterFunctor( "RegisterSwaption", boost::shared_ptr<IFunctor>( new RegisterSwaptionFunctor() ) );
	RegisterFunctor( "RegisterCapVol", boost::shared_ptr<IFunctor>( new RegisterCapVolFunctor() ) );
	RegisterFunctor( "RemoveCurve", boost::shared_ptr<IFunctor>( new RemoveCurveFunctor() ) );
	RegisterFunctor( "RegisterCurve" , boost::shared_ptr<IFunctor>( new RegisterCurveFunctor() ) );
	RegisterFunctor( "BuildCurve" , boost::shared_ptr<IFunctor>( new BuildCurveFunctor() ) );
}

FunctorFactory::FunctorFactory()
{
	BuildFunctor();
}
