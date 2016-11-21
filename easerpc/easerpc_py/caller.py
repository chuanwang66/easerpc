#-*- coding: utf8 -*-
'''
	calling (remote) rpc function in .dll from .py
	NOTE:
		rpc function must be defined in c instead of python
		for python function is just a script
'''
import easerpc

import ctypes
import json
import sys, time

if __name__ == "__main__":
        if len(sys.argv) != 3:
                print 'usage: %s cid sid'%(sys.argv[0])
                exit(1)
	cid = int(sys.argv[1])
	sid = int(sys.argv[2])
	
	if easerpc.rpc_initialize(cid) != 0:
		print 'rpc_initialize() failed. ret=%d'%(ret)
		sys.exit(1)

	response = ctypes.create_string_buffer('\000'*0x10000)
	while 1:
		easerpc.rpc_request(cid, sid, 'add', '{"param1":2, "param2":3}', response, len(response), 5000)
		try:
                        response_json = json.loads(response.value)
                        print response_json
                except:
                        pass
		time.sleep(1)
	
	easerpc.rpc_destory()
