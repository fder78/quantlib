#include "StdAfx.h"

#include "IRProductJob.h"

#include "IRProductParam.h"

#include "PricingSetting.h"
#include "YieldCurveInfoWrapper.h"
#include "YieldCurveInfoWrapperProxy.h"
#include "RemoteXMLJob.h"

#include "XMLValue.h"

IRProductJob::IRProductJob( IRProductParam* param )
	: m_param( param )
{
}

void IRProductJob::SetResJob( boost::shared_ptr<RemoteXMLJob> job )
{
	m_resJob = job;
	m_jobs.push_back( job );
}

void IRProductJob::SetPDResJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> job )
{
	m_respdJob[ curveName ] = job;
	m_jobs.push_back( job );
}

void IRProductJob::SetMDResJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> job )
{
	m_resmdJob[ curveName ] = job;
	m_jobs.push_back( job );
}

void IRProductJob::SetKeyJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> pdJob, boost::shared_ptr<RemoteXMLJob> mdJob )
{
	m_keyJobMap[ curveName ].push_back( std::make_pair( pdJob, mdJob ) );
	m_jobs.push_back( pdJob );
	m_jobs.push_back( mdJob );
}

void IRProductJob::SetKeyXGammaJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> ppdJob, boost::shared_ptr<RemoteXMLJob> pmdJob, boost::shared_ptr<RemoteXMLJob> mpdJob, boost::shared_ptr<RemoteXMLJob> mmdJob )
{
	m_keyXGammaJobMap[ curveName ].push_back( boost::shared_ptr<XGammaJob>( new XGammaJob( ppdJob, pmdJob, mpdJob, mmdJob ) ) );

	m_jobs.push_back( ppdJob );
	m_jobs.push_back( pmdJob );
	m_jobs.push_back( mpdJob );
	m_jobs.push_back( mmdJob );
}

void IRProductJob::SetChunkKeyJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> pdJob, boost::shared_ptr<RemoteXMLJob> mdJob )
{
	m_keyChunkJobMap[ curveName ].push_back( std::make_pair( pdJob, mdJob ) );
	m_jobs.push_back( pdJob );
	m_jobs.push_back( mdJob );
}

void IRProductJob::SetChunkKeyXGammaJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> ppdJob, boost::shared_ptr<RemoteXMLJob> pmdJob, boost::shared_ptr<RemoteXMLJob> mpdJob, boost::shared_ptr<RemoteXMLJob> mmdJob )
{
	m_keyChunkXGammaJobMap[ curveName ].push_back( boost::shared_ptr<XGammaJob>( new XGammaJob( ppdJob, pmdJob, mpdJob, mmdJob ) ) );

	m_jobs.push_back( ppdJob );
	m_jobs.push_back( pmdJob );
	m_jobs.push_back( mpdJob );
	m_jobs.push_back( mmdJob );
}

void IRProductJob::SetChunkKeyXCurveGammaJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> ppdJob, boost::shared_ptr<RemoteXMLJob> pmdJob, boost::shared_ptr<RemoteXMLJob> mpdJob, boost::shared_ptr<RemoteXMLJob> mmdJob )
{
	m_keyChunkXCurveGammaJobMap[ curveName ].push_back( boost::shared_ptr<XGammaJob>( new XGammaJob( ppdJob, pmdJob, mpdJob, mmdJob ) ) );

	m_jobs.push_back( ppdJob );
	m_jobs.push_back( pmdJob );
	m_jobs.push_back( mpdJob );
	m_jobs.push_back( mmdJob );
}

void IRProductJob::SetPVegaResJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> job )
{
	m_respVegaJob[ curveName ] = job;
	m_jobs.push_back( job );
}

void IRProductJob::SetMVegaResJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> job )
{
	m_resmVegaJob[ curveName ] = job;
	m_jobs.push_back( job );
}

void IRProductJob::SetKeyVegaJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> pdJob, boost::shared_ptr<RemoteXMLJob> mdJob )
{
	m_keyVegaJobMap[ curveName ].push_back( std::make_pair( pdJob, mdJob ) );
	m_jobs.push_back( pdJob );
	m_jobs.push_back( mdJob );
}

void IRProductJob::FetchResult()
{
	for each( boost::shared_ptr<RemoteXMLJob> job in m_jobs )
	{
		job->WaitForTerminate();
	}

	m_param->m_npv = ExtractNPV( *m_resJob );

	if( !PricingSetting::instance().CalcGreek() )
	{
		return;
	}

	double deltastep;
	double dbFlag;

	dbFlag = m_param->m_discountCurveInfo->GetYieldCurveWrapper()->GetSpotValue();
	deltastep = GetDeltaStep( dbFlag );

	if( m_param->CalcChunkDelta() )
	{
		for each( const KeyXJobMap::value_type& v in m_keyChunkXCurveGammaJobMap )
		{
			const KeyXJobMap::mapped_type& xCurveGammaVec = v.second;
			for each( boost::shared_ptr<XGammaJob> xGammaJob in xCurveGammaVec )
			{
				// double keyExposures = ( ExtractNPV( *pdmd.first ) - ExtractNPV( *m_resJob ) );
				double keyExposures = ( ExtractNPV( *xGammaJob->ppdJob ) - ExtractNPV( *xGammaJob->pmdJob ) - ExtractNPV( *xGammaJob->mpdJob ) + ExtractNPV( *xGammaJob->mmdJob ) ) / 4. / deltastep / deltastep / 10000. / 10000.;
				m_param->m_resInfoForChunkXCurveGammaExp[ v.first ].push_back( keyExposures );
			}
		}
	}

	for each( const IRProductParam::YCurveMap::value_type& v in m_param->m_curves )
	{
		boost::shared_ptr<YieldCurveInfoWrapper> curveWrapper = v.second.first->GetYieldCurveWrapper();
		deltastep = ::GetDeltaStep( curveWrapper->GetSpotValue() );

		std::wstring curveName = v.second.first->GetCurveName();
		
		if( m_param->CalcDelta() )
		{
			m_param->m_keyExpShift[ curveName ] = ( ExtractNPV( *m_respdJob[ curveName ] ) - ExtractNPV( *m_resmdJob[ curveName ] ) ) / 2 / deltastep / 10000.;
			m_param->m_keyGammaExpShift[ curveName ] = ( ExtractNPV( *m_respdJob[ curveName ] ) + ExtractNPV( *m_resmdJob[ curveName ] ) - 2. * m_param->m_npv ) / deltastep / deltastep / 10000. / 10000.;
			const KeyJobMap::mapped_type& vec = m_keyJobMap[ curveName ];
			for each( KeyJobMap::mapped_type::value_type pdmd in vec )
			{
				double keyExposures = ( ExtractNPV( *pdmd.first ) - ExtractNPV( *pdmd.second ) ) / 2 / deltastep / 10000.;
				m_param->m_resInfoForKeyExp[ curveName ].push_back( keyExposures );
			}
			
			for each( KeyJobMap::mapped_type::value_type pdmd in vec )
			{
				double keyExposures = ( ExtractNPV( *pdmd.first ) + ExtractNPV( *pdmd.second ) - 2. * m_param->m_npv ) / deltastep / deltastep / 10000. / 10000.;
				m_param->m_resInfoForKeyGammaExp[ curveName ].push_back( keyExposures );
			}
		}

		if( m_param->CalcChunkDelta() )
		{
			const KeyJobMap::mapped_type& vec = m_keyChunkJobMap[ curveName ];
			for each( KeyJobMap::mapped_type::value_type pdmd in vec )
			{
				double keyExposures = ( ExtractNPV( *pdmd.first ) - ExtractNPV( *pdmd.second ) ) / 2 / deltastep / 10000.;
				m_param->m_resInfoForChunkDeltaExp[ curveName ].push_back( keyExposures );
			}

			for each( KeyJobMap::mapped_type::value_type pdmd in vec )
			{
				double keyExposures = ( ExtractNPV( *pdmd.first ) + ExtractNPV( *pdmd.second ) - 2. * m_param->m_npv ) / deltastep / deltastep / 10000. / 10000.;
				m_param->m_resInfoForChunkGammaExp[ curveName ].push_back( keyExposures );
			}

			const KeyXJobMap::mapped_type& xGammaVec = m_keyChunkXGammaJobMap[ curveName ];
			for each( boost::shared_ptr<XGammaJob> xGammaJob in xGammaVec )
			{
				// double keyExposures = ( ExtractNPV( *pdmd.first ) - ExtractNPV( *m_resJob ) );
				double keyExposures = ( ExtractNPV( *xGammaJob->ppdJob ) - ExtractNPV( *xGammaJob->pmdJob ) - ExtractNPV( *xGammaJob->mpdJob ) + ExtractNPV( *xGammaJob->mmdJob ) ) / 4. / deltastep / deltastep / 10000. / 10000.;
				m_param->m_resInfoForChunkXGammaExp[ curveName ].push_back( keyExposures );
			}
		}

		if( m_param->CalcVega() && v.second.second )
		{
			m_param->m_keyVegaExpShift[ curveName ] = ( ExtractNPV( *m_respVegaJob[ curveName ] ) - ExtractNPV( *m_resmVegaJob[ curveName ] ) ) / 2. / 0.001 / 100.;
			const KeyJobMap::mapped_type& vegavec = m_keyVegaJobMap[ curveName ];
			for each( KeyJobMap::mapped_type::value_type pdmd in vegavec )
			{
				// double keyExposures = ( ExtractNPV( *pdmd.first ) - ExtractNPV( *m_resJob ) );
				double keyExposures = ( ExtractNPV( *pdmd.first ) - ExtractNPV( *pdmd.second ) ) / 0.001 / 100.;
				m_param->m_resInfoForKeyVegaExp[ curveName ].push_back( keyExposures );
			}

			// vegaSum은 위쪽만, shiftvega는 위아래가 반영된 것이기 때문에 조금 더 정확한 위아래 vega값으로 key rate vega를 보정해준다.
			Real vegaSum = m_param->m_resInfoForKeyVegaExp[ curveName ][ 0 ];
			Real shiftVega = m_param->m_keyVegaExpShift[ curveName ];
			Real ratio = 0.5; //shiftVega / vegaSum;
			if( ratio > 0. )
			{
				for( int i = 0; i < static_cast<int>( vegavec.size() ) - 1; i++ )
				{
					m_param->m_resInfoForKeyVegaExp[ curveName ][ i ] -= m_param->m_resInfoForKeyVegaExp[ curveWrapper->GetCurveName() ][ i + 1 ];
					m_param->m_resInfoForKeyVegaExp[ curveName ][ i ] *= ratio;
				}

				m_param->m_resInfoForKeyVegaExp[ curveName ].back() *= ratio;
			}
		}

		if( m_param->CalcXGamma() )
		{
			const KeyXJobMap::mapped_type& xGammaVec = m_keyXGammaJobMap[ curveName ];
			for each( boost::shared_ptr<XGammaJob> xGammaJob in xGammaVec )
			{
				// double keyExposures = ( ExtractNPV( *pdmd.first ) - ExtractNPV( *m_resJob ) );
				double keyExposures = ( ExtractNPV( *xGammaJob->ppdJob ) - ExtractNPV( *xGammaJob->pmdJob ) - ExtractNPV( *xGammaJob->mpdJob ) + ExtractNPV( *xGammaJob->mmdJob ) ) / 4. / deltastep / deltastep / 10000. / 10000.;
				m_param->m_resInfoForKeyXGammaExp[ curveName ].push_back( keyExposures );
			}
		}
	}
}

QuantLib::Real IRProductJob::ExtractNPV( const RemoteXMLJob& job )
{
	return XMLValue( job.GetResult().FirstChildElement(), "npv" );
}
