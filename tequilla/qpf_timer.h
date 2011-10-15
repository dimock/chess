#pragma once

class QpfTimer
{
public:

	QpfTimer();
  ~QpfTimer();

	void start();
	void stop();
	bool started();
	double getDt() const;

private:
	bool started_;
	volatile __int64 freq_, t0_;
};
