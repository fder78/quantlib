#pragma once

#include <string>
#include <tinyXML/tinyxml2.h>
using namespace tinyxml2;

namespace QuantLib {
	std::string makeErrorXML(std::string& msg);
}
