#include <Windows.h>

#include <stdio.h>
#include <string.h>

#include "shared_block.h"

int shared_block_create(shared_block_ptr sblock_ptr, const char *name, unsigned long size) {
	int ret = 0;

	if (name == NULL || *name == 0){
		printf("shared block name is null\n");
		return -1;
	}
	if (size <= 0) {
		printf("shared block size should be > 0");
		return -2;
	}

	sblock_ptr->name = NULL;
	sblock_ptr->size = 0;
	sblock_ptr->mapFile = NULL;
	sblock_ptr->view = NULL;

	int name_len = strlen(name) + 1;
	sblock_ptr->name = malloc(name_len);
	strcpy(sblock_ptr->name, name);

	sblock_ptr->size = size;

	sblock_ptr->mapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,	//Use Paging file - shared memory
		NULL,	//default security attributes
		PAGE_READWRITE,
		0,		//high-order DWORD of file mapping max size
		size,	//low-order DOWRD of file mapping max size
		name	//name of the file mapping obj
		);
	if (sblock_ptr->mapFile == NULL) {
		printf("CreateFileMapping failed %d\n", GetLastError());
		ret = -3;
		goto create_failed;
	}
	
	sblock_ptr->view = MapViewOfFile(
		sblock_ptr->mapFile,
		FILE_MAP_ALL_ACCESS,
		0,			//high-order DWORD of the file offset
		0,			//low-order DWORD of the file offset
		size		//# of bytes to map to view
		);
	if (sblock_ptr->view == NULL) {
		printf("MapViewOfFile failed %d\n", GetLastError());
		ret = -4;
		goto create_failed;
	}

	return 0;

create_failed:
	if (sblock_ptr->name) {
		free(sblock_ptr->name);
		sblock_ptr->name = NULL;
	}
	sblock_ptr->size = 0;
	if (sblock_ptr->mapFile) {
		CloseHandle(sblock_ptr->mapFile);
		sblock_ptr->mapFile = NULL;
	}
	if (sblock_ptr->view) {
		UnmapViewOfFile(sblock_ptr->view);
		sblock_ptr->view = NULL;
	}

	return ret;
}

int shared_block_open(shared_block_ptr sblock_ptr, const char *name, unsigned long size) {
	int ret = 0;

	if (name == NULL || *name == 0){
		printf("shared block name is null\n");
		return -1;
	}
	if (size <= 0) {
		printf("shared block size should be > 0");
		return -2;
	}

	sblock_ptr->name = NULL;
	sblock_ptr->size = 0;
	sblock_ptr->mapFile = NULL;
	sblock_ptr->view = NULL;

	int name_len = strlen(name) + 1;
	sblock_ptr->name = malloc(name_len);
	strcpy(sblock_ptr->name, name);

	sblock_ptr->size = size;

	sblock_ptr->mapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
	if (sblock_ptr->mapFile == NULL) {
		printf("OpenFileMapping failed %d, object name=%s\n", GetLastError(), name);
		ret = -3;
		goto open_failed;
	}

	sblock_ptr->view = MapViewOfFile(
		sblock_ptr->mapFile,
		FILE_MAP_ALL_ACCESS,
		0,			//high-order DWORD of the file offset
		0,			//low-order DWORD of the file offset
		size		//# of bytes to map to view
		);
	if (sblock_ptr->view == NULL) {
		printf("MapViewOfFile failed %d\n", GetLastError());
		ret = -4;
		goto open_failed;
	}

	return 0;

open_failed:
	if (sblock_ptr->name) {
		free(sblock_ptr->name);
		sblock_ptr->name = NULL;
	}
	sblock_ptr->size = 0;
	if (sblock_ptr->mapFile) {
		CloseHandle(sblock_ptr->mapFile);
		sblock_ptr->mapFile = NULL;
	}
	if (sblock_ptr->view) {
		UnmapViewOfFile(sblock_ptr->view);
		sblock_ptr->view = NULL;
	}
	return ret;
}

void shared_block_close(struct shared_block sb) {
	if (sb.name)
		free(sb.name);
	sb.size = 0;
	if (sb.view) {
		UnmapViewOfFile(sb.view);
		sb.view = NULL;
	}
	if (sb.mapFile) {
		CloseHandle(sb.mapFile);
		sb.mapFile = NULL;
	}
}

void shared_block_info(struct shared_block sb) {
	printf("shared_block_info(\n"
		"\tname=%s,\n"
		"\tsize=%ld,\n"
		"\tmapFile=0x%08x, \n"
		"\tview=0x%08x\n"
		")\n",
		sb.name,
		sb.size,
		sb.mapFile,
		sb.view
		);
}