#pragma once

#include "RAUsingProcessParam.h"

class RAFDMParam : public RAUsingProcessParam
{
public:
	virtual void SetDataImpl( TiXmlElement* record ) override;

private:
	virtual ResultInfo DoCalculation() override;

protected:
	int m_tGrid;
	int m_rGrid;

private:
	Real m_obs1FXVol;
	Real m_obs1FXCorr;
	Real m_obs2FXVol;
	Real m_obs2FXCorr;
};