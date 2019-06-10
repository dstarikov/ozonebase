# CMakeLists.txt for the ozone processors subfolder

set(OZ_PROCESSORS_SRC_FILES
    ozAVFilter.cpp ozDelayBuffer.cpp ozFaceDetector.cpp ozFilterSwapUV.cpp ozImageConvert.cpp ozImageScale.cpp
    ozImageTimestamper.cpp ozInputFallback.cpp ozMotionDetector.cpp ozMatrixVideo.cpp
    ozRateLimiter.cpp ozShapeDetector.cpp ozSignalChecker.cpp ozVideoFilter.cpp myYOLOv3.cpp
    myIdentityFilter.cpp
   )


set(OZ_PROCESSORS_HDR_FILES
    ozAVFilter.h ozDelayBuffer.h ozFaceDetector.h ozFilterSwapUV.h ozImageConvert.h ozImageScale.h
    ozImageTimestamper.h ozInputFallback.h ozMotionDetector.h ozMatrixVideo.h
    ozRateLimiter.h ozShapeDetector.h ozSignalChecker.h ozVideoFilter.h myYOLOv3.h myIdentityFilter.h
   )

if (HAVE_LIBCOPENALPR)
	list(APPEND OZ_PROCESSORS_SRC_FILES ozRecognizer.cpp)
	list(APPEND OZ_PROCESSORS_HDR_FILES ozRecognizer.h)
endif (HAVE_LIBCOPENALPR)

add_library(processors STATIC ${OZ_PROCESSORS_SRC_FILES} ${OZ_PROCESSORS_HDR_FILES})

find_library(CUDNN_LIBRARY cudnn HINTS /usr/lib/cuda/lib64/)
find_library(DARKNET_LIBRARY darknet HINTS /usr/local/lib)
target_link_libraries(processors ${OZ_EXTRA_LIBS} ${OZ_BIN_LIBS} "cuda" "cudart" "cublas" "curand" ${CUDNN_LIBRARY} ${DARKNET_LIBRARY})

install(TARGETS processors DESTINATION "${CMAKE_INSTALL_LIBDIR}/ozonebase/processors")
install(FILES ${OZ_PROCESSORS_HDR_FILES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/ozonebase/processors")

