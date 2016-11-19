#include "lock.h"

#include <stdio.h>
#include "timer.h"

static timer_callback_t timer_callback = NULL;
void setTimerCallback(timer_callback_t callback) {
	timer_callback = callback;
}

VOID CALLBACK timer_routine(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
	void *param = lpParam;
	if (timer_callback) {
		timer_callback(param);
	}
}

Timer::Timer()
	: timer(NULL), timerQueue(NULL) {
	timerQueue = CreateTimerQueue();
	if (NULL == timerQueue) {
		printf("CreateTimerQueue failed %d\n", GetLastError());
	}
}

int Timer::start(void *param, unsigned long delay, unsigned long period) {
	if (timerQueue == NULL) return -1;

	if (!CreateTimerQueueTimer(&timer, timerQueue, (WAITORTIMERCALLBACK)timer_routine, param,
		delay, period, 0)) {
		printf("CreateTimerQueueTimer failed %d\n", GetLastError());
		return -2;
	}
	return 0;
}

int Timer::stop() {
	if (timerQueue == NULL) return -1;
	if (timer == NULL) return -2;
	if (!DeleteTimerQueueTimer(timerQueue, timer, NULL)) {
		printf("DeleteTimerQueueTimer failed %d\n", GetLastError());
		return -3;
	}
	return 0;
}

Timer::~Timer() {
	if (timerQueue) {
		if (!DeleteTimerQueue(timerQueue)) {
			printf("DeleteTimerQueue failed %d\n", GetLastError());
		}
	}
}