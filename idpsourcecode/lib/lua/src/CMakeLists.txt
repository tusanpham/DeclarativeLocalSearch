cmake_minimum_required(VERSION 2.8)

macro(add_target_properties _target _name)
	set(_properties)
	foreach(_prop ${ARGN})
		set(_properties "${_properties} ${_prop}")
	endforeach(_prop)
	get_target_property(_old_properties ${_target} ${_name})
	if(NOT _old_properties)
		# in case it's NOTFOUND
		set(_old_properties)
	endif(NOT _old_properties)
	set_target_properties(${_target} PROPERTIES ${_name} "${_old_properties} ${_properties}")
endmacro(add_target_properties)

set(LUA_CORE_SRC
	lapi.cpp
	lauxlib.cpp
	lbaselib.cpp
	lcode.cpp
	ldblib.cpp
	ldebug.cpp
	ldo.cpp
	ldump.cpp
	lfunc.cpp
	lgc.cpp
	linit.cpp
	liolib.cpp
	llex.cpp
	lmathlib.cpp
	lmem.cpp
	loadlib.cpp
	lobject.cpp
	lopcodes.cpp
	loslib.cpp
	lparser.cpp
	lstate.cpp
	lstring.cpp
	lstrlib.cpp
	ltable.cpp
	ltablib.cpp
	ltm.cpp
	lundump.cpp
	lvm.cpp
	lzio.cpp
)
set(LUA_LIB_HEADERS
	lua.h
	lualib.h
	lauxlib.h
	luaconf.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
				${CMAKE_CURRENT_BINARY_DIR})

#
# Lua library.
#
if(STATIC_LUA)
	add_library(lua STATIC ${LUA_CORE_SRC})
	add_target_properties(lua COMPILE_FLAGS "${COMMON_CFLAGS}")
	add_target_properties(lua LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")
	set(LUA_STATIC_LIB lua)
	set(LUA_LIBS lua)
else(STATIC_LUA)
	add_library(lua_static STATIC ${LUA_CORE_SRC})
	add_target_properties(lua_static COMPILE_FLAGS "${COMMON_CFLAGS}")
	add_target_properties(lua_static LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")
	set(LUA_STATIC_LIB lua_static)

	add_library(lua SHARED ${LUA_CORE_SRC})
	add_target_properties(lua COMPILE_FLAGS "${COMMON_CFLAGS}")
	add_target_properties(lua LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")

	set(LUA_LIBS lua_static lua)
endif(STATIC_LUA)

set_target_properties(
	${LUA_LIBS} PROPERTIES
	VERSION ${LUA_VERSION}
	SOVERSION ${LUA_SOVERSION}
	CLEAN_DIRECT_OUTPUT 1
)
if(CMAKE_COMPILER_IS_MINGW)
	set_target_properties(lua PROPERTIES  
		OUTPUT_NAME lua5.1
		PREFIX "")
endif()
#
# install lua/luac & library.
#
install(TARGETS ${LUA_LIBS}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
)

install(FILES
	${LUA_LIB_HEADERS}
	DESTINATION include)
