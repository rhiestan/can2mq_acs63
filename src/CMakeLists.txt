CMAKE_MINIMUM_REQUIRED(VERSION 3.2 FATAL_ERROR)
project (can2mq)

# Switch off color output of Makefile
SET( CMAKE_COLOR_MAKEFILE OFF)
# Switch on verbose Makefile (i.e. printing out command lines)
#SET( CMAKE_VERBOSE_MAKEFILE ON )

IF(WIN32)
	SET(MOSQUITTO_INCLUDE_DIR "C:/Tools/mosquitto/devel")
	SET(MOSQUITTO_LIBRARY "C:/Tools/mosquitto/devel/mosquitto.lib")
ENDIF()

if (NOT MOSQUITTO_INCLUDE_DIR)
  find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
endif()

if (NOT MOSQUITTO_LIBRARY)
  find_library(
    MOSQUITTO_LIBRARY
    NAMES mosquitto)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  MOSQUITTO DEFAULT_MSG
  MOSQUITTO_LIBRARY MOSQUITTO_INCLUDE_DIR)

message(STATUS "libmosquitto include dir: ${MOSQUITTO_INCLUDE_DIR}")
message(STATUS "libmosquitto: ${MOSQUITTO_LIBRARY}")
set(MOSQUITTO_LIBRARIES ${MOSQUITTO_LIBRARY})

mark_as_advanced(MOSQUITTO_INCLUDE_DIR MOSQUITTO_LIBRARY)


SET(LIBCAN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../submodules/libcan/")
SET(JSON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../submodules/json/single_include")
ADD_LIBRARY(libcan STATIC
	${LIBCAN_DIR}/src/CANAdapter.cpp
	${LIBCAN_DIR}/src/CANFrameParser.cpp
	${LIBCAN_DIR}/src/SLCAN.cpp
	${LIBCAN_DIR}/src/SocketCAN.cpp
	${LIBCAN_DIR}/include/CANAdapter.h
	${LIBCAN_DIR}/include/CANFrame.h
	${LIBCAN_DIR}/include/CANFrameParser.h
	${LIBCAN_DIR}/include/SLCAN.h
	${LIBCAN_DIR}/include/SocketCAN.h
)
target_include_directories(libcan PUBLIC ${LIBCAN_DIR}/include)

add_executable(can2mq can2mq.cpp config.cpp config.h)
target_include_directories(can2mq PUBLIC ${MOSQUITTO_INCLUDE_DIR} ${JSON_DIR})
target_link_libraries(can2mq PUBLIC libcan ${MOSQUITTO_LIBRARIES})
target_compile_features(can2mq PUBLIC cxx_std_17)
