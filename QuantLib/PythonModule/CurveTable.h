#pragma once

#include "PerProcessSingleton.h"

namespace QuantLib
{
	struct YieldCurveData;
	struct HullWhiteTimeDependentParameters;
}

struct CDSCurveData
{
	std::vector<Period> m_tenors;
	std::vector<Real> m_quoted_spreads;
};

struct FXCurveData
{
	std::vector<Date> m_fwdDate;
	std::vector<Real> m_fwdValue;
};

class YieldCurveInfoWrapper;

class CurveTable : public PerProcessSingleton<CurveTable>
{
private:
	typedef std::map<std::wstring, std::pair<boost::shared_ptr<YieldCurveInfoWrapper>, std::vector<Real> > > YieldCurveTable;
	typedef std::map<std::wstring, boost::shared_ptr<CDSCurveData> > CDSCurveTable;
	typedef std::map<std::wstring, std::pair<Real, Real> > CorrRcvyTable;
	typedef std::map<std::wstring, boost::shared_ptr<FXCurveData> > FXCurveTable;
	typedef std::map<std::wstring, boost::shared_ptr<HullWhiteTimeDependentParameters> > HWTable;
	typedef std::map<std::pair<std::wstring, std::wstring>, Real> CorrMap;

public:
	CurveTable();

	typedef YieldCurveTable::mapped_type yield_mapped_type;

	void Init();

	boost::shared_ptr<YieldCurveInfoWrapper> GetYieldCurve( const std::wstring& code );
	yield_mapped_type GetCurveData( const std::wstring& code );

	boost::shared_ptr<FXCurveData> GetFXCurve( const std::wstring& code );
	boost::shared_ptr<CDSCurveData> GetCDSCurve( const std::wstring& code );

	boost::shared_ptr<HullWhiteTimeDependentParameters> GetHWParams( const std::wstring& code, const YieldCurveInfoWrapper* wrapper );

	Real GetCorr( const std::wstring& code1, const std::wstring& code2 );

	std::pair<Real, Real> GetCorrRcvy( const std::wstring& code );

private:
	YieldCurveTable m_table;
	CDSCurveTable m_cdsTable;
	FXCurveTable m_fxTable;
	HWTable m_hwTable;
	CorrMap m_corrMap;
	CorrRcvyTable m_corrRcvyTable;
};

struct YieldCurveInfo;
class YieldCurveInfoWrapper
{
public:
	YieldCurveInfoWrapper( const std::wstring& curveName, boost::shared_ptr<YieldCurveData> curveData );

	YieldCurveInfo* GetInfo() const;

	std::wstring GetCurveName() const { return m_curveName; }
	const HullWhiteTimeDependentParameters& GetHWParam() const { return *m_hwParam; }

	Real GetPastFixing() const { return m_yields[ 0 ]; }

	void ShiftCurve( Real delta );

	boost::shared_ptr<YieldCurveData> GetCurveData() const { return m_curveData; }

private:
	std::wstring m_curveName;
	boost::scoped_array<int> m_daysToMat;
	boost::scoped_array<double> m_yields;
	boost::shared_ptr<YieldCurveData> m_curveData;
	boost::shared_ptr<YieldCurveInfo> m_curveInfo;
	boost::shared_ptr<HullWhiteTimeDependentParameters> m_hwParam;
};