#include "StdAfx.h"

#include "IRProductParam.h" 
#include "RemoteXMLJob.h"
#include "IRProductJob.h"
#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "PricingSetting.h"
#include "ProductIndex.h"
#include "XMLValue.h"
#include "StringUtil.h"
#include "CalculationProxy.h"

#include "pricing_functions/hull_white_calibration.hpp"

double GetDeltaStep( double dbFlag )
{		
	return 0.0001;

	if( dbFlag < 0.005 )
	{
		return 0.0002;
	}
	else 
	{
		return 0.002;
	}
}

IRProductParam::IRProductParam()
{
}

void IRProductParam::Calculate()
{
	if( PricingSetting::instance().UseProxy() )
	{
		CalcWithProxy();
	}
	else
	{
		CalcWithoutProxy();
	}
}

void IRProductParam::CalcWithProxy()
{
	boost::shared_ptr<IRProductJob> raJob( new IRProductJob( this ) );
	CalculationProxy::instance().AddJob( raJob );
	raJob->SetResJob( CreateJob() );
	if( !PricingSetting::instance().CalcGreek() )
	{
		return;
	}

	double deltastep;
	double dbFlag;

	dbFlag = m_discountCurveInfo->GetYieldCurveWrapper()->GetSpotValue();
	deltastep = ::GetDeltaStep( dbFlag );

	for each(const YCurveMap::value_type& v in m_curves )
	{
		boost::shared_ptr<YieldCurveInfoWrapper> curveWrapper = v.second.first->GetYieldCurveWrapper();
		std::wstring curveName = v.second.first->GetCurveName();

		if( m_calcDelta )
		{
			dbFlag = curveWrapper->GetSpotValue();
			deltastep = ::GetDeltaStep( dbFlag );

			v.second.first->ShiftCurve( deltastep );
			raJob->SetPDResJob( curveName, CreateJob() );

			v.second.first->ShiftCurve( -deltastep );
			raJob->SetMDResJob( curveName, CreateJob() );

			for(int i = 0; i < curveWrapper->GetTenorCount( GetRemainingTime() ) ; i++)
			{
				boost::shared_ptr<RemoteXMLJob> pdJob, mdJob;
				v.second.first->ShiftCurvePerTenor( deltastep, i );
				pdJob = CreateJob();

				v.second.first->ShiftCurvePerTenor( -deltastep, i );
				mdJob = CreateJob();

				v.second.first->ShiftCurvePerTenor( 0., i );
				raJob->SetKeyJob( curveWrapper->GetCurveName(), pdJob, mdJob );
			}
		}

		if( m_calcChunkDelta )
		{
			dbFlag = curveWrapper->GetSpotValue();
			deltastep = ::GetDeltaStep( dbFlag );

			for(int i = 0; i < curveWrapper->GetChunkCount( GetRemainingTime() ); i++)
			{
				boost::shared_ptr<RemoteXMLJob> pdJob, mdJob;
				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunk, deltastep, i ) );
				pdJob = CreateJob();

				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunk, -deltastep, i ) );
				mdJob = CreateJob();

				v.second.first->ShiftCurvePerTenor( 0., i );
				raJob->SetChunkKeyJob( curveWrapper->GetCurveName(), pdJob, mdJob );
			}

			for(int i = 0; i < curveWrapper->GetChunkCount( GetRemainingTime() ); i++)
			{
				for( int j = 0; j <= i; j++ )
				{
					boost::shared_ptr<RemoteXMLJob> ppdJob, pmdJob, mpdJob, mmdJob;
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunkTwo, deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., deltastep, j ) );
					ppdJob = CreateJob();

					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunkTwo, deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., -deltastep, j) );
					pmdJob = CreateJob();

					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunkTwo, -deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., deltastep, j) );
					mpdJob = CreateJob();

					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunkTwo, -deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., -deltastep, j) );
					mmdJob = CreateJob();

					v.second.first->ShiftCurvePerTenor( 0., i );
					raJob->SetChunkKeyXGammaJob( curveWrapper->GetCurveName(), ppdJob, pmdJob, mpdJob, mmdJob );
				}
			}

			for each( const YCurveMap::value_type& w in m_curves )
			{
				boost::shared_ptr<YieldCurveInfoWrapper> curveWrapper2 = w.second.first->GetYieldCurveWrapper();
				if( w.second.first == v.second.first )
				{
					break;
				}

				for(int i = 0; i < curveWrapper->GetChunkCount( GetRemainingTime() ); i++ )
				{
					for(int j = 0; j < curveWrapper2->GetChunkCount( GetRemainingTime() ); j++ )
					{
						boost::shared_ptr<RemoteXMLJob> resXGamma[ 4 ];
						Real dirX[ 2 ] = { -1., 1. };
						Real dirY[ 2 ] = { -1., 1. };
						for( int k = 0; k < 2; k++ )
						{
							for( int l = 0; l < 2; l++ )
							{
								v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunk, deltastep * dirX[ k ], i ) );
								w.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunk, deltastep * dirY[ l ], j ) );
								resXGamma[ k * 2 + l ] = CreateJob();
							}
						}
						
						w.second.first->ShiftCurve( ShiftOption::ShiftNothing );
						v.second.first->ShiftCurve( ShiftOption::ShiftNothing );

						raJob->SetChunkKeyXCurveGammaJob( curveName + L"_" + w.second.first->GetCurveName(), resXGamma[ 0 ], resXGamma[ 1 ], resXGamma[ 2 ], resXGamma[ 3 ] );
					}
				}
			}
		}

		if( m_calcXGamma )
		{
			for(int i = 0; i < curveWrapper->GetTenorCount( GetRemainingTime() ); i++)
			{
				for( int j = 0; j < curveWrapper->GetTenorCount( GetRemainingTime() ); j++ )
				{
					boost::shared_ptr<RemoteXMLJob> ppdJob, pmdJob, mpdJob, mmdJob;
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftTwo, deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., deltastep, j ) );
					ppdJob = CreateJob();
				
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftTwo, deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., -deltastep, j) );
					pmdJob = CreateJob();

					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftTwo, -deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., deltastep, j) );
					mpdJob = CreateJob();

					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftTwo, -deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., -deltastep, j) );
					mmdJob = CreateJob();

					v.second.first->ShiftCurvePerTenor( 0., i );
					raJob->SetKeyXGammaJob( curveWrapper->GetCurveName(), ppdJob, pmdJob, mpdJob, mmdJob );
				}
			}
		}

		if( m_calcVega && v.second.second )
		{
			v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftNothing, 0., 0, 0., ShiftOption::VST_ShiftAll, 0.001 ) );
			raJob->SetPVegaResJob( curveName, CreateJob() );

			v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftNothing, 0., 0, 0., ShiftOption::VST_ShiftAll, -0.001 ) );
			raJob->SetMVegaResJob( curveName, CreateJob() );

			std::vector<Size> vegaIdx = curveWrapper->GetVegaTenors( GetRemainingTime() );

			for each( Size idx in vegaIdx )
			{
				boost::shared_ptr<RemoteXMLJob> pdJob, mdJob;

				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftNothing, 0., 0, 0., ShiftOption::VST_ShiftOne, 0.001, idx ) );
				pdJob = CreateJob();

				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftNothing, 0., 0, 0., ShiftOption::VST_ShiftOne, -0.001, idx ) );
				mdJob = CreateJob();

				raJob->SetKeyVegaJob( curveWrapper->GetCurveName(), pdJob, mdJob );
			}

			v.second.first->ShiftCurve( 0. );
		}
	}
}


void IRProductParam::CalcWithoutProxy()
{
	double deltastep;
	std::wstring strTmp;
	Real shiftBias = GetBiasForProduct();

	m_discountCurveInfo->ShiftCurve( shiftBias );
	m_npv = DoCalculation().npv;

	if( !PricingSetting::instance().CalcGreek() )
	{
		return;
	}

	m_discountCurveInfo->ShiftCurve( 0 );
	for each( const YCurveMap::value_type& v in m_curves )
	{
		boost::shared_ptr<YieldCurveInfoWrapper> curveWrapperTmp = v.second.first->GetYieldCurveWrapper();
		std::wstring curveName = curveWrapperTmp->GetCurveName();
		deltastep = 0.0001;

		if( m_calcDelta )
		{
			Real resDeltaShift[ 2 ];
			v.second.first->ShiftCurve( -deltastep + shiftBias );
			resDeltaShift[0] = DoCalculation().npv;

			v.second.first->ShiftCurve( deltastep + shiftBias );
			resDeltaShift[1] = DoCalculation().npv;

			m_keyExpShift[ curveName ] = ( resDeltaShift[ 1 ] - resDeltaShift[ 0 ] ) / 2 / deltastep / 10000.;
			m_keyGammaExpShift[ curveName ] = ( resDeltaShift[ 1 ] + resDeltaShift[ 0 ] - 2 * m_npv ) / deltastep / deltastep / 10000. / 10000.;
			v.second.first->ShiftCurve( 0 );

			for( int i = 0; i< curveWrapperTmp->GetTenorCount( GetRemainingTime() ); i++ )
			{
				Real resDaltaKey[ 2 ];
				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAllPlusOne, -deltastep, i, shiftBias, ShiftOption::VST_ShiftNothing, 0., 0,0.) );
				resDaltaKey[ 0 ] = DoCalculation().npv;

				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAllPlusOne, deltastep, i, shiftBias, ShiftOption::VST_ShiftNothing, 0., 0,0.) );
				resDaltaKey[ 1 ] = DoCalculation().npv;

				m_resInfoForKeyExp[ curveName ].push_back( ( resDaltaKey[ 1 ] - resDaltaKey[ 0 ] ) / 2 / deltastep / 10000. );
				m_resInfoForKeyGammaExp[ curveName ].push_back( ( resDaltaKey[ 0 ] + resDaltaKey[ 1 ] - 2 * m_npv ) / deltastep / deltastep / 10000. / 10000. );
			}

			v.second.first->ShiftCurve( 0 );
		}

		if( m_calcChunkDelta )
		{
			Real dbFlag = curveWrapperTmp->GetSpotValue();
			deltastep = ::GetDeltaStep( dbFlag );

			for(int i = 0; i < curveWrapperTmp->GetChunkCount( GetRemainingTime() ); i++)
			{
				Real resDaltaKey[ 2 ];
				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunk, -deltastep, i ) );
				resDaltaKey[ 0 ] = DoCalculation().npv;

				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunk, deltastep, i ) );
				resDaltaKey[ 1 ] = DoCalculation().npv;

				m_resInfoForChunkDeltaExp[ curveName ].push_back( ( resDaltaKey[ 1 ] - resDaltaKey[ 0 ] ) / 2 / deltastep / 10000. );
				m_resInfoForChunkGammaExp[ curveName ].push_back( ( resDaltaKey[ 0 ] + resDaltaKey[ 1 ] - 2 * m_npv ) / deltastep / deltastep / 10000. / 10000. );
			}

			for(int i = 0; i < curveWrapperTmp->GetChunkCount( GetRemainingTime() ); i++)
			{
				for( int j = 0; j <= i; j++ )
				{
					Real resXGamma[ 4 ];
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunkTwo, deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., deltastep, j ) );
					resXGamma[0] = DoCalculation().npv;
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunkTwo, deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., -deltastep, j) );
					resXGamma[1] = DoCalculation().npv;
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunkTwo, -deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., deltastep, j) );
					resXGamma[2] = DoCalculation().npv;
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunkTwo, -deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., -deltastep, j) );
					resXGamma[3] = DoCalculation().npv;

					m_resInfoForChunkXGammaExp[ curveName ].push_back( ( resXGamma[ 0 ] + resXGamma[ 3 ] - resXGamma[ 1 ] - resXGamma[ 2 ] ) / 4 / deltastep / deltastep / 10000. / 10000. );
				}
			}

			for each( const YCurveMap::value_type& w in m_curves )
			{
				if( w.second.first == v.second.first )
				{
					break;
				}

				boost::shared_ptr<YieldCurveInfoWrapper> curveWrapperTmp2 = w.second.first->GetYieldCurveWrapper();
				for(int i = 0; i < curveWrapperTmp->GetChunkCount( GetRemainingTime() ); i++ )
				{
					for(int j = 0; j < curveWrapperTmp->GetChunkCount( GetRemainingTime() ); j++ )
					{
						Real resXGamma[ 4 ];
						Real dirX[ 2 ] = { -1., 1. };
						Real dirY[ 2 ] = { -1., 1. };
						for( int k = 0; k < 2; k++ )
						{
							for( int l = 0; l < 2; l++ )
							{
								v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunk, deltastep * dirX[ k ], i ) );
								w.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftChunk, deltastep * dirY[ l ], j ) );
								resXGamma[ k * 2 + l ] = DoCalculation().npv;
							}
						}
						
						m_resInfoForChunkXGammaExp[ curveName + L"_" + w.second.first->GetCurveName() ].push_back( ( resXGamma[ 0 ] + resXGamma[ 3 ] - resXGamma[ 1 ] - resXGamma[ 2 ] ) / 4 / deltastep / deltastep / 10000. / 10000. );
					}
				}
			}
		}
		
		if( m_calcXGamma )
		{
			Real resXGamma[ 4 ];
			int k=0;
			for(int i = 1; i < 6; i++)
			{
				for(int j = i + 1; j < 6; j++)
				{
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftTwo, deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., deltastep, j ) );
					resXGamma[0] = DoCalculation().npv;
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftTwo, deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., -deltastep, j) );
					resXGamma[1] = DoCalculation().npv;
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftTwo, -deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., deltastep, j) );
					resXGamma[2] = DoCalculation().npv;
					v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftTwo, -deltastep, i, 0., ShiftOption::VST_ShiftNothing, 0., 0,0., -deltastep, j) );
					resXGamma[3] = DoCalculation().npv;

					m_resInfoForKeyXGammaExp[ curveName ].push_back( ( resXGamma[ 0 ] + resXGamma[ 3 ] - resXGamma[ 1 ] - resXGamma[ 2 ] ) / 4 / deltastep / deltastep / 10000. / 10000. );
				}
			}
			v.second.first->ShiftCurve( 0 );
		}

		if( m_calcVega && v.second.second )
		{
			Real resEachVShiftp;
			Real resEachVShiftm;
			v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAll, shiftBias, 0, 0., ShiftOption::VST_ShiftAll, 0.001) );
			resEachVShiftp = DoCalculation().npv;

			v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAll, shiftBias, 0, 0., ShiftOption::VST_ShiftAll, -0.001) );
			resEachVShiftm = DoCalculation().npv;

			m_keyVegaExpShift[ curveName ] = ( resEachVShiftp - resEachVShiftm ) / 2 / 0.001 / 100.;
			v.second.first->ShiftCurve( 0 );

			std::vector<Size> vegaIdx = curveWrapperTmp->GetVegaTenors( GetRemainingTime() );
			for each( Size idx in vegaIdx )
			{
				Real respd, resmd;

				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAll, shiftBias, 0, 0., ShiftOption::VST_ShiftOne, 0.001, idx ) );
				respd = DoCalculation().npv;

				v.second.first->ShiftCurve( ShiftOption( ShiftOption::ST_ShiftAll, shiftBias, 0, 0., ShiftOption::VST_ShiftOne, -0.001, idx ) );
				resmd = DoCalculation().npv;

				m_resInfoForKeyVegaExp[ curveName ].push_back( ( respd - resmd ) / 2 / 0.001 / 100. );
			}

			for( int i = 0; i < static_cast<int>( vegaIdx.size() ) - 1; i++ )
			{
				m_resInfoForKeyVegaExp[ curveName ][ i ] -= m_resInfoForKeyVegaExp[ curveName ][ i + 1 ];
			}

			v.second.first->ShiftCurve( 0 );
		}
	}
}

void IRProductParam::FetchResult()
{
	TiXmlElement npvNode( "npv" );
	npvNode.SetAttribute( "type", "double" );
	npvNode.SetAttribute( "value", ::ToString( ::ToWString(  m_npv ) ) );
	GetResultObject()->InsertEndChild( npvNode );

	if( !PricingSetting::instance().ShowDetail() )
	{
		return;
	}

	TiXmlElement unitPrice( "unitPrice" );
	unitPrice.SetAttribute( "type", "double" );
	unitPrice.SetAttribute( "value", ::ToString( ::ToWString( m_npv / m_notional * 10000. ) ) );
	GetResultObject()->InsertEndChild( unitPrice );

	TiXmlElement notional( "notional" );
	notional.SetAttribute( "type", "double" );
	notional.SetAttribute( "value", ::ToString( ::ToWString( m_notional ) ) );
	GetResultObject()->InsertEndChild( notional );
	

	TiXmlElement* cashFlowNode = new TiXmlElement( "cashFlow" );
	std::vector<std::pair<Date, Real> > cashFlows = GetCashFlow();
	GetResultObject()->LinkEndChild( cashFlowNode );
	for each( std::pair<Date, Real> cashFlow in cashFlows )
	{
		TiXmlElement* record = new TiXmlElement( "record");
		cashFlowNode->LinkEndChild( record );

		TiXmlElement dateNode( "date" );
		dateNode.SetAttribute( "type", "double");
		dateNode.SetAttribute( "value", ::ToString( ::ToWString( cashFlow.first ) ) );
		record->InsertEndChild( dateNode );

		TiXmlElement cashFlowValue( "cashFlowValue" );
		cashFlowValue.SetAttribute( "type", "double");
		cashFlowValue.SetAttribute( "value", ::ToString( ::ToWString( cashFlow.second ) ) );
		record->InsertEndChild( cashFlowValue );
	}

	TiXmlElement evalTime( "evalTime" );
	evalTime.SetAttribute( "type", "string" );
	evalTime.SetAttribute( "value", ::ToString( ::ToWString( Date::todaysDate() ) + L" " + ::ToWString( Date::todaysDate().second() ) ) );
	GetResultObject()->InsertEndChild( evalTime );

	TiXmlElement *kExpNodeTitle = new TiXmlElement("KeyExposure" );
	GetResultObject()->LinkEndChild( kExpNodeTitle );

	for each( const KeyResultMap::value_type& v in m_resInfoForKeyExp )
	{
		TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString( v.first ) );
		kExpNodeTitle->LinkEndChild( kExpNodeType );

		TiXmlElement kShiftExpNode( "kexpShift" );
		kShiftExpNode.SetAttribute( "type", "double" );
		kShiftExpNode.SetAttribute( "value", ::ToString( ::ToWString( m_keyExpShift[ v.first ] ) ) );
		kExpNodeType->InsertEndChild( kShiftExpNode );
		int i = 0;
		for each( Real val in v.second )
		{
			std::ostringstream buf;
			buf << boost::format( "kexp%d" ) % i++;

			TiXmlElement kExpNode( buf.str() );
			kExpNode.SetAttribute( "type", "double" );
			kExpNode.SetAttribute( "value", ::ToString( ::ToWString( val ) ) );
			kExpNodeType->InsertEndChild( kExpNode );
		}
	}

	TiXmlElement *kGammaExp = new TiXmlElement("GammaExposure" );
	GetResultObject()->LinkEndChild( kGammaExp );

	for each( const KeyResultMap::value_type& v in m_resInfoForKeyGammaExp )
	{
		TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString( v.first ) );
		kGammaExp->LinkEndChild( kExpNodeType );

		TiXmlElement kShiftExpNode( "kexpShift" );
		kShiftExpNode.SetAttribute( "type", "double" );
		kShiftExpNode.SetAttribute( "value", ::ToString( ::ToWString( m_keyGammaExpShift[ v.first ] ) ) );
		kExpNodeType->InsertEndChild( kShiftExpNode );
		int i = 0;
		for each( Real val in v.second )
		{
			std::ostringstream buf;
			buf << boost::format( "kexp%d" ) % i++;

			TiXmlElement kExpNode( buf.str() );
			kExpNode.SetAttribute( "type", "double" );
			kExpNode.SetAttribute( "value", ::ToString( ::ToWString( val ) ) );
			kExpNodeType->InsertEndChild( kExpNode );
		}
	}

	TiXmlElement *kXGammaExp = new TiXmlElement("XGammaExposure" );
	GetResultObject()->LinkEndChild( kXGammaExp );

	for each( const KeyResultMap::value_type& v in m_resInfoForKeyXGammaExp )
	{
		TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString( v.first ) );
		kXGammaExp->LinkEndChild( kExpNodeType );
		int i = 0;
		for each( Real val in v.second )
		{
			std::ostringstream buf;
			buf << boost::format( "kxgammaexp%d" ) % i++;

			TiXmlElement kExpNode( buf.str() );
			kExpNode.SetAttribute( "type", "double" );
			kExpNode.SetAttribute( "value", ::ToString( ::ToWString( val ) ) );
			kExpNodeType->InsertEndChild( kExpNode );
		}
	}

	TiXmlElement *kChunkDeltaExp = new TiXmlElement("ChunkDeltaExposure" );
	GetResultObject()->LinkEndChild( kChunkDeltaExp );

	for each( const KeyResultMap::value_type& v in m_resInfoForChunkDeltaExp )
	{
		TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString( v.first ) );
		kChunkDeltaExp->LinkEndChild( kExpNodeType );

		int i = 0;
		for each( Real val in v.second )
		{
			std::ostringstream buf;
			buf << boost::format( "kexp%d" ) % i++;

			TiXmlElement kExpNode( buf.str() );
			kExpNode.SetAttribute( "type", "double" );
			kExpNode.SetAttribute( "value", ::ToString( ::ToWString( val ) ) );
			kExpNodeType->InsertEndChild( kExpNode );
		}
	}

	TiXmlElement *kChunkGammaExp = new TiXmlElement("ChunkGammaExposure" );
	GetResultObject()->LinkEndChild( kChunkGammaExp );

	for each( const KeyResultMap::value_type& v in m_resInfoForChunkGammaExp )
	{
		TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString( v.first ) );
		kChunkGammaExp->LinkEndChild( kExpNodeType );

		int i = 0;
		for each( Real val in v.second )
		{
			std::ostringstream buf;
			buf << boost::format( "kexp%d" ) % i++;

			TiXmlElement kExpNode( buf.str() );
			kExpNode.SetAttribute( "type", "double" );
			kExpNode.SetAttribute( "value", ::ToString( ::ToWString( val ) ) );
			kExpNodeType->InsertEndChild( kExpNode );
		}
	}

	TiXmlElement *kChunkXGammaExp = new TiXmlElement("ChunkXGammaExposure" );	
	GetResultObject()->LinkEndChild( kChunkXGammaExp );

	for each( const KeyResultMap::value_type& v in m_resInfoForChunkXGammaExp )
	{
		TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString( v.first ) );
		kChunkXGammaExp->LinkEndChild( kExpNodeType );

		int i = 0;
		for each( Real val in v.second )
		{
			std::ostringstream buf;
			buf << boost::format( "kexp%d" ) % i++;

			TiXmlElement kExpNode( buf.str() );
			kExpNode.SetAttribute( "type", "double" );
			kExpNode.SetAttribute( "value", ::ToString( ::ToWString( val ) ) );
			kExpNodeType->InsertEndChild( kExpNode );
		}
	}

	TiXmlElement *kChunkXCurveGammaExp = new TiXmlElement("ChunkXCurveGammaExposure" );	
	GetResultObject()->LinkEndChild( kChunkXCurveGammaExp );

	for each( const KeyResultMap::value_type& v in m_resInfoForChunkXCurveGammaExp )
	{
		TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString( v.first ) );
		kChunkXCurveGammaExp->LinkEndChild( kExpNodeType );

		int i = 0;
		for each( Real val in v.second )
		{
			std::ostringstream buf;
			buf << boost::format( "kexp%d" ) % i++;

			TiXmlElement kExpNode( buf.str() );
			kExpNode.SetAttribute( "type", "double" );
			kExpNode.SetAttribute( "value", ::ToString( ::ToWString( val ) ) );
			kExpNodeType->InsertEndChild( kExpNode );
		}
	}

	TiXmlElement *kVegaExpNodeTitle = new TiXmlElement("VegaExposure" );
	GetResultObject()->LinkEndChild( kVegaExpNodeTitle );

	for each( const KeyResultMap::value_type& v in m_resInfoForKeyVegaExp )
	{
		TiXmlElement *kExpNodeType = new TiXmlElement( ::ToString( v.first ) );
		kVegaExpNodeTitle->LinkEndChild( kExpNodeType );

		std::vector<Size> tenorVal = m_curves[ v.first ].first->GetYieldCurveWrapper()->GetVegaTenors( GetRemainingTime() );
		
		TiXmlElement kShiftExpNode( "kexpShift" );
		kShiftExpNode.SetAttribute( "type", "double" );
		kShiftExpNode.SetAttribute( "value", ::ToString( ::ToWString( m_keyVegaExpShift[ v.first ] ) ) );
		kExpNodeType->InsertEndChild( kShiftExpNode );

		int i = 0;
		for each( Real val in v.second )
		{
			std::ostringstream buf;
			buf << boost::format( "kexp%d" ) % tenorVal[ i++ ];

			TiXmlElement kExpNode( buf.str() );
			kExpNode.SetAttribute( "type", "double" );
			kExpNode.SetAttribute( "value", ::ToString( ::ToWString( val ) ) );
			kExpNodeType->InsertEndChild( kExpNode );
		}
	}

	TiXmlElement curveInfo( "curves" );
	for each( const YCurveMap::value_type& v in m_curves )
	{
		v.second.first->GetYieldCurveWrapper()->ParseCurveInfo( curveInfo, GetRemainingTime() );
	}

	GetResultObject()->InsertEndChild( curveInfo );
}

boost::shared_ptr<RemoteXMLJob> IRProductParam::CreateJob()
{
	TiXmlElement curveRoot( "curve_root" );
	for each( const YCurveMap::value_type& v in m_curves )
	{
		curveRoot.InsertEndChild( *v.second.first->ParseToXML( GetRemainingTime() ) );
	}

	boost::shared_ptr<RemoteXMLJob> res( new RemoteXMLJob( *GetRecord(), curveRoot ) );
	CalculationProxy::instance().AddJob( res );
	return res;
}

void IRProductParam::AddCurve( const std::wstring& curveName, boost::shared_ptr<YieldCurveInfoWrapperProxy> curve, bool hasVega )
{
	YCurveMap::mapped_type& val = m_curves[ curveName ];
	if( !val.first )
	{
		val.second = hasVega;
	}
	else
	{
		val.second = val.second || hasVega;
	}

	val.first = curve;	
}

