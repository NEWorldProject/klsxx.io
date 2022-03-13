cmake_minimum_required(VERSION 3.11)

project(KLSXX)

kls_add_library_module(kls.io klsxx::io)
kls_public_source_directory(kls.io Common/Published)
kls_module_source_directory(kls.io Common/Module)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message("Configuring Linux 5 Specific Source. Note that kernel version < 5.6 will not work")
    kls_public_source_directory(kls.io Linux5/Published)
    kls_module_source_directory(kls.io Linux5/Module)
    include(FindPkgConfig)
    pkg_check_modules(liburing REQUIRED IMPORTED_TARGET GLOBAL liburing>=2.0)
    target_link_libraries(kls.io PRIVATE PkgConfig::liburing)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message("Configuring NTOS Specific Source")
    kls_public_source_directory(kls.io NTOS/Published)
    kls_module_source_directory(kls.io NTOS/Module)
    target_link_libraries(kls.io PRIVATE ws2_32 mswsock)
endif()

target_link_libraries(kls.io PUBLIC kls.essential kls.coroutine kls.thread)

kls_define_tests(tests.kls.io kls.io Tests)
