cmake_minimum_required(VERSION 3.22)
project(chukl_levi LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 1) Host build: pure-logic tests, runs on any PC / CI runner.
if(NOT ANDROID)
  enable_testing()
  add_executable(test_core tests/test_core.cpp)
  target_include_directories(test_core PRIVATE src)
  add_test(NAME core COMMAND test_core)
endif()

# 2) Android build: the real mod library.
# Modelled on LiteLDev/LeviLaunchroid examples/full-cpp-mod/CMakeLists.txt.
if(ANDROID)
  # Point at a preloader-android checkout, or leave empty to fetch the pinned commit.
  set(PRELOADER_ANDROID_ROOT "" CACHE PATH "Path to LiteLDev/preloader-android checkout")
  include(FetchContent)

  if(NOT PRELOADER_ANDROID_ROOT)
    FetchContent_Declare(preloader_android
      GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
      GIT_TAG 5d108da350ce3e7f13f51415f81e65344ecd5d14)
    FetchContent_GetProperties(preloader_android)
    if(NOT preloader_android_POPULATED)
      FetchContent_Populate(preloader_android)
    endif()
    set(PRELOADER_ANDROID_ROOT "${preloader_android_SOURCE_DIR}")
  endif()

  if(NOT EXISTS "${PRELOADER_ANDROID_ROOT}/include/pl/Config.hpp")
    message(FATAL_ERROR "Missing ${PRELOADER_ANDROID_ROOT}/include/pl/Config.hpp")
  endif()

  # pl/*.hpp pulls these in (same set as the official example).
  set(CHUKL_DEPS)
  if(NOT TARGET nlohmann_json::nlohmann_json)
    FetchContent_Declare(nlohmann_json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.11.3)
    list(APPEND CHUKL_DEPS nlohmann_json)
  endif()
  if(NOT TARGET Boost::pfr)
    FetchContent_Declare(boost_pfr GIT_REPOSITORY https://github.com/boostorg/pfr.git GIT_TAG boost-1.88.0)
    list(APPEND CHUKL_DEPS boost_pfr)
  endif()
  if(NOT TARGET magic_enum::magic_enum)
    FetchContent_Declare(magic_enum GIT_REPOSITORY https://github.com/Neargye/magic_enum.git GIT_TAG v0.9.7)
    list(APPEND CHUKL_DEPS magic_enum)
  endif()
  if(NOT TARGET fmt::fmt)
    FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 11.2.0)
    set(FMT_DOC OFF CACHE BOOL "" FORCE)
    set(FMT_TEST OFF CACHE BOOL "" FORCE)
    set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
    list(APPEND CHUKL_DEPS fmt)
  endif()
  if(CHUKL_DEPS)
    FetchContent_MakeAvailable(${CHUKL_DEPS})
  endif()

  option(CHUKL_LINK_PRELOADER "Build and link the preloader target (as the official example does)" ON)
  if(CHUKL_LINK_PRELOADER AND NOT TARGET preloader)
    add_subdirectory("${PRELOADER_ANDROID_ROOT}" "${CMAKE_BINARY_DIR}/_preloader_android")
  endif()

  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/${ANDROID_ABI}")
  find_library(CHUKL_LOG_LIB log)

  add_library(chukl_levi SHARED
    src/main.cpp
    src/mod/ChuklLeviMod.cpp
    src/mod/Hooks.cpp)
  set_target_properties(chukl_levi PROPERTIES OUTPUT_NAME "chukl_levi")
  target_include_directories(chukl_levi PRIVATE src "${PRELOADER_ANDROID_ROOT}/include")
  target_compile_definitions(chukl_levi PRIVATE CHUKL_WITH_SDK=1)
  target_compile_options(chukl_levi PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wall -Wextra -fvisibility=hidden -fvisibility-inlines-hidden>)
  target_link_libraries(chukl_levi PRIVATE
    nlohmann_json::nlohmann_json Boost::pfr magic_enum::magic_enum fmt::fmt)
  if(CHUKL_LOG_LIB)
    target_link_libraries(chukl_levi PRIVATE "${CHUKL_LOG_LIB}")
  endif()
  if(TARGET preloader)
    target_link_libraries(chukl_levi PRIVATE preloader)
  else()
    target_link_options(chukl_levi PRIVATE -Wl,--unresolved-symbols=ignore-all)
  endif()
endif()
