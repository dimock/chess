
/*************************************************************
  shallow.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <string>
#include <fstream>
#include "xparser.h"
#include "xboard.h"

#include <windows.h>

#if ((defined _DEBUG) && (defined _MSC_VER))
#include "minidump.h"
#endif

using namespace std;

void main_loop(xBoardMgr & xbrd)
{
#if ( (defined WRITE_ERROR_PGN) && (defined _DEBUG) && (defined _MSC_VER) )
  __try
  {
#endif

    cout.setf(ios_base::unitbuf);

    for ( ; xbrd.do_cmd(); );


#if ( (defined WRITE_ERROR_PGN) && (defined _DEBUG) && (defined _MSC_VER) )
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

  BitMask mask = (1ULL<<27) + (1ULL<<25);
  int x = _lsb64(mask);
  int y = _msb64(mask);
  int n = log2(0);
  int m = log2(mask);

  std::cout << "x = " << x << ", y = " << y << ", n = " << n << ", m = " << m << std::endl;

  main_loop(xbrd);

	return 0;
}

