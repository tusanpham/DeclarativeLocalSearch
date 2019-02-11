cmake_minimum_required(VERSION 2.8)
project (lua CXX)

SET(BUILD_SHARED_LIBS ON)

OPTION(SAFELUA "Disable the Lua libraries which enable interaction with the filesystem" OFF) 		
option(STATIC_LUA "build static library" OFF)

if(SAFELUA)
	addToCXX("-D SAFELUA")
endif()

set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build.")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "DebugProfile" "Release" "StaticRelease" "StaticDebug" "Profile")

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")	
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "DebugProfile$")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "Release")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticRelease$")
    set(STATIC_LUA ON)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticDebug$")
    set(STATIC_LUA ON)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

enable_testing()

message(STATUS "static lua: ${STATIC_LUA}")

option(LUA_USE_DLOPEN "Enable dlopen support.")
option(LUA_USE_ISATTY "Enable isatty support.")
option(LUA_USE_READLINE "Enable readline support.")
option(LUA_USE_POPEN "Enable lua_popen support.")
option(LUA_USE_ULONGJMP "Try using _longjmp/_setjmp (more efficient)")
option(LUA_USE_APICHECK "Enable API checks.")
option(LUA_USE_MKSTEMP "Use mkstemp.")
option(LUA_ANSI "Disable non-ansi features." OFF)

if(LUA_USE_DLOPEN)
	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		addToCXX("-DLUA_USE_DLOPEN")
		set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -ldl ")
	elseif(WIN32)
		MESSAGE(STATUS "LUA: Windows DL_DLL")
		addToCXX("-DLUA_DL_DLL")
	endif()
endif()

#
# Lua version
#
set(LUA_VERSION_MAJOR 5)
set(LUA_VERSION_MINOR 1)
set(LUA_VERSION_PATCH 4)
set(LUA_VERSION
				"${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}.${LUA_VERSION_PATCH}")
set(LUA_SOVERSION
				"${LUA_VERSION_MAJOR}")

set(COMMON_CFLAGS " -fPIC ")
set(COMMON_LDFLAGS " -lm ")
#
# Detect system type
#
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(LUA_USE_LINUX TRUE)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_LINUX")
	MESSAGE(STATUS "Lua in linux mode")
	set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -Wl,-E -ldl")
else(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(APPLE)
		set(LUA_USE_MACOSX TRUE)
		set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_LINUX")
#		set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -lreadline ")
		# use this on Mac OS X 10.3-
		#set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_MACOSX")
	elseif(WIN32)
		set(LUA_WIN TRUE)
		set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_BUILD_AS_DLL")
	elseif(UNIX)
		set(LUA_USE_POSIX TRUE)
		set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_POSIX")
	else(APPLE)
		set(LUA_ANSI TRUE)
		set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_ANSI")
	endif(APPLE)
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

add_subdirectory(src)
