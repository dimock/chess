#pragma once

#include <vector>
#include <string>
#include <cstdlib>

#include "xcommands.h"

class xParser
{
public:

	xParser();

	xCmd parse(char * str);

private:

	xCmd parseMove(char * str);

	bool split_str(char * str, std::vector<std::string> & result);

};