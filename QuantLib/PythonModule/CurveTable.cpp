#include "StdAfx.h"

#include "CurveTable.h"

#include "InterestRateCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "FXCurveInfoWrapper.h"

#include "ParamParseUtil.h"
#include "PricingSetting.h"

#include "CQuery.h"
#include "GlobalSetting.h"

#include "StringUtil.h"
#include "EnumParser.h"

#include "yield_builder.hpp"
#include "CurveTableUtil.h"

#include "XMLValue.h"
#include "XMLStream.h"

#include <ql/indexes/ibor/HIBOR3M.hpp>
#include <ql/indexes/ibor/cd91.hpp>
#include "BloombergCaller.h"
#include "ShiftOption.h"

#include "pricing_functions/hull_white_calibration.hpp"
#include "pricing_functions/single_hull_white_calibration.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

CurveTable::CurveTable()
{
}

void CurveTable::Init()
{
	m_table.clear();
	m_fxTable.clear();
	m_cdsTable.clear();
	m_corrMap.clear();
	m_capVolTable.clear();
	m_swaptionVolTable.clear();
	m_hwPeriodTable.clear();
	m_volTable.clear();
	m_proxyTable.clear();
}

boost::shared_ptr<YieldCurveInfoWrapper> CurveTable::GetYieldCurve( const std::wstring& codedCode, const ShiftOption& shiftOption )
{
	YieldCurveTable::const_iterator iter = m_table.find( std::make_pair( codedCode, shiftOption ) );

	if( iter != m_table.end() )
	{
		return iter->second.first;
	}

	if( boost::find_first( codedCode, L"_FX" ) )
	{
		std::vector<std::wstring> codeVec;
		boost::split( codeVec, codedCode, boost::is_any_of( L"_" ), boost::algorithm::token_compress_on );

		std::wstring irsCode = codeVec[ 0 ];
		std::wstring crsCode = irsCode + L"_CRS";

		boost::shared_ptr<InterestRateCurveInfoWrapper> irs = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( GetYieldCurve( irsCode, ShiftOption::ShiftNothing ) );
		boost::shared_ptr<InterestRateCurveInfoWrapper> crs;
		YieldCurveTable::const_iterator iter2 = m_table.find( std::make_pair( crsCode, shiftOption ) );
		if( iter2 != m_table.end() )
		{
			crs = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( iter2->second.first );
		}
		else
		{
			crs.reset( new InterestRateCurveInfoWrapper( crsCode, ::LoadCRSCurve( crsCode, ShiftOption::ShiftNothing, shiftOption ).first, shiftOption ) );
		}

		boost::shared_ptr<FXCurveData> fxData = GetFXCurve( codedCode, shiftOption );

		HistoricalVolMap::iterator vol_iter = m_volTable.find( codedCode );
		Real hVol;
		if( vol_iter == m_volTable.end() )
		{
			hVol = ::GetHistoricalVolWithBloomberg( irsCode + L"KRW Curncy", 120 );
			m_volTable.insert( std::make_pair( codedCode, hVol ) );
		}
		else
		{
			hVol = vol_iter->second;
		}

		boost::shared_ptr<FXCurveInfoWrapper> fxCurveWrapper( new FXCurveInfoWrapper( fxData->fwdValue[ 0 ], codedCode, irs, crs, hVol, fxData ) );
		m_table.insert( std::make_pair( std::make_pair( codedCode, shiftOption ), std::make_pair( fxCurveWrapper, irs->GetCurveData()->yields ) ) );

		return fxCurveWrapper;
	}

	std::pair<boost::shared_ptr<YieldCurveData>, std::vector<Real> > curveData;
	boost::shared_ptr<YieldCurveInfoWrapper> wrapper;

	if( boost::find_first( codedCode, L"_CRS" ) )
	{
		curveData = ::LoadCRSCurve( codedCode, shiftOption, ShiftOption::ShiftNothing );
		wrapper.reset( new InterestRateCurveInfoWrapper( codedCode, curveData.first, shiftOption ) );
	}
	else
	{
		std::vector<std::wstring> codes = ::Split( codedCode, boost::is_any_of( L"-" ) );
		std::wstring code = codes[ 0 ];
		ShiftOption opt( shiftOption );
		if( codes.size() > 1 )
		{
			Real shift = boost::lexical_cast<Real>( codes[ 1 ] ) / 100.;
			opt = ShiftOption( ShiftOption::ST_ShiftAll, shift );
		}
		curveData = ::LoadCurve( code, opt );

		wrapper.reset( new InterestRateCurveInfoWrapper( codedCode, curveData.first, shiftOption ) );
	}

	m_table.insert( std::make_pair( std::make_pair( codedCode, shiftOption ), std::make_pair( wrapper, curveData.second ) ) );
	return wrapper;
}

boost::shared_ptr<CDSCurveData> CurveTable::GetCDSCurve( const std::wstring& code )
{
	CDSCurveTable::const_iterator iter = m_cdsTable.find( code );
	if( iter == m_cdsTable.end() )
	{
		boost::shared_ptr<CDSCurveData> curveData = ::LoadCDSCurve( code );
		m_cdsTable.insert( std::make_pair( code, curveData ) );

		return curveData;
	}

	return iter->second;
}

boost::shared_ptr<FXCurveData> CurveTable::GetFXCurve( const std::wstring& code, const ShiftOption& shiftOption )
{
	FXCurveTable::const_iterator iter = m_fxTable.find( std::make_pair( code, shiftOption ) );
	if( iter == m_fxTable.end() )
	{
		boost::shared_ptr<FXCurveData> curveData = ::LoadFXCurve( code, shiftOption );
		m_fxTable.insert( std::make_pair( std::make_pair( code, shiftOption ), curveData ) );

		return curveData;
	}

	return iter->second;
}

boost::shared_ptr<HullWhiteParameters> CurveTable::GetHWParams( const std::wstring& codedCode, Period tenor, const ShiftOption& so )
{
	std::wstring code = Split( codedCode, boost::is_any_of( "_|-" ) )[ 0 ];

	HWPeriodTable::const_iterator iter = m_hwPeriodTable.find( std::make_pair( std::make_pair( code, so ), tenor ) );
	if( iter != m_hwPeriodTable.end() )
	{
		return iter->second;
	}

	// Global한 HWParam이 있으면 그걸 쓴다
	iter = m_hwPeriodTable.find( std::make_pair( std::make_pair( code, so ), 0 * Days ) );
	if( iter != m_hwPeriodTable.end() )
	{
		return iter->second;
	}

	boost::shared_ptr<InterestRateCurveInfoWrapper> ircw = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( GetYieldCurve( codedCode, ShiftOption::ShiftNothing ) );

	// 스왑션이 있는 경우 먼저 스왑션으로 해보고
	SwaptionVolTable::iterator volTableIter = m_swaptionVolTable.find( std::make_pair( code, so ) );
	boost::shared_ptr<SwaptionVolData> swaptionVolData;
	if( volTableIter != m_swaptionVolTable.end() )
	{
		swaptionVolData = volTableIter->second;
	}
	else
	{
		swaptionVolData = ::LoadSwaptionVol( code, so, ircw );
		m_swaptionVolTable.insert( std::make_pair( std::make_pair( code, so ), swaptionVolData ) );
	}

	if( swaptionVolData )
	{
		boost::shared_ptr<HullWhiteParameters> hwParam( ::CalcFromSwaption( swaptionVolData, tenor, ircw ) );
		m_hwPeriodTable.insert( std::make_pair( std::make_pair( std::make_pair( code, so ), tenor ), hwParam ) );
		return hwParam;
	}

	// 없으면 캡볼로
	CapVolTable::iterator capVolTableIter = m_capVolTable.find( std::make_pair( code, so ) );
	boost::shared_ptr<CapVolData> capVolData;
	if( capVolTableIter != m_capVolTable.end() )
	{
		// 지저분하지만 일단 두자.. 나중 리팩터링해야..
		capVolData = capVolTableIter->second;
	}
	else
	{
		capVolData = ::LoadCapVol( code, so, ircw );
		m_capVolTable.insert( std::make_pair( std::make_pair( codedCode, so ), capVolData ) );
	}

	boost::shared_ptr<HullWhiteParameters> hwParam( ::CalcFromCapVol( *capVolData, tenor ) );
	m_hwPeriodTable.insert( std::make_pair( std::make_pair( std::make_pair( code, so ), tenor ), hwParam ) );

	return hwParam;
}

QuantLib::Real CurveTable::GetCorr( const std::wstring& codedCode1, const std::wstring& codedCode2 )
{
	std::vector<std::wstring> codes1 = ::Split( codedCode1, boost::is_any_of( L"-" ) );
	std::wstring code1 = codes1[ 0 ];

	std::vector<std::wstring> codes2 = ::Split( codedCode2, boost::is_any_of( L"-" ) );
	std::wstring code2 = codes2[ 0 ];

	std::wstring minCode = min( code1, code2 );
	std::wstring maxCode = max( code1, code2 );

	CorrMap::const_iterator iter = m_corrMap.find( std::make_pair( minCode, maxCode ) );

	if( iter != m_corrMap.end() )
	{
		return iter->second;
	}

	CQuery dbConnector;
	Date rcvDate;
	::QueryRecentCurveData( minCode + L"_" + maxCode, dbConnector, rcvDate );

	dbConnector.Fetch();

	Real corr = boost::lexical_cast<Real>( dbConnector.GetStr( L"Value" ) );

	m_corrMap.insert( std::make_pair( std::make_pair( minCode, maxCode ), corr ) );

 	return corr;
}

std::pair<Real, Real> CurveTable::GetCorrRcvy( const std::wstring& code )
{
	CorrRcvyTable::iterator iter = m_corrRcvyTable.find( code );
	if( iter != m_corrRcvyTable.end() )
	{
		return iter->second;
	}

	CQuery dbConnector;
	BOOL conRes = dbConnector.Connect( 4, DB_SERVER_NAME, DB_ID, DB_PASSWORD );
	
	std::wostringstream buf;
	buf << boost::wformat( L"select count(*) as cnt, max(ReceiveDate) as RcvDate from `ficc_drvs`.`percontractinfo` where ReceiveDate<='%s' and ContractCode='%s'" ) % ::ToWString( PricingSetting::instance().GetDataDate() ).c_str() % code.c_str();

	dbConnector.Exec( buf.str().c_str() );
	dbConnector.Fetch();

	if( !boost::lexical_cast<int>( dbConnector.GetStr( L"cnt" ) ) )
	{
		return std::make_pair( 0.9, 0.4 );
	}

	Date rcvDate = ::ConvertToDate( dbConnector.GetStr( L"RcvDate" ) );
	dbConnector.Clear();


	buf << boost::wformat( L"select * from ficc_drvs.percontractinfo where ContractCode='%s' and ReceiveDate='%s'" ) % code.c_str() % ::ToWString( rcvDate ).c_str();
	dbConnector.Exec( buf.str().c_str() );

	dbConnector.Fetch();

	std::pair<Real, Real> res( boost::lexical_cast<Real>( dbConnector.GetStr( L"Corr" ) ), boost::lexical_cast<Real>( dbConnector.GetStr( L"RecoveryRate") ) );

	m_corrRcvyTable.insert( std::make_pair( code, res ) );

	return res;
}

enum E_CurveType
{
	CT_Yield,
  CT_FX,
	CT_CORR,
	CT_CDS,
	CT_CDS_PARAM,
};

class CurveTypeParser : public EnumParser<CurveTypeParser, E_CurveType>
{
public:
	void BuildEnumMap()
	{
		AddEnum( L"Yield", CT_Yield );
		AddEnum( L"FX", CT_FX );
		AddEnum( L"Corr", CT_CORR );
		AddEnum( L"CDS", CT_CDS );
		AddEnum( L"CDS_PARAM", CT_CDS_PARAM );
	}
private:
};

void CurveTable::AddCurveTable( const TiXmlElement* curveRecord )
{
 	E_CurveType curveType = CurveTypeParser::ParseEnum( ::ToWString( curveRecord->Attribute( "value" ) ) );

	switch( curveType )
	{
	case CT_Yield:
		{
			while( true )
			{
				try
				{
					XMLIStream stream( *curveRecord->FirstChildElement( "ShiftOption" ) );
					ShiftOption so( 0. );

					stream >> so;
					
					XMLIStream stream2( *curveRecord->FirstChildElement( "Tenor" ) );

					double tenorLength, tenorUnit;
					stream2 >> tenorLength >> tenorUnit;
				
					Period tenor( (int)( tenorLength ), (TimeUnit)( (int)tenorUnit ) );

					std::pair<boost::shared_ptr<YieldCurveData>, std::vector<Real> > curveDataPair( ::LoadCurve( ::ToWString( curveRecord->ValueStr() ), so ) );
					boost::shared_ptr<YieldCurveData> curveData( curveDataPair.first );

					boost::shared_ptr<InterestRateCurveInfoWrapper> wrapper( new InterestRateCurveInfoWrapper( ::ToWString( curveRecord->ValueStr() ), curveData, so ) );
					boost::shared_ptr<InterestRateCurveInfoWrapper> wrapperShiftNothing( new InterestRateCurveInfoWrapper( ::ToWString( curveRecord->ValueStr() ), curveData, so ) ); // ShiftOption::ShiftNothing ) );

					boost::shared_ptr<HullWhiteParameters> hwParam;
					if( boost::shared_ptr<SwaptionVolData> swaptionVolData = ::LoadSwaptionVol( wrapper->GetCurveName(), so, wrapperShiftNothing ) )
					{
						hwParam = ::CalcFromSwaption( swaptionVolData, tenor, wrapperShiftNothing );
					}
					else
					{
						boost::shared_ptr<CapVolData> capVolData = ::LoadCapVol( wrapper->GetCurveName(), so, wrapperShiftNothing );
						hwParam = ::CalcFromCapVol( *capVolData, tenor );
					}

					m_hwPeriodTable.insert( std::make_pair( std::make_pair( std::make_pair( wrapper->GetCurveName(), so ), 0 * Days ), hwParam ) );
					m_hwPeriodTable.insert( std::make_pair( std::make_pair( std::make_pair( wrapper->GetCurveName(), ShiftOption::ShiftNothing ), 0 * Days ), hwParam ) );

					m_table[ std::make_pair( wrapper->GetCurveName(), ShiftOption::ShiftNothing ) ] = std::make_pair( wrapper, curveDataPair.second );
					m_table[ std::make_pair( wrapper->GetCurveName(), so ) ] = std::make_pair( wrapper, curveDataPair.second );
					break;
				}
				catch ( ... )
				{
					Sleep( 1000 );
				}
			}
		}
		break;
	case CT_FX:
		{
			boost::shared_ptr<FXCurveData> curveData( new FXCurveData() );
			for( const TiXmlElement* record = curveRecord->FirstChildElement( "Maturity" )->FirstChildElement(); record != NULL; record = record->NextSiblingElement() )
			{
				curveData->fwdDate.push_back( ::ConvertToDateFromBloomberg( ::ToWString( record->Attribute( "value" ) ) ) );
			}

			for( const TiXmlElement* record = curveRecord->FirstChildElement( "Spot" )->FirstChildElement(); record != NULL; record = record->NextSiblingElement() )
			{
				curveData->fwdValue.push_back( boost::lexical_cast<Real>( record->Attribute( "value" ) ) );
			}

			curveData->fwdDate.push_back( curveData->fwdDate.back() + 20 * Years );
			curveData->fwdValue.push_back( curveData->fwdValue.back()	);

			std::wstring curveName = ::ToWString( curveRecord->ValueStr() );
			m_fxTable.insert( std::make_pair( std::make_pair( curveName, ShiftOption::ShiftNothing ), curveData ) );

			AddCurveTable( curveRecord->FirstChildElement( "irs" )->FirstChildElement() );
			AddCurveTable( curveRecord->FirstChildElement( "crs" )->FirstChildElement() );

			m_volTable.insert( std::make_pair( curveName, XMLValue( curveRecord, "Vol" ).GetValue<Real>() ) );
		}
		break;
	case CT_CORR:
		{
			typedef std::vector<std::pair<std::wstring, std::wstring> > TickerVec;
			TickerVec tickerVec;
			for( const TiXmlElement* record = curveRecord->FirstChildElement(); record != NULL; record = record->NextSiblingElement() )
			{
				tickerVec.push_back( std::make_pair( ::ToWString( record->ValueStr() ), ::ToWString( record->Attribute( "value" ) ) ) );
			}

			for each( const TickerVec::value_type& v1 in tickerVec )
			{
				for each( const TickerVec::value_type& v2 in tickerVec )
				{
					Real corr = ::GetHistoricalCorrWithBloomberg( v1.second, v2.second, L"WEEKLY", 52, L"ALL_CALENDAR_DAYS" );
					m_corrMap.insert( std::make_pair( std::make_pair( v1.first, v2.first ), corr ) );
				}
			}
		}
		break;
	case CT_CDS:
		{
			boost::shared_ptr<CDSCurveData> curveData( new CDSCurveData() );
			for( const TiXmlElement* record = curveRecord->FirstChildElement( "Maturity" )->FirstChildElement(); record != NULL; record = record->NextSiblingElement() )
			{
				curveData->tenors.push_back( ::ParsePeriod( ::ToWString( record->Attribute( "value" ) ) ) );
			}

			for( const TiXmlElement* record = curveRecord->FirstChildElement( "Spot" )->FirstChildElement(); record != NULL; record = record->NextSiblingElement() )
			{
				curveData->quotedSpreads.push_back( boost::lexical_cast<Real>( record->Attribute( "value" ) ) );
			}

			m_cdsTable.insert( std::make_pair( ::ToWString( curveRecord->ValueStr() ), curveData ) );
		}
		break;
	case CT_CDS_PARAM:
		{
			Real corr = XMLValue( curveRecord, "Corr" );
			Real rcvry = XMLValue( curveRecord, "RecoveryRate" );

			m_corrRcvyTable.insert( std::make_pair( ::ToWString( curveRecord->ValueStr() ), std::make_pair( corr, rcvry ) ) );
		}
		break;
	}
}

boost::shared_ptr<YieldCurveInfoWrapperProxy> CurveTable::GetYieldCurveProxy( const std::wstring& code )
{
	YieldCurveWrapperProxyTable::iterator iter = m_proxyTable.find( code );
	if( iter != m_proxyTable.end() )
	{
		return iter->second;
	}

	boost::shared_ptr<YieldCurveInfoWrapperProxy> proxy( new YieldCurveInfoWrapperProxy( code ) );
	m_proxyTable.insert( std::make_pair( code, proxy ) );

	return proxy;
}

boost::shared_ptr<CapVolData> CurveTable::GetCapVolData( const std::wstring& curveName, const ShiftOption& so )
{
	CapVolTable::const_iterator iter = m_capVolTable.find( std::make_pair( curveName, so ) );

	if( iter == m_capVolTable.end() )
	{
		boost::shared_ptr<CapVolData> capVol = ::LoadCapVol( curveName, so, boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( GetYieldCurve( curveName, so ) ) );
		m_capVolTable.insert( std::make_pair( std::make_pair( curveName, so ), capVol ) );

		return capVol;
	}

	return iter->second;
}

void CurveTable::SetVol( const std::wstring& curveName, Real vol )
{
	m_volTable[ curveName ] = vol;
}

void CurveTable::AddManualInputYTM( const std::wstring& curveCode, const std::wstring& Tenor, Real value )
{
	m_manualInputYTM[ std::make_pair( curveCode, Tenor ) ] = value;
}

Real CurveTable::GetNullableManualInputYTM( const ManualCurveDataTable::key_type& codeTenor ) const
{
	ManualCurveDataTable::const_iterator iter = m_manualInputYTM.find( codeTenor );
	if( iter == m_manualInputYTM.end() )
	{
		return Null<Real>();
	}

	return iter->second;
}

void CurveTable::AddSwaptionVolTable( const std::wstring& curveCode, boost::shared_ptr<SwaptionVolData> volData )
{
	m_swaptionVolTable.insert( std::make_pair( std::make_pair( curveCode, ShiftOption::ShiftNothing ), volData ) );
}

void CurveTable::AddCapVolTable( const std::wstring& curveCode, const ShiftOption& shiftOption, boost::shared_ptr<CapVolData> capVolData )
{
	m_capVolTable.insert( std::make_pair( std::make_pair( curveCode, shiftOption ), capVolData ) );
}

boost::shared_ptr<SwaptionVolData> CurveTable::GetSwaptionVolData( const std::wstring& curveName, const ShiftOption& so )
{
	SwaptionVolTable::const_iterator iter = m_swaptionVolTable.find( std::make_pair( curveName, so ) );

	if( iter == m_swaptionVolTable.end() )
	{
		boost::shared_ptr<SwaptionVolData> capVol = ::LoadSwaptionVol( curveName, so, boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( GetYieldCurve( curveName, so ) ) );
		m_swaptionVolTable.insert( std::make_pair( std::make_pair( curveName, so ), capVol ) );

		return capVol;
	}

	return iter->second;
}
