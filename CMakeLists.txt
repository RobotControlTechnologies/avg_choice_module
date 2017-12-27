cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
set(PROJECT avg_module)
project(${PROJECT} LANGUAGES C CXX)

if(NOT API_VERSION)
  message(SEND_ERROR "Please set API_VERSION")
endif()

if(NOT BUILD_NUMBER)
  message(SEND_ERROR "Please set BUILD_NUMBER")
endif()

get_filename_Component(SQLITE_DIR "../../../libraries/3rdparty/sqlite" ABSOLUTE)

#################################################################################
# Sources
#################################################################################
set(SOURCES
  src/avg_choice_module.cpp
  ${SQLITE_DIR}/sqlite3.c
)

set(PUBLIC_HEADERS
)

add_library(${PROJECT} SHARED ${SOURCES})

#################################################################################
# Properties
#################################################################################

set_source_files_properties(
  ${SQLITE_DIR}/sqlite3.c
  PROPERTIES
  COMPILE_FLAGS "-Wno-unused-but-set-variable -Wno-maybe-uninitialized"
)


target_compile_definitions(${PROJECT} PRIVATE
  API_VERSION=${API_VERSION}
  BUILD_NUMBER=${BUILD_NUMBER}
)

set_property(TARGET ${PROJECT} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT} PROPERTY C_STANDARD 11)

target_compile_options(${PROJECT} PRIVATE
  -Wall -Werror
  -pedantic
  -fno-strict-aliasing
  $<$<BOOL:API_VERSION LESS 101>:-Wno-write-strings>
)

#if(UNIX AND (X86_64 OR X86)) need detect X86
  target_compile_options(${PROJECT} PUBLIC -m32)
  set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS -m32)
#endif()

set_target_properties(${PROJECT} PROPERTIES
  PREFIX ""
  DEBUG_POSTFIX "_debug"
)

#################################################################################
# Libraries
#################################################################################

###############################
## Module_headers
###############################
if(NOT MODULE_HEADERS_PATH)
  set(MODULE_HEADERS_PATH "../../../libraries/public/module_headers")
endif()

if(NOT MODULE_HEADERS_PATH STREQUAL MODULE_HEADERS_PATH_INTERNAL)
  unset(MODULE_HEADERS_DIR)
endif()
set(MODULE_HEADERS_PATH_INTERNAL ${MODULE_HEADERS_PATH} CACHE INTERNAL "" FORCE)

find_path(MODULE_HEADERS_DIR NAMES "current/module.h" PATHS ${MODULE_HEADERS_PATH} NO_DEFAULT_PATH)
if(MODULE_HEADERS_DIR)
  target_include_directories(${PROJECT} PRIVATE "${MODULE_HEADERS_DIR}/${API_VERSION}")
else()
  message(SEND_ERROR "Please set MODULE_HEADERS_PATH, current value: ${MODULE_HEADERS_PATH}")
endif()

###############################
## Sqlite
###############################
target_include_directories(${PROJECT} PRIVATE ${SQLITE_DIR})

###############################
## SimpleIni
###############################
if(NOT SIMPLE_INI_PATH)
  set(SIMPLE_INI_PATH "../../../libraries/3rdparty/simpleini")
endif()

if(NOT SIMPLE_INI_PATH STREQUAL SIMPLE_INI_PATH_INTERNAL)
  unset(SIMPLE_INI_DIR)
endif()
set(SIMPLE_INI_PATH_INTERNAL ${SIMPLE_INI_PATH} CACHE INTERNAL "" FORCE)

find_path(SIMPLE_INI_DIR NAMES "SimpleIni.h" PATHS "${SIMPLE_INI_PATH}" NO_DEFAULT_PATH)
if(SIMPLE_INI_DIR)
  target_include_directories(${PROJECT} PRIVATE ${SIMPLE_INI_DIR})
else()
  message(SEND_ERROR "Please set SIMPLE_INI_PATH, current value: ${SIMPLE_INI_PATH}")
endif()

#################################################################################
# Install
#################################################################################

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
target_include_directories(${PROJECT} INTERFACE $<INSTALL_INTERFACE:include>)

install(TARGETS ${PROJECT} EXPORT ${PROJECT}
  RUNTIME DESTINATION lib_${API_VERSION}
  LIBRARY DESTINATION lib_${API_VERSION}
  COMPONENT runtime)