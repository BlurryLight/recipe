CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
project(MSVC_TEST)

add_library(MyClass SHARED MyClass.cc)
target_compile_definitions(MyClass PRIVATE -DSIMPLE_DLL_EXPORT)
add_executable(main main.cc)
target_link_libraries(main PRIVATE MyClass)
