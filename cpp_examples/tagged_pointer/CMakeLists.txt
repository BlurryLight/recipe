cmake_minimum_required(VERSION 3.14)
project(tagged_pointer_example LANGUAGES CXX)

add_executable(tagged_pointer_example main.cc)
target_compile_features(tagged_pointer_example PRIVATE cxx_std_17)
target_compile_options(tagged_pointer_example PRIVATE
    "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-pedantic>"
    "$<$<CXX_COMPILER_ID:MSVC>:/W3;/utf-8>"
)
