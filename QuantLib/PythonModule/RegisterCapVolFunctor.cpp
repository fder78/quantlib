#include "StdAfx.h"

#include "RegisterCapVolFunctor.h"

#include "CurveTable.h"
#include "CurveTableUtil.h"
#include "ShiftOption.h"
#include "StringUtil.h"

#include "pricing_functions/hull_white_calibration.hpp"

void RegisterCapVolFunctor::Run( const TiXmlElement* param_root ) const
{
	const TiXmlElement* curves = param_root->FirstChildElement();
	while( curves )
	{
		boost::shared_ptr<CapVolData> capVolData( new CapVolData );
		std::wstring curveCode = ::ToWString( curves->Value() );
		const TiXmlElement* tenors = curves->FirstChildElement();
		while( tenors )
		{
			Period tenor = ::ParsePeriod( ::ToWString( tenors->Value() ).substr( 2 ) );
			Real value = boost::lexical_cast<Real>( ::ToWString( tenors->Attribute( "value" ) ) );

			capVolData->tenors.push_back( tenor.length() );
			capVolData->vols.push_back( value );

			tenors = tenors->NextSiblingElement();
		}
		CurveTable::instance().AddCapVolTable( curveCode, ShiftOption::ShiftNothing, capVolData );
		curves = curves->NextSiblingElement();
	}
}
