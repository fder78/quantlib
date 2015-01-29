#pragma once

#include "structuredproduct_ir.h"
#include "IRProductParam.h"


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

class RAKAPParam : public IRProductParam
{
public:
	RAKAPParam();

private:
	void ApplyCurveInfo();
	virtual void SetDataImpl( TiXmlElement* record ) override;

	void LoadSwapInfo( const TiXmlElement* record );
	void LoadCurveCorrInfo( const TiXmlElement* record );
	void LoadCallPutInfo( const TiXmlElement* record );
	void LoadReceiveScheduleInfo( const TiXmlElement* record );
	void LoadBondInfo( const TiXmlElement* record );
	void LoadValuationInfo( const TiXmlElement* record );


	virtual ResultInfo DoCalculation() override;
	virtual Real GetBiasForProduct() const override { return m_biasForScenario; }

	virtual Period GetRemainingTime() const override;

	virtual std::vector<std::pair<Date, Real> > GetCashFlow() const override;

protected:
	std::string m_evalDate;
	std::string m_effectiveDate;
	std::string m_terminationDate;

	std::string m_firstDate;

	std::string m_nextToLastDate;

	std::string m_callStartDate;
	std::string m_callEndDate;

	boost::shared_ptr<ValuationInfo> m_valuationInfo;
	boost::shared_ptr<RangeAccrualBondInfo> m_bondInfo;
	boost::shared_ptr<ScheduleInfo> m_scheduleInfo;
	boost::shared_ptr<CallPutInfo> m_callPutInfo;
	boost::shared_ptr<CurveCorrelationInfo> m_curveCorrInfo;

	boost::shared_ptr<InterestRateCurveInfoWrapper> m_paymentCurveWrapper;

	boost::shared_ptr<InterestRateCurveInfoWrapper> m_obs1Curve1Wrapper;
	boost::shared_ptr<InterestRateCurveInfoWrapper> m_obs1Curve2Wrapper;

	boost::scoped_array<double> m_lowerTrigger1;
	boost::scoped_array<double> m_upperTrigger1;

	boost::shared_ptr<InterestRateCurveInfoWrapper> m_obs2Curve1Wrapper;
	boost::shared_ptr<InterestRateCurveInfoWrapper> m_obs2Curve2Wrapper;

	boost::scoped_array<double> m_lowerTrigger2;
	boost::scoped_array<double> m_upperTrigger2;

	boost::scoped_array<double> m_gearing;
	boost::scoped_array<double> m_spread;

	boost::scoped_array<YieldCurveInfo*> m_curveInfos;
	boost::scoped_array<boost::scoped_array<Real> > m_corrMat;
	boost::scoped_array<Real*> m_corrMatData;
	boost::scoped_array<HwParams> m_hwParams;

	boost::shared_ptr<RangeAccrualSwapInfo> m_swapInfo;
	boost::shared_ptr<InterestRateCurveInfoWrapper> m_payerRfInfo;
	boost::shared_ptr<CallableBondInfo> m_callableBondInfo;

	Real m_payerFixedCoupon;

	boost::shared_ptr<InterestRateCurveInfoWrapper> m_floatingCurve;
	Real m_payerFloatingCouponSpread;
	Real m_payerFloatingFloor;
	Real m_payerFloatingCap;

	boost::shared_ptr<ScheduleInfo> m_payerScheduleInfo;
	std::string m_payerEffectiveDate;
	std::string m_payerTerminationDate;

	std::string m_payerFirstDate;
	std::string m_payerNextToLastDate;

	typedef std::map<std::wstring, boost::shared_ptr<InterestRateCurveInfoWrapper> > CurveMap;
	CurveMap m_curves;
};