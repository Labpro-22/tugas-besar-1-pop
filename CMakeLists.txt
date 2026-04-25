cmake_minimum_required(VERSION 3.15)
project(Nimonspoli CXX)

# Pengaturan kompilator sesuai spesifikasi makefile
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Implementasi Recursive Source Finding
file(GLOB_RECURSE ALL_SRCS "src/*.cpp")

# Pengecualian berkas tertentu (Dynamic Object Mapping)
list(FILTER ALL_SRCS EXCLUDE REGEX "src/test_.*\\.cpp$")
list(FILTER ALL_SRCS EXCLUDE REGEX "src/core/SaveLoadManager\\.cpp$")

# Pembuatan Target executable
add_executable(Nimonspoli ${ALL_SRCS})

# Aktifkan warning hanya untuk target game (kode kita), BUKAN untuk library pihak ketiga
target_compile_options(game PRIVATE -Wall -Wextra)

# Pendefinisian direktori (Directories)
target_include_directories(Nimonspoli PRIVATE
    include
    include/core
    include/models
    include/views
)
# Tetapkan batas minimum kebijakan untuk kompatibilitas Raylib
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Manajemen Dependensi Raylib Lintas Platform
find_package(raylib QUIET)

# Manajemen Dependensi Raylib Lintas Platform
find_package(raylib QUIET)
if (NOT raylib_FOUND)
    message(STATUS "Raylib tidak ditemukan secara lokal. Memulai pengunduhan otomatis...")
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.0 
    )
    FetchContent_MakeAvailable(raylib)
endif()

# Proses Taut (Link) pustaka
target_link_libraries(Nimonspoli PRIVATE raylib)
