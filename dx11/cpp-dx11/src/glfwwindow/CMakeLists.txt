add_executable(glfw_test11 dx11.cc)
target_link_libraries(glfw_test11 glfw)

add_executable(glfw_test12 dx12.cc)
target_link_libraries(glfw_test12 glfw)
target_link_libraries(glfw_test12 d3d12)
#target_include_directories(glfw_test PRIVATE "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include")
#target_link_directories(glfw_test PRIVATE "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x64")
