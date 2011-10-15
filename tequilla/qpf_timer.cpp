#include "qpf_timer.h"
#include <windows.h>
#include <winnt.h>

QpfTimer::QpfTimer() : freq_(0), t0_(0), started_(false)
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq_);
}

QpfTimer::~QpfTimer()
{
}

void QpfTimer::start()
{
	QueryPerformanceCounter((LARGE_INTEGER*)&t0_);
	started_ = true;
}

void QpfTimer::stop()
{
	started_ = false;
}

bool QpfTimer::started()
{
	return started_;
}

double QpfTimer::getDt() const
{
	if ( !started_ )
		return 0;

	__int64 t1;
	QueryPerformanceCounter((LARGE_INTEGER*)&t1);
	double dt = (double)(t1 - t0_)/(double)freq_;
	return dt;
}
