#pragma once

/*************************************************************
  xboard.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "xparser.h"
#include "Thinking.h"

class xBoardMgr
{
public:

  xBoardMgr();
  ~xBoardMgr();

  bool do_cmd();
  void write_error(const std::exception * e = 0);

  bool peekInput();
  void printPV(SearchResult * sres);
  void printStat(SearchData * sres);

#ifdef WRITE_LOG_FILE_
  std::ostream & get_log()
  {
    return ofs_log_;
  }
#endif

private:

  void out_state(std::ostream & os, uint8 state, bool white);

private:

  void read_cmd(xCmd & cmd);
  void process_cmd(xCmd & cmd);

  Thinking thk_;
  xParser parser_;

  int  vNum_;
  bool stop_;
  bool force_;
  bool fenOk_;

  std::ostream & os_;

#ifdef WRITE_LOG_FILE_
  std::ofstream ofs_log_;
#endif

  HANDLE hinput_;
  bool   in_pipe_;

};