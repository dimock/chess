#include <io.h>
#include <iostream>
#include <string>
#include <fstream>
#include "xboard.h"

using namespace std;

xBoardMgr::xBoardMgr() :
  os_(cout)
{
#ifdef WRITE_LOG_FILE_
  ofs_log_.open("log.txt");
#endif

  vNum_ = 0;
  stop_ = false;
  force_ = false;
  fenOk_ = true;
}

void xBoardMgr::out_state(ostream & os, Board::State state, bool white)
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

void xBoardMgr::write_error(const std::exception * e /*= 0*/)
{
#ifdef WRITE_LOG_FILE_
  if ( e )
    ofs_log_ << "exception: " << e->what() << endl;
  else
    ofs_log_ << "some other exception" << endl;
  thk_.fen2file("fen.txt");
#endif
}

bool xBoardMgr::do_cmd()
{
  xCmd cmd;
  read_cmd(cmd);

  if ( !cmd )
    return !stop_;

  process_cmd(cmd);

  return !stop_;
}

void xBoardMgr::read_cmd(xCmd & cmd)
{
  const int slineSize = 4096;
  char sline[slineSize];
  cin.getline(sline, slineSize);

  char * s = strchr(sline, '\n');
  if ( s )
    *s = 0;

#ifdef WRITE_LOG_FILE_
  ofs_log_ << string(sline) << endl;
#endif

  cmd = parser_.parse(sline);
}

void xBoardMgr::process_cmd(xCmd & cmd)
{
  switch ( cmd.type() )
  {
  case xCmd::xBoard:
    break;

  case xCmd::xPing:
    os_ << "pong " << cmd.asInt(0) << endl;
    break;

  case xCmd::xNew:
    thk_.init();
    break;

  case xCmd::xOption:
    {
      int v = cmd.getOption("enablebook");
      if ( v >= 0 )
        thk_.enableBook(v);

#ifdef WRITE_LOG_FILE_
      ofs_log_ << "  book was " << (v ? "enables" : "disabled") << endl;
#endif
    }
    break;

  case xCmd::xMemory:
    thk_.setMemory(cmd.asInt(0));
    break;

  case xCmd::xProtover:
    vNum_ = cmd.asInt(0);
    if ( vNum_ > 1 )
    {
      os_ << "feature done=0" << endl;
      os_ << "feature setboard=1" << endl;
      os_ << "feature myname=\"Tequilla 1.0\" memory=1" << endl;
      os_ << "feature done=1" << endl;
    }
    break;

  case xCmd::xSaveBoard:
    thk_.save();
    break;

  case xCmd::xSetboardFEN:
    if ( !(fenOk_ = thk_.fromFEN(cmd)) )
    {
#ifdef WRITE_LOG_FILE_
      if ( cmd.paramsNum() > 0 )
        ofs_log_ << "invalid FEN given: " << cmd.packParams() << endl;
      else
        ofs_log_ << "there is no FEN in setboard command" << endl;
#endif
      os_ << "tellusererror Illegal position" << endl;
    }
    break;

  case xCmd::xEdit:
  case xCmd::xChgColor:
  case xCmd::xClearBoard:
  case xCmd::xSetFigure:
  case xCmd::xLeaveEdit:
    thk_.editCmd(cmd);
    break;

  case xCmd::xPost:
  case xCmd::xNopost:
    thk_.setPost(cmd.type() == xCmd::xPost);
    break;

  case xCmd::xAnalyze:
    thk_.analyze();
    break;

  case xCmd::xExit:
    thk_.stop();
    break;

  case xCmd::xQuit:
    stop_ = true;
    thk_.stop();
#ifdef WRITE_LOG_FILE_
    thk_.save();
#endif
    break;

  case xCmd::xForce:
    force_ = true;
    break;

  case xCmd::xSt:
    thk_.setTimePerMove(cmd.asInt(0)*1000);
    break;

  case xCmd::xSd:
    thk_.setDepth(cmd.asInt(0));
    break;

  case xCmd::xTime:
    thk_.setXtime(cmd.asInt(0)*10);
    break;

  case xCmd::xOtime:
    break;

  case xCmd::xLevel:
    thk_.setMovesLeft(cmd.asInt(0));
    break;

  case xCmd::xUndo:
    thk_.undo();
    break;

  case xCmd::xRemove:
    thk_.undo();
    thk_.undo();
    break;

  case xCmd::xGo:
    if ( !fenOk_ )
    {
#ifdef WRITE_LOG_FILE_
      ofs_log_ << " illegal move. fen is invalid" << endl;
#endif
      os_ << "Illegal move" << endl;
    }
    else
    {
      force_ = false;
      char str[256];
      Board::State state = Board::Invalid;
      bool white;
      bool b = thk_.reply(str, state, white);
      if ( b )
      {
#ifdef WRITE_LOG_FILE_
        ofs_log_ << " " << str << endl;
#endif

        os_ << str << endl;

        if ( Board::isDraw(state) || state == Board::ChessMat )
        {
          out_state(os_, state, white);

#ifdef WRITE_LOG_FILE_
          if ( Board::isDraw(state) )
            ofs_log_ << " draw" << endl; 
          else
            ofs_log_ << (white ? " white" : " black") << endl;
#endif
        }
      }
    }
    break;

  case xCmd::xMove:
    if ( !fenOk_ )
    {
#ifdef WRITE_LOG_FILE_
      ofs_log_ << " illegal move. fen is invalid" << endl;
#endif
      os_ << "Illegal move" << endl;
    }
    else
    {
      Board::State state;
      bool white;
      if ( thk_.move(cmd, state, white) )
      {
        if ( Board::isDraw(state) || Board::ChessMat == state )
        {
          out_state(os_, state, white);

#ifdef WRITE_LOG_FILE_
          if ( Board::isDraw(state) )
            ofs_log_ << " draw" << endl; 
          else
            ofs_log_ << (white ? " white" : " black") << endl;
#endif
        }
        else if ( !force_ )
        {
          char str[256];
          bool b = thk_.reply(str, state, white);
          if ( b )
          {
#ifdef WRITE_LOG_FILE_
            ofs_log_ << " ... " << str << endl; 
#endif

            os_ << str << endl;

            if ( Board::isDraw(state) || state == Board::ChessMat )
            {
              out_state(os_, state, white);

#ifdef WRITE_LOG_FILE_
              if ( Board::isDraw(state) )
                ofs_log_ << " draw" << endl; 
              else
                ofs_log_ << (white ? " white" : " black") << endl;
#endif
            }
          }
        }
      }
      else
      {
        os_ << "Illegal move: " << cmd.str() << endl;

#ifdef WRITE_LOG_FILE_
        ofs_log_ << " Illegal move: " << cmd.str() << endl;
#endif
      }
    }
    break;
  }

}