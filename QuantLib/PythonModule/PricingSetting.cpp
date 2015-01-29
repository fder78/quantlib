#include "StdAfx.h"

#include "PricingSetting.h"

#include "CQuery.h"
#include "GlobalSetting.h"

#include "StringUtil.h"

PricingSetting::PricingSetting()
	: m_calcGreek( true )
	, m_showDetail( true )
{
}


void UpdateHolidays()
{
	SouthKorea sk;
	SouthKorea sk2( SouthKorea::Settlement );

	CQuery dbConnector;
	BOOL conRes = dbConnector.Connect( 4, DB_SERVER_NAME, DB_ID, DB_PASSWORD );

	dbConnector.Exec( L"select ���� from `ficc`.`holidays` where weekday(����)<5 and ����='Y'" );

	while( true )
	{
		SQLRETURN res = dbConnector.Fetch();
		if( res )
		{
			break;
		}

		std::wstring holiday = dbConnector.GetStr( L"����" );
		sk.addHoliday( ::ConvertToDate( holiday ) );
		sk2.addHoliday( ::ConvertToDate( holiday ) );
	}	

	dbConnector.Clear();

	UnitedStates usa;
	dbConnector.Exec( L"select ���� from `ficc`.`holidays` where weekday(����)<5 and ����='Y'" );

	while( true )
	{
		SQLRETURN res = dbConnector.Fetch();
		if( res )
		{
			break;
		}

		std::wstring holiday = dbConnector.GetStr( L"����" );
		usa.addHoliday( ::ConvertToDate( holiday ) );
	}	

	dbConnector.Clear();
}

void PricingSetting::Init( const Date& evalDate, int dataDateAlias )
{
	m_evaluationDate = Date( evalDate.serialNumber() );
	m_useProxy = false;
	m_dataDateAlias = dataDateAlias;

	while( !m_isHolidaysUpdated )
	{
		try
		{
			::UpdateHolidays();
			m_isHolidaysUpdated = true;
		}
		catch (...)
		{
			Sleep( 1000 );
		}
	}
}

QuantLib::Date PricingSetting::GetDataDate() const
{
	return NullCalendar().advance( m_evaluationDate, m_dataDateAlias * Days, Unadjusted );
}
