add_executable(d3d12test WIN32 InitDirect3DApp.cpp)
target_link_libraries(d3d12test PRIVATE d3d12lib)