#pragma once

#include "PerProcessSingleton.h"

class PricingSetting : public PerProcessSingleton<PricingSetting>
{
public:
	PricingSetting();

	void Init( const Date& evalDate, int dataDateAlias );

	Date GetEvaluationDate() const { return m_evaluationDate; }

	void SetCalcGreek( bool calcGreek ) { m_calcGreek = calcGreek; }
	bool CalcGreek() const { return m_calcGreek; }

	void SetShowDetail( bool showDetail ) { m_showDetail = showDetail; }
	bool ShowDetail() const { return m_showDetail; }

	void SetUseProxy( bool useProxy, const std::string& address )
	{
		m_useProxy = useProxy;
		m_proxyAddress = address;
	}
	bool UseProxy() const { return m_useProxy; }
	const std::string& GetProxyAddress() { return m_proxyAddress; }

	std::string GetOutputFileName() const { return m_outputFileName; }
	void SetOutputFileName( const std::string& outputFileName ) { m_outputFileName = outputFileName; }

	int GetDataDateAlias() const { return m_dataDateAlias; }
	Date GetDataDate() const;
	
private:
	Date m_evaluationDate;
	int m_dataDateAlias;

	bool m_calcGreek;
	bool m_useProxy;
	bool m_showDetail;

	std::string m_outputFileName;
	std::string m_proxyAddress;

	bool m_isHolidaysUpdated;
};
