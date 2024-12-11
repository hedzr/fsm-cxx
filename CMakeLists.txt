cmake_minimum_required(VERSION 3.11)

# ### PREPARATIONS ##################################################
set(CMAKE_SCRIPTS "cmake")
set(CMAKE_MODULE_PATH
	${CMAKE_SOURCE_DIR}/${CMAKE_SCRIPTS}/modules
	${CMAKE_SOURCE_DIR}/${CMAKE_SCRIPTS}
	${CMAKE_SOURCE_DIR} # for .version.cmake & .options.cmake
	${CMAKE_MODULE_PATH})

# ### INCLUDES ##################################################
if(EXISTS ${CMAKE_SOURCE_DIR}/.version.cmake)
	message("   version decl file found: ${CMAKE_SOURCE_DIR}/.version.cmake")
	include(.version)
	message("   USING VERSION = ${VERSION}")
else()
	message("   version decl file ignored")
	set(VERSION 0.1.0.1)
endif()

# ### OPTIONS ##################################################

# cmake -S. -Bbuild -DFSM_CXX_STANDARD:STRING=20
set(FSM_CXX_STANDARD 17 CACHE STRING "Define The C++ Standard, default is 17")

option(FSM_CXX_BUILD_TEST_EXAMPLE "PL-OK" OFF)
option(FSM_CXX_BUILD_DOC "PL-OK" OFF)

option(FSM_CXX_ENABLE_ASSERTIONS "PL-OK" ON)
option(FSM_CXX_ENABLE_PRECONDITION_CHECKS "PL-OK" ON)

# ### PROJECTS ##################################################
project(fsm_cxx
	VERSION ${VERSION}
	DESCRIPTION "fsm-cxx is a finite state machine within c++17."
	LANGUAGES CXX)

set(detail_header_files
	${CMAKE_CURRENT_SOURCE_DIR}/include/fsm_cxx/detail/fsm-if.hh
)
set(header_files
	${CMAKE_CURRENT_SOURCE_DIR}/include/fsm_cxx/fsm-assert.hh
	${CMAKE_CURRENT_SOURCE_DIR}/include/fsm_cxx/fsm-common.hh
	${CMAKE_CURRENT_SOURCE_DIR}/include/fsm_cxx/fsm-config.hh
	${CMAKE_CURRENT_SOURCE_DIR}/include/fsm_cxx/fsm-debug.hh
	${CMAKE_CURRENT_SOURCE_DIR}/include/fsm_cxx/fsm-def.hh
	${CMAKE_CURRENT_SOURCE_DIR}/include/fsm_cxx/fsm-sm.hh
)

set(CMAKE_CXX_STANDARD ${FSM_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Just for QT app ...
# set(CMAKE_AUTOMOC ON)    # Qt moc, meta-object compiler
# set(CMAKE_AUTORCC ON)    # Qt rcc, resources compiler
# set(CMAKE_AUTOUIC ON)    # Qt uic, User-Interface compiler
add_library(fsm_cxx INTERFACE)
target_sources(fsm_cxx INTERFACE "$<BUILD_INTERFACE:${detail_header_files};${header_files}>")
target_include_directories(fsm_cxx INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>)
target_include_directories(fsm_cxx SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
target_compile_definitions(fsm_cxx INTERFACE
	FSM_CXX_ENABLE_ASSERTIONS=${_fsm_cxx_enable_assertions}
	FSM_CXX_ENABLE_PRECONDITION_CHECKS=${_fsm_cxx_enable_precondition_checks})

# target_link_libraries(fsm_cxx INTERFACE debug_assert)
if(MSVC)
	target_compile_options(fsm_cxx INTERFACE /wd4800) # truncation to bool warning
endif()

# Setup package config
include(CMakePackageConfigHelpers)
set(CONFIG_PACKAGE_INSTALL_DIR lib/cmake/fsm_cxx)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fsm_cxx-config.cmake "
include(\${CMAKE_CURRENT_LIST_DIR}/fsm_cxx-targets.cmake)
set(fsm_cxx_LIBRARY fsm_cxx)
set(fsm_cxx_LIBRARIES fsm_cxx)
")

write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/fsm_cxx-config-version.cmake
	VERSION ${VERSION}
	COMPATIBILITY SameMajorVersion
)

# Install target and header
install(DIRECTORY include/fsm_cxx DESTINATION include)

install(FILES include/fsm_cxx.hh DESTINATION include)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/fsm_cxx-config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/fsm_cxx-config-version.cmake
	DESTINATION
	${CONFIG_PACKAGE_INSTALL_DIR})

# Only export target when using imported targets
if(FSM_CXX_HAS_IMPORTED_TARGETS)
	install(TARGETS fsm_cxx
		EXPORT fsm_cxx-targets
		DESTINATION lib)

	install(EXPORT fsm_cxx-targets
		DESTINATION
		${CONFIG_PACKAGE_INSTALL_DIR}
	)
endif()

# other subdirectories
# only add if not inside add_subdirectory()
option(FSM_CXX_BUILD_TESTS_EXAMPLES "build test and example" OFF)

if(${FSM_CXX_BUILD_TESTS_EXAMPLES} OR(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
	enable_testing()
	add_subdirectory(examples/)
	add_subdirectory(tests/)
endif()

option(FSM_CXX_BUILD_DOCS "generate documentation" OFF)

if(FSM_CXX_BUILD_DOCS)
	add_subdirectory(docs/)
endif()

message(STATUS "tail of top level :  Using default C++ Standard ${FSM_CXX_STANDARD}")
