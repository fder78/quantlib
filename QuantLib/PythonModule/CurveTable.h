#pragma once

#include "PerProcessSingleton.h"
#include "ShiftOption.h"

namespace QuantLib
{
	struct YieldCurveData;
	struct HullWhiteParameters;
	struct SwaptionVolData;
	struct CapVolData;
}

struct CDSCurveData
{
	std::vector<Period> tenors;
	std::vector<Real> quotedSpreads;
};

struct FXCurveData
{
	std::vector<Date> fwdDate;
	std::vector<Real> fwdValue;
	std::vector<Size> chunkIndex;
};

class YieldCurveInfoWrapper;
class InterestRateCurveInfoWrapper;
class YieldCurveInfoWrapperProxy;

class CurveTable : public PerProcessSingleton<CurveTable>
{
private:
	typedef std::map<std::pair<std::wstring, ShiftOption>, std::pair<boost::shared_ptr<YieldCurveInfoWrapper>, std::vector<Real> > > YieldCurveTable;
	typedef std::map<std::wstring, boost::shared_ptr<CDSCurveData> > CDSCurveTable;
	typedef std::map<std::wstring, std::pair<Real, Real> > CorrRcvyTable;
	typedef std::map<std::pair<std::wstring, ShiftOption>, boost::shared_ptr<FXCurveData> > FXCurveTable;
	typedef std::map<std::pair<std::pair<std::wstring, ShiftOption>, Period>, boost::shared_ptr<HullWhiteParameters> > HWPeriodTable;
	typedef std::map<std::pair<std::wstring, ShiftOption>, boost::shared_ptr<SwaptionVolData> > SwaptionVolTable;
	typedef std::map<std::pair<std::wstring, ShiftOption>, boost::shared_ptr<CapVolData> > CapVolTable;
	typedef std::map<std::pair<std::wstring, std::wstring>, Real> CorrMap;
	typedef std::map<std::wstring, Real> HistoricalVolMap;

	typedef std::map<std::wstring, boost::shared_ptr<YieldCurveInfoWrapperProxy> > YieldCurveWrapperProxyTable;
	typedef std::map<std::pair<std::wstring, std::wstring>, Real> ManualCurveDataTable;

public:
	CurveTable();

	typedef YieldCurveTable::mapped_type yield_mapped_type;

	void Init();

	boost::shared_ptr<YieldCurveInfoWrapper> GetYieldCurve( const std::wstring& code, const ShiftOption& shiftOption );
	boost::shared_ptr<YieldCurveInfoWrapperProxy> GetYieldCurveProxy( const std::wstring& code );
	
	boost::shared_ptr<FXCurveData> GetFXCurve( const std::wstring& code, const ShiftOption& shiftOption );
	boost::shared_ptr<CDSCurveData> GetCDSCurve( const std::wstring& code );

	boost::shared_ptr<HullWhiteParameters> GetHWParams( const std::wstring& code, Period tenor, const ShiftOption& so );

	Real GetCorr( const std::wstring& code1, const std::wstring& code2 );

	std::pair<Real, Real> GetCorrRcvy( const std::wstring& code );

	void AddCurveTable( const TiXmlElement* curveRecord );

	boost::shared_ptr<CapVolData> GetCapVolData( const std::wstring& curveName, const ShiftOption& so );
	boost::shared_ptr<SwaptionVolData> GetSwaptionVolData( const std::wstring& curveName, const ShiftOption& so );
	
	void SetVol( const std::wstring& curveName, Real vol );
	
	Real GetNullableManualInputYTM( const ManualCurveDataTable::key_type& codeTenor ) const;
	void AddManualInputYTM( const std::wstring& curveCode, const std::wstring& Tenor, Real value );
	void RemoveManualInputYTM() { m_manualInputYTM.clear(); }
	void AddSwaptionVolTable( const std::wstring& curveCode, boost::shared_ptr<SwaptionVolData> volData );
	void RemoveSwaptionVolTable() { m_swaptionVolTable.clear(); }
	void AddCapVolTable( const std::wstring& curveCode, const ShiftOption& shiftOption, boost::shared_ptr<CapVolData> capVolData);
	void RemoveCapVolTable() { m_capVolTable.clear(); }

private:
	YieldCurveTable m_table;
	YieldCurveWrapperProxyTable m_proxyTable;
	CDSCurveTable m_cdsTable;
	FXCurveTable m_fxTable;
	HWPeriodTable m_hwPeriodTable;
	CorrMap m_corrMap;
	CorrRcvyTable m_corrRcvyTable;
	SwaptionVolTable m_swaptionVolTable;
	CapVolTable m_capVolTable;
	HistoricalVolMap m_volTable;

	ManualCurveDataTable m_manualInputYTM;
};

