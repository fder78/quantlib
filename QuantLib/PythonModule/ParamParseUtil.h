#pragma once

#include "EnumParser.h"

enum E_ProductType;

enum E_CallTenor
{
	CT_Annually = 12,
	CT_SemiAnnually = 6,
	CT_Quarterly = 3,
	CT_Once = -1,
};


class ProductParser : public EnumParser<ProductParser, E_ProductType>
{
public:
	void BuildEnumMap();
};

class DayCounterParser : public EnumParser<DayCounterParser, DayCounter>
{
public:
	void BuildEnumMap();
};

class TenorParser : public EnumParser<TenorParser, Period>
{
public:
	void BuildEnumMap();
};

class CalendarParser : public EnumParser<CalendarParser, Calendar>
{
public:
	void BuildEnumMap();
};

class DateGenerationRuleParser : public EnumParser<DateGenerationRuleParser, DateGeneration::Rule>
{
public:
	void BuildEnumMap();
};

class BusinessDayConventionParser : public EnumParser<BusinessDayConventionParser, BusinessDayConvention>
{
public:
	void BuildEnumMap();	
};

class ProtectionParser : public EnumParser<ProtectionParser, Protection::Side>
{
public:
	void BuildEnumMap();
};

enum E_CallTenor;

class PaymentTenorParser : public EnumParser<PaymentTenorParser, E_CallTenor>
{
public:
	void BuildEnumMap();
};

class FrequencyParser : public EnumParser<FrequencyParser, Frequency>
{
public:
	void BuildEnumMap();
};

class IborIndexParser : public EnumParser<IborIndexParser, boost::shared_ptr<IborIndex> >
{
public:
	typedef boost::shared_ptr<IborIndex> IborIndexPtr;

	void BuildEnumMap();
};

