# easerpc
interprocess rpc stub (Windows)
===
* Windows supported
* built & run in VS2013
===
test:
* start up rpc stub 2
>callee.exe 2
rpc stub id: 2, 's'(start stub), 't'(stop stub) ...
s
function [add] registered, function no: 0.
function [dec] registered, function no: 1.
function [dec] unregistered, function no: 1.

* >start rpc stub 1; invoke "add(param1, param2)" at stub 2 which is started up above
caller.exe 1 2
tid=80476: ret=0, response: {
        "code": 0,
        "sum":  5
}
