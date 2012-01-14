#pragma once

#include "Player.h"
#include "xcommands.h"
#include <Windows.h>

class Thinking
{
public:

	Thinking();
	~Thinking();

	bool init();
  void enableBook(int v);
  void setMemory(int mb);
	void setDepth(int depth);
  void setTimePerMove(int ms);
  void setXtime(int ms);
  void setMovesLeft(int mleft);
  void setPost(bool);
	void undo();

	void save();
  void fen2file(const char * fname);

  bool fromFEN(xCmd & cmd);
	void editCmd(xCmd & cmd);

	bool move(xCmd & moveCmd, Board::State & state, bool & white);
	bool reply(char (&)[256], Board::State & state, bool & white);
  void analyze();
  void stop();

private:

  static DWORD WINAPI analyze_proc(void *);

  bool is_analyzing() const
  {
    return analyze_thread_ != INVALID_HANDLE_VALUE;
  }

  void performAnalyze();
  void updateTiming();
	void setFigure(xCmd & cmd);

	Figure::Color boardColor_, figureColor_;
	Player player_;
  int movesLeft_;
  int xtimeMS_;
  int maxDepth_;
  int timePerMoveMS_;
  bool post_;
  HANDLE analyze_thread_;
};