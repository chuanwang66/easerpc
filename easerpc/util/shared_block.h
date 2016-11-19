#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct shared_block{
	char *name;
	long size;

	void *mapFile;
	void *view;
}shared_block;

typedef struct shared_block *shared_block_ptr;

int shared_block_create(shared_block_ptr sblock_ptr, const char *name, unsigned long size);

int shared_block_open(shared_block_ptr sblock_ptr, const char *name, unsigned long size);

void shared_block_close(struct shared_block sb);

void shared_block_info(struct shared_block sb);

#ifdef __cplusplus
}
#endif