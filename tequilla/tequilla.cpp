#include <iostream>
#include <string>
#include <fstream>
#include "xparser.h"
#include "Thinking.h"
//#include "StopTimer.h"
#include <io.h>

#define WRITE_LOG_FILE_

using namespace std;

void out_state(ostream & os, Board::State state, bool white)
{
	if ( Board::ChessMat == state )
	{
		if ( white )
			os << "1-0 {White}" << endl;
		else
			os << "0-1 {Black}" << endl;
	}
	else if ( Board::DrawInsuf == state )
	{
		os << "1/2-1/2 {Draw by material insufficient}" << endl;
	}
  else if ( Board::Stalemat == state )
  {
    os << "1/2-1/2 {Stalemate}" << endl;
  }
  else if ( Board::DrawReps == state )
  {
    os << "1/2-1/2 {Draw by repetition}" << endl;
  }
  else if ( Board::Draw50Moves == state )
  {
    os << "1/2-1/2 {Draw by fifty moves rule}" << endl;
  }
}

int main(int argc, char * argv[])
{
#ifdef WRITE_LOG_FILE_
	ofstream ofs_log("log.txt");
#endif

	cout.setf(ios_base::unitbuf);
	int vNum = 1;
	Thinking thk;

	xParser parser;

	bool stop = false;
	bool force = false;
  bool fenOk = true;

	for ( ; !stop; )
	{

		xCmd cmd;

		const int slineSize = 4096;
		char sline[slineSize];
		cin.getline(sline, slineSize);

		char * s = strchr(sline, '\n');
		if ( s )
			*s = 0;

#ifdef WRITE_LOG_FILE_
		ofs_log << string(sline) << endl;
#endif

		cmd = parser.parse(sline);

		if ( !cmd )
			continue;

		switch ( cmd.type() )
		{
		case xCmd::xBoard:
			break;

    case xCmd::xPing:
      cout << "pong " << cmd.asInt(0) << endl;
      break;

		case xCmd::xNew:
			thk.init();
			break;

    case xCmd::xOption:
      {
        int v = cmd.getOption("enablebook");
        if ( v >= 0 )
          thk.enableBook(v);

#ifdef WRITE_LOG_FILE_
        ofs_log << "  book was " << (v ? "enables" : "disabled") << endl;
#endif
      }
      break;

    case xCmd::xMemory:
      thk.setMemory(cmd.asInt(0));
      break;

		case xCmd::xProtover:
			vNum = cmd.asInt(0);
      if ( vNum > 1 )
      {
        cout << "feature done=0" << endl;
        cout << "feature setboard=1" << endl;
        cout << "feature myname=\"Tequilla 1.0\" memory=1" << endl;
        cout << "feature option=\"enablebook -check 1\"" << endl;
        cout << "feature done=1" << endl;
      }
			break;

		case xCmd::xSaveBoard:
			thk.save();
			break;

    case xCmd::xSetboardFEN:
      if ( !(fenOk = thk.fromFEN(cmd)) )
      {
#ifdef WRITE_LOG_FILE_
        if ( cmd.paramsNum() > 0 )
          ofs_log << "invalid FEN given: " << cmd.packParams() << endl;
        else
          ofs_log << "there is no FEN in setboard command" << endl;
#endif
        cout << "tellusererror Illegal position" << endl;
      }
      break;

		case xCmd::xEdit:
		case xCmd::xChgColor:
		case xCmd::xClearBoard:
		case xCmd::xSetFigure:
		case xCmd::xLeaveEdit:
			thk.editCmd(cmd);
			break;

		case xCmd::xQuit:
			stop = true;
#ifdef WRITE_LOG_FILE_
			thk.save();
#endif
			break;

		case xCmd::xForce:
			force = true;
			break;

		case xCmd::xSt:
      thk.setTimePerMove(cmd.asInt(0)*1000);
			break;

		case xCmd::xSd:
			thk.setDepth(cmd.asInt(0));
			break;

    case xCmd::xTime:
      thk.setXtime(cmd.asInt(0)*10);
      break;

    case xCmd::xOtime:
      break;

    case xCmd::xLevel:
      thk.setMovesLeft(cmd.asInt(0));
      break;

		case xCmd::xUndo:
			thk.undo();
			break;

		case xCmd::xRemove:
			thk.undo();
			thk.undo();
			break;

		case xCmd::xGo:
      if ( !fenOk )
      {
#ifdef WRITE_LOG_FILE_
        ofs_log << " illegal move. fen is invalid" << endl;
#endif
        cout << "Illegal move" << endl;
      }
      else
      {
				force = false;
				char str[256];
        Board::State state = Board::Invalid;
				bool white;
				bool b = thk.reply(str, state, white);
				if ( b )
				{
#ifdef WRITE_LOG_FILE_
					ofs_log << " " << str << endl;
#endif

          cout << str << endl;

          if ( Board::isDraw(state) || state == Board::ChessMat )
          {
            out_state(cout, state, white);

#ifdef WRITE_LOG_FILE_
            if ( Board::isDraw(state) )
              ofs_log << " draw" << endl; 
            else
              ofs_log << (white ? " white" : " black") << endl;
#endif
          }
				}
			}
			break;

		case xCmd::xMove:
      if ( !fenOk )
      {
#ifdef WRITE_LOG_FILE_
        ofs_log << " illegal move. fen is invalid" << endl;
#endif
        cout << "Illegal move" << endl;
      }
      else
      {
				Board::State state;
				bool white;
				if ( thk.move(cmd, state, white) )
				{
					if ( Board::isDraw(state) || Board::ChessMat == state )
					{
						out_state(cout, state, white);

#ifdef WRITE_LOG_FILE_
            if ( Board::isDraw(state) )
              ofs_log << " draw" << endl; 
            else
              ofs_log << (white ? " white" : " black") << endl;
#endif
					}
					else if ( !force )
					{
						char str[256];
						bool b = thk.reply(str, state, white);
						if ( b )
						{
#ifdef WRITE_LOG_FILE_
							ofs_log << " ... " << str << endl; 
#endif

              cout << str << endl;

              if ( Board::isDraw(state) || state == Board::ChessMat )
              {
                out_state(cout, state, white);

#ifdef WRITE_LOG_FILE_
                if ( Board::isDraw(state) )
                  ofs_log << " draw" << endl; 
                else
                  ofs_log << (white ? " white" : " black") << endl;
#endif
              }
						}
					}
				}
				else
				{
					cout << "Illegal move: " << cmd.str() << endl;

#ifdef WRITE_LOG_FILE_
					ofs_log << " Illegal move: " << cmd.str() << endl;
#endif
				}
			}
      break;
		}
	}


	return 0;
}

