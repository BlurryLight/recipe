add_library(assimp SHARED IMPORTED GLOBAL)

# 设置为动态链接库
set(assimp_ROOT_PATH "${CMAKE_SOURCE_DIR}/third_party/assimp")
message("assimp_ROOT_PATH: " ${assimp_ROOT_PATH})

# 引入lib文件
set_property(TARGET assimp PROPERTY IMPORTED_IMPLIB "${assimp_ROOT_PATH}/assimp-vc142-mt.lib")

# 引入dll文件
set_property(TARGET assimp PROPERTY IMPORTED_LOCATION "${assimp_ROOT_PATH}/assimp-vc142-mt.dll")

# 给target加入include
# https://cmake.org/cmake/help/latest/prop_tgt/INTERFACE_SYSTEM_INCLUDE_DIRECTORIES.html
# https://cmake.org/cmake/help/latest/prop_tgt/INTERFACE_INCLUDE_DIRECTORIES.html
# method 1
# set_property(TARGET assimp PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${assimp_ROOT_PATH})
# method 2
# use modern target way
# we add SYSTEM keyword to let compilers ignore warnings from those headers
target_include_directories(assimp SYSTEM INTERFACE
    ${assimp_ROOT_PATH}
)

# 增加个别名
add_library(assimp::assimp ALIAS assimp)