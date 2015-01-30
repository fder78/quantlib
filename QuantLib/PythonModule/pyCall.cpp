#include <iostream>
#include <pyCall.h>
//#include <functor/IFunctor.h>

//»ý¼ºÀÚ
pyCall::pyCall(std::string inputXML) {
	inputXML_ = inputXML;
	isError_ = false;

	////Parse XML INPUT
	//XMLDocument doc;
	//doc.Parse(inputXML_.c_str());
	//XMLElement* root = doc.FirstChildElement( "root" );
	//std::string funcName = static_cast<XMLElement*>( root->FirstChildElement("func_root") )->Attribute("name");

	//try
	//{
	//	FunctorFactory::instance().GetFunctor( funcName )->Run( root->FirstChildElement( "param_root" ) );
	//}
	//catch( const std::exception& e )
	//{
	//	isError_ = true;
	//	retXML_ = makeErrorXML( std::string(e.what()) );
	//}
	//catch( ... )
	//{
	//	isError_ = true;
	//	retXML_ = makeErrorXML( std::string( "unknown error") );
	//}

}

std::string pyCall::getInput() {
	return inputXML_;
}

std::string pyCall::calc() {

	return retXML_;	

}