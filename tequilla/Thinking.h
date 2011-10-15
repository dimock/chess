#pragma once

#include "ChessAlg.h"
#include <windows.h>
#include "xcommands.h"

class Thinking
{
public:

	Thinking(ChessAlgorithm * pcalg);
	~Thinking();

	bool init();
  void enableBook(int v);
  void setMemory(int mb);
	void setDepth(int depth);
  void setMode(int mode);
	void undo();

	void save();

	void editCmd(xCmd & cmd);

	bool move(xCmd & moveCmd, Board::State & state, bool & white);
	bool reply(char * smove, Board::State & state, bool & white);

private:

	void setFigure(xCmd & cmd);

	Figure::Color boardColor_, figureColor_;

	ChessAlgorithm * pcalg_;
};