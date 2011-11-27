#include "xparser.h"

using namespace std;

xParser::xParser()
{

}

bool xParser::split_str(char * str, std::vector<std::string> & result)
{
	if ( !str || strlen(str) == 0 )
		return false;

	const char * sepr = " \t\n\r";
	char * s = strtok(str, sepr);
	for ( ; s; )
	{
		result.push_back(s);
		s = strtok(0, sepr);
	}

	return !result.empty();
}

struct CommandLine
{
  const char * commandName_;
  int commandType_;
};

xCmd xParser::parse(char * str)
{
	if ( !str || strlen(str) == 0 )
		return xCmd();

  static CommandLine command_lines [] = {
    { "xboard", xCmd::xBoard },
    { "option", xCmd::xOption },
    { "ping", xCmd::xPing },
    { "new", xCmd::xNew },
    { "go", xCmd::xGo },
    { "undo", xCmd::xUndo},
    { "remove", xCmd::xRemove },
    { "force", xCmd::xForce },
    { "st", xCmd::xSt },
    { "sd", xCmd::xSd },
    { "time", xCmd::xTime },
    { "otim", xCmd::xOtime },
    { "level", xCmd::xLevel },
    { "memory", xCmd::xMemory },
    { "saveboard", xCmd::xSaveBoard },
    { "edit", xCmd::xEdit },
    { "#", xCmd::xClearBoard },
    { "c", xCmd::xChgColor },
    { ".", xCmd::xLeaveEdit }, 
    { "protover", xCmd::xProtover },
    { "quit",xCmd::xQuit }
  };

  _strlwr(str);

  vector<string> params;
  split_str(str, params);

  if ( params.empty() )
    return xCmd();

  for (int i = 0; i < sizeof(command_lines)/sizeof(CommandLine); ++i)
  {
    const CommandLine & cmdLine = command_lines[i];

    if ( string(cmdLine.commandName_) == params[0] )
	  {
		  if ( params.size() )
			  params.erase(params.begin());

		  return xCmd(cmdLine.commandType_, params);
	  }
  }

  if ( strlen(str) == 3 && isalpha(str[0]) && isalpha(str[1]) && isdigit(str[2]) )
    return xCmd(xCmd::xSetFigure, str);

	xCmd moveCmd = parseMove(str);
	if ( moveCmd )
		return moveCmd;

	return xCmd();
}

xCmd xParser::parseMove(char * str)
{
	if ( !str || strlen(str) < 4 )
		return xCmd();

	if ( strlen(str) >= 4 && isalpha(str[0]) && isdigit(str[1]) && isalpha(str[2]) && isdigit(str[3]) )
		return xCmd(xCmd::xMove, str);

	return xCmd();
}
