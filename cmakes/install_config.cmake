if (EXISTS "/etc/wsserver/config.json")
	file(COPY "/etc/wsserver/config.json" DESTINATION "/etc/wsserver/config.json.bak")
endif ()

install(
	FILES ${CMAKE_CURRENT_SOURCE_DIR}/bin/config.json DESTINATION /etc/wsserver/
	PERMISSIONS OWNER_READ OWNER_WRITE #0644
	GROUP_READ
	WORLD_READ
)