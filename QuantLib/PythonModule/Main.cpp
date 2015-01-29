#include "StdAfx.h"

#include "Main.h"

#include "IFunctor.h"
#include "FunctorFactory.h"

#include "StringUtil.h"
#include "PricingSetting.h"

#include <ostream>

#if defined(QL_ENABLE_SESSIONS)
namespace QuantLib {

	Integer sessionId() { return 0; }

	void* mutex = NULL;
	void* createMutex() { return NULL; }
	void __Lock::AchieveLock() { }
	void __Lock::ReleaseLock() { }
}

#endif

FICCDERIVATIVES_ENTRY int __stdcall RealCall( TiXmlDocument &doc, bool showMsgBox )
{
	TiXmlNode* root = doc.FirstChild( "root" );

	std::string funcName = static_cast<TiXmlElement*>( root->FirstChild("func_root") )->Attribute( "name" );

	while( true )
	{
		try
		{
			FunctorFactory::instance().GetFunctor( funcName )->Run( root->FirstChildElement( "param_root" ) );
			break;
		}
		catch( const std::exception& e )
		{
			if( boost::find_first( ::ToWString( e.what() ), L"Bloomberg" ) )
			{
				continue;
			}

			if( showMsgBox )
			{
				::MessageBox( NULL, ::ToWString( e.what() ).c_str(), TEXT( "진단 정보" ), 0 );
			}
			else
			{
				std::cout << e.what();
			}
			return E_FAIL;
		}
		catch( ... )
		{
			if( showMsgBox )
			{
				::MessageBox( NULL, L"알 수 없는 오류 발생!", TEXT( "진단 정보" ), 0 );
			}
			else
			{
				std::cout << "알 수 없는 오류 발생!";
			}

			return E_FAIL;
		}
	}

	return S_OK;
}

FICCDERIVATIVES_ENTRY int __stdcall VBA_Call( BSTR data )
{
	TiXmlDocument doc;
	std::wstring ucs2Data = ::ToWString( reinterpret_cast<const char*>( data ) );
	int n = ::WideCharToMultiByte( CP_UTF8, 0, ucs2Data.c_str(), -1, NULL, 0, NULL, NULL );
	boost::scoped_array<char> utf8Data( new char[ n + 1 ] );
	::WideCharToMultiByte( CP_UTF8, 0, ucs2Data.c_str(), -1, utf8Data.get(), n, NULL, NULL );
	utf8Data[ n ] = '\0';

	doc.Parse( utf8Data.get(), NULL, TIXML_ENCODING_UTF8 );

	return RealCall( doc, true );
}

FICCDERIVATIVES_ENTRY int __stdcall VBA_CallByFile( BSTR fileName, bool useProxy, bool calcGreek, bool showDetail )
{	
	TiXmlDocument doc;
	bool succeded = false;
	for( int i = 0; i < 10; i++ )
	{
		doc.ClearError();
		doc.LoadFile( reinterpret_cast<const char*>( fileName ), TIXML_ENCODING_UTF8 );
		if( !doc.Error() )
		{
			succeded = true;
			break;
		}

		Sleep( 500 );
	}

	if( !succeded )
	{
		std::cout << "File Load Error!" << doc.ErrorDesc();
		return 0;
	}

	std::string nameStr( reinterpret_cast<const char*>( fileName ) );

	PricingSetting::instance().SetOutputFileName( nameStr + ".res" );
	PricingSetting::instance().SetCalcGreek( calcGreek );
	PricingSetting::instance().SetShowDetail( showDetail );

	return RealCall( doc, false );
}

