//server side

#include <windows.h>

#include <stdio.h>
#include <math.h>

#include "cJSON.h"
#include "easerpc.h"

void add(const char *arg, char *res, unsigned int res_maxsize) {
	//1. parse
	cJSON *arg_json = cJSON_Parse(arg);
	int param1 = cJSON_GetObjectItem(arg_json, "param1")->valueint;
	int param2 = cJSON_GetObjectItem(arg_json, "param2")->valueint;

	//2. process
	int sum = param1 + param2;
	//Sleep(1000);

	//3. response
	cJSON *res_json = cJSON_CreateObject();
	cJSON_AddNumberToObject(res_json, "code", RPC_RESCODE_SUCCESS);
	cJSON_AddNumberToObject(res_json, "sum", sum);
	cJSON_AddNumberToObject(res_json, "tid", GetCurrentThreadId());
	char *res_str = cJSON_Print(res_json);

	int num = min(res_maxsize - 1, strlen(res_str));
	memcpy(res, res_str, num);
	res[num] = 0;
	
	cJSON_Delete(arg_json);
	cJSON_Delete(res_json);
}

void dec(const char *command, char *res, unsigned int res_maxsize) {
	char res_str[128];
	sprintf(res_str, "{code=%d}", RPC_RESCODE_SUCCESS);

	int num = min(res_maxsize - 1, strlen(res_str));
	memcpy(res, res_str, num);
	res[num] = 0;
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("usage: %s sid\n", argv[0]);
		exit(1);
	}

	short server_id = 0;
	if (sscanf(argv[1], "%d", &server_id) != 1) {
		printf("usage: %s sid [sid should be an integer!]\n", argv[0]);
		exit(2);
	}

	printf("rpc stub id: %d, 's'(start stub), 't'(stop stub) ...\n", server_id);

	int ret;
	while (true) {
		while (getchar() != 's');
		ret = rpc_initialize(server_id);
		if (ret == 0) {
			rpc_register_function("add", add);
			rpc_register_function("dec", dec);
			rpc_unregister_function("dec");

			while (getchar() != 't');

			rpc_destory();
			//rpc_destory();
		}
		else {
			printf("rpc_server_initialize() failed: %d\n", ret);
		}
	}

	return 0;
}

