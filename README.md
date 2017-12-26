# WsServer - WebSocket message server (Development stage - unstable, but works)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/274ad89f657b4c0695568ec42f7f39bb)](https://www.codacy.com/app/edwardstock/wsserver?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=edwardstock/wsserver&amp;utm_campaign=Badge_Grade)

## Features
* Multi-threaded (boost thread pool)
* Undelivered messages queue (with TTL in future)
* Multiple recipients in one message
* Transparent admin user (use sender=0)
* ws/wss protocols (text, binary, continuation (fragmented)) 
* Support fragmented frame buffer
* Benchmark (almost stress-test) included
* Custom payload via json
* User-independent
* Payload size limit
* Multi-connection support (multiple connections for single user, example: connected with few devices)
* Watchdog. Check for alive connections, using PING-PONG.
* REST Api server
	* list active users with simple statistics
	* sending message
* Event notifier. Send message copy to your server. Supports couple auth methods: **basic**, **header-based**, **bearer**
	* Postbacks: set url on where notifier will send messages payload
	
### Todo features
* Lock-free queues (now implemented only for events [thx to cameron314](https://github.com/cameron314/concurrentqueue))
* Horizontal scaling (custom cluster or using another solution)
* Persistence for queued messages
* Runtime switch using **ws**/**wss** protocol (now only at compile time: `-DUSE_SSL=ON`)
* Event notifier targets:
	* Redis pub/sub
	* SQL (PostgreSQL, MySQL)
	* MongoDB
	* Maybe: Cloud DBs (like Firebase RTD)
	* Maybe: some message queue like RabbitMQ
	* Maybe: unix socket, tcp or udp, or all of this
* Maybe: Process forks instead of threads

 

## Requirements
* GCC/CLang: -stdlib=c++14
* CMake
* Boost 1.54.0+ (recommended 1.60+)
	* Boost (system thread coroutine context random, GCC < 4.9 - regex)
	* Boost 1.63 requires CMake 3.7 or newer
	* Boost 1.64 requires CMake 3.8 or newer
	* Boost 1.65 and 1.65.1 require CMake 3.9.3 or newer
	* Boost.Asio
* OpenSSL
* ToolBox++ (https://github.com/edwardstock/toolboxpp) 
 
 
## Cloning (with submodules)
```bash
git clone --recursive git@github.com/edwardstock/wsserver.git
```
 
## Build
```bash
# clone repo and cd /to/repo/path
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SSL=ON/OFF # enables SSL if set ON, default - OFF
# if boost installed in specific directory, just set it
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SSL=ON/OFF -DBOOST_ROOT_DIR=/opt/myboost/x.x.x

cmake --build .
sudo cmake --build . --target install
```

## Run
For Debian users:
```bash
/etc/init.d/wsserver start # start service
```

For RedHat users (tested on Centos7)
```bash
sudo systemctl start wsserver.service
# or oldschool style:
sudo service wsserver start
```

## Documentation (Doxygen)
```bash
chmod +x doc_gen.sh && ./doc_gen.sh
```

Then look for html doc inside **docs/** directory

## Configuring

Look at the config [config.json](https://github.com/edwardstock/wsserver/blob/master/bin/config.json)

|                Field               | Value type | Default value        | Description                                                                                                                                                                                                                                                                |   |   |
|:----------------------------------:|------------|----------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---|---|
| endpoint                           | string     | "/chat"              | Target websocket endpoint. Finally, address will loks like: ws://myserver/myendpoint                                                                                                                                                                                       |   |   |
| address                            | string     | "*" (any)            | Server address. Leave asterisk (*) for apply any address, or set your server IP-address                                                                                                                                                                                    |   |   |
| port                               | uint16     | 8085                 | Server incoming port. By default, is 8085. Don't forget to add rule for your **iptables** of **firewalld** rule: *8085/tcp*                                                                                                                                                |   |   |
| workers                            | uint32     | (system dependent)   | Number of threads for incoming connections. Recommended value - processor cores number. If wsserver can't determine number of cores, will set value to: 2                                                                                                                  |   |   |
| tmpDir                             | string     | "/tmp"               | Temporary dir. Reserved, not used now.                                                                                                                                                                                                                                     |   |   |
|                                    |            |                      |                                                                                                                                                                                                                                                                            |   |   |
| secure                             | object     |                      |                                                                                                                                                                                                                                                                            |   |   |
| secure.crtPath                     | string     | "../certs/debug.crt" | If server compiled with `-DUSE_SSL`, you must pass SSL cerificate file path.                                                                                                                                                                                               |   |   |
| secure.keyPath                     | string     | "../certs/debug.key" | If server compiled with `-DUSE_SSL`, you must pass SSL private key file path.                                                                                                                                                                                              |   |   |
|                                    |            |                      |                                                                                                                                                                                                                                                                            |   |   |
| watchdog                           | object     |                      |                                                                                                                                                                                                                                                                            |   |   |
| watchdog.enabled                   | bool       | false                | Enables watchdog. Server will send every ~1 minute PING requests to clients, if they will not respond PONG or detected dangling connection, it will disconnected. Other case, if connection is unused `watchdog.connectionLifetimeSeconds` seconds, will disconnected too. |   |   |
| watchdog.connectionLifetimeSeconds | ulong      | 600                  | Lifetime for inactive connection. Default: 10 minutes (600 seconds)                                                                                                                                                                                                        |   |   |
|                                    |            |                      |                                                                                                                                                                                                                                                                            |   |   |
| auth                               | object     |                      |                                                                                                                                                                                                                                                                            |   |   |
| auth.type                          | string     | "noauth"             |                                                                                                                                                                                                                                                                            |   |   |
|                                    |            |                      |                                                                                                                                                                                                                                                                            |   |   |