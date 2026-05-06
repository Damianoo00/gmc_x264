cmake_minimum_required(VERSION 3.28)

project(
    gmc_x264
    VERSION     0.1.0
    DESCRIPTION "GMC preprocessor — LD_PRELOAD wrapper dla libx264"
    LANGUAGES   CXX
)

# ── Standard C++ ────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ── Flagi kompilatora ────────────────────────────────────────
add_compile_options(
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow 
    -Werror
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2)
endif()

# ── Zależności ───────────────────────────────────────────────
find_package(OpenCV  REQUIRED COMPONENTS core imgproc video)
find_package(libx264 REQUIRED)

# ── Biblioteka główna ────────────────────────────────────────
add_library(gmc_x264 SHARED
    src/grass_detector.cpp
    src/gmc_estimator.cpp
    src/qp_map.cpp
    # src/wrapper.cpp
)

target_include_directories(gmc_x264
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gmc_x264
    PRIVATE opencv::opencv
    PRIVATE libx264::libx264
    PRIVATE dl
)

target_compile_definitions(gmc_x264
    PRIVATE GRASS_REFRESH_FRAMES=${GRASS_REFRESH_FRAMES}
    PRIVATE $<$<BOOL:${DEBUG_LOGGING}>:GMC_DEBUG_LOGGING>
)

set_target_properties(gmc_x264 PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    OUTPUT_NAME               "x264_gmc"
)

# ── Testy ────────────────────────────────────────────────────
if(WITH_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()

    add_executable(gmc_tests
        tests/test_grass_detector.cpp
        tests/test_gmc_estimator.cpp
        tests/test_qp_map.cpp
    )

    target_link_libraries(gmc_tests
        PRIVATE gmc_x264
        PRIVATE opencv::opencv
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(gmc_tests)
endif()

# ── Instalacja ───────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS gmc_x264
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)