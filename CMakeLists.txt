cmake_minimum_required(VERSION 3.22)
project(chukl_levi CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1) Host build: runs the pure-logic tests on any PC / CI runner. Verified to pass.
if(NOT ANDROID)
  enable_testing()
  add_executable(test_core tests/test_core.cpp)
  add_test(NAME core COMMAND test_core)
endif()

# 2) Android build: the real mod library.
# UNVERIFIED. Copy the Preloader SDK fetch/link block from Camera Overhaul's CMakeLists.txt
# (github.com/beyondbday69/camera-overhaul) into the marked spot below. Use the same SDK tag and NDK (r28c there).
if(ANDROID)
  # --- begin: paste Preloader SDK FetchContent + target name here ---
  # --- end ---
  add_library(chukl_levi SHARED src/main.cpp src/mod/ChuklLeviMod.cpp)
  target_include_directories(chukl_levi PRIVATE src)
  target_compile_definitions(chukl_levi PRIVATE CHUKL_WITH_SDK=1)
  # target_link_libraries(chukl_levi PRIVATE <preloader target name>)
endif()
