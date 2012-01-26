#include <iostream>
#include <string>
#include <fstream>
#include "xparser.h"
#include "xboard.h"

using namespace std;


int main(int argc, char * argv[])
{
  Board::ticks_ = 0;
  xBoardMgr xbrd;

#ifdef WRITE_LOG_FILE_
#ifndef NDEBUG
  try
  {
#endif
#endif

	cout.setf(ios_base::unitbuf);

	for ( ; xbrd.do_cmd(); );


#ifdef WRITE_LOG_FILE_
//  xbrd.get_log() << "   LMR-errors count = " << (int)Board::ticks_ << endl;
#ifndef NDEBUG
  }
  catch ( const std::exception & e )
  {
    xbrd.write_error(&e);
  }
  catch ( ... )
  {
    xbrd.write_error();
  }
#endif
#endif

	return 0;
}

