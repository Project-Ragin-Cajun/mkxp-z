# ==============================================================================
# mkxp-z CMake Build System
# ==============================================================================
# Replaces meson.build + src/meson.build + binding/meson.build
#
# Usage (after building deps with the platform Makefile):
#   source linux/vars.sh   (or windows/vars.sh)
#   cmake -B build -G Ninja
#   cmake --build build
# ==============================================================================

cmake_minimum_required(VERSION 4.00)

project(mkxp-z
        VERSION 2.4.2
        LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if not specified (matches Meson's default)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ==============================================================================
# Options: Mirrors meson_options.txt
# ==============================================================================

set(MRI_VERSION "3.1" CACHE STRING "Ruby MRI version to link against (e.g. 3.1, 2.7)")
option(MKXPZ_STATIC_EXECUTABLE "Link statically (Windows only)" ON)
option(MKXPZ_MINIFFI "Enable MiniFFI / Win32API compatibility" ON)
option(MKXPZ_SHARED_FLUID "Dynamically load FluidSynth at runtime" ON)
option(MKXPZ_HTTPS "Enable HTTPS support via OpenSSL" ON)
option(MKXPZ_WORKDIR_CURRENT "Use current directory instead of binary directory" OFF)
option(MKXPZ_FORCE32 "Force 32-bit build on 64-bit OS" OFF)

set(MKXPZ_GFX_BACKEND "gl" CACHE STRING "Graphics backend: gl or gles")
set_property(CACHE MKXPZ_GFX_BACKEND PROPERTY STRINGS "gl" "gles")

set(MKXPZ_STEAMWORKS_PATH "" CACHE PATH "Path to Steamworks SDK (leave empty to disable)")
set(MKXPZ_STEAM_APPID "" CACHE STRING "Steam App ID")
option(MKXPZ_STEAMSHIM_DEBUG "Enable Steamshim debug output" OFF)

set(MKXPZ_WINDOWS_RESOURCE_DIR "" CACHE STRING
        "Relative path to custom Windows resource directory (icon, manifest, etc.)")

# AppImage (Linux only)
option(MKXPZ_APPIMAGE "Build for AppImage packaging" OFF)
set(MKXPZ_APPIMAGEKIT_PATH "" CACHE PATH "Path to AppImageTool")

# ==============================================================================
# Git hash
# ==============================================================================

find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE MKXPZ_GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
    )
endif()
if(NOT MKXPZ_GIT_HASH)
    set(MKXPZ_GIT_HASH "unknown")
endif()

message(STATUS "mkxp-z ${PROJECT_VERSION} (${MKXPZ_GIT_HASH})")

# ==============================================================================
# Platform detection
# ==============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(FATAL_ERROR
            "This CMake project does not support macOS. Use the Xcode project instead.")
endif()

math(EXPR MKXPZ_PTR_SIZE "${CMAKE_SIZEOF_VOID_P} * 8")
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MKXPZ_WIN64 TRUE)
else()
    set(MKXPZ_WIN64 FALSE)
endif()

# ==============================================================================
# Dependency prefix discovery
# ==============================================================================
# Three ways to tell CMake where deps live (checked in order):
#   1. CMAKE_PREFIX_PATH (set by CMakePresets.json or -D flag)
#   2. PKG_CONFIG_PATH environment variable (from sourcing vars.sh)
#   3. Auto-parse vars.sh as a last resort
# ==============================================================================

# If CMAKE_PREFIX_PATH was provided (e.g. by preset), use it directly
if(CMAKE_PREFIX_PATH)
    list(GET CMAKE_PREFIX_PATH 0 MKXPZ_PREFIX)
    # Also ensure PKG_CONFIG_PATH is set for pkg-config calls
    if(NOT DEFINED ENV{PKG_CONFIG_PATH} OR "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
        set(ENV{PKG_CONFIG_PATH} "${MKXPZ_PREFIX}/lib/pkgconfig")
    endif()
elseif(DEFINED ENV{PKG_CONFIG_PATH})
    # Derive prefix from PKG_CONFIG_PATH (strip /lib/pkgconfig)
    get_filename_component(MKXPZ_PREFIX "$ENV{PKG_CONFIG_PATH}" DIRECTORY)  # -> .../lib
    get_filename_component(MKXPZ_PREFIX "${MKXPZ_PREFIX}" DIRECTORY)        # -> .../
    list(APPEND CMAKE_PREFIX_PATH "${MKXPZ_PREFIX}")
else()
    # Last resort: try to read vars.sh
    if(WIN32)
        set(_vars_sh "${CMAKE_SOURCE_DIR}/windows/vars.sh")
    else()
        set(_vars_sh "${CMAKE_SOURCE_DIR}/linux/vars.sh")
    endif()

    if(EXISTS "${_vars_sh}")
        file(STRINGS "${_vars_sh}" _vars_lines)
        foreach(_line ${_vars_lines})
            if(_line MATCHES "MKXPZ_PREFIX=[\"']?([^\"']+)[\"']?")
                set(_raw "${CMAKE_MATCH_1}")
                get_filename_component(_platform_dir "${_vars_sh}" DIRECTORY)
                string(REPLACE "\${PWD}" "${_platform_dir}" _raw "${_raw}")
                string(REPLACE "$PWD" "${_platform_dir}" _raw "${_raw}")
                get_filename_component(MKXPZ_PREFIX "${_raw}" ABSOLUTE
                        BASE_DIR "${_platform_dir}")
            endif()
        endforeach()
    endif()

    if(NOT MKXPZ_PREFIX)
        message(FATAL_ERROR
                "Cannot locate dependency prefix.\n"
                "Build deps first (cd windows && make), then either:\n"
                "  - Set CMAKE_PREFIX_PATH in your preset/command line\n"
                "  - Source vars.sh before running CMake")
    endif()

    set(ENV{PKG_CONFIG_PATH} "${MKXPZ_PREFIX}/lib/pkgconfig")
    list(APPEND CMAKE_PREFIX_PATH "${MKXPZ_PREFIX}")
endif()

message(STATUS "MKXPZ_PREFIX:    ${MKXPZ_PREFIX}")
message(STATUS "PKG_CONFIG_PATH: $ENV{PKG_CONFIG_PATH}")

find_package(PkgConfig REQUIRED)

# --- Dependencies with .pc files ---
pkg_check_modules(SDL2       REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_TTF   REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(PIXMAN     REQUIRED IMPORTED_TARGET pixman-1)
pkg_check_modules(PHYSFS     REQUIRED IMPORTED_TARGET physfs)
pkg_check_modules(VORBISFILE REQUIRED IMPORTED_TARGET vorbisfile)
pkg_check_modules(VORBIS     REQUIRED IMPORTED_TARGET vorbis)
pkg_check_modules(OGG        REQUIRED IMPORTED_TARGET ogg)
pkg_check_modules(OPENAL     REQUIRED IMPORTED_TARGET openal)
pkg_check_modules(FREETYPE   REQUIRED IMPORTED_TARGET freetype2)
pkg_check_modules(UCHARDET   REQUIRED IMPORTED_TARGET uchardet)
pkg_check_modules(FLUIDSYNTH REQUIRED IMPORTED_TARGET fluidsynth)
pkg_check_modules(LIBPNG     REQUIRED IMPORTED_TARGET libpng)
pkg_check_modules(THEORA     REQUIRED IMPORTED_TARGET theoradec)

# Ruby MRI: pkg-config name is ruby-X.Y
pkg_check_modules(MRI REQUIRED IMPORTED_TARGET ruby-${MRI_VERSION})

# --- Static libs without .pc files — find directly in the prefix ---
find_library(SDL2_IMAGE_LIB NAMES SDL2_image
        PATHS "${MKXPZ_PREFIX}/lib" NO_DEFAULT_PATH)
find_library(SDL2_SOUND_LIB NAMES SDL2_sound
        PATHS "${MKXPZ_PREFIX}/lib" NO_DEFAULT_PATH)

if(NOT SDL2_IMAGE_LIB)
    message(FATAL_ERROR "Could not find libSDL2_image.a in ${MKXPZ_PREFIX}/lib")
endif()
if(NOT SDL2_SOUND_LIB)
    message(FATAL_ERROR "Could not find libSDL2_sound.a in ${MKXPZ_PREFIX}/lib")
endif()

# SDL2_image was built with JPEG XL support — needs libjxl and highway
find_library(JXL_DEC_LIB NAMES jxl_dec-static jxl_dec jxl
        PATHS "${MKXPZ_PREFIX}/lib" NO_DEFAULT_PATH)
find_library(HWY_LIB NAMES hwy
        PATHS "${MKXPZ_PREFIX}/lib" NO_DEFAULT_PATH)

# Headers for the .pc-less libs live alongside everything else
set(MKXPZ_PREFIX_INCLUDE "${MKXPZ_PREFIX}/include")

# --- System / MSYS2 dependencies (not in the custom prefix) ---
if(MKXPZ_GFX_BACKEND STREQUAL "gl")
    find_package(OpenGL REQUIRED)
endif()
pkg_check_modules(ZLIB REQUIRED IMPORTED_TARGET zlib)

if(MKXPZ_HTTPS)
    pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)
endif()

if(WIN32)
    # MinGW ships iconv in its runtime; link it directly
    find_library(ICONV_LIB NAMES iconv)
endif()

# ==============================================================================
# xxd shader/asset embedding
# ==============================================================================
# Meson uses xxd to convert shaders and assets into C byte arrays.
# We replicate this with a custom command per input file.
# ==============================================================================

find_program(XXD_EXE xxd REQUIRED
        HINTS /usr/bin D:/Dev/msys64/usr/bin  # MSYS2 base layer (xxd ships with vim)
        DOC "xxd executable"
)

set(MKXPZ_XXD_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/xxd")
file(MAKE_DIRECTORY ${MKXPZ_XXD_OUTPUT_DIR})

# Collect shader files
file(GLOB SHADER_FILES
        "${CMAKE_SOURCE_DIR}/shader/*.frag"
        "${CMAKE_SOURCE_DIR}/shader/*.vert"
        "${CMAKE_SOURCE_DIR}/shader/*.h"
)

# Collect asset files
file(GLOB ASSET_FILES
        "${CMAKE_SOURCE_DIR}/assets/*.ttf"
        "${CMAKE_SOURCE_DIR}/assets/*.png"
        "${CMAKE_SOURCE_DIR}/assets/*.ogg"
        "${CMAKE_SOURCE_DIR}/assets/*.txt"
)

set(XXD_OUTPUTS "")
foreach(input_file ${SHADER_FILES} ${ASSET_FILES})
    get_filename_component(base_name ${input_file} NAME)
    set(output_file "${MKXPZ_XXD_OUTPUT_DIR}/${base_name}.xxd")

    # xxd generates C identifier names from the input path.
    # The Meson build runs xxd from the build directory, so the input path
    # is "../shader/common.h" which produces "___shader_common_h" (the ../
    # becomes ___ after xxd's non-alnum-to-underscore conversion).
    # We replicate this by computing the relative path from the binary dir.
    file(RELATIVE_PATH rel_path "${CMAKE_BINARY_DIR}" "${input_file}")

    add_custom_command(
            OUTPUT ${output_file}
            COMMAND ${XXD_EXE} -i ${rel_path} ${output_file}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${input_file}
            COMMENT "xxd: ${base_name}"
            VERBATIM
    )
    list(APPEND XXD_OUTPUTS ${output_file})
endforeach()

# Custom target so the xxd files are built before the main executable
add_custom_target(mkxpz_embedded DEPENDS ${XXD_OUTPUTS})

# ==============================================================================
# Source files
# ==============================================================================

file(GLOB_RECURSE MKXPZ_SRC_FILES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/src/*.h"
        "${CMAKE_SOURCE_DIR}/src/*.hpp"
)

file(GLOB MKXPZ_BINDING_FILES
        "${CMAKE_SOURCE_DIR}/binding/*.cpp"
        "${CMAKE_SOURCE_DIR}/binding/*.h"
)

set(MKXPZ_ALL_SOURCES ${MKXPZ_SRC_FILES} ${MKXPZ_BINDING_FILES})

# Steamshim child source (compiled into mkxp-z itself)
if(MKXPZ_STEAMWORKS_PATH AND EXISTS "${MKXPZ_STEAMWORKS_PATH}")
    list(APPEND MKXPZ_ALL_SOURCES "${CMAKE_SOURCE_DIR}/steamshim/steamshim_child.c")
endif()

# Windows resources
if(WIN32)
    if(MKXPZ_WINDOWS_RESOURCE_DIR)
        set(_res_dir "${CMAKE_SOURCE_DIR}/${MKXPZ_WINDOWS_RESOURCE_DIR}")
    else()
        set(_res_dir "${CMAKE_SOURCE_DIR}/windows")
    endif()
    file(GLOB MKXPZ_WIN_RESOURCES "${_res_dir}/*.rc")
    list(APPEND MKXPZ_ALL_SOURCES ${MKXPZ_WIN_RESOURCES})
endif()

# ==============================================================================
# Executable
# ==============================================================================

# Determine executable name (mirrors meson.build logic)
set(MKXPZ_EXE_NAME "mkxp-z")
if(UNIX AND NOT MKXPZ_APPIMAGE)
    string(APPEND MKXPZ_EXE_NAME ".${CMAKE_SYSTEM_PROCESSOR}")
endif()

# If Steamworks is enabled, the main exe gets a steam_ prefix and a separate
# steamshim parent executable is built
set(MKXPZ_STEAMWORKS OFF)
if(MKXPZ_STEAMWORKS_PATH AND EXISTS "${MKXPZ_STEAMWORKS_PATH}")
    set(MKXPZ_STEAMWORKS ON)
    set(MKXPZ_EXE_NAME "steam_${MKXPZ_EXE_NAME}")
endif()

add_executable(${MKXPZ_EXE_NAME} WIN32 ${MKXPZ_ALL_SOURCES})
add_dependencies(${MKXPZ_EXE_NAME} mkxpz_embedded)

# ==============================================================================
# Include directories
# ==============================================================================

target_include_directories(${MKXPZ_EXE_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/audio
        ${CMAKE_SOURCE_DIR}/src/crypto
        ${CMAKE_SOURCE_DIR}/src/display
        ${CMAKE_SOURCE_DIR}/src/display/gl
        ${CMAKE_SOURCE_DIR}/src/display/libnsgif
        ${CMAKE_SOURCE_DIR}/src/etc
        ${CMAKE_SOURCE_DIR}/src/filesystem
        ${CMAKE_SOURCE_DIR}/src/input
        ${CMAKE_SOURCE_DIR}/src/net
        ${CMAKE_SOURCE_DIR}/src/system
        ${CMAKE_SOURCE_DIR}/src/theoraplay
        ${CMAKE_SOURCE_DIR}/src/util
        ${CMAKE_SOURCE_DIR}/src/util/sigslot
        ${CMAKE_SOURCE_DIR}/binding
        ${MKXPZ_XXD_OUTPUT_DIR}
        ${MKXPZ_PREFIX_INCLUDE}
        ${MKXPZ_PREFIX_INCLUDE}/SDL2
)

if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/windows")
    target_include_directories(${MKXPZ_EXE_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/windows
    )
endif()

if(MKXPZ_STEAMWORKS)
    target_include_directories(${MKXPZ_EXE_NAME} PRIVATE
            ${CMAKE_SOURCE_DIR}/steamshim
            ${MKXPZ_STEAMWORKS_PATH}/public
    )
endif()

# ==============================================================================
# Compile definitions
# ==============================================================================

target_compile_definitions(${MKXPZ_EXE_NAME} PRIVATE
        # Use a CMake-specific marker so code can distinguish build systems
        MKXPZ_BUILD_CMAKE
        # Also define the Meson one for compatibility with existing #ifdefs
        MKXPZ_BUILD_MESON
        MKXPZ_VERSION="${PROJECT_VERSION}"
        MKXPZ_GIT_HASH="${MKXPZ_GIT_HASH}"
        HAVE_NANOSLEEP
        MKXPZ_INIT_GL_LATER
)

if(MKXPZ_GFX_BACKEND STREQUAL "gles")
    target_compile_definitions(${MKXPZ_EXE_NAME} PRIVATE GLES2_HEADER)
endif()

if(MKXPZ_MINIFFI)
    target_compile_definitions(${MKXPZ_EXE_NAME} PRIVATE MKXPZ_MINIFFI)
endif()

if(MKXPZ_WORKDIR_CURRENT)
    target_compile_definitions(${MKXPZ_EXE_NAME} PRIVATE WORKDIR_CURRENT)
endif()

if(MKXPZ_HTTPS)
    target_compile_definitions(${MKXPZ_EXE_NAME} PRIVATE MKXPZ_SSL)
endif()

if(MKXPZ_STEAMWORKS)
    target_compile_definitions(${MKXPZ_EXE_NAME} PRIVATE MKXPZ_STEAM)
endif()

# ==============================================================================
# Compile options (warning suppression matching Meson config)
# ==============================================================================

target_compile_options(${MKXPZ_EXE_NAME} PRIVATE
        -Wno-non-virtual-dtor
        -Wno-reorder
        -Wno-uninitialized
        -Wno-unknown-pragmas
        -Wno-stringop-truncation
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${MKXPZ_EXE_NAME} PRIVATE
            -Wno-undefined-var-template
            -Wno-delete-non-abstract-non-virtual-dtor
    )
endif()

if(WIN32 AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${MKXPZ_EXE_NAME} PRIVATE -masm=intel)
endif()

if(MKXPZ_FORCE32)
    target_compile_options(${MKXPZ_EXE_NAME} PRIVATE -m32)
    target_link_options(${MKXPZ_EXE_NAME} PRIVATE -m32)
endif()

# ==============================================================================
# Link dependencies
# ==============================================================================

target_link_libraries(${MKXPZ_EXE_NAME} PRIVATE
        # pkg-config deps
        PkgConfig::SDL2
        PkgConfig::SDL2_TTF
        PkgConfig::PIXMAN
        PkgConfig::PHYSFS
        PkgConfig::VORBISFILE
        PkgConfig::VORBIS
        PkgConfig::OGG
        PkgConfig::OPENAL
        PkgConfig::FREETYPE
        PkgConfig::UCHARDET
        PkgConfig::FLUIDSYNTH
        PkgConfig::LIBPNG
        PkgConfig::THEORA
        PkgConfig::MRI
        # Static libs without .pc
        ${SDL2_IMAGE_LIB}
        ${SDL2_SOUND_LIB}
        # System deps
        PkgConfig::ZLIB
)

# SDL2_image JPEG XL transitive deps
if(JXL_DEC_LIB)
    target_link_libraries(${MKXPZ_EXE_NAME} PRIVATE ${JXL_DEC_LIB})
endif()
if(HWY_LIB)
    target_link_libraries(${MKXPZ_EXE_NAME} PRIVATE ${HWY_LIB})
endif()

if(MKXPZ_GFX_BACKEND STREQUAL "gl")
    target_link_libraries(${MKXPZ_EXE_NAME} PRIVATE OpenGL::GL)
endif()

if(MKXPZ_HTTPS)
    target_link_libraries(${MKXPZ_EXE_NAME} PRIVATE PkgConfig::OPENSSL)
endif()


# Windows system libraries required by static deps.
# Attach them as transitive deps of the IMPORTED targets that reference them,
# so CMake's dependency-aware link ordering places them correctly.
if(WIN32)
    # OpenAL's WASAPI backend needs avrt; WinMM backend needs winmm
    set_property(TARGET PkgConfig::OPENAL APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES avrt winmm)
    # iconv is used by config, font, and binding-mri directly
    target_link_libraries(${MKXPZ_EXE_NAME} PRIVATE iconv)
    # crypt32 needed by HTTPS cert loading
    target_link_libraries(${MKXPZ_EXE_NAME} PRIVATE crypt32)
endif()

# RPATH (Linux)
if(UNIX)
    set(_rpath "$ORIGIN/lib")
    if(NOT MKXPZ_APPIMAGE)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT MKXPZ_FORCE32)
            string(APPEND _rpath "64")
        else()
            string(APPEND _rpath "32")
        endif()
    endif()
    set_target_properties(${MKXPZ_EXE_NAME} PROPERTIES
            INSTALL_RPATH "${_rpath}"
            BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

# Static linking (Windows)
if(WIN32 AND MKXPZ_STATIC_EXECUTABLE)
    target_link_options(${MKXPZ_EXE_NAME} PRIVATE -static)
endif()

# ==============================================================================
# Steamworks parent shim (separate executable)
# ==============================================================================

if(MKXPZ_STEAMWORKS)
    # Locate the Steam API library
    set(_steam_libname "steam_api")
    if(WIN32)
        if(MKXPZ_WIN64)
            set(_steam_bindir "win64")
            string(APPEND _steam_libname "64")
        else()
            set(_steam_bindir "")
        endif()
    elseif(UNIX)
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(_steam_bindir "linux32")
        else()
            set(_steam_bindir "linux64")
        endif()
    endif()

    set(_steam_libpath "${MKXPZ_STEAMWORKS_PATH}/redistributable_bin/${_steam_bindir}")

    find_library(STEAM_API_LIB
            NAMES ${_steam_libname}
            PATHS ${_steam_libpath}
            NO_DEFAULT_PATH
    )

    if(STEAM_API_LIB)
        set(_shim_name "${PROJECT_NAME}")
        set(_shim_args -DGAME_LAUNCH_NAME="${MKXPZ_EXE_NAME}")

        if(MKXPZ_STEAM_APPID)
            list(APPEND _shim_args -DSTEAM_APPID=${MKXPZ_STEAM_APPID})
        endif()
        if(MKXPZ_STEAMSHIM_DEBUG)
            list(APPEND _shim_args -DSTEAMSHIM_DEBUG)
        endif()

        add_executable(${_shim_name}
                ${CMAKE_SOURCE_DIR}/steamshim/steamshim_parent.cpp
        )
        target_include_directories(${_shim_name} PRIVATE
                ${MKXPZ_STEAMWORKS_PATH}/public
        )
        target_compile_definitions(${_shim_name} PRIVATE ${_shim_args})
        target_link_libraries(${_shim_name} PRIVATE ${STEAM_API_LIB})

        if(WIN32 AND MKXPZ_STATIC_EXECUTABLE)
            target_link_options(${_shim_name} PRIVATE -static)
        endif()
    else()
        message(WARNING "Steamworks path set but steam_api library not found at ${_steam_libpath}")
    endif()
endif()

# ==============================================================================
# Install
# ==============================================================================

if(NOT WIN32)
    install(TARGETS ${MKXPZ_EXE_NAME} RUNTIME DESTINATION bin)
    if(MKXPZ_STEAMWORKS AND TARGET ${PROJECT_NAME})
        install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    endif()
endif()

# ==============================================================================
# Summary
# ==============================================================================

message(STATUS "")
message(STATUS "=== mkxp-z build configuration ===")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Git hash:       ${MKXPZ_GIT_HASH}")
message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:       ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  GFX backend:    ${MKXPZ_GFX_BACKEND}")
message(STATUS "  Ruby MRI:       ${MRI_VERSION}")
message(STATUS "  MiniFFI:        ${MKXPZ_MINIFFI}")
message(STATUS "  HTTPS:          ${MKXPZ_HTTPS}")
message(STATUS "  Steamworks:     ${MKXPZ_STEAMWORKS}")
message(STATUS "  Static (Win):   ${MKXPZ_STATIC_EXECUTABLE}")
message(STATUS "  Exe name:       ${MKXPZ_EXE_NAME}")
message(STATUS "")