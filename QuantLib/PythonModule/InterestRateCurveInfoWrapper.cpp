#include "StdAfx.h"

#include "InterestRateCurveInfoWrapper.h"

#include "pricing_functions/single_hull_white_calibration.hpp"
#include "pricing_functions/hull_white_calibration.hpp"

#include "CurveTable.h"
#include "StringUtil.h"
#include "PricingSetting.h"

#include "structuredproduct_ir.h"

#include "yield_builder.hpp"

#include "XMLStream.h"

#include "CurveTableUtil.h"

InterestRateCurveInfoWrapper::InterestRateCurveInfoWrapper( const std::wstring& curveName, boost::shared_ptr<YieldCurveData> curveData, const ShiftOption& opt )
	: YieldCurveInfoWrapper( curveName )
	, m_curveData( curveData )
	, m_shiftOption( opt )
	, m_vegaTenorCount( 0 )
{
	m_daysToMat.reset( new int[ m_curveData->dates.size() ] );
	m_yields.reset( new double[ m_curveData->yields.size() ] );
	m_curveInfo.reset( new YieldCurveInfo() );

	for( size_t i = 0; i < m_curveData->dates.size(); i++ )
	{
		m_daysToMat[ i ] = std::max<int>( m_curveData->dates[ i ].serialNumber() - PricingSetting::instance().GetEvaluationDate().serialNumber(), 1 );
		m_yields[ i ] = m_curveData->yields[ i ];
	}

	m_curveInfo->day = m_daysToMat.get();
	m_curveInfo->rate = m_yields.get();
	m_curveInfo->size = m_curveData->dates.size();
}

YieldCurveInfo* InterestRateCurveInfoWrapper::GetInfo() const
{
	return m_curveInfo.get();
}

void InterestRateCurveInfoWrapper::ShiftCurve( Real delta )
{
	for( int i = 0; i < m_curveInfo->size; i++ )
	{
		const_cast<Real&>( m_curveInfo->rate[ i ] ) += delta;
		const_cast<Real&>( m_curveData->yields[ i ] ) += delta;
	}
}

void InterestRateCurveInfoWrapper::ShiftCurvePerTenor( Real delta, int i )
{
	const_cast<Real&>( m_curveInfo->rate[ i ] ) += delta;
	const_cast<Real&>( m_curveData->yields[ i ] ) += delta;
}

boost::shared_ptr<StochasticProcess1D> InterestRateCurveInfoWrapper::GetStochasticProcess( Period tenor ) const
{
	HullWhiteParameters hwParam( *GetHWParam( tenor ) );
	return boost::shared_ptr<StochasticProcess1D>( new GeneralizedHullWhiteProcess( Handle<YieldTermStructure>( build_yield_curve( *GetCurveData() ) ), hwParam, GetCurveName(), m_shiftOption, tenor ) );
}

boost::shared_ptr<HullWhiteParameters> InterestRateCurveInfoWrapper::GetHWParam( Period tenor ) const
{
	return CurveTable::instance().GetHWParams( GetCurveName(), tenor, m_shiftOption );
}

QuantLib::Real InterestRateCurveInfoWrapper::GetSpotValue() const 
{
	return m_curveData->yields.front();
}

int InterestRateCurveInfoWrapper::GetTenorCount( Period tenor ) const 
{
	int cnt = 0;
	for each( const Date& dt in m_curveData->dates )
	{
		if( ( Actual365Fixed().dayCount( PricingSetting::instance().GetEvaluationDate(), dt ) * Days ) <= tenor )
		{
			cnt++;
		}
	}

	cnt = std::min<int>( cnt + 2, m_curveData->yields.size() );

	return cnt;
}

int InterestRateCurveInfoWrapper::GetChunkCount( Period tenor ) const 
{
	int maxChunk = 0;
	int totalChunk = 0;
	for( Size i = 0; i < m_curveData->dates.size(); i++ )
	{
		const Date& dt = m_curveData->dates[ i ];
		if( ( Actual365Fixed().dayCount( PricingSetting::instance().GetEvaluationDate(), dt ) * Days ) <= tenor )
		{
			maxChunk = std::max<int>( maxChunk, m_curveData->chunkIndex[ i ] );
		}

		totalChunk = std::max<int>( totalChunk, m_curveData->chunkIndex[ i ] );
	}

	maxChunk = std::min<int>( maxChunk + 2, totalChunk + 1 );

	return maxChunk;
}

boost::shared_ptr<TiXmlElement> InterestRateCurveInfoWrapper::ParseToXML( Period tenor ) const 
{
	boost::shared_ptr<TiXmlElement> res( new TiXmlElement( ::ToString( GetCurveName() ) ) );
	res->SetAttribute( "type", "string" );
	res->SetAttribute( "value", "Yield" );

	XMLOStream stream( "ShiftOption" );
	stream << m_shiftOption;
	res->InsertEndChild( *stream.GetResult() );

	XMLOStream stream2( "Tenor" );
	stream2 << tenor.length() << (Integer)( tenor.units() );
	res->InsertEndChild( *stream2.GetResult() );

	return res;
}

std::vector<Size> InterestRateCurveInfoWrapper::GetVegaTenors( Period tenor ) const 
{
	if( boost::shared_ptr<SwaptionVolData> swaptionVolDataOrg = CurveTable::instance().GetSwaptionVolData( GetCurveName(), GetShiftOption() ) )
	{
		return ::GetDataIndexFromSwaption( *swaptionVolDataOrg, tenor );
	}
	else
	{
		boost::shared_ptr<CapVolData> capVolDataOrg = CurveTable::instance().GetCapVolData( GetCurveName(), GetShiftOption() );
		return ::GetDataIndexFromCapVol( *capVolDataOrg, tenor );
	}
}

void InterestRateCurveInfoWrapper::ParseCurveInfo( TiXmlElement& parent, Period remainingTime )
{
	TiXmlElement curve( "curve" );

	TiXmlElement name( "name" );
	name.SetAttribute( "type", "string" );
	name.SetAttribute( "value", ::ToString( ::ToWString( GetCurveName() ) ) );
	curve.InsertEndChild( name );
	
// 	TiXmlElement meanReverse( "a" );
// 	meanReverse.SetAttribute( "type", "double" );
// 	meanReverse.SetAttribute( "value", ::ToString( ::ToWString( GetHWParam( remainingTime )->a ) ) );
// 	curve.InsertEndChild( meanReverse );
// 
// 	TiXmlElement sigma( "sigma" );
// 	sigma.SetAttribute( "type", "double" );
// 	sigma.SetAttribute( "value", ::ToString( ::ToWString( GetHWParam( remainingTime )->sigma ) ) );
// 	curve.InsertEndChild( sigma );
	
	parent.InsertEndChild( curve );
}

ShiftOption InterestRateCurveInfoWrapper::GetShiftOption() const
{
	return m_shiftOption;
}

GeneralizedHullWhiteProcess::GeneralizedHullWhiteProcess( const Handle<YieldTermStructure>& h, const HullWhiteParameters& hwParam, const std::wstring& curveName, ShiftOption so, Period tenor )
	: HullWhiteProcess( h, hwParam.a, hwParam.sigma )
	, m_curveName( curveName )
	, m_shiftOption( so )
	, m_tenor( tenor )
{
}

const HullWhiteTimeDependentParameters& GeneralizedHullWhiteProcess::GetParam()
{
	if( !m_param )
	{
		std::vector<Size> dataIdx;
		if( boost::shared_ptr<SwaptionVolData> swaptionVolDataOrg = CurveTable::instance().GetSwaptionVolData( m_curveName, m_shiftOption ) )
		{
			boost::shared_ptr<SwaptionVolData> volData( new SwaptionVolData( *swaptionVolDataOrg ) );
			volData->maturities.clear();
			volData->lengths.clear();
			volData->vols.clear();
			boost::shared_ptr<InterestRateCurveInfoWrapper> ircw = boost::dynamic_pointer_cast<InterestRateCurveInfoWrapper>( CurveTable::instance().GetYieldCurve( m_curveName, ShiftOption::ShiftNothing ) );
			YieldCurveData curveData = *( ircw->GetCurveData() );
			for( Size i = 0; i < curveData.dates.size(); i++ )
			{
				curveData.dates[ i ] += PricingSetting::instance().GetDataDateAlias();
			}
			boost::shared_ptr<YieldTermStructure> yts = build_yield_curve( curveData );
			
			volData->index = volData->index->clone( Handle<YieldTermStructure>( yts ) );
			dataIdx = ::GetDataIndexFromSwaption( *swaptionVolDataOrg, m_tenor );

			for each( Size idx in dataIdx )
			{
				volData->maturities.push_back( swaptionVolDataOrg->maturities[ idx ] );
				volData->lengths.push_back( swaptionVolDataOrg->lengths[ idx ] );
				volData->vols.push_back( swaptionVolDataOrg->vols[ idx ] );
			}

			volData->initialSigma = std::vector<Real>( volData->vols.size(), 0.005 );
			m_param.reset( new HullWhiteTimeDependentParameters( ::calibration_hull_white( PricingSetting::instance().GetDataDate(), *volData ) ) ); 
			for( Size i = 0; i < m_param->tenor.size(); i++ )
			{
				m_param->tenor[ i ] -= PricingSetting::instance().GetDataDateAlias();
			}
		}
		else
		{
			boost::shared_ptr<CapVolData> capVolDataOrg = CurveTable::instance().GetCapVolData( m_curveName, m_shiftOption );
			CapVolData capVolData( *capVolDataOrg );
			capVolData.tenors.clear();
			capVolData.vols.clear();
			dataIdx = ::GetDataIndexFromCapVol( *capVolDataOrg, m_tenor );
			for each( Size idx in dataIdx )
			{
				capVolData.tenors.push_back( capVolDataOrg->tenors[ idx ] );
				capVolData.vols.push_back( capVolDataOrg->vols[ idx ] );
			}

			m_param.reset( new HullWhiteTimeDependentParameters( ::calibration_hull_white( PricingSetting::instance().GetDataDate(), capVolData ) ) ); 
		}
	}

	return *m_param;
}
