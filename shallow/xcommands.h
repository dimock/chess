#pragma once

/*************************************************************
  xcommands.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <vector>
#include <string>
#include "Figure.h"

class xCmd
{
public:

	enum {
		xNone,
		xOption,
		xPing,
		xBoard,
		xMemory,
		xNew,
		xMove,
		xProtover,
		xGo,
		xSt,
		xSd,
		xUndo,
		xRemove,
		xForce,
		xEdit,
    xPost,
    xNopost,
    xAnalyze,
    xExit,
		xTime,
		xOtime,
		xLevel,
		xLeaveEdit,
    xGoNow,
		xChgColor,
		xClearBoard,
		xSetFigure,
		xSaveBoard,
		xSetboardFEN,
		xQuit
	};

	xCmd(int type = xNone) :
		type_(type), str_(0)
	{}

	xCmd(int type, char * str) :
		type_(type), str_(str)
	{}
	
	xCmd(int type, const std::vector<std::string> & params) :
		type_(type), params_(params), str_(0)
	{}

	int type() const { return type_; }
	size_t paramsNum() const { return params_.size(); }
	const std::string & param(size_t i) const { return params_.at(i); }

	operator bool () const
	{
		return type_ != xNone;
	}

	int asInt(size_t i) const
	{
		if ( params_.size() <= i )
			return 0;

		int n;
		if ( sscanf(params_.at(i).c_str(), "%d", &n) == 1 )
			return n;

		return 0;
	}

  int getOption(const char * oname)
  {
    if ( !oname )
      return -1;

    for (size_t i = 0; i < params_.size(); ++i)
    {
      std::string option = params_[i];
      size_t n = option.find(oname);
      if ( n == std::string::npos )
        continue;
      n = option.find('=');
      if ( n == std::string::npos )
        continue;
      option.erase(0, n+1);
      int v;
      if ( sscanf(option.c_str(), "%d", &v) == 1 )
        return v;
    }
    return -1;
  }

	char * str()
	{
		return str_;
	}

  std::string packParams() const
  {
    std::string res;
    for (size_t i = 0; i < params_.size(); ++i)
    {
      res += params_.at(i);
      res += " ";
    }
    return res;
  }

private:

	int type_;
	char * str_;
	std::vector<std::string> params_;
};