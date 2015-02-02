#include "StdAfx.h"

#include "ParamParseUtil.h"
#include "IProductParam.h"
#include "RACMSParam.h"
#include "RASingleTreeParam.h"

#include "StringUtil.h"

void ProductParser::BuildEnumMap()
{
	AddEnum( L"CLN_Single", PT_CLNSingle );
	AddEnum( L"CLN_NtD", PT_CLNNtD);
	AddEnum( L"CDS_Single", PT_CDSSingle );
	AddEnum( L"CDS_NtD", PT_CDSNtD );
	AddEnum( L"RANote", PT_RANote );
	AddEnum( L"RASwap", PT_RASwap );
	AddEnum( L"RAFDMNote", PT_RAFDMNote );
	AddEnum( L"RAFDMSwap", PT_RAFDMSwap );
	AddEnum( L"RANoteCMS", PT_RANoteCMS );
	AddEnum( L"RASwapCMS", PT_RASwapCMS );
	AddEnum( L"RANoteSingleTree", PT_RANoteSingleTree );
	AddEnum( L"RASwapSingleTree", PT_RASwapSingleTree );
	AddEnum( L"RANoteKAP", PT_RANoteKAP );
	AddEnum( L"RASwapKAP", PT_RASwapKAP );
	AddEnum( L"RASpreadNote", PT_RASpreadNote );
	AddEnum( L"PSSwap", PT_PSSwap );
	AddEnum( L"PSNote", PT_PSNote );
	AddEnum( L"VanillaSwap", PT_VanillaSwap );
	AddEnum( L"FixedRateBond", PT_FixedRateBond );
	AddEnum( L"CapFloor", PT_CapFloor );
	AddEnum( L"Swaption", PT_Swaption );
	AddEnum( L"FXDigital", PT_FXDigital );
}

namespace QuantLib
{
	bool operator < ( const DayCounter& lhs, const DayCounter& rhs )
	{
		return lhs.name() < rhs.name();
	}

	bool operator < ( const Calendar& lhs, const Calendar& rhs )
	{
		return lhs.name() < rhs.name();
	}
}

void DayCounterParser::BuildEnumMap()
{
	AddEnum( L"Actual360", Actual360() );
	AddEnum( L"Actual365Fixed", Actual365Fixed());
	AddEnum( L"ActualActual(ISDA)", ActualActual( ActualActual::ISDA ) );
	AddEnum( L"ActualActual(ISMA)", ActualActual( ActualActual::ISMA ) );
	AddEnum( L"ActualActual(Euro)", ActualActual( ActualActual::Euro ) );
	AddEnum( L"Thirty360", Thirty360( Thirty360::BondBasis ) );
	AddEnum( L"30E/360", Thirty360( Thirty360::EurobondBasis ) );
	AddEnum( L"SimpleDayCounter", SimpleDayCounter() );
	AddEnum( L"30/360", Thirty360( Thirty360::BondBasis ) );
}

void TenorParser::BuildEnumMap()
{
	AddEnum( L"Annually", 1 * Years );
	AddEnum( L"SemiAnnually", 6 * Months );
	AddEnum( L"Quarterly", 3 * Months );
}

void CalendarParser::BuildEnumMap()
{
	AddEnum( L"Seoul", SouthKorea( SouthKorea( SouthKorea::Settlement )) );
	AddEnum( L"London", UnitedKingdom() );
	AddEnum( L"Tokyo", Japan() );
	AddEnum( L"HongKong", HongKong() );
	AddEnum( L"NewYork", UnitedStates(UnitedStates::Settlement) );
}

void DateGenerationRuleParser::BuildEnumMap()
{
	AddEnum( L"Forward", DateGeneration::Forward );
	AddEnum( L"Backward", DateGeneration::Backward );
}

void BusinessDayConventionParser::BuildEnumMap()
{
	AddEnum( L"Following", Following );
	AddEnum( L"Preceding", Preceding );
	AddEnum( L"ModifiedFollowing", ModifiedFollowing );
	AddEnum( L"ModifiedPreceding", ModifiedPreceding ); 
	AddEnum( L"Unadjusted", Unadjusted );
}

void ProtectionParser::BuildEnumMap()
{
	AddEnum( L"Buyer", Protection::Buyer );
	AddEnum( L"Seller", Protection::Seller );
}

void PaymentTenorParser::BuildEnumMap()
{
	AddEnum( L"Annually", CT_Annually );
	AddEnum( L"SemiAnnually", CT_SemiAnnually );
	AddEnum( L"Quarterly", CT_Quarterly );
	AddEnum( L"Once", CT_Once );
}

void FrequencyParser::BuildEnumMap()
{
	AddEnum( L"NoFrequency", NoFrequency );
	AddEnum( L"Once", Once );
	AddEnum( L"Annual", Annual );
	AddEnum( L"SemiAnnual", Semiannual );
	AddEnum( L"EveryFourthMonth", EveryFourthMonth );
	AddEnum( L"Quarterly", Quarterly );
	AddEnum( L"Bimonthly", Bimonthly );
	AddEnum( L"Monthly", Monthly );
	AddEnum( L"EveryFourthWeek", EveryFourthWeek );
	AddEnum( L"Biweekly", Biweekly );
	AddEnum( L"Weekly", Weekly );
	AddEnum( L"Daily", Daily );
	AddEnum( L"OtherFrequency", OtherFrequency );
}

#include <ql/indexes/ibor/HIBOR3M.hpp>
#include <ql/indexes/ibor/cd91.hpp>

void IborIndexParser::BuildEnumMap()
{
	AddEnum( L"CD91", IborIndexPtr( new CD91() ) );
	AddEnum( L"KRWCMS3M", IborIndexPtr( new CD91() ) );
	AddEnum( L"USDLibor3M", IborIndexPtr( new USDLibor( Period( Quarterly ) ) ) );
	AddEnum( L"Hibor3M", IborIndexPtr( new HIBOR3M() ) );
	AddEnum( L"Tibor", IborIndexPtr( new Tibor( Period( Semiannual ) ) ) );
	AddEnum( L"JPYLibor3M", IborIndexPtr( new Tibor( Period( Quarterly ) ) ) );
	AddEnum( L"JPYLibor6M", IborIndexPtr( new Tibor( Period( Semiannual ) ) ) );
	AddEnum( L"Euribor6M", IborIndexPtr( new Euribor6M() ) );
}


