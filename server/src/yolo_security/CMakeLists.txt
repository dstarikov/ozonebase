# CMakeLists.txt for cse576 final project application

add_executable(yolo_home_security yolo_home_security.cpp)
target_include_directories(yolo_home_security PRIVATE "/home/starikov/cse576/darknet/include")

set (SERVERAPP yolo_home_security)

target_link_libraries(yolo_home_security ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})

install(
	TARGETS ${SERVERAPP}
       RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
       )


