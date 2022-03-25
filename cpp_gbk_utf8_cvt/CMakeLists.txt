cmake_minimum_required(VERSION 3.10)
project(UTF8_test)
if (DEFINED VCPKG_TARGET_TRIPLET) # we are using vcpkg
    message("VCPKG found!")
    find_package(iconv REQUIRED)
endif (DEFINED VCPKG_TARGET_TRIPLET)

add_executable(main main.cc)
if (DEFINED VCPKG_TARGET_TRIPLET)
    target_link_libraries(main PRIVATE Iconv::Charset Iconv::Iconv)
endif (DEFINED VCPKG_TARGET_TRIPLET)
target_compile_options(main PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
configure_file(test_gbk.txt test_gbk.txt COPYONLY)
configure_file(test_utf8.txt test_utf8.txt COPYONLY)