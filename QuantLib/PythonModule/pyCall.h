#pragma once

#include <ql/quantlib.hpp>
#include <tinyXML/tinyxml2.h>
#include <makeResultXML.h>

using namespace QuantLib;
using namespace tinyxml2;

class pyCall
{
public:
	pyCall() : inputXML_(std::string()) {}
	pyCall(std::string inputXML);
	std::string getInput();
	std::string calc();

private:
	std::string inputXML_;
	std::string retXML_;
	bool isError_;
};

