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

//ע: ����A signal event֮�󣬽���B�������������B�ղ���event. ��ˣ���Ҫ��֤������̶������󣬲�ʹ��eventͬ������
int event_signal(void *handle);

void event_reset(void *handle);

#ifdef __cplusplus
}
#endif