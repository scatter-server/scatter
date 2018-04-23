### Sources
set(SERVER_SRC
    src/server/chat/ChatServer.h
    src/server/chat/ChatServer.cpp
    src/server/defs.h
    src/server/chat/Message.h
    src/server/chat/Message.cpp
    src/server/helpers/threadsafe.hpp
    src/server/restapi/RestServer.cpp
    src/server/restapi/RestServer.h
    src/server/restapi/ChatRestServer.cpp
    src/server/restapi/ChatRestServer.h
    src/server/helpers/helpers.h
    src/server/helpers/helpers.cpp
    src/server/web/HttpClient.cpp
    src/server/web/HttpClient.h
    src/server/event/EventNotifier.h
    src/server/event/PostbackTarget.cpp
    src/server/event/PostbackTarget.h
    src/server/event/Target.hpp
    src/server/helpers/base64.cpp
    src/server/helpers/base64.h
    src/server/base/StandaloneService.h
    src/server/event/EventNotifier.cpp
    src/server/base/ServerStarter.cpp
    src/server/base/ServerStarter.h
    src/server/base/Settings.hpp
    src/server/base/Auth.h
    src/server/base/Auth.cpp
    src/server/chat/ConnectionStorage.cpp
    src/server/chat/ConnectionStorage.h
    src/server/chat/Statistics.cpp
    src/server/chat/Statistics.h
    src/server/base/unid.cpp
    src/server/base/unid.h
    )

if (ENABLE_REDIS_TARGET)
	set(SERVER_SRC
	    ${SERVER_SRC}
	    src/server/event/RedisTarget.cpp
	    src/server/event/RedisTarget.h)
endif ()


set(COMMON_LIBS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/json/src/json.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/args/cmdline.hpp
    )
set(WS_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/crypto.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/utility.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/status_code.hpp
    )
set(WS_SERVER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/server_ws.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/server_wss.hpp
    )

set(WS_CLIENT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/client_ws.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/ws/client_wss.hpp
    )

set(HTTP_COMMON_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/crypto.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/utility.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/status_code.hpp
    )

set(HTTP_SERVER_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/server_http.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/server_https.hpp
    )

set(HTTP_CLIENT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/client_http.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/http/client_https.hpp
    )

set(LOCK_FREE_QUEUE_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/concurrentqueue/concurrentqueue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/concurrentqueue/blockingconcurrentqueue.h
    )