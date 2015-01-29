#pragma once

#include "PerProcessSingleton.h"

#include <xmlrpc-c/girerr.hpp>
#include <xmlrpc-c/client_simple.hpp>

class IProductParam;
class RemoteXMLJob;
class IJob;
class CalculationProxy : public PerProcessSingleton<CalculationProxy>
{
public:
	void Init();
	void AddJob( boost::shared_ptr<IJob> job );

	void RemoteCalculate();

	void FetchResult();

	void AddRemoteJob( RemoteXMLJob* job );

private:
	typedef std::vector<boost::shared_ptr<IJob> > JobVec;
	JobVec m_jobs;

	typedef std::map<std::string, RemoteXMLJob*> RemoteJobMap;
	RemoteJobMap m_remoteJobs;

	int m_jobID;
};
