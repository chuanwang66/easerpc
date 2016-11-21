import sys, time, os, struct

import ctypes
from ctypes import c_int, c_short, c_char, c_char_p, c_void_p, c_size_t, c_ulong, c_uint
from ctypes import byref, c_long

def _loadlib (fn):
    try: _dl = ctypes.cdll.LoadLibrary(fn)
    except: return None
    return _dl

#sys.path = sys.path + ['../Debug']
_dllpath = os.path.abspath(os.path.join('../Debug', 'easerpc.dll'))
_dll = _loadlib(_dllpath)
if not _dll:
    print 'cannot load library: %s'%(_dllpath)
else:
    print 'load library %s ok'%(_dllpath)

try:
    rpc_initialize = _dll.rpc_initialize
    rpc_initialize.argtypes = [c_short]
    rpc_initialize.restype = c_int
    print 'export rpc_initialize() ok'
except:
    print 'export rpc_initialize() failed'

'''
try:
    rpc_register_function = _dll.rpc_register_function
    rpc_functype = ctypes.CFUNCTYPE(c_char_p, c_char_p, c_uint)
    rpc_register_function.argtypes = [c_char_p, rpc_functype]
    rpc_register_function.restype = c_int
    print 'export rpc_register_function() ok'
except:
    print 'export rpc_register_function() failed'

try:
	rpc_unregister_function = _dll.rpc_unregister_function
	rpc_unregister_function.argtypes = [c_char_p]
	rpc_unregister_function.restype = c_int
	print 'export rpc_unregister_function() ok'
except:
	print 'export rpc_unregister_function() failed'
'''

try:
	rpc_destory = _dll.rpc_destory
	rpc_destory.argtypes = []
	rpc_destory.restype = None
	print 'export rpc_destory() ok'
except:
	print 'export rpc_destory() failed'

try:
	rpc_request = _dll.rpc_request
	rpc_request.argtypes = [c_short, c_short, c_char_p, c_char_p, c_char_p, c_ulong, c_long]
	rpc_request.restype = c_int
	print 'export rpc_request() ok'
except:
	print 'export rpc_request() failed'

if __name__ == '__main__':
    print rpc_initialize(2)

    raw_input()
