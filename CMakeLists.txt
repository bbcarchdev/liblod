##############
# Declaration
##############
cmake_minimum_required(VERSION 3.3)
project(liblod)


##############
# Parameters
##############
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/. ${CMAKE_MODULE_PATH})
set(CMAKE_VERBOSE_MAKEFILE false)


##############
# Dependencies
##############
include(FindPkgConfig)

# Redland
pkg_check_modules(REDLAND REQUIRED redland)
message(STATUS ${REDLAND_INCLUDE_DIRS})
include_directories(${REDLAND_INCLUDE_DIRS})

# LibXML
include(FindLibXml2)
include_directories(${LIBXML2_INCLUDE_DIR})

# Curl
include(FindCURL)
include_directories(${LIBXML2_INCLUDE_DIR})

# LibURI
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../liburi)


##############
# Output
##############
# Declare the library liblod
set(LIBLOD_SOURCE_FILES p_liblod.h context.c instance.c resolve.c fetch.c sniff.c html.c response.c)
add_library(liblod SHARED ${LIBLOD_SOURCE_FILES})
target_link_libraries(liblod
        ${REDLAND_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        ${CURL_LIBRARIES}
        )

# Declare the executable lod-util
set(LODUTIL_SOURCE_FILES lod-util.c)
add_library(lod-util ${LODUTIL_SOURCE_FILES})
target_link_libraries(lod-util
        liblod
        ${REDLAND_LIBRARIES}
        )

# Generate the pkg-config file.
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)
