#include <iostream>
#include <string>
#include <fstream>
#include "xparser.h"
#include "xboard.h"

#include <windows.h>
#include "minidump.h"

using namespace std;

void main_loop(xBoardMgr & xbrd)
{
#ifdef WRITE_LOG_FILE_
#ifndef NDEBUG
  __try
  {
#endif
#endif

    cout.setf(ios_base::unitbuf);

    for ( ; xbrd.do_cmd(); );


#ifdef WRITE_LOG_FILE_
    //  xbrd.get_log() << "   LMR-errors count = " << (int)Board::ticks_ << endl;
#ifndef NDEBUG
  }
  __except ( TopLevelFilter(GetExceptionInformation()) )//catch ( const std::exception & e )
  {
    xbrd.write_error();
  }
#endif
#endif

}


int main(int argc, char * argv[])
{
  Board::ticks_ = 0;
  xBoardMgr xbrd;

  main_loop(xbrd);

	return 0;
}

