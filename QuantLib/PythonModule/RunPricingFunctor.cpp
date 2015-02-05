#include "StdAfx.h"
#include <iostream>
#include <boost/timer.hpp>

#include "RunPricingFunctor.h"

#include "PricingSetting.h"
#include "CurveTable.h"
#include "XMLValue.h"
#include "IProductParam.h"
#include "CalculationProxy.h"

void RunPricingFunctor::Run( const TiXmlElement* param_root ) const
{
	std::string outFileName( PricingSetting::instance().GetOutputFileName() );
	if( outFileName.empty() )
	{
		outFileName = "result.trn";
	}

	CurveTable::instance().Init();
	PricingSetting::instance().Init( XMLValue( param_root, "eval_time" ), XMLValue( param_root, "data_date_alias" ) );
	CalculationProxy::instance().Init();

	std::vector<boost::shared_ptr<IProductParam> > params;

	const TiXmlElement* curveRoot = param_root->FirstChildElement( "curve_root" );
	if( curveRoot )
	{
		const TiXmlElement* curveRecord = curveRoot->FirstChildElement();

		while( curveRecord )
		{
			CurveTable::instance().AddCurveTable( curveRecord );
			curveRecord = curveRecord->NextSiblingElement();
		}
	}

	const TiXmlElement* dataRoot = param_root->FirstChildElement( "data_root" );
	const TiXmlElement* record = dataRoot->FirstChildElement( "record" );

	TiXmlDocument doc;
	TiXmlElement* resRoot = new TiXmlElement( "result_root" );
	doc.LinkEndChild( resRoot );
	int idx = 0;

	if( param_root->FirstChildElement( "useproxy" ) != NULL )
	{
		std::string address = XMLValue( param_root, "useproxy" ).GetValue<const char*>();
		PricingSetting::instance().SetUseProxy( true, address );
	}

	boost::timer timer;
	while( record )
	{		
		timer.restart();
		boost::shared_ptr<IProductParam> param = IProductParam::Create( record );
		params.push_back( param );

		param->Calculate();

		resRoot->InsertEndChild( *param->GetFinalResult() );
		record = record->NextSiblingElement();		

		std::cout<<++idx<<"\t"<<param->GetProductName()<<"\t"<<timer.elapsed()<<std::endl;
		timer.restart();
	}

	if( PricingSetting::instance().UseProxy() )
	{
		CalculationProxy::instance().RemoteCalculate();
		CalculationProxy::instance().FetchResult();
	}

	resRoot->Clear();
	for each( boost::shared_ptr<IProductParam> param in params )
	{
		param->FetchResult();
		resRoot->InsertEndChild( *param->GetFinalResult() );
	}
	doc.SaveFile( outFileName );
}