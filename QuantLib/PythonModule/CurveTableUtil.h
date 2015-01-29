#pragma once

class CQuery;
class ShiftOption;
class InterestRateCurveInfoWrapper;
struct CDSCurveData;
struct FXCurveData;

namespace QuantLib
{
	struct YieldCurveData;
	struct HullWhiteTimeDependentParameters;
	struct HullWhiteParameters;
	struct SwaptionVolData;
	struct CapVolData;
}

void QueryRecentCurveData( const std::wstring& code, OUT CQuery& dbConnector, OUT Date& rcvDate );
Period ParsePeriod( const std::wstring& period );

std::pair<boost::shared_ptr<YieldCurveData>, std::vector<Real> > LoadCurve( const std::wstring& code, const ShiftOption& shiftOption );
boost::shared_ptr<CDSCurveData> LoadCDSCurve( const std::wstring& code );

boost::shared_ptr<FXCurveData> LoadFXCurve( const std::wstring& code, const ShiftOption& shiftOption );
std::pair<boost::shared_ptr<YieldCurveData>, std::vector<Real> > LoadCRSCurve( const std::wstring& code, const ShiftOption& shiftOption, const ShiftOption& fxShiftOption );

boost::shared_ptr<HullWhiteParameters> CalcFromSwaption( boost::shared_ptr<SwaptionVolData> volDataOrg, Period tenor, boost::shared_ptr<InterestRateCurveInfoWrapper> ircw );
boost::shared_ptr<HullWhiteParameters> CalcFromCapVol( const CapVolData& capVolData, Period tenor );

boost::shared_ptr<CapVolData> LoadCapVol( const std::wstring& code, const ShiftOption& so, boost::shared_ptr<InterestRateCurveInfoWrapper> ircw );
boost::shared_ptr<SwaptionVolData> LoadSwaptionVol( const std::wstring& code, const ShiftOption& so, boost::shared_ptr<InterestRateCurveInfoWrapper> ircw );

std::vector<Size> GetDataIndexFromSwaption( const SwaptionVolData& volDataOrg, Period tenor );
std::vector<Size> GetDataIndexFromCapVol( const CapVolData& volDataOrg, Period tenor );