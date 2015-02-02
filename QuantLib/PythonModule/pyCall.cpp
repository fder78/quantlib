#include <iostream>
#include <ostream>

#include <pyCall.h>
#include "IFunctor.h"
#include "FunctorFactory.h"

#include "StringUtil.h"
#include "PricingSetting.h"

//생성자
pyCall::pyCall(std::wstring inputXML) {	
	std::wstring ucs2Data = inputXML;
	int n = ::WideCharToMultiByte( CP_UTF8, 0, ucs2Data.c_str(), -1, NULL, 0, NULL, NULL );
	boost::scoped_array<char> utf8Data( new char[ n + 1 ] );
	::WideCharToMultiByte( CP_UTF8, 0, ucs2Data.c_str(), -1, utf8Data.get(), n, NULL, NULL );
	utf8Data[ n ] = '\0';

	doc.Parse( utf8Data.get(), NULL, TIXML_ENCODING_UTF8 );
}


std::string pyCall::calc() {
	int status = RealCall( doc, true );
	return retXML_;
}

int pyCall::RealCall( TiXmlDocument &doc, bool showMsgBox )
{

	while( true )
	{
		try	{
			TiXmlNode* root = doc.FirstChild( "root" );
			std::string funcName = static_cast<TiXmlElement*>( root->FirstChild("func_root") )->Attribute( "name" );
			FunctorFactory::instance().GetFunctor( funcName )->Run( root->FirstChildElement( "param_root" ) );
			break;
		}
		catch( const std::exception& e )
		{
			if( boost::find_first( ::ToWString( e.what() ), L"Bloomberg" ) ) {
				continue;
			}

			if( showMsgBox ) {
				::MessageBox( NULL, ::ToWString( e.what() ).c_str(), TEXT( "진단 정보" ), 0 );
			}
			else {
				std::cout << e.what();
			}
			return E_FAIL;
		}
		catch( ... )
		{
			if( showMsgBox ) {
				::MessageBox( NULL, L"알 수 없는 오류 발생!", TEXT( "진단 정보" ), 0 );
			}
			else {
				std::cout << "알 수 없는 오류 발생!";
			}
			return E_FAIL;
		}
	}
	return S_OK;

}