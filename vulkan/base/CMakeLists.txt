# https://github.com/zeux/volk
if (WIN32)
   set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
endif()

file(GLOB BASE_SRC "*.cpp" "*.cc" "*.hh" "*.hpp" )
add_library(base STATIC ${BASE_SRC})
target_include_directories(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_directories(base PUBLIC ${VK_SDK_LIB_DIRS})
target_link_libraries(base PUBLIC spdlog::spdlog glfw  volk)
target_compile_definitions(base PUBLIC -DVK_USE_PLATFORM_WIN32_KHR)