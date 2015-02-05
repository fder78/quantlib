#include "StdAfx.h"

#include "RegisterCurveFunctor.h"

#include "CurveTable.h"
#include "YieldCurveInfoWrapperProxy.h"
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

	boost::shared_ptr<YieldCurveInfoWrapperProxy> curveInfo
		= CurveTable::instance().GetYieldCurveProxy( XMLValue( record, "Curve" ) );

}
