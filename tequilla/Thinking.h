#pragma once

#include "Player.h"
#include "xcommands.h"
#include <Windows.h>

#define WRITE_LOG_FILE_

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
	bool undo();

	void save();
  void fen2file(const char * fname);
  void hash2file(const char * fname);
  void toFEN(char * );

  bool fromFEN(xCmd & cmd);
	void editCmd(xCmd & cmd);

	bool move(xCmd & moveCmd, Board::State & state, bool & white);
	bool reply(char (&)[256], Board::State & state, bool & white);
  void analyze();
  void stop();

  void setPlayerCallback(PLAYER_CALLBACK );
  
  int giveMoreTime();

  bool is_thinking() const { return thinking_; }

#ifdef WRITE_LOG_FILE_
  void set_logfile(std::ofstream * ofslog);
#endif

private:

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
  bool thinking_;
  unsigned int givetimeCounter_;

#ifdef WRITE_LOG_FILE_
  std::ofstream * ofs_log_;
#endif
};