//client side
#include <Windows.h>
#include <stdio.h>
#include <math.h>

#include "easerpc.h"

#define REQUEST_THREAD_NUM 5

static short cid = -1;
static short sid = -1;

static DWORD CALLBACK request_proc(LPVOID param) {

	char response[128];
	int ret;
	int req_no = 0;

	while (req_no++ < MAXINT) {
		ret = rpc_request2(
			cid,			//stub id (me)
			sid,			//stub id (remote)
			"add",				//fname
			"{\"param1\":2, \"param2\":3}", //arguments
			response,
			sizeof(response), 5000);
		printf("no %d: tid=%d: ret=%d, response: %s\n", req_no, GetCurrentThreadId(), ret, ret == 0 ? response : "invalid");

		//Sleep(1);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		printf("usage: %s cid sid\n", argv[0]);
		exit(1);
	}

	if (sscanf(argv[1], "%d", &cid) != 1) {
		printf("usage: %s cid sid [cid should be an integer]\n", argv[0]);
		exit(2);
	}
	if (sscanf(argv[2], "%d", &sid) != 1) {
		printf("usage: %s cid sid [sid should be an integer]\n", argv[0]);
		exit(3);
	}

	if (rpc_initialize(cid) != 0) {
		exit(4);
	}
	Sleep(500);

	HANDLE request_thread[3];
	for (int i = 0; i < REQUEST_THREAD_NUM; ++i) {
		request_thread[i] = CreateThread(NULL, 0, request_proc, NULL, 0, NULL);
	}

	//getchar();
	WaitForMultipleObjects(REQUEST_THREAD_NUM, request_thread, TRUE, INFINITE);
	printf("finished\n");
	for (int i = 0; i < REQUEST_THREAD_NUM; ++i) {
		CloseHandle(request_thread[i]);
		request_thread[i] = NULL;
	}

	rpc_destory();
	
	return 0;
}

