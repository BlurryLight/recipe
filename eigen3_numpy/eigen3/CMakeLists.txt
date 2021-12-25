cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME eigen3_example)
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Eigen3 CONFIG REQUIRED)
 find_package(fmt CONFIG REQUIRED)

add_executable(${PROJECT_NAME} main.cc)
target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen fmt::fmt)
