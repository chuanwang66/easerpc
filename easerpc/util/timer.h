#pragma once

/*
	static void timer_cb(void *param) {
		printf("in timer_cb()\n");
	}

	setTimerCallback(timer_cb);
	Timer *timer = new Timer();
	timer->start(NULL, 5000, 1000);	//start in 5000ms, callback every 1000ms
	...
	timer->stop();
	delete timer;
	timer = NULL;
*/

#include <windows.h>

typedef void(*timer_callback_t)(void *);
void setTimerCallback(timer_callback_t callback);

class Timer{
public:
	Timer();
	int start(void *param, unsigned long delay, unsigned long period);
	int stop();
	virtual ~Timer();
private:
	HANDLE timer;
	HANDLE timerQueue;

	void *param;
};