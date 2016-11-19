#include "event.h"

#include <errno.h>
#include <Windows.h>

void *event_create(enum event_type type, const char *name) {
	HANDLE handle = CreateEvent(NULL, (type == EVENT_TYPE_MANUAL), FALSE, name);
	return handle;
}

void *event_open(const char *name) {
	if (name == NULL || *name == 0)
		return NULL;
	HANDLE handle = OpenEvent(EVENT_ALL_ACCESS, FALSE, name);
	return handle;
}

void event_close(void *handle) {
	if (handle)
		CloseHandle((HANDLE)handle);
}

int event_wait(void *handle) {
	DWORD code;
	if (!handle) 
		return EINVAL;

	code = WaitForSingleObject((HANDLE)handle, INFINITE);
	if (code != WAIT_OBJECT_0)
		return EINVAL;
	return 0;
}

int event_timedwait(void *handle, unsigned long milliseconds) {
	DWORD code;
	if (!handle)
		return EINVAL;

	code = WaitForSingleObject((HANDLE)handle, milliseconds);
	if (code == WAIT_TIMEOUT)
		return ETIMEDOUT;
	else if (code != WAIT_OBJECT_0)
		return EINVAL;
	return 0;
}

int event_try(void *handle) {
	DWORD code;
	if (!handle)
		return EINVAL;

	code = WaitForSingleObject((HANDLE)handle, 0);
	if (code == WAIT_TIMEOUT)
		return EAGAIN;
	else if (code != WAIT_OBJECT_0)
		return EINVAL;
	return 0;
}

int event_signal(void *handle) {
	if (!handle)
		return EINVAL;
	if (!SetEvent((HANDLE)handle))
		return EINVAL;
	return 0;
}

void event_reset(void *handle) {
	if (!handle)
		return;
	ResetEvent((HANDLE)handle);
}