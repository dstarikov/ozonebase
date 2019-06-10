# CMakeLists.txt for cse576 final project application

add_executable(yolo_home_security yolo_home_security.cpp)

set (SERVERAPP yolo_home_security)

target_link_libraries(yolo_home_security ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} ${OZ_LIBS})

install(
	TARGETS ${SERVERAPP}
       RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
       )

