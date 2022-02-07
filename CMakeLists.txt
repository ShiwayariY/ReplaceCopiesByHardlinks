cmake_minimum_required(VERSION 3.14)
project(ReplaceCopiesByHardlinks)

add_compile_options(-Wall -Wextra -pedantic)
set(CMAKE_CXX_STANDARD 20)

if(NOT DEFINED PLATFORM)
	set(PLATFORM "." CACHE STRING "current platform")
endif()
set(BUILD_SFX "${PLATFORM}/${CMAKE_BUILD_TYPE}")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "CMake install prefix" FORCE)
endif()

include_directories(
	include
#	"${Dependency_ROOT}/include"
)

#find_library(Dependency_LIB lib_name "${Dependency_ROOT}/lib/${BUILD_SFX}")

add_executable(ReplaceCopiesByHardlinks
	src/main.cc
)

#target_link_libraries(ReplaceCopiesByHardlinks
#	"${Dependency_LIB}"
#)

install(TARGETS
	ReplaceCopiesByHardlinks
DESTINATION "bin/${BUILD_SFX}")
