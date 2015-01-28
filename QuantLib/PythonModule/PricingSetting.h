#pragma once

#include "PerProcessSingleton.h"

class PricingSetting : public PerProcessSingleton<PricingSetting>
{
public:
	void Init( const Date& evalDate );

	Date GetEvaluationDate() const { return m_evaluationDate; }
	
private:
	Date m_evaluationDate;
};