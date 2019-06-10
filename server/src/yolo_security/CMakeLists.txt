# CMakeLists.txt for cse576 final project application


link_directories(/usr/local/cuda/lib64 /usr/lib/cuda/lib64)
find_library(DARKNET "darknet" HINTS /usr/local/lib)
find_library(CUDART "cudart" HINTS /usr/local/cuda/lib64)

add_executable(yolo_home_security yolo_home_security.cpp)

target_link_libraries(yolo_home_security ${CUDART})
target_link_libraries(yolo_home_security "cuda" "cublas" "curand" "cudnn" ${DARKNET} ${OZ_BIN_LIBS} ${OZ_LIBS})

set (SERVERAPP yolo_home_security)

install(
	TARGETS ${SERVERAPP}
       RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
       )

