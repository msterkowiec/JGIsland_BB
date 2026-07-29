//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <stdint.h>
#include <cstdint>
#include <cassert>
#include <bit>

#ifdef _MSC_VER
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

typedef unsigned char FIGURE;
using Bitboard = uint64_t; // Bitboard used sometimes and not very consequently instead of uint64_t that is treated as a synonym for bitboard anyway...
using BYTE = std::uint8_t;


