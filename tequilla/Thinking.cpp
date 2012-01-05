#include "Thinking.h"
#include <iostream>
#include <fstream>
//#include "qpf_timer.h"

using namespace std;

Thinking::Thinking() :
	boardColor_(Figure::ColorWhite), figureColor_(Figure::ColorWhite),
  xtimeMS_(0), movesLeft_(0), timePerMoveMS_(0), maxDepth_(-1)
{
}

Thinking::~Thinking()
{
}

void Thinking::setDepth(int depth)
{
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
  maxDepth_ = -1;
  xtimeMS_ = 0;
  movesLeft_ = 0;
  timePerMoveMS_ = ms;
  if ( timePerMoveMS_ < 100 )
    timePerMoveMS_ = 100;
  player_.setTimeLimit(ms);
  player_.setMaxDepth(16);
}

void Thinking::setXtime(int ms)
{
  maxDepth_ = -1;
  xtimeMS_ = ms;
  if ( xtimeMS_ < 100 )
    xtimeMS_ = 100;
  timePerMoveMS_ = -1;
  player_.setMaxDepth(16);
}

void Thinking::setMovesLeft(int mleft)
{
  maxDepth_ = -1;
  movesLeft_ = mleft;
  timePerMoveMS_ = -1;
  player_.setMaxDepth(16);
}

void Thinking::enableBook(int v)
{
}

void Thinking::undo()
{
  Board & board = player_.getBoard();
  if ( board.halfmovesCount() > 0 )
    board.unmakeMove();
}

void Thinking::setMemory(int mb)
{
  player_.setMemory(mb);
}

bool Thinking::init()
{
  return player_.fromFEN(0);
}

bool Thinking::reply(char (& smove)[256], Board::State & state, bool & white)
{
  updateTiming();

  Board & board = player_.getBoard();
	white = Figure::ColorWhite == board.getColor();
  state = board.getState();

  if ( Board::isDraw(state) || Board::ChessMat == state )
    return true;

	SearchResult sres;
  if ( player_.findMove(sres, &cout) )
  {
    if ( board.makeMove(sres.best_) )
      board.verifyState();
    else
    {
      board.unmakeMove();
      return false;
    }
  }

  state = board.getState();

	if ( !moveToStr(sres.best_, smove, true) )
		return false;

	return true;
}

bool Thinking::move(xCmd & moveCmd, Board::State & state, bool & white)
{
	if ( moveCmd.type() != xCmd::xMove )
		return false;

  Board & board = player_.getBoard();
	Figure::Color color = board.getColor();
	Figure::Color ocolor = Figure::otherColor(color);
  state = board.getState();

  if ( Board::isDraw(state) || Board::ChessMat == state )
    return true;

	white = Figure::ColorWhite == color;

	Move move;
	if ( !strToMove(moveCmd.str(), board, move) )
		return false;

  if ( board.makeMove(move) )
    board.verifyState();
  else
  {
    board.unmakeMove();
    return false;
  }

	state = board.getState();
  updateTiming();
	return true;
}

void Thinking::save()
{
	ofstream ofs("game_001.pgn");
  const Board & board = player_.getBoard();
  bool res = Board::save(board, ofs);
}

void Thinking::fen2file(const char * fname)
{
  if ( !fname )
    return;

  ofstream ofs(fname);
  char fen[256];
  player_.getBoard().toFEN(fen);
  ofs << std::string(fen) << endl;
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
  return player_.fromFEN(fen);
}

//////////////////////////////////////////////////////////////////////////
// edit mode
//////////////////////////////////////////////////////////////////////////
void Thinking::editCmd(xCmd & cmd)
{
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

	Figure fig(ftype, figureColor_, x, y, firstStep);

	player_.getBoard().addFigure(fig);
}

//////////////////////////////////////////////////////////////////////////
void Thinking::updateTiming()
{
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
    player_.setTimeLimit(xtimeMS_/30);
    return;
  }

  int mcount = player_.getBoard().movesCount();
  if ( movesLeft_ > 0 && xtimeMS_ > 0 )
  {
    mcount = movesLeft_ - (mcount-1) % movesLeft_;
    mcount += 3;
    player_.setTimeLimit(xtimeMS_/mcount);
  }
}
