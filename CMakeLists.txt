cmake_minimum_required(VERSION 3.11)

project(KLSXX)

set(_KLS_THS_PUB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Common/Published)
set(_KLS_THS_MOD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Common/Module)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message("Configuring Linux 5 Specific Source. Note that kernel version < 5.6 will not work")
    set(_KLS_THS_SYS_PUB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Linux5/Published)
    set(_KLS_THS_SYS_MOD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Linux5/Module)
    include(FindPkgConfig)
    pkg_check_modules(liburing REQUIRED IMPORTED_TARGET GLOBAL liburing>=2.0)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message("Configuring NTOS Specific Source")
    set(_KLS_THS_SYS_PUB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/NTOS/Published)
    set(_KLS_THS_SYS_MOD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/NTOS/Module)
endif()

file(GLOB_RECURSE SRC_PUB ${_KLS_THS_PUB_PATH}/*.*)
file(GLOB_RECURSE SRC_MOD ${_KLS_THS_MOD_PATH}/*.*)
file(GLOB_RECURSE SRC_SYS_PUB ${_KLS_THS_SYS_PUB_PATH}/*.*)
file(GLOB_RECURSE SRC_SYS_MOD ${_KLS_THS_SYS_MOD_PATH}/*.*)

add_library(kls.io STATIC ${SRC_PUB} ${SRC_MOD} ${SRC_SYS_PUB} ${SRC_SYS_MOD})
target_include_directories(kls.io PUBLIC ${_KLS_THS_PUB_PATH} ${_KLS_THS_SYS_PUB_PATH})
target_include_directories(kls.io PRIVATE ${_KLS_THS_MOD_PATH} ${_KLS_THS_SYS_MOD_PATH})
target_include_directories(kls.io INTERFACE ${_KLS_THS_PUB_PATH} ${_KLS_THS_SYS_PUB_PATH})

target_link_libraries(kls.io PUBLIC kls.essential kls.coroutine kls.thread)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(kls.io PRIVATE ws2_32 mswsock)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(kls.io PRIVATE PkgConfig::liburing)
endif()

add_library(klsxx::io ALIAS kls.io)

if (KLS_BUILD_TESTS)
    file(GLOB_RECURSE SRC_TEST ${CMAKE_CURRENT_SOURCE_DIR}/Tests/*.*)
    add_executable(tests.kls.io ${SRC_TEST})
    target_link_libraries(tests.kls.io GTest::gtest_main kls.io)
    include(GoogleTest)
    gtest_discover_tests(tests.kls.io)
endif()
