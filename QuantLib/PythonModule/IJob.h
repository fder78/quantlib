#pragma once

class IJob
{
public:
	IJob() : m_isCalculated( false ) { }
	virtual ~IJob() { }

	void CreateThread();
	virtual void WaitForTerminate();

	void Calculate()
	{
		m_isCalculated = true;
		FetchResult();
	}

	bool IsCalculated() const;

protected:
	virtual void FetchResult() = 0;

private:
	bool m_isCalculated;
	HANDLE m_thread;
};