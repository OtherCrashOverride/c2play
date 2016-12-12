#pragma once

#include <cmath>
#include <csignal>
#include <ctime>



class Timer
{
	timer_t timer_id;


	double interval = 0;
	bool isRunning = false;


	static void timer_thread(sigval arg)
	{
		Timer* timerPtr = (Timer*)arg.sival_ptr;
		timerPtr->Expired.Invoke(timerPtr, EventArgs::Empty());

		//printf("Timer: expired arg=%p.\n", arg.sival_ptr);
	}

public:
	Event<EventArgs> Expired;


	double Interval() const
	{
		return interval;
	}
	void SetInterval(double value)
	{
		interval = value;
	}



	Timer()
	{
		sigevent se = { 0 };

		se.sigev_notify = SIGEV_THREAD;
		se.sigev_value.sival_ptr = &timer_id;
		se.sigev_notify_function = &Timer::timer_thread;
		se.sigev_notify_attributes = NULL;
		se.sigev_value.sival_ptr = this;

		int ret = timer_create(CLOCK_REALTIME, &se, &timer_id);
		if (ret != 0)
		{
			throw Exception("Timer creation failed.");
		}
	}

	~Timer()
	{
		timer_delete(timer_id);
	}


	void Start()
	{
		if (isRunning)
			throw InvalidOperationException();


		itimerspec ts = { 0 };

		// arm value
		ts.it_value.tv_sec = floor(interval);
		ts.it_value.tv_nsec = floor((interval - floor(interval)) * 1e9);

		// re-arm value
		ts.it_interval.tv_sec = ts.it_value.tv_sec;
		ts.it_interval.tv_nsec = ts.it_value.tv_nsec;

		int ret = timer_settime(timer_id, 0, &ts, 0);
		if (ret != 0)
		{
			throw Exception("timer_settime failed.");
		}

		isRunning = true;
	}

	void Stop()
	{
		if (!isRunning)
			throw InvalidOperationException();


		itimerspec ts = { 0 };

		int ret = timer_settime(timer_id, 0, &ts, 0);
		if (ret != 0)
		{
			throw Exception("timer_settime failed.");
		}

		isRunning = false;
	}
};
