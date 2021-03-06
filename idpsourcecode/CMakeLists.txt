#-------------------------------------------------------------------------------
#  Copyright 2010-2012 Katholieke Universiteit Leuven
# 
#  Use of this software is governed by the GNU LGPLv3.0 license
# 
#  Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
#  Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
#  K.U.Leuven, Departement Computerwetenschappen,
#  Celestijnenlaan 200A, B-3001 Leuven, Belgium
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.8)
project (idp)

### The version number
set (idp_VERSION_MAJOR 3)
set (idp_VERSION_MINOR 7)
set (idp_VERSION_PATCH 1)
set (idp_VERSION "${idp_VERSION_MAJOR}.${idp_VERSION_MINOR}.${idp_VERSION_PATCH}")

execute_process (COMMAND "git" "rev-parse" "HEAD"
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                RESULT_VARIABLE git_result
		ERROR_VARIABLE git_error
                OUTPUT_VARIABLE git_hash)
if (git_result OR git_error)
  set (git_hash "\"Source not version controlled.\"")
endif (git_result OR git_error)

string(STRIP ${git_hash} git_hash)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")
set(CMAKE_INSTALL_MESSAGE LAZY)

include(cmake/outofsourcecheck.cmake)
assureOutOfSourceBuilds()

option(BUILDTESTS  "Run the gtest tests" ON)
enable_testing()
option(BUILDDOCUMENTATION "Build the documentation" ON)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build.")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "DebugProfile" "Release" "StaticRelease" "StaticDebug" "Profile")

message(STATUS "Install path (CMAKE_INSTALL_PREFIX): ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build type (CMAKE_BUILD_TYPE): ${CMAKE_BUILD_TYPE}")

### Adding to cxx flags NOTE: do not use when escape characters matter
macro(addToCXX ARGUMENT)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARGUMENT}")
endmacro()

macro(alsoClean ARGUMENT)
	set(ADDITIONAL_CLEAN_FILES_TO_CLEAN "${ADDITIONAL_CLEAN_FILES_TO_CLEAN};${ARGUMENT}")
endmacro()

### Fix for newest ubuntu+gcc
IF("${CMAKE_SYSTEM}" MATCHES "Linux")
	addToCXX("-Wl,--no-as-needed")
endif()

### Rpath handling
include(cmake/rpathhandling.cmake)

### Target properties helper 
include(cmake/settargetproperties.cmake)

addToCXX("-static-libstdc++")

if(CMAKE_COMPILER_IS_MINGW)
	message(STATUS "Mingw compiler")
	addToCXX("-static-libgcc")
	addToCXX("-Wl,--stack,4194304")
endif()

### Gmp handling
OPTION(GMP "Enable gmp support" OFF)
if(GMP)
	addToCXX("-D GMP")
	find_package(GMP)
	if(GMP_FOUND)
		include_directories(${GMP_INCLUDE_DIR})
		set(LIBS ${LIBS} ${GMP_LIBRARIES})
		set(LIBS ${LIBS} ${GMPXX_LIBRARIES})
	else()
		message(FATAL_ERROR "GMP library not found")
	endif()
else()
	addToCXX("-D NO_GMP")
	message("WARNING: this IntegerType may not be suitable for some input file. Consider using GMP")
endif()

### 32 bits
option(32BITS "Compile for 32 bit architectures" OFF)
if(NOT 32BITS) # Cmake user didn't "force" 32bit compilation - do autodetection
	if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8) # Cmake built-in to detect whether you're on a 64b machine or not
		set(32BITS ON)
	endif()
endif()

if(32BITS)
	addToCXX("-m32")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")	  
endif()
message(STATUS "Compiling for 32 bit machine (32BITS): ${32BITS}")


### Build type
if(${CMAKE_BUILD_TYPE} MATCHES "^Profile$")
	addToCXX("-DNDEBUG -O3 -pg ")
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^DebugProfile$")
	addToCXX("-O0 -ggdb -DDEBUG -pg ")
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticRelease$")
	addToCXX("-DNDEBUG -O3 ")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG ")
	set(BUILD_SHARED_LIBS OFF)
	set(BUILD_IDP_STATIC ON)
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^Release$")
	addToCXX("-O3 -DNDEBUG ")
	set(BUILD_SHARED_LIBS ON)
	handleRPATHUnix()
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^StaticDebug$")
	addToCXX("-O0 -ggdb -DDEBUG -D_GLIBCXX_DEBUG ")
	set(BUILD_SHARED_LIBS OFF) # to allow stepping
	set(BUILD_IDP_STATIC ON)   # to allow stepping
endif()
if(${CMAKE_BUILD_TYPE} MATCHES "^Debug$")
	addToCXX("-O0 -ggdb -DDEBUG -D_GLIBCXX_DEBUG ")
	set(BUILD_SHARED_LIBS ON)
	handleRPATHUnix()
endif()

if(UNIX)
	addToCXX("-D UNIX")
endif()

option(GPERF  "Enable profiling support with google-perftools" OFF)
if(GPERF)
	addToCXX("-fno-omit-frame-pointer")
endif()

### Printing information
message(STATUS "IDP CXX flags: ")
message(STATUS "${CMAKE_CXX_FLAGS}")
message(STATUS "build idp shared ${BUILD_SHARED_LIBS}")

if(BUILD_IDP_STATIC)
	message(STATUS "build idp allstatic")
	SET(STATIC_LUA ON CACHE BOOL "Build lua static library") # only this works correctly
endif()
message(STATUS "Rpath for building is \"${CMAKE_BUILD_RPATH}\"")
message(STATUS "Rpath for installation is \"${CMAKE_INSTALL_RPATH}\"")


### Adding subprojects
include(ExternalProject)

#SPASS
option(SPASS  "Compile and install the SPASS theorem prover" ON)
if(SPASS)
	ExternalProject_Add(
		spass
		SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/SPASS-3.7
		CONFIGURE_COMMAND ${PROJECT_SOURCE_DIR}/lib/SPASS-3.7/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/spass
		BUILD_IN_SOURCE 0
	)
	install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/spass/bin/ DESTINATION bin FILES_MATCHING PATTERN "*" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
else()
	message(STATUS "SPASS will not be installed automatically. To be able to use it, install it manually and change the appropriate call in your config.idp file.")
endif()

#XSB
option(WITHXSB  "Compile with XSB support" ON)
if(WITHXSB AND (NOT 32BITS)) # Don't use XSB for 32Bits, this results in currently unfixable segfaults
	set(XSBUSED ON)
	message(STATUS "compile with XSB support: ON")
	addToCXX("-D WITHXSB")
	set(XSB_VERSION "3.7.0")
	set(XSB_INSTALL_URL "bin/xsb-${XSB_VERSION}/")
	addToCXX("-D XSB_INSTALL_URL=\\\\\"\"${XSB_INSTALL_URL}/\\\\\"\" ") # This has a bunch of layers of \ characters, because it gets evaluated a lot... don't ask. Version below also works (less unnesting)
#	 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D XSB_INSTALL_URL2=\\\"\"${XSB_INSTALL_URL}/\\\"\" ")
	set(XSB_COPIED_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/XSB")
	set(XSB_BUILD_DIR "${XSB_COPIED_SOURCE_DIR}/build")
	add_custom_target(copy_xsb_source_code)
	file(GLOB_RECURSE XSB_SOURCE_FILES ${PROJECT_SOURCE_DIR}/lib/XSB/*)
	add_custom_command(
		TARGET copy_xsb_source_code PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/lib/XSB  ${XSB_COPIED_SOURCE_DIR})
	alsoClean(${XSB_COPIED_SOURCE_DIR})

	set(XSB_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/xsbinstall")
	alsoClean(${XSB_INSTALL_DIR})

	execute_process(COMMAND ${PROJECT_SOURCE_DIR}/lib/XSB/build/config.guess 
					OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/XSB_CONFIG)
	file (STRINGS "${CMAKE_CURRENT_BINARY_DIR}/XSB_CONFIG" XSB_CONFIG_STRING)

	if (32BITS)   # Currently never used, since it's filtered out in the top-level if above
		set(XSB_CONFIG_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/configure --prefix=${XSB_INSTALL_DIR} --with-bits32)
		set(XSB_BUILD_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb -j 1 --config-tag=bits32)
		set(XSB_INSTALL_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb install -j 1 --config-tag=bits32)
		set(XSB_CONFIG_STRING ${XSB_CONFIG_STRING}-bits32)
	elseif((${CMAKE_BUILD_TYPE} MATCHES "^Debug$"))
		file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/XSB_CONFIG "${XSB_CONFIG_STRING}-dbg")
		file (STRINGS "${CMAKE_CURRENT_BINARY_DIR}/XSB_CONFIG" XSB_CONFIG_STRING)
		set(XSB_CONFIG_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/configure --prefix=${XSB_INSTALL_DIR} --enable-debug)
		set(XSB_BUILD_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb --config-tag=dbg -j 1)
		set(XSB_INSTALL_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb --config-tag=dbg install -j 1)
	else()
		set(XSB_CONFIG_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/configure --prefix=${XSB_INSTALL_DIR})
		set(XSB_BUILD_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb -j 1)
		set(XSB_INSTALL_COMMAND ${XSB_COPIED_SOURCE_DIR}/build/makexsb install -j 1)
	endif()

	include_directories (
		"${XSB_COPIED_SOURCE_DIR}/emu"
		"${XSB_COPIED_SOURCE_DIR}/config/${XSB_CONFIG_STRING}"
	)
	file(MAKE_DIRECTORY ${XSB_BUILD_DIR}/dummy) # Needed to stop complaining from adding XSB project - souce dir specified has to be non-empty
	ExternalProject_Add(
		xsb
		SOURCE_DIR ${XSB_COPIED_SOURCE_DIR}/build
		CONFIGURE_COMMAND ${XSB_CONFIG_COMMAND}
		BUILD_COMMAND ${XSB_BUILD_COMMAND}
		INSTALL_COMMAND ${XSB_INSTALL_COMMAND}
		BUILD_IN_SOURCE 1
	)
	add_dependencies(xsb copy_xsb_source_code)
else()
	set(XSBUSED OFF)
	message(STATUS "compile with XSB support: OFF")
	if(WITHXSB AND 32BITS)
		message(STATUS "  .. because currently 32 bit compilation is not supported with XSB")
	endif()
endif()

#REST
add_subdirectory(lib/minisatid)
add_subdirectory(lib/interactiveShell)
if(NOT TARGET gtest AND BUILDTESTS)
	add_subdirectory(lib/gtest-1.6.0)
endif() 
add_subdirectory(lib/gtestparser)
add_subdirectory(lib/lua)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_subdirectory(lib/multiprocessrun)
endif()
add_subdirectory(lib/tinythread/source)
add_subdirectory(src)
add_subdirectory(tests)
if(BUILDDOCUMENTATION)
	add_subdirectory(docs/official)
endif()

### Allow packing
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING.LESSER")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VERSION_MAJOR	${idp_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${idp_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${idp_VERSION_PATCH})
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_SOURCE_GENERATOR TGZ)
# The following components are regex's to match anywhere (unless anchored)
# in absolute path + filename to find files or directories to be excluded
# from source tarball.
# note: escape char is \\\\
set(CPACK_SOURCE_IGNORE_FILES
	"^${PROJECT_SOURCE_DIR}/\\\\..*"
	".*/.git/"
	"^${PROJECT_SOURCE_DIR}/builds/"
	"^${PROJECT_SOURCE_DIR}/build/"
	"^${PROJECT_SOURCE_DIR}/docs/visuals"
	"^${PROJECT_SOURCE_DIR}/docs/code"
	"^${PROJECT_SOURCE_DIR}/docs/technical"
)
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
set(CPACK_PACKAGE_NAME "idp3")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local/")
set(CPACK_PACKAGE_CONTACT "krr@cs.kuleuven.be")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The IDP3 Knowledge Base System")
execute_process (COMMAND "dpkg" "--print-architecture"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	RESULT_VARIABLE DPKG_ARCHITECTURE_RESULT
	ERROR_VARIABLE DPKG_ARCHITECTURE_ERROR
	OUTPUT_VARIABLE DPKG_ARCHITECTURE)
if (DPKG_ARCHITECTURE_RESULT OR DPKG_ARCHITECTURE_ERROR)
	set (DPKG_ARCHITECTURE "\"Unable to detect architecture through dpkg --print-architecture.\"")
	message("WARNING: Unable to detect architecture through dpkg --print-architecture. This prevents package building.")
endif (DPKG_ARCHITECTURE_RESULT OR DPKG_ARCHITECTURE_ERROR)
message(STATUS "source package name: ${CPACK_SOURCE_PACKAGE_FILE_NAME}")
message(STATUS "source package ignores: ${CPACK_SOURCE_IGNORE_FILES}")
include(CPack)

message(STATUS "make clean cleans the following additional files:")
message(STATUS "${ADDITIONAL_CLEAN_FILES_TO_CLEAN}")
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${ADDITIONAL_CLEAN_FILES_TO_CLEAN}")

#### Set Dependencies of Debian package
# ATTENTION: hard-coding dependencies. When building package on systems older than Ubuntu 16.04, replace these with the older versions still maintained by the older system version (e.g. 14.04).
# TODO: add GMP as dependency if GMP is used in build
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.23), libgcc1 (>= 1:6.0.1)") 
