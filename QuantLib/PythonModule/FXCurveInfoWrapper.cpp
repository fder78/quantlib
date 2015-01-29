#include "StdAfx.h"

#include "FXCurveInfoWrapper.h"
#include "InterestRateCurveInfoWrapper.h"

#include "PricingSetting.h"
#include "StringUtil.h"

#include "CurveTable.h"

#include "yield_builder.hpp"

FXCurveInfoWrapper::FXCurveInfoWrapper( Real spot, const std::wstring& curveName, boost::shared_ptr<InterestRateCurveInfoWrapper> irs, boost::shared_ptr<InterestRateCurveInfoWrapper> crs, Real fxVol, boost::shared_ptr<FXCurveData> fxCurveData )
	: YieldCurveInfoWrapper( curveName )
	, m_spot( spot )
	, m_irs( build_yield_curve( *irs->GetCurveData() ) )
	, m_crs( build_yield_curve( *crs->GetCurveData() ) )
	, m_irsWrapper( irs )
	, m_crsWrapper( crs )
	, m_vol( fxVol )
	, m_tenorCount( crs->GetCurveData()->yields.size() )
	, m_curveData( fxCurveData )
{
}

boost::shared_ptr<StochasticProcess1D> FXCurveInfoWrapper::GetStochasticProcess( Period tenor ) const 
{
	return boost::shared_ptr<StochasticProcess1D>(
		new 
		BSM_QuantoAdjusted_Process(Handle<Quote>(new SimpleQuote( 100. ) ), 
		Handle<YieldTermStructure>( m_irs ),
		Handle<YieldTermStructure>( m_crs ),
		Handle<BlackVolTermStructure>( new BlackConstantVol( PricingSetting::instance().GetEvaluationDate(), NullCalendar(), m_vol, Actual365Fixed() ) ),
		0., 0.
		)
		);
}

boost::shared_ptr<TiXmlElement> FXCurveInfoWrapper::ParseToXML( Period tenor ) const 
{
	boost::shared_ptr<TiXmlElement> res( new TiXmlElement( ::ToString( GetCurveName() ) ) );

	TiXmlElement maturity( "Maturity" );
	TiXmlElement spot( "Spot" ); 

	for each( const Date& date in m_curveData->fwdDate )
	{
		TiXmlElement record( "record" );
		record.SetAttribute( "type", "date" );
		record.SetAttribute( "value", ::ConvertToBloombergDate( date ) );

		maturity.InsertEndChild( record );
	}

	for each( Real yield in m_curveData->fwdValue )
	{
		TiXmlElement record( "record" );
		record.SetAttribute( "type", "double" );
		record.SetAttribute( "value", ::ToString( yield ) );

		spot.InsertEndChild( record );
	}

	res->InsertEndChild( maturity );
	res->InsertEndChild( spot );

	TiXmlElement vol( "Vol" );
	vol.SetAttribute( "type", "double" );
	vol.SetAttribute( "value", ::ToString( m_vol ) );

	res->InsertEndChild( vol );

	res->SetAttribute( "type", "string" );
	res->SetAttribute( "value", "FX" );

	TiXmlElement irs( "irs" );
	irs.InsertEndChild( *m_irsWrapper->ParseToXML( tenor ) );

	TiXmlElement crs( "crs" );
	crs.InsertEndChild( *m_crsWrapper->ParseToXML( tenor ) );

	res->InsertEndChild( irs );
	res->InsertEndChild( crs );

	return res;
}

void FXCurveInfoWrapper::ParseCurveInfo( TiXmlElement& parent, Period remainingTime )
{
}

int FXCurveInfoWrapper::GetChunkCount( Period tenor ) const 
{
	int maxChunk = 0;
	int totalChunk = 0;
	for( Size i = 0; i < m_curveData->fwdDate.size(); i++ )
	{
		const Date& dt = m_curveData->fwdDate[ i ];
		if( ( Actual365Fixed().dayCount( PricingSetting::instance().GetEvaluationDate(), dt ) * Days ) <= tenor )
		{
			maxChunk = std::max<int>( maxChunk, m_curveData->chunkIndex[ i ] );
		}

		totalChunk = std::max<int>( totalChunk, m_curveData->chunkIndex[ i ] );
	}

	maxChunk = std::min<int>( maxChunk + 2, totalChunk + 1 );

	return maxChunk;
}
