#pragma once

/*************************************************************
  Thinking.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Player.h"
#include "xcommands.h"
#include <Windows.h>

#undef WRITE_LOG_FILE_
#define WRITE_ERROR_PGN

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
  void pgn2file(const char * fname);
  void hash2file(const char * fname);
  void toFEN(char * );

  bool fromFEN(xCmd & cmd);
	void editCmd(xCmd & cmd);

	bool move(xCmd & moveCmd, uint8 & state, bool & white);
	bool reply(char (&)[256], uint8 & state, bool & white);
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