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
using BYTE = std::uint8_t;

// Using type Bitboard rather than uint64_t is recommended, since it ensures alignment:
#if defined(_MSC_VER)
	typedef __declspec(align(8)) uint64_t Bitboard;
#elif defined(__GNUC__) || defined(__clang__)
	typedef uint64_t Bitboard __attribute__((aligned(8)));
#else
	// Fallback for any other compiler:
	using Bitboard = uint64_t;
#endif

#if defined(__clang__) 
	#define CONST_RESTRICT const
#elif defined(__GNUC__)
	#define CONST_RESTRICT const __restrict__	
#elif defined(_MSC_VER)
	#define CONST_RESTRICT const __restrict
#else
	#define CONST_RESTRICT const
#endif
