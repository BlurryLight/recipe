CMAKE_MINIMUM_REQUIRED(VERSION 3.15)
PROJECT(AllocatorTest)
set(CMAKE_CXX_STANDARD 17)
add_executable(test FixedAllocator.cc)
# Enable Address Sanitizer for GCC and Clang
if((${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang") AND NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

# Enable Address Sanitizer for MSVC
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
endif()