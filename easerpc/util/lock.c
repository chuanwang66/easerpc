#include "lock.h"

#include <stdio.h>
#include <windows.h>

void *lock_create(const char *name) {
	//CreateMutexW (Unicode) vs. CreateMutexA (ANSI)
	HANDLE mutex = CreateMutex(NULL, FALSE, name);
	if (mutex == NULL) {
		printf("CreateMutex error: %d\n", GetLastError());
	}
	else {
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			; //printf("CreateMutex opened an existing mutex\n");
		else
			; //printf("CreateMutex created a new mutex\n");
	}
	return mutex;
}

void *lock_open(const char *name) {
	if (name == NULL || *name == 0) {
		//printf("lock_open(): lock name is null");
		return NULL;
	}

	HANDLE mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, name);
	//if (mutex == NULL) {
	//	printf("OpenMutex error: %d\n", GetLastError());
	//}
	return mutex;
}

void lock_close(void *handle) {
	if (handle) {
		//ReleaseMutex(handle);
		CloseHandle(handle);
	}
}

void lock_lock(void *handle) {
	if (handle) {
		WaitForSingleObject(handle, INFINITE);
	}
}

void lock_unlock(void *handle) {
	if (handle) {
		ReleaseMutex(handle);
	}
}