#pragma once

#include "IJob.h"

class IRProductParam;
class RemoteXMLJob;

class IRProductJob : public IJob
{
private:
	struct XGammaJob
	{
		XGammaJob( boost::shared_ptr<RemoteXMLJob> ppdJob, boost::shared_ptr<RemoteXMLJob> pmdJob, boost::shared_ptr<RemoteXMLJob> mpdJob, boost::shared_ptr<RemoteXMLJob> mmdJob )
			: ppdJob( ppdJob )
			, pmdJob( pmdJob )
			, mpdJob( mpdJob )
			, mmdJob( mmdJob )
		{
		}

		boost::shared_ptr<RemoteXMLJob> ppdJob;
		boost::shared_ptr<RemoteXMLJob> pmdJob;
		boost::shared_ptr<RemoteXMLJob> mpdJob;
		boost::shared_ptr<RemoteXMLJob> mmdJob;
	};

public:
	IRProductJob( IRProductParam* param );

	void SetResJob( boost::shared_ptr<RemoteXMLJob> job );
	void SetPDResJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> job );
	void SetMDResJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> job );
	void SetKeyJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> pdJob, boost::shared_ptr<RemoteXMLJob> mdJob );

	void SetKeyXGammaJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> ppdJob, boost::shared_ptr<RemoteXMLJob> pmdJob, boost::shared_ptr<RemoteXMLJob> mpdJob, boost::shared_ptr<RemoteXMLJob> mmdJob );

	void SetPVegaResJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> job );
	void SetMVegaResJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> job );
	void SetKeyVegaJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> pdJob, boost::shared_ptr<RemoteXMLJob> mdJob );

	void SetChunkKeyJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> pdJob, boost::shared_ptr<RemoteXMLJob> mdJob );
	void SetChunkKeyXGammaJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> ppdJob, boost::shared_ptr<RemoteXMLJob> pmdJob, boost::shared_ptr<RemoteXMLJob> mpdJob, boost::shared_ptr<RemoteXMLJob> mmdJob );
	void SetChunkKeyXCurveGammaJob( const std::wstring& curveName, boost::shared_ptr<RemoteXMLJob> ppdJob, boost::shared_ptr<RemoteXMLJob> pmdJob, boost::shared_ptr<RemoteXMLJob> mpdJob, boost::shared_ptr<RemoteXMLJob> mmdJob );

private:
	virtual void FetchResult();
	static Real ExtractNPV( const RemoteXMLJob& job );

private:
	IRProductParam* m_param;

	boost::shared_ptr<RemoteXMLJob> m_resJob;
	typedef std::map<std::wstring, boost::shared_ptr<RemoteXMLJob> > ResJobMap;
	typedef std::map<std::wstring, std::vector<std::pair<boost::shared_ptr<RemoteXMLJob>, boost::shared_ptr<RemoteXMLJob> > > > KeyJobMap;

	ResJobMap m_respdJob;
	ResJobMap m_resmdJob;
	KeyJobMap m_keyJobMap;

	typedef std::map<std::wstring, std::vector<boost::shared_ptr<XGammaJob> > > KeyXJobMap;
	KeyXJobMap m_keyXGammaJobMap;

	KeyJobMap m_keyChunkJobMap;
	KeyXJobMap m_keyChunkXGammaJobMap;
	KeyXJobMap m_keyChunkXCurveGammaJobMap;

	ResJobMap m_respVegaJob;
	ResJobMap m_resmVegaJob;
	KeyJobMap m_keyVegaJobMap;

	typedef std::vector<boost::shared_ptr<RemoteXMLJob> > JobVec;
	JobVec m_jobs;
};