cmake_minimum_required(VERSION 3.16)
project(Nimonspoli VERSION 1.0 LANGUAGES CXX)

# =============================================================================
# STANDAR C++
# =============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# OUTPUT DIRECTORIES
# =============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# =============================================================================
# RAYLIB — DETEKSI PLATFORM DAN CARI LIBRARY
# =============================================================================
# Strategi:
#   1. Coba find_package standar (jika raylib sudah di-install via package manager)
#   2. Fallback ke path Homebrew di macOS
#   3. Fallback ke path manual via variabel RAYLIB_DIR

find_package(raylib 4.0 QUIET)

if (NOT raylib_FOUND)
    # Coba path Homebrew (macOS)
    if (APPLE)
        set(HOMEBREW_RAYLIB_DIR "/opt/homebrew/opt/raylib")
        if (EXISTS "${HOMEBREW_RAYLIB_DIR}")
            set(RAYLIB_INCLUDE_DIR "${HOMEBREW_RAYLIB_DIR}/include")
            set(RAYLIB_LIBRARY     "${HOMEBREW_RAYLIB_DIR}/lib/libraylib.a")
            message(STATUS "Raylib ditemukan via Homebrew: ${HOMEBREW_RAYLIB_DIR}")
        endif()
    endif()

    # Fallback: user bisa set -DRAYLIB_DIR=/path/to/raylib saat cmake
    if (DEFINED RAYLIB_DIR AND NOT DEFINED RAYLIB_LIBRARY)
        set(RAYLIB_INCLUDE_DIR "${RAYLIB_DIR}/include")
        set(RAYLIB_LIBRARY     "${RAYLIB_DIR}/lib/libraylib.a")
        message(STATUS "Raylib ditemukan via RAYLIB_DIR: ${RAYLIB_DIR}")
    endif()

    # Linux: coba path standar
    if (UNIX AND NOT APPLE AND NOT DEFINED RAYLIB_LIBRARY)
        find_path(RAYLIB_INCLUDE_DIR raylib.h
            PATHS /usr/local/include /usr/include
        )
        find_library(RAYLIB_LIBRARY raylib
            PATHS /usr/local/lib /usr/lib
        )
        if (RAYLIB_INCLUDE_DIR AND RAYLIB_LIBRARY)
            message(STATUS "Raylib ditemukan di sistem Linux: ${RAYLIB_LIBRARY}")
        endif()
    endif()
endif()

# =============================================================================
# KUMPULKAN SOURCE FILES
# =============================================================================

# Core sources (selain SaveLoadManager — sesuai makefile asli yang exclude ini)
file(GLOB_RECURSE CORE_SOURCES
    "src/core/*.cpp"
)
# Exclude SaveLoadManager sementara (belum selesai diimplementasi)
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*SaveLoadManager\\.cpp$")

# Model sources
file(GLOB_RECURSE MODEL_SOURCES
    "src/models/*.cpp"
)

# View sources
file(GLOB_RECURSE VIEW_SOURCES
    "src/views/*.cpp"
)

# Exclude test files dari build utama
list(FILTER CORE_SOURCES  EXCLUDE REGEX ".*test_.*\\.cpp$")
list(FILTER MODEL_SOURCES EXCLUDE REGEX ".*test_.*\\.cpp$")
list(FILTER VIEW_SOURCES  EXCLUDE REGEX ".*test_.*\\.cpp$")

# =============================================================================
# TARGET UTAMA: game
# =============================================================================
add_executable(game
    src/main.cpp
    ${CORE_SOURCES}
    ${MODEL_SOURCES}
    ${VIEW_SOURCES}
)

# Include directories
target_include_directories(game PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/models
    ${CMAKE_SOURCE_DIR}/include/views
)

# Warning flags
target_compile_options(game PRIVATE
    -Wall
    -Wextra
)

# =============================================================================
# LINK RAYLIB + PLATFORM-SPECIFIC FRAMEWORKS
# =============================================================================
if (raylib_FOUND)
    # find_package berhasil — gunakan target modern
    target_link_libraries(game PRIVATE raylib)
    target_include_directories(game PRIVATE ${RAYLIB_INCLUDE_DIRS})

elseif (DEFINED RAYLIB_LIBRARY AND EXISTS "${RAYLIB_LIBRARY}")
    # Fallback manual
    target_include_directories(game PRIVATE ${RAYLIB_INCLUDE_DIR})
    target_link_libraries(game PRIVATE ${RAYLIB_LIBRARY})

    # macOS butuh framework tambahan
    if (APPLE)
        target_link_libraries(game PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreAudio"
            "-framework CoreVideo"
        )
    endif()

    # Linux butuh beberapa lib tambahan
    if (UNIX AND NOT APPLE)
        target_link_libraries(game PRIVATE
            GL dl pthread m
        )
    endif()

else()
    message(WARNING
        "Raylib tidak ditemukan. Build akan gagal saat link.\n"
        "Solusi:\n"
        "  Linux  : sudo apt install libraylib-dev  (atau build dari source)\n"
        "  macOS  : brew install raylib\n"
        "  Manual : cmake -DRAYLIB_DIR=/path/to/raylib .."
    )
endif()

# =============================================================================
# TARGET TEST (opsional, tidak masuk build default)
# =============================================================================
option(BUILD_TESTS "Build test executables" OFF)

if (BUILD_TESTS)
    # Test GameWindow
    add_executable(test_gamewindow
        src/test_gamewindow.cpp
        ${CORE_SOURCES}
        ${MODEL_SOURCES}
        ${VIEW_SOURCES}
    )
    target_include_directories(test_gamewindow PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/include/models
        ${CMAKE_SOURCE_DIR}/include/views
    )
    if (raylib_FOUND)
        target_link_libraries(test_gamewindow PRIVATE raylib)
    elseif (DEFINED RAYLIB_LIBRARY AND EXISTS "${RAYLIB_LIBRARY}")
        target_include_directories(test_gamewindow PRIVATE ${RAYLIB_INCLUDE_DIR})
        target_link_libraries(test_gamewindow PRIVATE ${RAYLIB_LIBRARY})
        if (APPLE)
            target_link_libraries(test_gamewindow PRIVATE
                "-framework OpenGL" "-framework Cocoa"
                "-framework IOKit"  "-framework CoreAudio"
                "-framework CoreVideo"
            )
        elseif (UNIX)
            target_link_libraries(test_gamewindow PRIVATE GL dl pthread m)
        endif()
    endif()

    # Test SaveLoadManager
    add_executable(test_saveload
        src/test_saveloadmanager.cpp
        ${CORE_SOURCES}
        ${MODEL_SOURCES}
        ${VIEW_SOURCES}
    )
    target_include_directories(test_saveload PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/core
        ${CMAKE_SOURCE_DIR}/include/models
        ${CMAKE_SOURCE_DIR}/include/views
    )
    if (raylib_FOUND)
        target_link_libraries(test_saveload PRIVATE raylib)
    elseif (DEFINED RAYLIB_LIBRARY AND EXISTS "${RAYLIB_LIBRARY}")
        target_include_directories(test_saveload PRIVATE ${RAYLIB_INCLUDE_DIR})
        target_link_libraries(test_saveload PRIVATE ${RAYLIB_LIBRARY})
        if (APPLE)
            target_link_libraries(test_saveload PRIVATE
                "-framework OpenGL" "-framework Cocoa"
                "-framework IOKit"  "-framework CoreAudio"
                "-framework CoreVideo"
            )
        elseif (UNIX)
            target_link_libraries(test_saveload PRIVATE GL dl pthread m)
        endif()
    endif()
endif()

# =============================================================================
# INFORMASI BUILD
# =============================================================================
message(STATUS "")
message(STATUS "=== Nimonspoli Build Configuration ===")
message(STATUS "  Compiler    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Output dir  : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Build tests : ${BUILD_TESTS}")
message(STATUS "=======================================")
message(STATUS "")
