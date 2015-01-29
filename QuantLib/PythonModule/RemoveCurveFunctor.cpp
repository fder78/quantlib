#include "StdAfx.h"

#include "RemoveCurveFunctor.h"

#include "XMLValue.h"
#include "CurveTable.h"
#include "EnumParser.h"

enum E_ManualCurveType
{
	MCT_YTM,
	MCT_CAPVOL,
	MCT_Swaption
};

class ManualCurveTypeParser : public EnumParser<ManualCurveTypeParser, E_ManualCurveType>
{
public:
	ManualCurveTypeParser() {}
	void BuildEnumMap()
	{
		AddEnum( L"YTM", MCT_YTM );
		AddEnum( L"CAPVOL", MCT_CAPVOL );
		AddEnum( L"Swaption", MCT_Swaption );
	}
};

void RemoveCurveFunctor::Run( const TiXmlElement* param_root ) const
{
	E_ManualCurveType curveType = ManualCurveTypeParser::ParseEnum( XMLValue( param_root, "type" ) );
	switch ( curveType )
	{
	case MCT_YTM:
		CurveTable::instance().RemoveManualInputYTM();
		break;
	case MCT_CAPVOL:
		CurveTable::instance().RemoveCapVolTable();
		break;
	case MCT_Swaption:
		CurveTable::instance().RemoveSwaptionVolTable();
		break;
	}
}
