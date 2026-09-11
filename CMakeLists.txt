# Portable build for the lab programs.
#
# The Visual Studio projects next to this file are still the primary way to work
# on the labs on Windows, but they hardcode an absolute path to a sibling SDL
# checkout, so nothing outside that one machine can build them. This file builds
# the same sources anywhere, which is what lets CI compile every lab on every
# push and run each one headless for a few frames.
cmake_minimum_required(VERSION 3.21)  # 3.21 for $<TARGET_RUNTIME_DLLS:...>

project(ComputerGraphicsLabs
  VERSION 1.0.0
  DESCRIPTION "Computer graphics labs built on SDL3"
  LANGUAGES C)

# Matches <LanguageStandard_C>stdc17</LanguageStandard_C> in the .vcxproj files.
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# On single-config generators an unset build type means no optimisation flags
# and no NDEBUG, which is rarely what anyone wants; pick a default.
get_property(labs_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT labs_multi_config AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# MSVC resolves the <math.h> functions straight out of its C runtime, but most
# Unix toolchains keep them in a separate libm that has to be linked explicitly
# -- AffineTransformation calls sinf/cosf and does not link anywhere without it.
include(CheckLibraryExists)
check_library_exists(m cosf "" LABS_HAVE_LIBM)
set(LABS_MATH_LIBRARY "$<$<BOOL:${LABS_HAVE_LIBM}>:m>")

option(LABS_BUILD_TESTS "Build and register the headless smoke tests" ON)
option(LABS_WERROR "Treat compiler warnings as errors" OFF)

set(LABS_SDL_TAG "release-3.4.16" CACHE STRING
  "SDL tag to fetch when no SDL3 can be found locally")
set(LABS_SDL_SOURCE_DIR "" CACHE PATH
  "Path to an SDL source tree to build alongside the labs (overrides fetching)")

# ---------------------------------------------------------------------------
# SDL3
# ---------------------------------------------------------------------------
# Three ways to get SDL, in order of preference:
#   1. an SDL3 installed on the machine (vcpkg, apt, brew, -DCMAKE_PREFIX_PATH)
#   2. an SDL source tree on disk -- either LABS_SDL_SOURCE_DIR, or the sibling
#      checkout the Visual Studio projects already expect (../SDL-release-3.4.16)
#   3. a pinned checkout fetched from GitHub -- what CI uses
#
# 3.2 is the floor: the labs are written against the main-callback API and
# SDL_SetRenderLogicalPresentation, neither of which settled before it.
find_package(SDL3 3.2 CONFIG QUIET)

if(SDL3_FOUND)
  message(STATUS "SDL3: using installed ${SDL3_VERSION}")
else()
  if(NOT LABS_SDL_SOURCE_DIR)
    set(labs_sibling_sdl "${CMAKE_CURRENT_SOURCE_DIR}/../SDL-${LABS_SDL_TAG}")
    if(EXISTS "${labs_sibling_sdl}/CMakeLists.txt")
      set(LABS_SDL_SOURCE_DIR "${labs_sibling_sdl}")
    endif()
  endif()

  # Build only what the labs link against. Without this SDL also builds its own
  # test library and test programs, which roughly doubles a cold CI build.
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "")
  set(SDL_TESTS        OFF CACHE BOOL "")
  set(SDL_EXAMPLES     OFF CACHE BOOL "")
  set(SDL_INSTALL      OFF CACHE BOOL "")
  set(SDL_SHARED       ON  CACHE BOOL "")
  set(SDL_STATIC       OFF CACHE BOOL "")

  if(LABS_SDL_SOURCE_DIR)
    message(STATUS "SDL3: building source tree at ${LABS_SDL_SOURCE_DIR}")
    add_subdirectory("${LABS_SDL_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/SDL" EXCLUDE_FROM_ALL)
  else()
    message(STATUS "SDL3: fetching ${LABS_SDL_TAG} from GitHub")
    include(FetchContent)
    FetchContent_Declare(SDL3
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG        "${LABS_SDL_TAG}"
      GIT_SHALLOW    TRUE
      GIT_PROGRESS   TRUE)
    FetchContent_MakeAvailable(SDL3)
  endif()
endif()

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
# SDL fixes the callback signatures, so most of them necessarily ignore their
# appstate parameter; that is not a defect worth warning about. MSVC spells the
# same warning C4100 and only raises it at /W4.
function(labs_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE
      /W4 /wd4100 "$<$<BOOL:${LABS_WERROR}>:/WX>")
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wno-unused-parameter "$<$<BOOL:${LABS_WERROR}>:-Werror>")
  endif()
endfunction()

# SDL is a shared library, so on Windows its DLL has to sit next to the .exe.
function(labs_copy_runtime_dlls target)
  if(WIN32)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:${target}>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:${target}>;$<TARGET_FILE_DIR:${target}>>"
      COMMAND_EXPAND_LISTS
      VERBATIM)
  endif()
endfunction()

# Builds one lab: the app itself, plus a smoke-test binary that links the same
# source against tests/smoke_main.c instead of the entry point SDL_main.h emits.
function(add_lab name)
  add_executable(${name} WIN32 "${name}/program.c")
  target_link_libraries(${name} PRIVATE SDL3::SDL3 ${LABS_MATH_LIBRARY})
  labs_set_warnings(${name})
  labs_copy_runtime_dlls(${name})

  if(NOT LABS_BUILD_TESTS)
    return()
  endif()

  add_executable(${name}_smoke "${name}/program.c" "tests/smoke_main.c")
  target_link_libraries(${name}_smoke PRIVATE SDL3::SDL3 ${LABS_MATH_LIBRARY})
  # Stops SDL_main.h from generating its platform entry point (WinMain and
  # friends) so that smoke_main.c can supply main() itself. The lab source is
  # otherwise compiled exactly as it is for the real app.
  target_compile_definitions(${name}_smoke PRIVATE SDL_MAIN_NOIMPL)
  labs_set_warnings(${name}_smoke)
  labs_copy_runtime_dlls(${name}_smoke)

  add_test(NAME ${name}_smoke COMMAND ${name}_smoke --frames 120)
  set_tests_properties(${name}_smoke PROPERTIES
    TIMEOUT 120
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy;SDL_RENDER_DRIVER=software")
endfunction()

# ---------------------------------------------------------------------------
# The labs
# ---------------------------------------------------------------------------
if(LABS_BUILD_TESTS)
  enable_testing()
endif()

# Every directory holding a program.c is a lab. Discovering them keeps a new
# lab from quietly sitting outside CI because someone forgot to list it here.
file(GLOB labs_sources CONFIGURE_DEPENDS
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*/program.c")

set(labs_found "")
foreach(lab_source IN LISTS labs_sources)
  get_filename_component(lab_name "${lab_source}" DIRECTORY)
  add_lab("${lab_name}")
  list(APPEND labs_found "${lab_name}")
endforeach()

if(NOT labs_found)
  message(FATAL_ERROR "No labs found: expected at least one <LabName>/program.c")
endif()

list(JOIN labs_found ", " labs_found_pretty)
message(STATUS "Labs: ${labs_found_pretty}")
