file(GLOB BASE_SRC "*.cpp" "*.cc" "*.hh" "*.hpp" )
add_library(base STATIC ${BASE_SRC})
target_include_directories(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})