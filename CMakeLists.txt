#
# JGIsland_BB
#
# Copyright Marcin Sterkowiec, 2026. Use, modification and
# distribution is subject to license (see accompanying file license.txt)
#

cmake_minimum_required(VERSION 3.14)
project(JGIsland_BB VERSION 1.0.0 LANGUAGES CXX)

if(MSVC)
    # RelWithDebInfo is expected to generate exactly the same code as Release, so replace the default /Ob1 with /Ob2 to avoid a warning later about conflicting settings
    string(REPLACE "/Ob1" "/Ob2" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()

add_library (JGIsland_BB INTERFACE)


target_include_directories(JGIsland_BB INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(JGIsland_BB INTERFACE cxx_std_20)

# 1. GCC/Clang Release Options
target_compile_options(JGIsland_BB INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-O3 -march=native>
    >
)

# 2. MSVC Release Options
if(MSVC)
	target_compile_options(JGIsland_BB INTERFACE 
		$<$<CXX_COMPILER_ID:MSVC>:$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:/O2 /Oi /arch:AVX2>>
	)

    # RelWithDebInfo is expected to generate exactly the same code as Release so include all the optimizations as in Release
    # /Ob2 - inlining
    # /Oy  - omit frame pointer
    # /GS- - skip seccurity check
    target_compile_options(JGIsland_BB INTERFACE
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/Ob2;/Oy;/GS->
    )

endif()


if(CMAKE_PROJECT_NAME STREQUAL JGIsland_BB)
    include(CTest)
    add_subdirectory(tests)
endif()
