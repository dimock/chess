#include "Thinking.h"
#include <iostream>
#include <fstream>
//#include "qpf_timer.h"

using namespace std;

Thinking::Thinking(ChessAlgorithm * pcalg) :
	pcalg_(pcalg), boardColor_(Figure::ColorWhite), figureColor_(Figure::ColorWhite)
{
}

Thinking::~Thinking()
{
}

void Thinking::setDepth(int depth)
{
	if ( !pcalg_ )
		return;

	pcalg_->setDepth(depth);
  pcalg_->setMode(0);
}

void Thinking::enableBook(int v)
{
  if ( pcalg_ )
    pcalg_->enableBook(v != 0);
}

void Thinking::setMode(int mode)
{
  if ( pcalg_ )
    pcalg_->setMode(mode);
}

void Thinking::undo()
{
	if ( !pcalg_ )
		return;

	pcalg_->prevPos();
}

void Thinking::setMemory(int mb)
{
  if ( !pcalg_ )
    return;

  pcalg_->setMemory(mb);
}

bool Thinking::init()
{
	if ( !pcalg_ )
		return false;

	pcalg_->init(Figure::ColorWhite);
  pcalg_->setMode(0);

	Board & board = *pcalg_->getCurrent();

	for (char i = 'a'; i <= 'h'; ++i)
	{
		Figure pawn(Figure::TypePawn, i, 7, Figure::ColorBlack, true);
		board.addFigure(pawn);
	}

	for (char i = 'a'; i <= 'h'; ++i)
	{
		Figure pawn(Figure::TypePawn, i, 2, Figure::ColorWhite, true);
		board.addFigure(pawn);
	}

	{
		Figure knight1(Figure::TypeKnight, 'b', 8, Figure::ColorBlack, true);
		Figure knight2(Figure::TypeKnight, 'g', 8, Figure::ColorBlack, true);

		Figure bishop1(Figure::TypeBishop, 'c', 8, Figure::ColorBlack, true);
		Figure bishop2(Figure::TypeBishop, 'f', 8, Figure::ColorBlack, true);

		Figure rook1(Figure::TypeRook, 'a', 8, Figure::ColorBlack, true);
		Figure rook2(Figure::TypeRook, 'h', 8, Figure::ColorBlack, true);

		Figure queen(Figure::TypeQueen, 'd', 8, Figure::ColorBlack, true);
		Figure king(Figure::TypeKing, 'e', 8, Figure::ColorBlack, true);

		board.addFigure(knight1);
		board.addFigure(knight2);

		board.addFigure(bishop1);
		board.addFigure(bishop2);

		board.addFigure(rook1);
		board.addFigure(rook2);

		board.addFigure(queen);
		board.addFigure(king);
	}
	{
		Figure knight1(Figure::TypeKnight, 'b', 1, Figure::ColorWhite, true);
		Figure knight2(Figure::TypeKnight, 'g', 1, Figure::ColorWhite, true);

		Figure bishop1(Figure::TypeBishop, 'c', 1, Figure::ColorWhite, true);
		Figure bishop2(Figure::TypeBishop, 'f', 1, Figure::ColorWhite, true);

		Figure rook1(Figure::TypeRook, 'a', 1, Figure::ColorWhite, true);
		Figure rook2(Figure::TypeRook, 'h', 1, Figure::ColorWhite, true);

		Figure queen(Figure::TypeQueen, 'd', 1, Figure::ColorWhite, true);
		Figure king(Figure::TypeKing, 'e', 1, Figure::ColorWhite, true);

		board.addFigure(knight1);
		board.addFigure(knight2);

		board.addFigure(bishop1);
		board.addFigure(bishop2);

		board.addFigure(rook1);
		board.addFigure(rook2);

		board.addFigure(queen);
		board.addFigure(king);
	}

	return true;
}

bool Thinking::reply(char * smove, Board::State & state, bool & white)
{
	if ( !pcalg_ || !pcalg_->getCurrent() )
		return false;

	white = Figure::ColorWhite == pcalg_->getCurrent()->getColor();

  Board & board0 = *pcalg_->getCurrent();
  state = board0.getState();

  if ( Board::isDraw(state) || Board::ChessMat == state )
    return true;

	CalcResult cres;
	int depth = pcalg_->doBestStep(cres, cout);
	if ( 0 == depth || !cres.best_ )
    return false;

	Step step;
	if ( pcalg_->lastStep() )
		step = *pcalg_->lastStep();

	if ( !step )
		return false;

	Board & board = *pcalg_->getCurrent();
	state = board.getState();

	if ( !moveToStr(step, smove) )
		return false;

	return true;
}

bool Thinking::move(xCmd & moveCmd, Board::State & state, bool & white)
{
	if ( !pcalg_ || !pcalg_->getCurrent() || moveCmd.type() != xCmd::xMove )
		return false;

	Figure::Color color = pcalg_->getCurrent()->getColor();
	Figure::Color ocolor = Figure::otherColor(color);

  Board & board0 = *pcalg_->getCurrent();
  state = board0.getState();

  if ( Board::isDraw(state) || Board::ChessMat == state )
    return true;

	white = Figure::ColorWhite == color;

	StepId sid;
	if ( !strToMove(moveCmd.str(), pcalg_->getCurrent(), sid) )
		return false;

	vector<Step> steps;
	if ( !pcalg_->calculateSteps(steps) )
    return false;

	Step step;
	step.clear();

	for (size_t i = 0; i < steps.size(); ++i)
	{
		Step & s = steps[i];
		if ( sid == s )
		{
			step = s;
			break;
		}
	}

	if ( !step )
		return false;

	pcalg_->applyStep(step);

	Board & board1 = *pcalg_->getCurrent();
	state = board1.getState();

	return true;
}

void Thinking::save()
{
	if ( !pcalg_ || !pcalg_->getCurrent() )
		return;

	ofstream ofs("board_001.pos");
	pcalg_->save(ofs);
}

//////////////////////////////////////////////////////////////////////////
// edit mode
//////////////////////////////////////////////////////////////////////////
void Thinking::editCmd(xCmd & cmd)
{
	if ( !pcalg_ )
		return;

	switch ( cmd.type() )
	{
	case xCmd::xEdit:
		figureColor_ = Figure::ColorWhite;
		boardColor_ = pcalg_->getCurrent() ? pcalg_->getCurrent()->getColor() : Figure::ColorWhite;
		break;

	case xCmd::xChgColor:
		figureColor_ = Figure::otherColor(figureColor_);
		break;

	case xCmd::xClearBoard:
		pcalg_->init(boardColor_);
		break;

	case xCmd::xSetFigure:
		setFigure(cmd);
		break;

	case xCmd::xLeaveEdit:
		if ( pcalg_->getCurrent() )
			pcalg_->getCurrent()->validatePosition();
		break;
	}
}

void Thinking::setFigure(xCmd & cmd)
{
	if ( !pcalg_ || !pcalg_->getCurrent() || !cmd.str() || strlen(cmd.str()) < 3 )
		return;

	char str[16];

	strncpy(str, cmd.str(), sizeof(str));
	strlwr(str);

	int y = str[2] - '1' + 1;
	if ( y < 1 || y > 8 )
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
	if ( Figure::TypeKing == ftype &&
		( (Figure::ColorWhite == figureColor_ && 'e' == str[1] && '1' == str[2]) ||
		  (Figure::ColorBlack == figureColor_ && 'e' == str[1] && '8' == str[2]) ) )
	{
		firstStep = true;
	}

	if ( Figure::TypeRook == ftype )
	{
		if ( 'a' == str[1] || 'h' == str[1] )
			firstStep = Figure::ColorWhite == figureColor_ && '1' == str[2] || Figure::ColorBlack == figureColor_ && '8' == str[2];
	}

	Figure fig(ftype, str[1], y, figureColor_, firstStep);

	pcalg_->getCurrent()->addFigure(fig);
}