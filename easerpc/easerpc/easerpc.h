#pragma once

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////// rpc server error code //////////////////////////////
enum RPC_RESCODE {
	RPC_RESCODE_SUCCESS = 0,
	RPC_RESCODE_FAIL,
	RPC_RESCODE_NOT_FOUND,
};

//Rpc remote function prototype
typedef void(ftype)(const char *command, char *res, unsigned int res_maxsize);

//Initialize a rpc stub-pair (a rpc server stub & a rpc client stub), sharing the same node id (nid)
//	return: 0 on success
//  NOTE: call rpc_server_initialize() once per process
EXPORT int rpc_initialize(short nid);

//Register rpc remote function (acting as a rpc server stub)
//	return: 0 on success
EXPORT int rpc_register_function(const char *fname, ftype fpointer);

//Unregister rpc remote function (acting as a rpc server stub)
//	return: 0 on success
EXPORT int rpc_unregister_function(const char *fname);

//Destroy the rpc stub-pair
EXPORT void rpc_destory();

//rpc client stub --> rpc server stub --> rpc client stub (react via mapping file)
//  milliseconds: request timeout in millisecond, -1 for blocking request
//	return: 0 on rpc success, otherwise failure
//			(1) if 0 is returned, the rpc reaction itself succeeds, and the response returned must be a json like "{code=%d, ...}"
//				code == RPC_RESCODE_SUCCESS		==> rpc function works well
//				code == RPC_RESCODE_FAIL		==> rpc function goes wrong
//				code == RPC_RESCODE_NOT_FOUND	==> rpc function not found
//			(2) if a negative is returned, the rpc reaction itself goes wrong, in which case you should ignore the response
//
//NOTE: This method is THREAD-UNSAFE (for I'm not sure about the thread-safety of cJSON)
//TODO: Asynchronous callback is not supported yet.
EXPORT int rpc_request(short cid, short sid, const char *fname, const char *args, char *response, unsigned long response_len, long milliseconds);

//a THREAD-SAFE version of rpc_request(), which may cause more overhead
EXPORT int rpc_request2(short cid, short sid, const char *fname, const char *args, char *response, unsigned long response_len, long milliseconds);

#ifdef __cplusplus
}
#endif