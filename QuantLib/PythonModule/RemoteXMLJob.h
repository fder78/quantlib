#pragma once

#include "IJob.h"

class RemoteXMLJob : public IJob
{
public:
	RemoteXMLJob( const TiXmlElement& data, const TiXmlElement& curveRoot );
	const TiXmlElement& GetResult() const { return *m_result; }

	void SetResult( const std::string& resStr );

	const std::string& GetCode() const { return m_code; }
	const std::string& GetParamStr() const { return m_paramStr; }

	virtual void WaitForTerminate() override;

private:
	virtual void FetchResult() override;

private:
	TiXmlElement m_data;
	TiXmlElement m_curveRoot;
	TiXmlElement m_calcRoot;
	boost::shared_ptr<TiXmlElement> m_result;

	std::string m_code;
	std::string m_paramStr;

	HANDLE m_event;
	HANDLE m_terminateEvent;
	std::string m_resStr;
};
