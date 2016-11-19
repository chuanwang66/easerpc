#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void *lock_create(const char *name);
void *lock_open(const char *name);
void lock_close(void *handle);

void lock_lock(void *handle);
void lock_unlock(void *handle);

#ifdef __cplusplus
}
#endif