include(ExternalProject)

set(EIGEN_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/eigen-install")

ExternalProject_Add(
        eigen
        URL  https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/eigen-src"
        BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/eigen-build"
        INSTALL_DIR "${EIGEN_INSTALL_DIR}"
        CONFIGURE_HANDLED_BY_BUILD ON
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Release
        -DEIGEN_BUILD_DOC=OFF
        -DEIGEN_BUILD_DOC=OFF
        -DBUILD_TESTING=OFF
        -DEIGEN_BUILD_PKGCONFIG=OFF
)

file(MAKE_DIRECTORY ${EIGEN_INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${EIGEN_INSTALL_DIR}/include/eigen3)

set(TARGET_NAME Eigen3::Eigen)
add_library(${TARGET_NAME} INTERFACE IMPORTED GLOBAL)
add_dependencies(${TARGET_NAME} eigen)
target_compile_features(${TARGET_NAME} INTERFACE cxx_std_14)

set_target_properties(${TARGET_NAME} PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${EIGEN_INSTALL_DIR}/include/eigen3
)