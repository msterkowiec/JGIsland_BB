#
# JGIsland_BB
#
# Copyright Marcin Sterkowiec, 2026. Use, modification and
# distribution is subject to license (see accompanying file license.txt)
#

cmake_minimum_required(VERSION 3.14)

# 1. Konfiguracja flag GTest dla kompilatora MSVC
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

# 2. Wskazanie lokalnego folderu z pobranym GTestem (wychodzimy z folderu tests do third_party)
add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/googletest-1.17.0" 
    "${CMAKE_CURRENT_BINARY_DIR}/googletest-build"
    EXCLUDE_FROM_ALL
)

# 3. Definicja Twojego pliku wykonywalnego z testami
add_executable(unit_tests test_main.cpp)

# 4. Linkowanie Twojej biblioteki oraz GTesta
target_link_libraries(unit_tests PRIVATE 
    JGIsland_BB 
    gtest_main
)

# 5. Integracja z CTest w MSVC
include(GoogleTest)
gtest_discover_tests(unit_tests)