file(GLOB IMGUI_SRCS *.cpp)
add_library(imguilib STATIC ${IMGUI_SRCS})
target_link_libraries(imguilib PRIVATE glfw)
