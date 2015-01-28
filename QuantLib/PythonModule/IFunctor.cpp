#include "StdAfx.h"

#include "IFunctor.h"

#include "IProductParam.h"
#include "PricingSetting.h"
#include "XMLValue.h"
#include "CurveTable.h"

#include "stringUtil.h"

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

class RunPricingFunctor : public IFunctor
{
public:
	virtual void Run( const TiXmlElement* param_root ) const
	{
		CurveTable::instance().Init();
		const TiXmlElement* dataRoot = param_root->FirstChildElement( "data_root" );
		const TiXmlElement* record = dataRoot->FirstChildElement( "record" );

		PricingSetting::instance().Init( XMLValue( param_root, "eval_time") );

		TiXmlDocument doc;
		TiXmlElement* resRoot = new TiXmlElement( "result_root" );
		doc.LinkEndChild( resRoot );
		int idx = 0;
		while( record )
		{
			boost::shared_ptr<IProductParam> param = IProductParam::Create( record );
			
			param->Calculate();

			resRoot->InsertEndChild( *param->GetFinalResult() );

			record = record->NextSiblingElement();
			idx++;

			doc.SaveFile( "result.trn" );
		}
	}
};

void FunctorFactory::BuildFunctor()
{
	RegisterFunctor( "RunPricing", boost::shared_ptr<IFunctor>( new RunPricingFunctor() ) );
}

FunctorFactory::FunctorFactory()
{
	BuildFunctor();
}
