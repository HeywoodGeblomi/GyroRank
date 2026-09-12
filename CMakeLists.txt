cmake_minimum_required(VERSION 3.14)
project(GyroRank VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(gyro_rank INTERFACE)
target_include_directories(gyro_rank INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

add_executable(gyro_rank_demo examples/demo.cpp)
target_link_libraries(gyro_rank_demo PRIVATE gyro_rank)

if(MSVC)
    target_compile_options(gyro_rank_demo PRIVATE /O2 /W4)
else()
    target_compile_options(gyro_rank_demo PRIVATE -O3 -Wall -Wextra -march=native)
endif()

add_executable(gyro_rank_identity tests/identity.cpp)
target_link_libraries(gyro_rank_identity PRIVATE gyro_rank)
if(MSVC)
    target_compile_options(gyro_rank_identity PRIVATE /O2 /W4)
else()
    target_compile_options(gyro_rank_identity PRIVATE -O2 -Wall -Wextra)
endif()

enable_testing()
add_test(NAME identity COMMAND gyro_rank_identity)

include(GNUInstallDirs)
install(TARGETS gyro_rank EXPORT GyroRankTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT GyroRankTargets
    FILE GyroRankTargets.cmake
    NAMESPACE GyroRank::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GyroRank
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/GyroRankConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
