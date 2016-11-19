#include "easerpc.h"

#include <windows.h>

#include <map>
#include <stdio.h>

#include "cJSON.h"
#include "event.h"
#include "lock.h"
#include "seq.h"
#include "shared_block.h"

const unsigned long SBLOCK_SIZE = 1024;	//shared memory size

#define SHARED_OBJ_SIZE 128

#define REQ_LOCK_OBJNAME "Local\\rpc_req_lock"

#define SRV_LOCK_OBJNAME "Local\\rpc_server_lock%d"
#define SRV_EVENT_OBJNAME "Local\\rpc_server_event%d"
#define SRV_SBLOCK_OBJNAME "Local\\rpc_server_sblock%d"

#define CLI_LOCK_OBJNAME "Local\\rpc_client_lock%d"
#define CLI_EVENT_OBJNAME "Local\\rpc_client_event%d"
#define CLI_SBLOCK_OBJNAME "Local\\rpc_client_sblock%d"

////////////////////////////// rpc server stub //////////////////////////////

/* function registry */
#define MAX_FUNC_NO 256
static int func_no = 0;

//std::map<..., func pointer> cannot be used in stl with ease :(
static void *func_lock = NULL;
static bool func_valid_array[MAX_FUNC_NO];
static std::string func_name_array[MAX_FUNC_NO];
static void(*func_pointer_array[MAX_FUNC_NO])(const char *, char *, int);

/* server stub thread */
static HANDLE thread_handle = NULL;
static volatile BOOL thread_stop = false;

/* server stub objects */
void *server_sblock_lock = NULL;
void *server_sblock_event = NULL;
shared_block server_sblock;

/* client stub objects */
void *client_sblock_lock = NULL;
void *client_sblock_event = NULL;
shared_block client_sblock;

/* current stub id, acting as both a server stub and a client stub*/
static short server_id = -1;

static int rpc_response(short sid, short cid, const char *response, const char *ctx, int seq);

static DWORD CALLBACK worker_thread_proc(LPVOID param) {
	cJSON *request_json = (cJSON *)param;

	short cid = cJSON_GetObjectItem(request_json, "cid")->valueint;
	short sid = cJSON_GetObjectItem(request_json, "sid")->valueint;
	char *fn = cJSON_GetObjectItem(request_json, "fn")->valuestring;
	char *arg = cJSON_GetObjectItem(request_json, "arg")->valuestring;
	int seq = cJSON_GetObjectItem(request_json, "seq")->valueint;
	const char *ctx = cJSON_GetObjectItem(request_json, "ctx") ?
		cJSON_GetObjectItem(request_json, "ctx")->valuestring : 0;

	char *res = (char *)malloc(SBLOCK_SIZE * sizeof(char));
	res[0] = 0;

	//find function by name
	void(*fp)(const char *, char *, int) = NULL;
	lock_lock(func_lock);
	for (int i = 0; i < MAX_FUNC_NO; ++i) {
		if (func_valid_array[i] == true && func_name_array[i].compare(fn) == 0) {
			fp = func_pointer_array[i];
		}
	}
	lock_unlock(func_lock);

	if (fp) {
		fp((const char *)arg, res, SBLOCK_SIZE);
	}
	else {
		sprintf(res, "{code=%d}", RPC_RESCODE_NOT_FOUND);
		printf("function [%s] not found.\n", fn);
	}
	
	rpc_response(sid, cid, res, ctx, seq);

	free(res);
	res = NULL;
	cJSON_Delete(request_json);
	request_json = NULL;
	return 0;
}

static void process_function(char *sblock_str, short server_id) {
	cJSON *request_json = cJSON_Parse(sblock_str);

	HANDLE worker_thread_handle = CreateThread(NULL, 0, worker_thread_proc, (LPVOID)request_json, 0, NULL);
	if (worker_thread_handle && worker_thread_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(worker_thread_handle);	//the worker thread is still running, but cannot be manifested
	}
	else {
		static char res[128];
		printf("create worker thread failed [%d].\n", GetLastError());

		short cid = cJSON_GetObjectItem(request_json, "cid")->valueint;
		short sid = cJSON_GetObjectItem(request_json, "sid")->valueint;
		int seq = cJSON_GetObjectItem(request_json, "seq")->valueint;
		const char *ctx = cJSON_GetObjectItem(request_json, "ctx") ?
			cJSON_GetObjectItem(request_json, "ctx")->valuestring : 0;

		sprintf(res, "{code=%d}", RPC_RESCODE_FAIL);
		rpc_response(sid, cid, res, ctx, seq);

		cJSON_Delete(request_json);
		request_json = NULL;
	}
	return;
}

static DWORD CALLBACK thread_proc(LPVOID param) {
	short sid = (short)param;
	server_id = sid;

	while (!thread_stop) {
		event_wait(server_sblock_event); //blocking wait

		if (thread_stop)
			break;

		lock_lock(server_sblock_lock);
		process_function((char *)server_sblock.view, server_id);	//process in a seperate thread to avoid blocking
		lock_unlock(server_sblock_lock);

		event_reset(server_sblock_event);
	}

	return 0;
}

int rpc_initialize(short sid) {
	int ret = 0;

	if (thread_handle && thread_handle != INVALID_HANDLE_VALUE) {
		printf("cannot initialize stub twice.\n");
		return -1;
	}

	char server_sblock_lock_name[SHARED_OBJ_SIZE];
	char server_sblock_event_name[SHARED_OBJ_SIZE];
	char server_sblock_name[SHARED_OBJ_SIZE];

	char client_sblock_lock_name[SHARED_OBJ_SIZE];
	char client_sblock_event_name[SHARED_OBJ_SIZE];
	char client_sblock_name[SHARED_OBJ_SIZE];

	sprintf_s(server_sblock_lock_name, SRV_LOCK_OBJNAME, sid);
	sprintf_s(server_sblock_event_name, SRV_EVENT_OBJNAME, sid);
	sprintf_s(server_sblock_name, SRV_SBLOCK_OBJNAME, sid);

	sprintf_s(client_sblock_lock_name, CLI_LOCK_OBJNAME, sid);
	sprintf_s(client_sblock_event_name, CLI_EVENT_OBJNAME, sid);
	sprintf_s(client_sblock_name, CLI_SBLOCK_OBJNAME, sid);

	//check existence of the same stub id
	{
		void *tmp_lock = lock_open(server_sblock_lock_name);
		if (tmp_lock) {
			printf("stub id %d already exists.\n", sid);
			lock_close(tmp_lock);
			return -2;
		}
	}

	//rpc function registry
	func_lock = lock_create(NULL);
	if (func_lock == NULL) {
		printf("create server function lock failed.\n");
		return -3;
	}

	lock_lock(func_lock);
	func_no = 0;
	for (int i = 0; i < MAX_FUNC_NO; ++i) {
		func_valid_array[i] = false;
		func_name_array[i] = "";
		func_pointer_array[i] = NULL;
	}
	lock_unlock(func_lock);

	//create server stub objects
	server_sblock_lock = NULL;
	server_sblock_event = NULL;

	server_sblock.name = NULL;
	server_sblock.size = 0;
	server_sblock.mapFile = NULL;
	server_sblock.view = NULL;

	client_sblock_lock = NULL;
	client_sblock_event = NULL;

	client_sblock.name = NULL;
	client_sblock.size = 0;
	client_sblock.mapFile = NULL;
	client_sblock.view = NULL;

	server_sblock_lock = lock_create(server_sblock_lock_name);
	if (server_sblock_lock == NULL) {
		printf("rpc_server_initialize() failed: create server lock failed.\n");
		ret = -4;
		goto rpc_initialize_fail;
	}
	server_sblock_event = event_create(EVENT_TYPE_MANUAL, server_sblock_event_name);
	if (server_sblock_event == NULL) {
		printf("rpc_server_initialize() failed: create server event failed.\n");
		ret = -5;
		goto rpc_initialize_fail;
	}

	if (shared_block_create(&server_sblock, server_sblock_name, SBLOCK_SIZE) != 0) {
		printf("rpc_server_initialize() failed: create server block failed.\n");
		ret = -6;
		goto rpc_initialize_fail;
	}

	client_sblock_lock = lock_create(client_sblock_lock_name);
	if (client_sblock_lock == NULL) {
		printf("rpc_server_initialize() failed: create client lock failed.\n");
		ret = -7;
		goto rpc_initialize_fail;
	}
	client_sblock_event = event_create(EVENT_TYPE_MANUAL, client_sblock_event_name);
	if (client_sblock_event == NULL) {
		printf("rpc_server_initialize() failed: create client event failed.\n");
		ret = -8;
		goto rpc_initialize_fail;
	}

	if (shared_block_create(&client_sblock, client_sblock_name, SBLOCK_SIZE) != 0) {
		printf("rpc_server_initialize() failed: create client block failed.\n");
		ret = -9;
		goto rpc_initialize_fail;
	}

	//start rpc server stub thread
	thread_stop = false;
	thread_handle = CreateThread(NULL, 10 * 1024 * 1024, thread_proc, (LPVOID)sid, 0, NULL); //default stack size: 1M; we use 10M
	if (thread_handle && thread_handle != INVALID_HANDLE_VALUE);
	else {
		printf("rpc_server_initialize() failed [%d].\n", GetLastError());
		ret = -10;
		goto rpc_initialize_fail;
	}

rpc_initialize_ok:
	return ret;

rpc_initialize_fail:
	shared_block_close(server_sblock);

	event_close(server_sblock_event);
	server_sblock_event = NULL;

	lock_close(server_sblock_lock);
	server_sblock_lock = NULL;

	shared_block_close(client_sblock);

	event_close(client_sblock_event);
	client_sblock_event = NULL;

	lock_close(client_sblock_lock);
	client_sblock_lock = NULL;

	return ret;
}

int rpc_register_function(const char *fname, ftype fpointer) {
	int ret = 0;

	if (fname == NULL || *fname == 0)
		return -1;
	if (fpointer == NULL)
		return -2;

	lock_lock(func_lock);

	//check existence
	bool exist = false;
	for (int i = 0; i < MAX_FUNC_NO; ++i) {
		if (func_name_array[i].compare(fname) == 0) {
			exist = true;
			break;
		}
	}
	if (exist) {
		printf("function [%s] exists.\n", fname);
		ret = -3;
		goto rpc_register_fail;
	}

	//find empty slot
	int pos = -1;
	for (int i = 0; i < MAX_FUNC_NO; ++i) {
		if (func_valid_array[(func_no + i) % MAX_FUNC_NO] == false) {
			pos = (func_no + i) % MAX_FUNC_NO;
			break;
		}
	}
	if (pos == -1) {
		printf("function registry is full.\n");
		ret = -4;
		goto rpc_register_fail;
	}

	func_name_array[pos] = fname;
	func_pointer_array[pos] = fpointer;
	func_valid_array[pos] = true;

	++func_no;

	lock_unlock(func_lock);

	printf("function [%s] registered, function no: %d.\n", fname, pos);

	return 0;

rpc_register_fail:
	lock_unlock(func_lock);
	return ret;
}

int rpc_unregister_function(const char *fname) {
	int ret = 0;

	if (fname == NULL || *fname == 0)
		return -1;

	lock_lock(func_lock);

	//check existence
	int pos = -1;
	for (int i = 0; i < MAX_FUNC_NO; ++i) {
		if (func_name_array[i].compare(fname) == 0) {
			pos = i;
			break;
		}
	}
	if (pos == -1) {
		printf("function [%s] does not exist.\n", fname);
		ret = -2;
		goto rpc_unregister_fail;
	}

	//unregister
	func_name_array[pos] = "";
	func_pointer_array[pos] = NULL;
	func_valid_array[pos] = false;

	printf("function [%s] unregistered, function no: %d.\n", fname, pos);

rpc_unregister_fail:
	lock_unlock(func_lock);
	return ret;
}

void rpc_destory() {
	if (thread_stop == true)
		return;
	thread_stop = true;

	//stop main loop
	if (server_sblock_event)
		event_signal(server_sblock_event);

	//waiting for main loop to stop
	WaitForSingleObject(thread_handle, INFINITE);
	CloseHandle(thread_handle);
	thread_handle = NULL;

	//cleanup
	lock_lock(func_lock);
	func_no = 0;
	for (int i = 0; i < MAX_FUNC_NO; ++i) {
		func_valid_array[i] = false;
		func_name_array[i] = "";
		func_pointer_array[i] = NULL;
	}
	lock_unlock(func_lock);
	lock_close(func_lock);

	lock_close(client_sblock_lock);
	client_sblock_lock = NULL;
	event_close(client_sblock_event);
	client_sblock_event = NULL;
	shared_block_close(client_sblock);

	lock_close(server_sblock_lock);
	server_sblock_lock = NULL;
	event_close(server_sblock_event);
	server_sblock_event = NULL;
	shared_block_close(server_sblock);

	server_id = -1;
}

////////////////////////////// rpc reaction //////////////////////////////
int rpc_request(short cid, short sid, const char *fname, const char *args, char *response, unsigned long response_len) {
	const char *ctx = NULL;

	int ret = 0;
	bool rpc_server_offline = false;

	struct shared_block server_sblock;
	server_sblock.name = NULL;
	server_sblock.size = 0;
	server_sblock.mapFile = NULL;
	server_sblock.view = NULL;

	void *server_lock = NULL;
	void *server_event = NULL;
	cJSON *request_json = NULL;

	char server_lock_name[SHARED_OBJ_SIZE];
	char server_event_name[SHARED_OBJ_SIZE];
	char server_sblock_name[SHARED_OBJ_SIZE];

	struct shared_block client_sblock;
	client_sblock.name = NULL;
	client_sblock.size = 0;
	client_sblock.mapFile = NULL;
	client_sblock.view = NULL;

	void *client_lock = NULL;
	void *client_event = NULL;
	cJSON *response_json = NULL;
	char *response_str = (char *)malloc(SBLOCK_SIZE * sizeof(char));

	char client_lock_name[SHARED_OBJ_SIZE];
	char client_event_name[SHARED_OBJ_SIZE];
	char client_sblock_name[SHARED_OBJ_SIZE];

	//check params
	if (fname == NULL || *fname == 0)
		return -1;
	if (args == NULL || *args == 0)
		args = "{}";
	if (response == NULL || response_len <= 0)
		return -2;

	//build name
	sprintf_s(server_lock_name, SRV_LOCK_OBJNAME, sid);
	sprintf_s(server_event_name, SRV_EVENT_OBJNAME, sid);
	sprintf_s(server_sblock_name, SRV_SBLOCK_OBJNAME, sid);

	sprintf_s(client_lock_name, CLI_LOCK_OBJNAME, cid);
	sprintf_s(client_event_name, CLI_EVENT_OBJNAME, cid);
	sprintf_s(client_sblock_name, CLI_SBLOCK_OBJNAME, cid);

	//build request
	request_json = cJSON_CreateObject();
	if (request_json == NULL) {	//failed to build request
		ret = -3;
		goto cleanup_request;
	}

	cJSON_AddNumberToObject(request_json, "cid", cid);
	cJSON_AddNumberToObject(request_json, "sid", sid);
	cJSON_AddStringToObject(request_json, "fn", fname);
	cJSON_AddStringToObject(request_json, "arg", args);
	cJSON_AddNumberToObject(request_json, "seq", fetch_new_sequence_no());
	if (ctx) cJSON_AddStringToObject(request_json, "ctx", ctx);

	char *request_str = cJSON_Print(request_json);
	//printf("send request: %s\n", request_str);

	if (SBLOCK_SIZE < strlen(request_str) + 1) {	//request too long
		ret = -4;
		goto cleanup_request;
	}

	/////////////////////////////////////////////////////////////////////////////////// send request within lock
	server_lock = lock_open(server_lock_name);
	if (server_lock == NULL) {
		rpc_server_offline = true;
		ret = -5;
		goto cleanup_request;
	}

	lock_lock(server_lock);
	if (shared_block_open(&server_sblock, server_sblock_name, SBLOCK_SIZE) != 0) {
		rpc_server_offline = true;
		ret = -5;
		goto cleanup_request;
	}
	memcpy_s(server_sblock.view, SBLOCK_SIZE, request_str, strlen(request_str) + 1);
	lock_unlock(server_lock);
	///////////////////////////////////////////////////////////////////////////////////

	//notify server
	server_event = event_open(server_event_name);
	if (server_event == NULL) {
		rpc_server_offline = true;
		ret = -5;
		goto cleanup_request;
	}
	event_signal(server_event);

	//waiting for response
	client_event = event_open(client_event_name);
	if (client_event == NULL) {
		ret = -6;
		goto cleanup_request;
	}
	event_wait(client_event);

	/////////////////////////////////////////////////////////////////////////////////// recv response within lock
	client_lock = lock_open(client_lock_name);
	if (client_lock == NULL) {
		ret = -7;
		goto cleanup_request;
	}

	lock_lock(client_lock);
	if (shared_block_open(&client_sblock, client_sblock_name, SBLOCK_SIZE) != 0) {
		ret = -8;
		goto cleanup_request;
	}
	memcpy(response_str, client_sblock.view, SBLOCK_SIZE);
	lock_unlock(client_lock);
	///////////////////////////////////////////////////////////////////////////////////

	//parse response
	response_json = cJSON_Parse(response_str);
	short res_sid = cJSON_GetObjectItem(response_json, "sid")->valueint;
	short res_cid = cJSON_GetObjectItem(response_json, "cid")->valueint;
	int res_seq = cJSON_GetObjectItem(response_json, "seq")->valueint;
	char *res_res = cJSON_GetObjectItem(response_json, "res")->valuestring;
	char *res_ctx = cJSON_GetObjectItem(response_json, "ctx") ?
		cJSON_GetObjectItem(response_json, "ctx")->valuestring : 0;

	if (strlen(res_res) + 1 > response_len) {
		memcpy(response, res_res, response_len - 1);
		response[response_len - 1] = 0;
		ret = -9;
		goto cleanup_request;
	}
	memcpy(response, res_res, strlen(res_res));
	response[strlen(res_res)] = 0;

	//printf("response: res_sid:%d ==> res_cid:%d, res_seq=%d\n, res_res=%s\n, res_ctx=%s\n",
	//	res_sid, res_cid, res_seq, res_res, res_ctx);

cleanup_request:
	cJSON_Delete(request_json);
	request_json = NULL;
	shared_block_close(server_sblock);
	event_close(server_event);
	server_event = NULL;
	lock_close(server_lock);
	server_lock = NULL;

	cJSON_Delete(response_json);
	response_json = NULL;
	shared_block_close(client_sblock);
	event_close(client_event);
	client_event = NULL;
	lock_close(client_lock);
	client_lock = NULL;

	if (response_str){
		free(response_str);
		response_str = NULL;
	}

	if (rpc_server_offline) {
		printf("rpc server stub %d offline.\n", sid);
	}

	return ret;
}

//rpc server stub --> rpc client stub
static int rpc_response(short sid, short cid, const char *response, const char *ctx, int seq) {
	int ret = 0;

	struct shared_block client_sblock;
	client_sblock.name = NULL;
	client_sblock.size = 0;
	client_sblock.mapFile = NULL;
	client_sblock.view = NULL;

	void *client_lock = NULL;
	void *client_event = NULL;
	cJSON *response_json = NULL;

	//build name
	char client_lock_name[SHARED_OBJ_SIZE];
	char client_event_name[SHARED_OBJ_SIZE];
	char client_sblock_name[SHARED_OBJ_SIZE];

	sprintf_s(client_lock_name, CLI_LOCK_OBJNAME, cid);
	sprintf_s(client_event_name, CLI_EVENT_OBJNAME, cid);
	sprintf_s(client_sblock_name, CLI_SBLOCK_OBJNAME, cid);

	//build response
	response_json = cJSON_CreateObject();
	if (response_json == NULL) { //failed to build response
		ret = -1;
		goto cleanup_response;
	}

	cJSON_AddNumberToObject(response_json, "sid", sid);
	cJSON_AddNumberToObject(response_json, "cid", cid);
	cJSON_AddStringToObject(response_json, "res", (response == NULL || *response == 0) ? "{}" : response);
	if (ctx) cJSON_AddStringToObject(response_json, "ctx", ctx);
	cJSON_AddNumberToObject(response_json, "seq", seq);

	char *response_str = cJSON_Print(response_json);
	printf("send response: %s\n", response_str);

	if (SBLOCK_SIZE < strlen(response_str) + 1) {	//response too long
		ret = -2;
		goto cleanup_response;
	}

	/////////////////////////////////////////////////////////////////////////////////// send response within lock
	client_lock = lock_open(client_lock_name);
	if (client_lock == NULL) {
		ret = -3;
		goto cleanup_response;
	}

	lock_lock(client_lock);
	shared_block_open(&client_sblock, client_sblock_name, SBLOCK_SIZE);
	memcpy_s(client_sblock.view, SBLOCK_SIZE, response_str, strlen(response_str) + 1);
	lock_unlock(client_lock);
	///////////////////////////////////////////////////////////////////////////////////

	client_event = event_open(client_event_name);
	if (client_event == NULL) {
		ret = -4;
		goto cleanup_response;
	}
	event_signal(client_event);
	
cleanup_response:
	cJSON_Delete(response_json);
	response_json = NULL;
	shared_block_close(client_sblock);
	event_close(client_event);
	client_event = NULL;
	lock_close(client_lock);
	client_lock = NULL;

	return ret;
}

static void *rpc_request_lock = NULL;
int rpc_request2(short cid, short sid, const char *fname, const char *args, char *response, unsigned long response_len) {
	int ret;

	rpc_request_lock = lock_create(REQ_LOCK_OBJNAME); //create at the first time, otherwise just open
	if (rpc_request_lock == NULL) {
		printf("create rpc_request_lock failed.\n");
		ret = -20;
		goto rpc_request2_exit;
	}

	lock_lock(rpc_request_lock);
	ret = rpc_request(cid, sid, fname, args, response, response_len);
	lock_unlock(rpc_request_lock);

rpc_request2_exit:
	return ret;
}