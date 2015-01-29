#pragma once

#include "IProductParam.h"
#include "structuredproduct_ir.h"

struct ValuationInfo;
struct RangeAccrualBondInfo;
struct ScheduleInfo;
class YieldCurveInfoWrapper;
class InterestRateCurveInfoWrapper;
struct CurveCorrelationInfo;
struct CallPutInfo;
struct HwParams;
struct YieldCurveInfo;
class YieldCurveInfoWrapperProxy;
class RemoteXMLJob;
class IRProductJob;

class IRProductParam : public IProductParam
{
public:
	friend class IRProductJob;
	IRProductParam();

	virtual void Calculate() override;
	virtual void FetchResult() override;

	void AddCurve( const std::wstring& curveName, boost::shared_ptr<YieldCurveInfoWrapperProxy> curve, bool hasVega );

private:
	boost::shared_ptr<RemoteXMLJob> CreateJob();

	virtual ResultInfo DoCalculation() = 0;
	virtual Real GetBiasForProduct() const = 0;

	virtual Period GetRemainingTime() const = 0;

	void CalcWithProxy();
	void CalcWithoutProxy();

	bool CalcDelta() const { return m_calcDelta; }
	bool CalcVega() const { return m_calcVega; }
	bool CalcXGamma() const { return m_calcXGamma; }
	bool CalcChunkDelta() const { return m_calcChunkDelta; }

	virtual std::vector<std::pair<Date, Real> > GetCashFlow() const = 0;

protected:
	Real m_biasForScenario;
	Real m_notional;

	boost::shared_ptr<YieldCurveInfoWrapperProxy> m_discountCurveInfo;

	bool m_isSwap;
	typedef std::map<std::wstring, std::vector<Real> > KeyResultMap;
	typedef std::map<std::wstring, Real> ShiftResultMap;

	Real m_npv;

	ShiftResultMap m_keyExpShift;
	KeyResultMap m_resInfoForKeyExp;
	
	ShiftResultMap m_keyGammaExpShift;
	KeyResultMap m_resInfoForKeyGammaExp;
	KeyResultMap m_resInfoForKeyXGammaExp;

	KeyResultMap m_resInfoForChunkDeltaExp;
	KeyResultMap m_resInfoForChunkGammaExp;
	KeyResultMap m_resInfoForChunkXGammaExp;
	KeyResultMap m_resInfoForChunkXCurveGammaExp;

	ShiftResultMap m_keyVegaExpShift;
	KeyResultMap m_resInfoForKeyVegaExp;

	bool m_calcDelta;
	bool m_calcVega;
	bool m_calcXGamma;

	bool m_calcChunkDelta;

private:
	typedef std::map<std::wstring, std::pair<boost::shared_ptr<YieldCurveInfoWrapperProxy>, bool> > YCurveMap;
	YCurveMap m_curves;
};

double GetDeltaStep( double dbFlag );