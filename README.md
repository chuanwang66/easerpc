# easerpc
interprocess rpc stub (Windows)
====
* Windows supported
* built & run in VS2013

sequence of rpc events 
====
  -- From Wikipedia (Remote procedure call)

* The client calls the client stub. The call is a local procedure call, with parameters pushed on to the stack in the normal way.
* The client stub packs the parameters into a message and makes a system call to send the message. Packing the parameters is called marshalling.
* The client's local operating system sends the message from the client machine to the server machine.
* The local operating system on the server machine passes the incoming packets to the server stub.
* The server stub unpacks the parameters from the message. Unpacking the parameters is called unmarshalling.
* Finally, the server stub calls the server procedure. The reply traces the same steps in the reverse direction.

sample
====
* start up rpc stub 2
```bash
#callee.exe 2
```

* start rpc stub 1; then invoke "add(param1, param2)" at stub 2 from multiple threads at stub 1
```bash
#caller.exe 1 2
```
