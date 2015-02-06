#include "StdAfx.h"
#include <sstream>

#include "RegisterCurveFunctor.h"

#include "CurveTable.h"
#include "InterestRateCurveInfoWrapper.h"
#include "yield_builder.hpp"
#include "PricingSetting.h"
#include "XMLValue.h"
#include "StringUtil.h"

void RegisterCurveFunctor::Run( const TiXmlElement* param_root ) const
{
	const TiXmlElement* curves = param_root->FirstChildElement();
	std::wstring curveCode;
	std::wstring tenor;
	Real value;
	while ( curves )
	{
		curveCode = ::ToWString( curves->Value() );
		const TiXmlElement* tenors = curves->FirstChildElement();
		while( tenors )
		{
			tenor = ::ToWString( tenors->Value() ).substr(2);
			value = boost::lexical_cast<Real>( ::ToWString( tenors->Attribute( "value" ) ) );
			CurveTable::instance().AddManualInputYTM( curveCode, tenor, value );
			tenors = tenors->NextSiblingElement();
		}
		curves = curves->NextSiblingElement();
	}
}


void BuildCurveFunctor::Run( const TiXmlElement* param_root ) const
{
	CurveTable::instance().Init();
	PricingSetting::instance().Init( XMLValue( param_root, "eval_time" ), XMLValue( param_root, "data_date_alias" ) );

	const TiXmlElement* dataRoot = param_root->FirstChildElement( "data_root" );
	const TiXmlElement* record = dataRoot->FirstChildElement( "record" );

	boost::shared_ptr<YieldCurveInfoWrapper> curveInfo 
		= CurveTable::instance().GetYieldCurve( XMLValue( record, "Curve" ), ShiftOption::ShiftNothing );

	boost::shared_ptr<YieldCurveData> curveData 
		= boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>(curveInfo)->GetCurveData();

	//Make Output File
	std::string outFileName( PricingSetting::instance().GetOutputFileName() );
	if( outFileName.empty() )
	{
		outFileName = "result.trn";
	}

	TiXmlDocument doc;
	TiXmlElement* resRoot = new TiXmlElement( "result_root" );
	doc.LinkEndChild( resRoot );

	TiXmlElement* resCurve = new TiXmlElement( "curve_data" );	
	resCurve->SetAttribute("name", record->FirstChildElement( "Curve" )->Attribute( "value" ));
	resCurve->SetAttribute("dc", "temp");


	for (Size i=0; i < curveData->dates.size(); ++i) {
		TiXmlElement record( "record" );

		std::stringstream ss;
		ss<<io::iso_date(curveData->dates[i]);
		TiXmlElement tenorNode( "tenor" );
		tenorNode.SetAttribute( "type", "string" );
		tenorNode.SetAttribute( "value",  ss.str() );
		record.InsertEndChild( tenorNode );

		TiXmlElement daysNode( "days" );
		daysNode.SetAttribute( "type", "int" );
		daysNode.SetAttribute( "value",  curveData->dates[i].serialNumber() - PricingSetting::instance().GetEvaluationDate().serialNumber());
		record.InsertEndChild( daysNode );

		TiXmlElement npvNode( "yield" );
		npvNode.SetAttribute( "type", "double" );
		npvNode.SetAttribute( "value", ::ToString( ::ToWString( curveData->yields[i] ) ) );
		record.InsertEndChild( npvNode );

		resCurve->InsertEndChild(record);
	}

	resRoot->InsertEndChild( *resCurve );

	doc.SaveFile( outFileName );

}
