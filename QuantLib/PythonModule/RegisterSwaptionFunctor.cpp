#include "StdAfx.h"

#include "RegisterSwaptionFunctor.h"

#include "CurveTable.h"
#include "CurveTableUtil.h"
#include "StringUtil.h"
#include "EnumParser.h"

#include "pricing_functions/hull_white_calibration.hpp"

void RegisterSwaptionFunctor::Run( const TiXmlElement* param_root ) const
{
	const TiXmlElement* curves = param_root->FirstChildElement();
	while ( curves )
	{
		std::wstring curveCode = ::ToWString( curves->Value() );

		boost::shared_ptr<SwaptionVolData> volData( new SwaptionVolData() );
		for( const TiXmlElement* record = curves->FirstChildElement( "SwaptionMaturity" )->FirstChildElement(); record != NULL; record = record->NextSiblingElement() )
		{
			volData->maturities.push_back( ::ParsePeriod( ::ToWString( record->Attribute( "value" ) ) ) );
		}
		for( const TiXmlElement* record = curves->FirstChildElement( "SwaptionTenor" )->FirstChildElement(); record != NULL; record = record->NextSiblingElement() )
		{
			volData->lengths.push_back( ::ParsePeriod( ::ToWString( record->Attribute( "value" ) ) ) );
		}
		for( const TiXmlElement* record = curves->FirstChildElement( "SwaptionSpot" )->FirstChildElement(); record != NULL; record = record->NextSiblingElement() )
		{
			volData->vols.push_back( boost::lexical_cast<Real>( ::ToWString( record->Attribute( "value" ) ) ) );
		}

		CurveTable::instance().AddSwaptionVolTable( curveCode, volData );

		curves = curves->NextSiblingElement();
	}
}
