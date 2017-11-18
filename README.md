# WsServer - WebSocket message server

## Features
* Multi-threaded (boost thread pool)
* Undelivered messages queue (with TTL in future)
* Multiple recipients in one message
* Transparent admin user (use sender=0)
* ws/wss protocols (text, binary)
* Support fragmented frame buffer
* Benchmark included
* Custom payload via json
* User-independent
* Multi-connection support (multiple connections for single user, example: connected with few devices)
* REST Api server
	* list active users
	* sending message
	
### Todo features
* persistence for queued messages

 

## Requirements
* CMake 3.7+
* GCC/CLang/MSVC (not tested) with -stdlib=c++14
* Boost (system thread coroutine context, GCC < 4.9 - regex)
* Boost.Asio or standalone Asio
* OpenSSL
* ToolBox++ (https://github.com/edwardstock/toolboxpp)
* IniParser (included) (https://github.com/edwardstock/iniparser) 
 
 
## Build
TBD

## Configuring
TBD