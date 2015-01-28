#pragma once

#include "IProductParam.h"
#include "structuredproduct_ir.h"

struct ValuationInfo;
struct RangeAccrualBondInfo;
struct ScheduleInfo;
class YieldCurveInfoWrapper;
struct CurveCorrelationInfo;
struct CallPutInfo;
struct HwParams;
struct YieldCurveInfo;

enum E_CallTenor
{
	CT_Annually = 12,
	CT_SemiAnnually = 6,
	CT_Quarterly = 3,
	CT_Once = -1,
};

class RangeAccrualBondParam : public IProductParam
{
public:
	RangeAccrualBondParam();

	virtual void Calculate() override;

private:
	virtual void SetDataImpl( const TiXmlElement* record ) override;

	void LoadSwapInfo( const TiXmlElement* record );

	void LoadCurveCorrInfo( const TiXmlElement* record );

	void LoadCallPutInfo( const TiXmlElement* record );

	void LoadReceiveScheduleInfo( const TiXmlElement* record );

	void LoadBondInfo( const TiXmlElement* record );
	void LoadValuationInfo( const TiXmlElement* record );

private:
	std::string m_evalDate;
	std::string m_effectiveDate;
	std::string m_terminationDate;

	std::string m_firstDate;

	std::string m_nextToLastDate;

	std::string m_callStartDate;
	std::string m_callEndDate;

	Real m_notional;

	boost::shared_ptr<ValuationInfo> m_valuationInfo;
	boost::shared_ptr<RangeAccrualBondInfo> m_bondInfo;
	boost::shared_ptr<ScheduleInfo> m_scheduleInfo;
	boost::shared_ptr<CallPutInfo> m_callPutInfo;
	boost::shared_ptr<YieldCurveInfoWrapper> m_rfCurveInfo;
	boost::shared_ptr<CurveCorrelationInfo> m_curveCorrInfo;

	boost::shared_ptr<YieldCurveInfoWrapper> m_paymentCurveWrapper;

	boost::shared_ptr<YieldCurveInfoWrapper> m_obs1Curve1Wrapper;
	boost::shared_ptr<YieldCurveInfoWrapper> m_obs1Curve2Wrapper;

	boost::scoped_array<double> m_lowerTrigger1;
	boost::scoped_array<double> m_upperTrigger1;

	boost::shared_ptr<YieldCurveInfoWrapper> m_obs2Curve1Wrapper;
	boost::shared_ptr<YieldCurveInfoWrapper> m_obs2Curve2Wrapper;

	boost::scoped_array<double> m_lowerTrigger2;
	boost::scoped_array<double> m_upperTrigger2;

	boost::scoped_array<double> m_gearing;
	boost::scoped_array<double> m_spread;

	typedef std::set<boost::shared_ptr<YieldCurveInfoWrapper> > CurveSet;
	CurveSet m_curves;
	
	boost::scoped_array<YieldCurveInfo*> m_curveInfos;
	boost::scoped_array<boost::scoped_array<Real> > m_corrMat;
	boost::scoped_array<Real*> m_corrMatData;
	boost::scoped_array<HwParams> m_hwParams;

	bool m_isSwap;
	boost::shared_ptr<RangeAccrualSwapInfo> m_swapInfo;
	boost::shared_ptr<YieldCurveInfoWrapper> m_payerRfInfo;
	boost::shared_ptr<CallableBondInfo> m_callableBondInfo;

	Real m_payerFixedCoupon;

	boost::shared_ptr<YieldCurveInfoWrapper> m_floatingCurve;
	Real m_payerFloatingCouponSpread;
	Real m_payerFloatingFloor;
	Real m_payerFloatingCap;

	boost::shared_ptr<ScheduleInfo> m_payerScheduleInfo;
	std::string m_payerEffectiveDate;
	std::string m_payerTerminationDate;

	std::string m_payerFirstDate;
	std::string m_payerNextToLastDate;
};