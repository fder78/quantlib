#include "StdAfx.h"

#include "IProductParam.h"

#include "ProductIndex.h"
#include "XMLValue.h"

#include "StringUtil.h"

#include "yield_builder.hpp"
#include "pricing_functions/single_hull_white_calibration.hpp"

#include "ParamParseUtil.h"

//#include "CLNParam.h"
//#include "CDSParam.h"
//#include "RangeAccrualBond.h"
#include "RAKAPParam.h"
#include "RAMCParam.h"
#include "RAFDMParam.h"
#include "RAKAPParam.h"
#include "RACMSParam.h"
#include "RASpreadParam.h"
#include "RASingleTreeParam.h"
#include "FXDigitalParam.h"
#include "PowerSpreadParam.h"
#include "VanillaSwapParam.h"
#include "FixedRateBondParam.h"
#include "CapFloorParam.h"
#include "SwaptionParam.h"


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
			param = boost::shared_ptr<IProductParam>( new RAMCParam() );
			break;
		case PT_RAFDMNote:
		case PT_RAFDMSwap:
			param = boost::shared_ptr<IProductParam>( new RAFDMParam() );
			break;
		case PT_RANoteKAP:
		case PT_RASwapKAP:
			param = boost::shared_ptr<IProductParam>( new RAKAPParam() );
			break;
		case PT_RANoteCMS:
		case PT_RASwapCMS:
			param = boost::shared_ptr<IProductParam>( new RACMSParam() );
			break;
		case PT_RANoteSingleTree:
		case PT_RASwapSingleTree:
			param = boost::shared_ptr<IProductParam>( new RASingleTreeParam() );
			break;
		case PT_RASpreadNote:
		case PT_RASpreadSwap:
			param = boost::shared_ptr<IProductParam>( new RASpreadParam() );
			break;
		case PT_PSSwap:
		case PT_PSNote:
			param = boost::shared_ptr<IProductParam>( new PowerSpreadParam() );
			break;		
		case PT_VanillaSwap:
			param = boost::shared_ptr<IProductParam>(new VanillaSwapParam());
			break;
		case PT_FixedRateBond:
			param = boost::shared_ptr<IProductParam>(new FixedRateBondParam());
			break;
		case PT_CapFloor:
			param = boost::shared_ptr<IProductParam>(new CapFloorParam());
			break;
		case PT_Swaption:
			param = boost::shared_ptr<IProductParam>(new SwaptionParam());
			break;
		case PT_FXDigital:
			param = boost::shared_ptr<IProductParam>(new FXDigitalParam());
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
	m_type = XMLValue( record, PI_Type ).GetValue<const char*>();

	m_strTraderID = XMLValue(record, PI_TraderID).GetValue<const char*>();
	m_strBook = XMLValue(record, PI_Book).GetValue<const char*>();

	m_record = boost::shared_ptr<TiXmlElement>( static_cast<TiXmlElement*>( record->Clone() ) );
	m_result = boost::shared_ptr<TiXmlElement>( new TiXmlElement( m_productName ) );

	TiXmlElement type( "type" );
	type.SetAttribute( "type", "string" );
	type.SetAttribute( "value", m_type );
	GetResultObject()->InsertEndChild( type );

	TiXmlElement book( "book" );
	book.SetAttribute( "type", "string" );
	book.SetAttribute( "value", m_strBook );
	GetResultObject()->InsertEndChild( book );

	SetDataImpl( m_record.get() );
}

void IProductParam::ParseResult( const TiXmlElement& result )
{
	m_result.reset( new TiXmlElement( *result.FirstChildElement() ) );
}
