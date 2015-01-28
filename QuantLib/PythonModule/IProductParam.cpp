#include "StdAfx.h"

#include "IProductParam.h"

#include "ProductIndex.h"
#include "XMLValue.h"

#include "stringUtil.h"

#include "yield_builder.hpp"
#include "hull_white_calibration.hpp"

#include "ParamParseUtil.h"

//#include "CLNParam.h"
//#include "CDSParam.h"
#include "RangeAccrualBond.h"

boost::shared_ptr<IProductParam> IProductParam::Create( const TiXmlElement* record )
{
	E_ProductType type = ProductParser::ParseEnum( ::ToWString( record->FirstChildElement( PI_Type )->Attribute( "value" ) ) );
	boost::shared_ptr<IProductParam> param;

	switch( type )
	{
	//case PT_CLNSingle:
	//case PT_CLNNtD:
	//	param = boost::shared_ptr<IProductParam>( new CLNParam() );
	//	break;
	//case PT_CDSSingle:
	//case PT_CDSNtD:
	//	param = boost::shared_ptr<IProductParam>( new CDSParam() );
	//	break;
	case PT_RANote:
	case PT_RASwap:
		param = boost::shared_ptr<IProductParam>( new RangeAccrualBondParam() );
		break;
	}

	param->SetData( record );
	return param;
}

IProductParam::IProductParam()
{
}

void IProductParam::SetData( const TiXmlElement* record )
{
	m_productName = XMLValue( record, PI_Code ).GetValue<const char*>();

	m_record = boost::shared_ptr<TiXmlElement>( static_cast<TiXmlElement*>( record->Clone() ) );
	m_result = boost::shared_ptr<TiXmlElement>( new TiXmlElement( m_productName ) );

	SetDataImpl( record );
}
