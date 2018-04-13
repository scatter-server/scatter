# WsServer - WebSocket Open Message Server
[![Build Status](https://travis-ci.org/edwardstock/wsserver.svg?branch=master)](https://travis-ci.org/edwardstock/wsserver)

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
* User-independent (negative side - user id can be only unsigned long number, strings not supported now)
* Payload size limit
* Multiple connections per user (hello **Whatsapp** 👽)
* Watchdog. Check for alive connections, using PING-PONG.
* REST Api server
	* list active users with simple statistics
	* sending message
	* simple statistics for all or each user
	* checking user is online
* Event notifier. Server send message copy to your server. Supports couple auth methods: **basic**, **header-based**, **bearer**, **cookie**, et cetera (see [Configuring](#configuring) section)
    * url-based **postbacks** (or **webhook** as you like)
	
### Todo features
* Lock-free queues (now implemented only for events [thx to cameron314](https://github.com/cameron314/concurrentqueue))
* Horizontal scaling (custom cluster or using another solution)
* Persistence for queued messages
* Runtime switch using **ws**/**wss** protocol (now only at compile time: `-DUSE_SSL=ON`)
* Event notifier targets:
	* SQL (PostgreSQL, MySQL)
	* MongoDB
	* Redis pub/sub
	* Maybe: Cloud DBs (like Firebase RTD)
	* Maybe: some message queue like RabbitMQ
	* Maybe: unix socket, tcp or udp, or all of this

## In development
* Storing to redis queue (**lpop/rpush**) undelivered messages.


## Downloads
* [Debian Jessie DEB (amd64)](https://github.com/edwardstock/wsserver/releases/download/1.0.0/wsserver-1.0.0-linux-amd64.jessie.deb)
* [Debian Stretch DEB (amd64)](https://github.com/edwardstock/wsserver/releases/download/1.0.0/wsserver-1.0.0-linux-amd64.stretch.deb)
* [RHEL7 RPM (x86_64)](https://github.com/edwardstock/wsserver/releases/download/1.0.0/wsserver-1.0.0-linux-x86_64.el7.rpm)

# Install
* Debian
```bash
sudo dpkg -i wsserver.deb
sudo apt-get -f install
```

* RedHat
```bash
yum install wsserver.rpm
# or
rpm -i wsserver.rpm

```

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
* libcurl-ssl-dev
* ToolBox++ (https://github.com/edwardstock/toolboxpp) 
 
 
## Cloning (with submodules)
```bash
git clone --recursive https://github.com/edwardstock/wsserver.git
```
 
## Building

### Prepare Centos7
* GCC-7 (if not installed (required 4.9+, recommended 5+))
```bash
yum install centos-release-scl
yum install devtoolset-7-gcc*
scl enable devtoolset-7 bash
```

* Dependencies
```bash
yum install openssl openssl-devel curl libcurl-devel
```

* Boost
```bash
wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
tar -xzf boost_1_66_0.tar.gz && cd boost_1_66_0
./bootstrap.sh --prefix=/opt/boost1660
./b2 install --prefix=/opt/boost1660 --with=all
```


### Prepare Debian 9 (stretch)
```
apt-get install libboost1.62-all-dev libcurl4-openssl-dev libssl-dev
```

### Build
```bash
# preparation

# clone repo and cd /to/repo/path
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SSL=ON/OFF # enables SSL if set ON, default - OFF
# if boost installed in specific directory, just set it
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SSL=ON/OFF -DBOOST_ROOT_DIR=/opt/boost1660


# making

cmake --build .
sudo cmake --build . --target install
# or
sudo make && make install
```

## Run (systemd)
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

|                Field               | Value type | Default value        | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|:----------------------------------:|------------|----------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|          **server** object         |            |                      | **Common server configurations**                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|              endpoint              | string     | "/chat"              | Target websocket endpoint. Finally, address will loks like: ws://myserver/myendpoint                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
|               address              | string     | "*" (any)            | Server address. Leave asterisk (*) for apply any address, or set your server IP-address                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|                port                | uint16     | 8085                 | Server incoming port. By default, is 8085. Don't forget to add rule for your **iptables** of **firewalld** rule: *8085/tcp*                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|               workers              | uint32     | (system dependent)   | Number of threads for incoming connections. Recommended value - processor cores number. If wsserver can't determine number of cores, will set value to: 2                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|               tmpDir               | string     | "/tmp"               | Temporary dir. Reserved, not used now.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|                                    |            |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|               secure               | object     |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|           secure.crtPath           | string     | "../certs/debug.crt" | If server compiled with `-DUSE_SSL`, you must pass SSL cerificate file path.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
|           secure.keyPath           | string     | "../certs/debug.key" | If server compiled with `-DUSE_SSL`, you must pass SSL private key file path.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|                                    |            |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|              watchdog              | object     |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|          watchdog.enabled          | bool       | false                | Enables watchdog. Server will send every ~1 minute PING requests to clients, if they will not respond PONG or detected dangling connection, it will disconnected. Other case, if connection is unused `watchdog.connectionLifetimeSeconds` seconds, will disconnected too.                                                                                                                                                                                                                                                                                                                                             |
| watchdog.connectionLifetimeSeconds | long       | 600                  | Lifetime for inactive connection. Default: 10 minutes (600 seconds)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|                                    |            |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|                auth                | object     |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|              auth.type             | string     | "noauth"             | Authentication mode for websocket server. ype: noauth     *has no fields* **Be carefully! JS clients supports only basic and cookie auth. You can use oneOf auth type to combine different auth types for js and non-js clients**                                                                                                                                                                                                                                                                                                                                                                                      |
|           auth.type.basic          | object     | "basic"              | user: basic_username<br/> value: basic_password                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|          auth.type.bearer          | object     | "bearer"             | value: bearer_token                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
|          auth.type.header          | object     | "header"             | name: header_name<br/>value: header_value                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|          auth.type.cookie          | object     | "cookie"             | name: cookie_name<br/>value: cookie_value                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|           auth.type.oneOf          | object     | "oneOf"              | types: [...list of above auth objects...]                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|           auth.type.allOf          | object     | "allOf"              | types: [...list of above auth objects...]                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|                                    |            |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|         **restApi** object         |            |                      | **Rest API configuration.**                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|               enabled              | bool       | true                 | Enable rest api server                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|               address              | string     | "*"                  | Server address. Leave asterisk (*) for apply any address, or set your server IP-address                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
|                port                | uint16     | 8092                 | Server incoming port. By default, is 8092. Don't forget to add rule for your **iptables** of **firewalld** rule: *8092/tcp*                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|                auth                | object     |                      | Same configuration as server.auth (see above)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|                                    |            |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|           **chat** object          |            |                      | **Messaging configuration**                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|       enableUndeliveredQueue       | bool       | false                | Enable queue where server will store undelivered messages (by any reason)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
|               message              | object     |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|           message.maxSize          | string     | "10M"                | Maximum message size. <br/>If global payload size will be more than this value, server will disconnect client with error code 1009 (MESSAGE_TOO_BIG). <br/>Value suffix must be "M" - megabytes or "K" - kilobytes                                                                                                                                                                                                                                                                                                                                                                                                     |
|    message.enableDeliveryStatus    | bool       | false                | Enable sending delivery status message to sender. When message will delivered to recipient, sender will receive a system message with type **notification_received**, informs about successfully delivery.  <br/><br/>*Notice: this option probably will be removed in the future, because it doesn't relates to sent messages by no means.*                                                                                                                                                                                                                                                                           |
|                                    |            |                      |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
|          **event** object          |            |                      | **Event notifier. Another words, its a message re-sender to custom target**                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
|               enabled              | bool       | false                | Enable event notifier                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|             enableRetry            | bool       | true                 | Enable send retry when caused error (for example, postback-server responded non 200 http status)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|        retryIntervalSeconds        | uint32     | 10                   | Interval for retries (in seconds)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|             retryCount             | uint32     | 3                    | Maximum retries count                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|           sendBotMessages          | bool       | false                | With this option, event notifier can ignore messages, that has come from Rest API method /send-message.  What is a bot messages? Bot message is a message with sender = 0 (at least, for now)                                                                                                                                                                                                                                                                                                                                                                                                                          |
|         maxParallelWorkers         | uint16     | 16                   | Maximum event notifier workers that sends messages to targets. Recommended workers count: not less than server workers count. Better value: server workers * 2, cause http request is longer than just tcp packet via WS. <br/>Why http request? See below.                                                                                                                                                                                                                                                                                                                                                            |
|             ignoreTypes            | string[]   | []                   | Ignored message types, that must be excluded from event notifier queue                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
|               targets              | object[]   |                      | Event notifier targets configuration.For now, only available "postback" target. This target send to your server copy of message payload via http and json.  <br/>Available: <br/>**postback**: <br/>**url**: postback url, for example - http://mydomain/postback-url, <br/>**connectionTimeoutSeconds**: maximum connection timeout to server. Big value can impact to performance and may require more event notifier workers. 10 seconds is most optimal (revealed by benchmarking). If 10 seconds is not enough, look at your server performance.,         **auth**: Same configuration as server.auth (see above) |
|          targets[idx].type         | string     | "postback"           |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |