
/*************************************************************
  xboard.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <io.h>
#include <iostream>
#include <string>
#include <fstream>
#include "xboard.h"

using namespace std;

static xBoardMgr * g_xboard_mgr_ = 0;

void queryInput()
{
  if ( !g_xboard_mgr_ )
    return;

  if ( !g_xboard_mgr_->peekInput() )
    return;

  g_xboard_mgr_->do_cmd();
}

void sendOutput(SearchResult * sres)
{
  if ( !g_xboard_mgr_ )
    return;

  g_xboard_mgr_->printPV(sres);
}

void sendStatus(SearchData * sdata)
{
  if ( !g_xboard_mgr_ )
    return;

  g_xboard_mgr_->printStat(sdata);
}

xBoardMgr::xBoardMgr() :
  os_(cout)
{
#ifdef WRITE_LOG_FILE_
  ofs_log_.open("log.txt", ios_base::app);
  thk_.set_logfile(&ofs_log_);
#endif

  vNum_ = 0;
  stop_ = false;
  force_ = false;
  fenOk_ = true;

  g_xboard_mgr_ = this;

  hinput_ = GetStdHandle(STD_INPUT_HANDLE);
  if ( hinput_ )
  {
    DWORD mode = 0;
    in_pipe_ = !GetConsoleMode(hinput_, &mode);
    if ( !in_pipe_ )
    {
      BOOL ok = SetConsoleMode(hinput_, mode & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
      FlushConsoleInputBuffer(hinput_);
    }
  }

  CallbackStruct cs;
  cs.sendOutput_ = &sendOutput;
  cs.sendStatus_ = &sendStatus;
  cs.queryInput_ = &queryInput;
  thk_.setPlayerCallbacks(cs);
}

xBoardMgr::~xBoardMgr()
{
  g_xboard_mgr_ = 0;
  thk_.clearPlayerCallbacks();
}
  
bool xBoardMgr::peekInput()
{
  if ( !hinput_ )
    return false;

  if ( in_pipe_ )
  {
    DWORD avaliable = 0;
    if ( !PeekNamedPipe(hinput_, 0, 0, 0, &avaliable, NULL) )
      return false;

    return avaliable != 0;
  }
  else
  {
    DWORD num = 0;
    if ( GetNumberOfConsoleInputEvents(hinput_, &num) )
    {
      if ( num == 0 )
        return false;

      INPUT_RECORD irecords[256];
      DWORD nread = 0;
      if ( PeekConsoleInput(hinput_, irecords, num, &nread) )
      {
        for (DWORD i = 0; i < nread; ++i)
        {
          if ( irecords[i].EventType & KEY_EVENT )
            return true;
        }
      }
    }
    return false;
  }
}


void xBoardMgr::out_state(ostream & os, uint8 state, bool white)
{
  if ( Board::ChessMat & state )
  {
    if ( white )
      os << "1-0 {White}" << endl;
    else
      os << "0-1 {Black}" << endl;
  }
  else if ( Board::DrawInsuf & state )
  {
    os << "1/2-1/2 {Draw by material insufficient}" << endl;
  }
  else if ( Board::Stalemat & state )
  {
    os << "1/2-1/2 {Stalemate}" << endl;
  }
  else if ( Board::DrawReps & state )
  {
    os << "1/2-1/2 {Draw by repetition}" << endl;
  }
  else if ( Board::Draw50Moves & state )
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
    ofs_log_ << "exception" << endl;
#endif

#ifdef WRITE_ERROR_PGN 
  time_t curtime;
  time(&curtime);
  tm * t = localtime(&curtime);
  char pgn_fname[MAX_PATH];
  strftime(pgn_fname, MAX_PATH, "fen_%d-%m-%Y_%H-%M-%S.pgn", t);
  thk_.pgn2file(pgn_fname);
#endif
}

void xBoardMgr::printPV(SearchResult * sres)
{
  if ( !sres || !sres->best_ )
    return;

  Board board = sres->board_;
  UndoInfo undoStack[Board::GameLength];
  board.set_undoStack(undoStack);

  cout << sres->depth_ << " " << sres->score_ << " " << (int)sres->dt_ << " " << sres->totalNodes_;
  for (int i = 0; i < sres->depth_ && sres->pv_[i]; ++i)
  {
    cout << " ";

    Move pv = sres->pv_[i];
    uint8 captured = pv.capture_;
    pv.clearFlags();
    pv.capture_ = captured;

    if ( !board.possibleMove(pv) )
      break;

    char str[64];
    if ( !printSAN(board, pv, str) )
      break;

    THROW_IF( !board.validateMove(pv), "move is invalid but it is not detected by printSAN()");

    board.makeMove(pv);

    cout << str;
  }
  cout << std::endl;
}

void xBoardMgr::printStat(SearchData * sdata)
{
  if ( sdata && sdata->depth_ <= 0 )
    return;

  Board board = sdata->board_;
  UndoInfo undoStack[Board::GameLength];
  board.set_undoStack(undoStack);

  char outstr[1024];
  clock_t t = clock() - sdata->tstart_;
  int movesLeft = sdata->numOfMoves_ - sdata->counter_;
  sprintf(outstr, "stat01: %d %d %d %d %d", t/10, sdata->totalNodes_, sdata->depth_-1, movesLeft, sdata->numOfMoves_);

  Move mv = sdata->best_;
  uint8 captured = mv.capture_;
  mv.clearFlags();
  mv.capture_ = captured;

  char str[64];
  if ( mv && board.validateMove(mv) && printSAN(board, mv, str) )
  {
    strcat(outstr, " ");
    strcat(outstr, str);
  }

  cout << outstr << std::endl;
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
      os_ << "feature myname=\"shallow\" memory=1" << endl;
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

  case xCmd::xGoNow:
    thk_.stop();
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
    if ( !thk_.undo() )
    {
#ifdef WRITE_LOG_FILE_
      ofs_log_ << " can't undo move" << endl;
      if ( thk_.is_thinking() )
        ofs_log_ << " thinking" << endl;
#endif
    }
    {
      char fen[256];
      thk_.toFEN(fen);

#ifdef WRITE_LOG_FILE_
      ofs_log_ << "       " << fen << endl;
#endif
    }
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
      uint8 state = Board::Invalid;
      bool white;
      bool b = thk_.reply(str, state, white);
      if ( b )
      {
#ifdef WRITE_LOG_FILE_
        ofs_log_ << " " << str << endl;
#endif

        os_ << str << endl;

        if ( Board::isDraw(state) || (state & Board::ChessMat) )
        {
          out_state(os_, state, white);

#ifdef WRITE_LOG_FILE_
          out_state(ofs_log_, state, white);
          //thk_.hash2file("hash");
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
#ifdef WRITE_LOG_FILE_
      if ( thk_.is_thinking() )
        ofs_log_ << " can't move - thinking" << endl;
#endif

      uint8 state;
      bool white;
      if ( thk_.move(cmd, state, white) )
      {
        if ( Board::isDraw(state) || (Board::ChessMat & state) )
        {
          out_state(os_, state, white);

#ifdef WRITE_LOG_FILE_
          out_state(ofs_log_, state, white);
          //thk_.hash2file("hash");
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

            if ( Board::isDraw(state) || (state & Board::ChessMat) )
            {
              out_state(os_, state, white);

#ifdef WRITE_LOG_FILE_
              out_state(ofs_log_, state, white);
              //thk_.hash2file("hash");
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
