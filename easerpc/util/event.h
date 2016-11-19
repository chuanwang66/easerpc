#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum event_type {
	EVENT_TYPE_AUTO,
	EVENT_TYPE_MANUAL
};

void *event_create(enum event_type type, const char *name);
void *event_open(const char *name);
void event_close(void *handle);

int event_wait(void *handle);
int event_timedwait(void *handle, unsigned long milliseconds);
int event_try(void *handle);

//注: 进程A signal event之后，进程B才启动，则进程B收不到event. 因此，需要保证多个进程都启动后，才使用event同步机制
int event_signal(void *handle);

void event_reset(void *handle);

#ifdef __cplusplus
}
#endif