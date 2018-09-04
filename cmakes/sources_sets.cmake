### Sources
set(SERVER_SRC
    src/chat/ChatServer.h
    src/chat/ChatServer.cpp
    src/wsserver_core.h
    src/chat/Message.h
    src/chat/Message.cpp
    src/restapi/RestServer.cpp
    src/restapi/RestServer.h
    src/restapi/ChatRestServer.cpp
    src/restapi/ChatRestServer.h
    src/helpers/helpers.h
    src/helpers/helpers.cpp
    src/web/HttpClient.cpp
    src/web/HttpClient.h
    src/event/EventNotifier.h
    src/event/PostbackTarget.cpp
    src/event/PostbackTarget.h
    src/event/Target.hpp
    src/helpers/base64.cpp
    src/helpers/base64.h
    src/base/StandaloneService.h
    src/event/EventNotifier.cpp
    src/base/ServerStarter.cpp
    src/base/ServerStarter.h
    src/base/Settings.hpp
    src/base/auth/Auth.h
    src/base/auth/Auth.cpp
    src/base/auth/OneOfAuth.cpp
    src/base/auth/OneOfAuth.h
    src/base/auth/AllOfAuth.cpp
    src/base/auth/AllOfAuth.h
    src/base/auth/BasicAuth.cpp
    src/base/auth/BasicAuth.h
    src/base/auth/HeaderAuth.cpp
    src/base/auth/HeaderAuth.h
    src/base/auth/BearerAuth.cpp
    src/base/auth/BearerAuth.h
    src/base/auth/CookieAuth.cpp
    src/base/auth/CookieAuth.h
    src/base/auth/RemoteAuth.cpp
    src/base/auth/RemoteAuth.h
    src/chat/ConnectionStorage.cpp
    src/chat/ConnectionStorage.h
    src/chat/Statistics.cpp
    src/chat/Statistics.h
    src/base/unid.cpp
    src/base/unid.h
    )

if (ENABLE_REDIS_TARGET)
	set(SERVER_SRC
	    ${SERVER_SRC}
	    src/event/RedisTarget.cpp
	    src/event/RedisTarget.h)
endif ()


set(COMMON_LIBS_SRC
    #    ${PROJECT_LIBS_DIR}/json/src/json.hpp
    ${PROJECT_LIBS_DIR}/args/cmdline.hpp
    )
set(WS_COMMON_SRC
    ${PROJECT_LIBS_DIR}/ws/crypto.hpp
    ${PROJECT_LIBS_DIR}/ws/utility.hpp
    ${PROJECT_LIBS_DIR}/ws/status_code.hpp
    )
set(WS_SERVER_SRC
    ${PROJECT_LIBS_DIR}/ws/server_ws.hpp
    ${PROJECT_LIBS_DIR}/ws/server_wss.hpp
    )

set(WS_CLIENT_SRC
    ${PROJECT_LIBS_DIR}/ws/client_ws.hpp
    ${PROJECT_LIBS_DIR}/ws/client_wss.hpp
    )

set(HTTP_COMMON_SRC
#    ${PROJECT_LIBS_DIR}/http/crypto.hpp
#    ${PROJECT_LIBS_DIR}/http/utility.hpp
    ${PROJECT_LIBS_DIR}/http/status_code.hpp
    )

set(HTTP_SERVER_SRC
    ${PROJECT_LIBS_DIR}/http/server_http.hpp
    ${PROJECT_LIBS_DIR}/http/server_https.hpp
    )

set(HTTP_CLIENT_SRC
    ${PROJECT_LIBS_DIR}/http/client_http.hpp
    ${PROJECT_LIBS_DIR}/http/client_https.hpp
    )

set(LOCK_FREE_QUEUE_SRC
    ${PROJECT_LIBS_DIR}/concurrentqueue/concurrentqueue.h
    ${PROJECT_LIBS_DIR}/concurrentqueue/blockingconcurrentqueue.h
    )


include_directories(${PROJECT_LIBS_DIR}/ws)
include_directories(${PROJECT_LIBS_DIR}/json/src)
include_directories(${PROJECT_LIBS_DIR})
include_directories(${PROJECT_LIBS_DIR}/ini-parser/include)
include_directories(${PROJECT_LIBS_DIR}/args)
include_directories(${PROJECT_LIBS_DIR}/concurrentqueue)


set(SERVER_EXEC_SRCS
    ${SERVER_SRC}
    ${COMMON_LIBS_SRC}
    ${WS_SERVER_SRC} ${WS_COMMON_SRC}
    ${HTTP_SERVER_SRC} ${HTTP_CLIENT_SRC} ${HTTP_COMMON_SRC})