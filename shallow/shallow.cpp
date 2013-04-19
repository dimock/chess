
/*************************************************************
  shallow.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <string>
#include <fstream>
#include "xparser.h"
#include "xboard.h"

#include <windows.h>

#if (defined _DEBUG)
#include "minidump.h"
#endif

using namespace std;

void main_loop(xBoardMgr & xbrd)
{
#if ( (defined WRITE_ERROR_PGN) && (defined _DEBUG) )
  __try
  {
#endif

    cout.setf(ios_base::unitbuf);

    for ( ; xbrd.do_cmd(); );


#if ( (defined WRITE_ERROR_PGN) && (defined _DEBUG) )
  }
  __except ( TopLevelFilter(GetExceptionInformation()) )
  {
    xbrd.write_error();
  }
#endif

}


int main(int argc, char * argv[])
{
  Board::ticks_ = 0;
  xBoardMgr xbrd;

  main_loop(xbrd);

	return 0;
}

