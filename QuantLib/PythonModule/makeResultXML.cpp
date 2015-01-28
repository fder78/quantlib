
#include <makeResultXML.h>

namespace QuantLib {
	std::string makeErrorXML(std::string& msg) {
		//Create XML RESULT
		XMLDocument doc;

		XMLElement* root = doc.NewElement("Result");
		root->SetAttribute("code", "error");
		doc.LinkEndChild(root);	

		XMLText* text = doc.NewText(msg.c_str());
		root->LinkEndChild(text);

		XMLPrinter streamer;
		doc.Print( &streamer );
		
		return std::string( streamer.CStr() );
	}
}