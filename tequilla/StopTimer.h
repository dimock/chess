#pragma once

#include "ChessAlg.h"
#include <windows.h>


class StopTimer
{
public:

	StopTimer(ChessAlgorithm * pcalg);
	~StopTimer();

  void setTime(int t);
  void setOTime(int t);
  void setMovesLeft(int movesN);
  void setDt(int t);

	void start();
	void stop();
	void wait();
  int  nextMove();

private:

	void run();
	void guard_started(bool );

  void adjustDT();

	static DWORD WINAPI threadProc(LPVOID lpParam);

	ChessAlgorithm * pcalg_;
	HANDLE hthread_;
	int dt_;
	volatile bool started_;
	CRITICAL_SECTION cs_;

  int leftT_, oleftT_, movesN_, movesLeft_;
};