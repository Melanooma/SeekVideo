#Version-------------------------------------------------------------------------------------------------------------------#

#Sources-------------------------------------------------------------------------------------------------------------------#

set(SOURCES
	src/seekware-video.c
)

#Project-------------------------------------------------------------------------------------------------------------------#

project(seekware-video DESCRIPTION "Seekware video encoder")

#Targets-------------------------------------------------------------------------------------------------------------------#

add_executable(${PROJECT_NAME} ${SOURCES})
if(MSVC)
	set_property(GLOBAL PROPERTY VS_STARTUP_PROJECT seekware-video)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH ../../lib/${TOOLCHAIN_DIR})


#Extra Compile Options------------------------------------------------------------------------------------------------------#

#Include--------------------------------------------------------------------------------------------------------------------#

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if (SDL2_FOUND)
  include_directories(${SDL2_EXEC_PREFIX}/include)
endif (SDL2_FOUND)

#Link------------------------------------------------------------------------------------------------------------------------#

#SDL2
target_link_libraries(${PROJECT_NAME} SDL2)

if(SDL2_FOUND)
	target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
endif()

#seekware
target_link_libraries(${PROJECT_NAME} seekware)

#libav
target_include_directories(${PROJECT_NAME} INTERFACE
	${AVCODEC_INCLUDE_DIRS}
	${AVUTIL_INCLUDE_DIRS}
	${SWSCALE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
	${AVCODEC}
	${AVUTIL}
	${SWSCALE}
)

target_link_options(${PROJECT_NAME} INTERFACE
	${AVCODEC_LDFLAGS}
	${AVUTIL_LDFLAGS}
	${SWSCALE_LDFLAGS}
)

#Runtime Dependencies--------------------------------------------------------------------------------------------------------#
if(MSVC)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE_DIR:seekware>/seekware.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/seekware.dll
	)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE_DIR:SDL2>/SDL2.dll
		$<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2.dll
	)
endif()

#Install---------------------------------------------------------------------------------------------------------------------#

install(TARGETS ${PROJECT_NAME} DESTINATION bin)

file(GLOB SRC ${CMAKE_CURRENT_LIST_DIR}/src/*)
install(FILES ${SRC} DESTINATION src/${PROJECT_NAME}/src)

file(GLOB INC ${CMAKE_CURRENT_LIST_DIR}/include/*)
install(FILES ${INC} DESTINATION src/${PROJECT_NAME}/include)

install(FILES ${CMAKE_CURRENT_LIST_DIR}/Makefile DESTINATION src/${PROJECT_NAME})
