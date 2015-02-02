#pragma once
#include "StdAfx.h"

#include <ql/quantlib.hpp>
#include <OAIdl.h>

#include <tinyXML/tinyxml2.h>
#include <makeResultXML.h>

using namespace QuantLib;
using namespace tinyxml2;

class pyCall
{
public:
	pyCall() {}
	pyCall(std::wstring inputXML);
	std::string getInput();
	std::string calc();

private:
	int RealCall( TiXmlDocument &doc, bool showMsgBox );
	TiXmlDocument doc;
	std::string retXML_;
};

