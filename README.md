# easerpc
interprocess rpc stub (Windows)
===
* Windows supported
* built & run in VS2013
* start up rpc stub 2
>callee.exe 2

* start rpc stub 1; then invoke "add(param1, param2)" at stub 2 from multiple threads at stub 1
>caller.exe 1 2
