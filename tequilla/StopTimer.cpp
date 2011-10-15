
#define _WIN32_WINNT 0x0500

#include "StopTimer.h"
#include <fstream>

using namespace std;

StopTimer::StopTimer(ChessAlgorithm * pcalg) :
	pcalg_(pcalg), dt_(0), hthread_(INVALID_HANDLE_VALUE), started_(false), leftT_(0), oleftT_(0), movesN_(0), movesLeft_(0)
{
	InitializeCriticalSection(&cs_);
}

StopTimer::~StopTimer()
{
	wait();
	DeleteCriticalSection(&cs_);
}

void StopTimer::wait()
{
	if ( INVALID_HANDLE_VALUE == hthread_ )
		return;

	WaitForSingleObject(hthread_, 3000);
	CloseHandle(hthread_);
	hthread_ = INVALID_HANDLE_VALUE;
}

void StopTimer::adjustDT()
{
  if ( movesLeft_ > 0 )
    dt_ = leftT_/(movesLeft_+5);
  else
    dt_ = leftT_/100;

  if ( oleftT_ > leftT_ )
    dt_ = dt_*leftT_/oleftT_;

  if ( pcalg_ )    
  {
    pcalg_->setMode(1);

    if ( leftT_ <= 5000 )
      pcalg_->setMode(0);
  }
}

void StopTimer::setTime(int t)
{
  leftT_ = t;
  adjustDT();
}

void StopTimer::setOTime(int t)
{
  oleftT_ = t;
  adjustDT();
}

void StopTimer::setMovesLeft(int movesN)
{
  movesLeft_ = movesN_ = movesN*2;
}

void StopTimer::setDt(int t)
{
  dt_ = t;
  movesN_ = movesLeft_ = 0;
  leftT_ = oleftT_ = 0;
  if ( pcalg_ )
    pcalg_->setMode(0);
}

int StopTimer::nextMove()
{
  if ( movesN_ <= 0 )
    return 0;

  movesLeft_--;
  if ( movesLeft_ <= 0 )
    movesLeft_ = movesN_;

  adjustDT();

  return movesLeft_;
}

void StopTimer::start()
{
	if ( dt_ <= 0 || started_ )
		return;

	guard_started(true);

	DWORD tid;
	hthread_ = CreateThread(0, 0, threadProc, this, 0, &tid);
}

void StopTimer::run()
{
	//{
	//	ofstream ofs("err.txt");
	//	ofs << "run entered";
	//}

	if ( !pcalg_ )
		return;

	HANDLE hTimer = NULL;
	LARGE_INTEGER liDueTime;

	liDueTime.QuadPart=-dt_*10000;//000;

	hTimer = CreateWaitableTimer(NULL, TRUE, "Dimock_WaitableTimer");

	if ( !hTimer )
	{
		pcalg_->stop();

		//ofstream ofs("err.txt");
		//ofs << "stopped, !timer";

		guard_started(false);
		return;
	}

	if ( !SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0) )
	{
		pcalg_->stop();

		//ofstream ofs("err.txt");
		//ofs << "stopped, !set";

		guard_started(false);
		return;
	}

	//{
	//	ofstream ofs("err.txt");
	//	ofs << "started for " << dt_;
	//}

	WaitForSingleObject(hTimer, INFINITE);

	pcalg_->stop();

	//{
	//	ofstream ofs("err.txt");
	//	ofs << "stopped, ok";
	//}


	guard_started(false);
}

void StopTimer::stop()
{
	guard_started(false);
}

DWORD WINAPI StopTimer::threadProc(LPVOID lpParam)
{
	if ( !lpParam )
		return 0;

	StopTimer * stimer = (StopTimer*)lpParam;
	stimer->run();
	return 1;
}

void StopTimer::guard_started(bool s)
{
	EnterCriticalSection(&cs_);
	started_ = s;
	LeaveCriticalSection(&cs_);
}