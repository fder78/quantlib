#include "StdAfx.h"

#include "RegisterYTMFunctor.h"

#include "CurveTable.h"
#include "StringUtil.h"

void RegisterYTMFunctor::Run( const TiXmlElement* param_root ) const
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
