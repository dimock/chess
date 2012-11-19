
/*************************************************************
  Thinking.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <fstream>
#include "Thinking.h"

static Thinking * g_thinking_ = 0;

// returns time in ms
int give_more_time()
{
  if ( !g_thinking_ )
    return 0;

  return g_thinking_->giveMoreTime();
}

using namespace std;

#define DEPTH_MAXIMUM 32

Thinking::Thinking() :
	boardColor_(Figure::ColorWhite), figureColor_(Figure::ColorWhite),
  xtimeMS_(0), movesLeft_(0), timePerMoveMS_(0), maxDepth_(-1),
  post_(false), thinking_(false), givetimeCounter_(0)
{
  g_thinking_ = this;

#ifdef WRITE_LOG_FILE_
  ofs_log_ = 0;
#endif
}

Thinking::~Thinking()
{
  g_thinking_ = 0;
}

void Thinking::setPlayerCallback(PLAYER_CALLBACK cbk)
{
  player_.setCallback(cbk);
}

void Thinking::setPost(bool p)
{
  if ( is_thinking() )
    return;

  post_ = p;
}

void Thinking::setDepth(int depth)
{
  if ( is_thinking() )
    return;

  xtimeMS_ = 0;
  movesLeft_ = 0;
  timePerMoveMS_ = -1;
  if ( depth < 2 )
    depth = 2;
  maxDepth_ = depth;
	player_.setMaxDepth(depth);
  player_.setTimeLimit(timePerMoveMS_);
}

void Thinking::setTimePerMove(int ms)
{
  if ( is_thinking() )
    return;

  maxDepth_ = -1;
  xtimeMS_ = 0;
  movesLeft_ = 0;
  timePerMoveMS_ = ms;
  if ( timePerMoveMS_ < 100 )
    timePerMoveMS_ = 100;
  player_.setTimeLimit(timePerMoveMS_);
  player_.setMaxDepth(DEPTH_MAXIMUM);
}

void Thinking::setXtime(int ms)
{
  if ( is_thinking() )
    return;

  maxDepth_ = -1;
  xtimeMS_ = ms;
  if ( xtimeMS_ < 100 )
    xtimeMS_ = 100;
  timePerMoveMS_ = -1;
  player_.setMaxDepth(DEPTH_MAXIMUM);
}

void Thinking::setMovesLeft(int mleft)
{
  if ( is_thinking() )
    return;

  maxDepth_ = -1;
  movesLeft_ = mleft;
  timePerMoveMS_ = -1;
  player_.setMaxDepth(DEPTH_MAXIMUM);
}


int Thinking::giveMoreTime()
{
  if ( timePerMoveMS_ > 0 || xtimeMS_ <= 0 || givetimeCounter_ > 0 )
    return 0;

  givetimeCounter_++;
  if ( movesLeft_ <= 0 && xtimeMS_ > 30 )
    return xtimeMS_/40;

  if ( movesLeft_ > 0 )
  {
    int mcount = player_.getBoard().movesCount();
    mcount = movesLeft_ - (mcount-1) % movesLeft_;
    if ( mcount > movesLeft_/2 )
      return xtimeMS_/mcount;
    else if ( mcount > movesLeft_/3 )
      return (xtimeMS_/mcount)/2;
    else if ( mcount > movesLeft_/6 )
      return (xtimeMS_/mcount)/4;
    else if ( mcount > 0 )
      return (xtimeMS_/mcount)/8;
  }
  return 0;
}


void Thinking::enableBook(int v)
{
}

bool Thinking::undo()
{
  if ( is_thinking() )
  {
    player_.postUndo();
    return false;
  }

  Board & board = player_.getBoard();
  if ( board.halfmovesCount() > 0 )
  {
    board.unmakeMove();
    return true;
  }

  return false;
}

void Thinking::setMemory(int mb)
{
  if ( is_thinking() )
    return;

  player_.setMemory(mb);
}

bool Thinking::init()
{
  if ( is_thinking() )
  {
    player_.postNew();
    return true;
  }

  return player_.fromFEN(0);
}

void Thinking::analyze()
{
  if ( is_thinking() )
    return;

  thinking_ = true;
  player_.setTimeLimit(0);
  player_.setMaxDepth(32);

  SearchResult sres;
  player_.setAnalyzeMode(true);
  player_.setGiveTimeCbk(0);
  player_.findMove(sres, post_ ? &cout : 0);
  player_.setAnalyzeMode(false);

  player_.setTimeLimit(timePerMoveMS_);
  player_.setMaxDepth(maxDepth_ < 0 ? DEPTH_MAXIMUM : maxDepth_);
  thinking_ = false;
}

void Thinking::stop()
{
  player_.pleaseStop();
}

bool Thinking::reply(char (& smove)[256], uint8 & state, bool & white)
{
  if ( is_thinking() )
    return false;

  updateTiming();

  Board & board = player_.getBoard();
	white = Figure::ColorWhite == board.getColor();
  state = board.getState();

  if ( board.drawState() || board.matState() )
    return true;

	SearchResult sres;

  player_.setAnalyzeMode(false);
  player_.setGiveTimeCbk(give_more_time);
  thinking_ = true;
  givetimeCounter_ = 0;
  if ( player_.findMove(sres, post_ ? &cout : 0) )
  {
    if ( board.validateMove(sres.best_) )
    {
      board.makeMove(sres.best_);
      board.verifyState();
    }
    else
      sres.best_.clear();
  }
  thinking_ = false;
  player_.setGiveTimeCbk(0);

  state = board.getState();

	if ( !moveToStr(sres.best_, smove, true) )
		return false;

	return true;
}

bool Thinking::move(xCmd & moveCmd, uint8 & state, bool & white)
{
  if ( is_thinking() )
    return false;

	if ( moveCmd.type() != xCmd::xMove )
		return false;

  Board & board = player_.getBoard();
	Figure::Color color = board.getColor();
	Figure::Color ocolor = Figure::otherColor(color);
  state = board.getState();

  if ( board.drawState() || board.matState() )
    return true;

	white = Figure::ColorWhite == color;

	Move move;
	if ( !strToMove(moveCmd.str(), board, move) )
		return false;

  if ( !board.validateMove(move) )
    return false;

  board.makeMove(move);
  board.verifyState();
	state = board.getState();
  updateTiming();
	return true;
}

void Thinking::save()
{
  if ( is_thinking() )
    return;

	ofstream ofs("game_001.pgn");
  const Board & board = player_.getBoard();
  bool res = Board::save(board, ofs);
}

void Thinking::fen2file(const char * fname)
{
  if ( is_thinking() )
    return;

  if ( !fname )
    return;

  try
  {
    ofstream ofs(fname);
    char fen[256];
    player_.getBoard().toFEN(fen);
    ofs << std::string(fen) << endl;
  }
  catch ( ... )
  {
  }
}

void Thinking::pgn2file(const char * fname)
{
  if ( !fname )
    return;

  try
  {
    ofstream ofs(fname);
    Board::save(player_.getBoard(), ofs);
  }
  catch ( ... )
  {
  }
}

void Thinking::hash2file(const char * fname)
{
  if ( is_thinking() || !fname )
    return;

  player_.saveHash(fname);
}

//////////////////////////////////////////////////////////////////////////
bool Thinking::fromFEN(xCmd & cmd)
{
  if ( !cmd.paramsNum() )
    return false;

  std::string str = cmd.packParams();
  if ( str.empty() )
    return false;

  const char * fen = str.c_str();

  if ( is_thinking() )
  {
    player_.postFEN(fen);
    return true;
  }

  return player_.fromFEN(fen);
}

void Thinking::toFEN(char * fen)
{
  if ( is_thinking() || !fen )
    return;

  player_.toFEN(fen);
}

//////////////////////////////////////////////////////////////////////////
// edit mode
//////////////////////////////////////////////////////////////////////////
void Thinking::editCmd(xCmd & cmd)
{
  if ( is_thinking() )
  {
    if ( cmd.type() == xCmd::xLeaveEdit ) // command '.'
      player_.postStatus();

    return;
  }

	switch ( cmd.type() )
	{
	case xCmd::xEdit:
		figureColor_ = Figure::ColorWhite;
		boardColor_ = player_.getBoard().getColor();
		break;

	case xCmd::xChgColor:
		figureColor_ = Figure::otherColor(figureColor_);
		break;

	case xCmd::xClearBoard:
      player_.getBoard().initEmpty(boardColor_);
		break;

	case xCmd::xSetFigure:
		setFigure(cmd);
		break;

	case xCmd::xLeaveEdit:
    player_.getBoard().invalidate();
		break;
	}
}

void Thinking::setFigure(xCmd & cmd)
{
  if ( is_thinking() )
    return;

	if ( !cmd.str() || strlen(cmd.str()) < 3 )
		return;

	char str[16];

	strncpy(str, cmd.str(), sizeof(str));
	_strlwr(str);

  int x = str[1] - 'a';
  if ( x < 0 || x > 7 )
    return;

	int y = str[2] - '1';
	if ( y < 0 || y > 7 )
		return;

	Figure::Type ftype = Figure::TypeNone;
	
	switch ( str[0] )
	{
	case 'p':
		ftype = Figure::TypePawn;
		break;

	case 'b':
		ftype = Figure::TypeBishop;
		break;

	case 'n':
		ftype = Figure::TypeKnight;
		break;

	case 'r':
		ftype = Figure::TypeRook;
		break;

	case 'q':
		ftype = Figure::TypeQueen;
		break;

	case 'k':
		ftype = Figure::TypeKing;
		break;
	}

	if ( Figure::TypeNone == ftype )
		return;

	bool firstStep = false;
	if ( Figure::TypeKing == ftype && ((Figure::ColorWhite == figureColor_ && 'e' == str[1] && '1' == str[2]) || (Figure::ColorBlack == figureColor_ && 'e' == str[1] && '8' == str[2])) )
	{
		firstStep = true;
	}

	if ( Figure::TypeRook == ftype )
	{
		if ( 'a' == str[1] || 'h' == str[1] )
			firstStep = Figure::ColorWhite == figureColor_ && '1' == str[2] || Figure::ColorBlack == figureColor_ && '8' == str[2];
	}

  Index pos(x, y);
	player_.getBoard().addFigure(figureColor_, ftype, pos);
}

//////////////////////////////////////////////////////////////////////////
void Thinking::updateTiming()
{
  if ( is_thinking() )
    return;

  if ( maxDepth_ > 0 )
  {
    player_.setTimeLimit(-1);
    return;
  }

  if ( timePerMoveMS_ > 0 )
  {
    player_.setTimeLimit(timePerMoveMS_);
    return;
  }

  if ( movesLeft_ <= 0 && xtimeMS_ > 0 )
  {
    player_.setTimeLimit(xtimeMS_/40);
    return;
  }

  int mcount = player_.getBoard().movesCount();
  if ( movesLeft_ > 0 && xtimeMS_ > 0 )
  {
    mcount = movesLeft_ - (mcount-1) % movesLeft_;
    mcount += 3;
    player_.setTimeLimit(xtimeMS_/mcount);

#ifdef WRITE_LOG_FILE_
    if ( ofs_log_ )
    {
      *ofs_log_ << "  time per move = " << xtimeMS_/mcount << " ms" << std::endl;
      *ofs_log_ << "  xtime  = " << xtimeMS_ << " ms" << std::endl;
      *ofs_log_ << "  mcount = " << mcount << std::endl;
    }
#endif
  }
}

//////////////////////////////////////////////////////////////////////////
#ifdef WRITE_LOG_FILE_
void Thinking::set_logfile(std::ofstream * ofslog)
{
  ofs_log_ = ofslog;
}
#endif
