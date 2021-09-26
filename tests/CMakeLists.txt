project(test
        VERSION ${VERSION}
        DESCRIPTION "tests - test apps for hicc cxx11 library"
        LANGUAGES C CXX)

set(PROJECT_ARCHIVE_NAME ${PROJECT_NAME}s-${PROJECT_VERSION})

if (ENABLE_TESTS)
    # include(deps_inc_catch2)
    # include(deps_inc_fmt)
endif ()

find_package(Threads REQUIRED)

function(define_test_program name)
    #set(src_list )
    foreach (f ${ARGN})
        list(APPEND src_list ${f})
    endforeach ()

    add_executable(${PROJECT_NAME}-${name} ${src_list})
    #target_compile_features(${PROJECT_NAME}-${name} PRIVATE cxx_std_11)
    target_include_directories(${PROJECT_NAME}-${name} PRIVATE
            $<BUILD_INTERFACE:${CMAKE_GENERATED_DIR}>
            ${CMAKE_SOURCE_DIR}
            )
    target_link_libraries(${PROJECT_NAME}-${name}
            PRIVATE
            #Catch2::Catch2
            Threads::Threads
            fsm_cxx
            # cmdr11::cmdr11
            # fmt::fmt-header-only
            )
    #    set_target_properties(${PROJECT_NAME}-${name} PROPERTIES
    #            CXX_STANDARD 17
    #            CXX_STANDARD_REQUIRED ON
    #            CXX_EXTENSIONS OFF
    #            )
    if (MSVC)
        target_compile_options(${PROJECT_NAME}-${name} PRIVATE /W4 /WX /utf-8
                /DFSM_CXX_UNIT_TEST=1)
    else ()
        target_compile_options(${PROJECT_NAME}-${name} PRIVATE
                -pedantic -Wall -Wextra -Wshadow -Werror -pthread
                -DHICC_CXX_UNIT_TEST=1
                )
        if (DEBUG)
            target_compile_options(${PROJECT_NAME}-${name} PRIVATE
                    -fsanitize=address -fno-omit-frame-pointer
                    # address(AddressSanitizer), leak(LeakSanitizer), thread(ThreadSanitizer), undefined(UndefinedBehaviorSanitizer), memory(MemorySanitizer) 
                    # [additional_options]: -fno-omit-frame-pointer, fsanitize-recover/fno-sanitize-recover, -fsanitize-blacklist, etc.
                    # [-g] [-OX]
                    )
            target_link_options(${PROJECT_NAME}-${name} PRIVATE -fsanitize=address)
        endif ()
    endif ()

    if (${ENABLE_AUTOMATE_TESTS})
        get_property(tmp GLOBAL PROPERTY UNIT_TEST_TARGETS)
        set(tmp ${UNIT_TEST_TARGETS} ${PROJECT_NAME}-${name})
        set_property(GLOBAL PROPERTY UNIT_TEST_TARGETS "${tmp}")
        message(">> add_test(${PROJECT_NAME}-${name}) ...")
    endif ()

    if (ANDROID)
        add_test(NAME ${PROJECT_NAME}-${name}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMAND ${CMAKE_COMMAND} "-DANDROID_NDK=${ANDROID_NDK}"
                "-DTEST_RESOURCES_DIR=${CMAKE_SOURCE_DIR}"
                "-DTEST_RESOURCES=tests/data;tests/file_data.txt;Makefile"
                "-DUNITTEST=${PROJECT_NAME}-${name}"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/ExecuteOnAndroid.cmake)
    else ()
        add_test(NAME ${PROJECT_NAME}-${name}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMAND $<TARGET_FILE:${PROJECT_NAME}-${name}>)
    endif ()
endfunction()

define_test_program(basic basic.cc)


message(STATUS "END of tests")