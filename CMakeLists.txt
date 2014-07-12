cmake_minimum_required(VERSION 2.8)

project(clickr)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_executable(clickr clickr.c)

find_package (CURL REQUIRED)
if (CURL_FOUND)
	include_directories(${CURL_INCLUDE_DIRS})
	target_link_libraries (clickr ${CURL_LIBRARIES})
endif (CURL_FOUND)

find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
        include_directories(${OPENSSL_INCLUDE_DIRS})
        target_link_libraries (clickr ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

find_package(LibConfig REQUIRED)
if (LIBCONFIG_FOUND)
	include_directories(${LIBCONFIG_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${LIBCONFIG_LIBRARIES})
	target_link_libraries (clickr ${LIBCONFIG_LIBRARIES})
endif (LIBCONFIG_FOUND)

install(TARGETS clickr DESTINATION bin)

