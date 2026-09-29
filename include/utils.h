//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "config.h"
#include "common.h"
#include "data.h"
#include "BetweenLookup.h"
#include "RayLookup.h"
#include "defs.h"

#include <stdint.h>
#include <cstdint>
#include <cassert>
#include <bit>

struct TMove
{
	BYTE nFrom;
	BYTE nTo;
	FIGURE fPromotion;
	BYTE unused;

	ALWAYS_INLINE void set(const BYTE from, const BYTE to)
	{
		assert(from < 64);
		assert(to < 64);

		nFrom = from;
		nTo = to;
		fPromotion = FGR_EMPTY;
	}
	ALWAYS_INLINE void set(const BYTE from, const BYTE to, const FIGURE f)
	{
		assert(from < 64);
		assert(to < 64);
		assert(f == FGR_BISHOP || f == FGR_KNIGHT || f == FGR_ROOK || f == FGR_QUEEN || f == FGR_EMPTY);

		nFrom = from;
		nTo = to;
		fPromotion = f;
	}
	ALWAYS_INLINE BYTE IsPromotion() const
	{
		BYTE res = fPromotion & 31;

		return res;
	}
	ALWAYS_INLINE bool operator == (const TMove& o) const
	{
		return nFrom == o.nFrom && nTo == o.nTo && fPromotion == o.fPromotion;
	}
	// operator < not needed for now...
};

inline constexpr bool IsValidPos(const unsigned char pos)
{
	return pos < 64;
}
inline constexpr bool IsValidPos(const char x, const char y)
{
	return (((BYTE)x) < 8) & (((BYTE)y) < 8);
}

template <typename T>
ALWAYS_INLINE int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

#ifdef __USE_STDBITLOOPING__ // this way of looping wins in integrated tests
	#define BEGIN_FOR_EACH_POS_IN_MASK(pos, mask) while (mask) { const int pos = std::countr_zero(mask);
	#define END_FOR_EACH_POS_IN_MASK(pos, mask)  mask &= mask - 1; } 
#else
	// Isolated perf.test show this kind of looping to be the fastest (keeping loop count makes loop branching more predictable for CPU)
	// Note that the value of the second parameter will always be zero after the loop - use "const" version, BEGIN_FOR_EACH_POS_IN_CONST_MASK, to prevent it:
	// However this version loses in integrated tests, most probably due to register spilling (maximum simplicity wins on hot path)
	#define BEGIN_FOR_EACH_POS_IN_MASK(pos, mask) if (mask) { const int loop_count = std::popcount(mask); int loop_iter = 0; do { const int pos = std::countr_zero(mask);
	#define END_FOR_EACH_POS_IN_MASK(pos, mask)  mask &= mask - 1; ++loop_iter; } while (loop_iter != loop_count); }
#endif

// Version that leaves mask intact (minimal overhead to make a copy of uint64_t)
#define BEGIN_FOR_EACH_POS_IN_CONST_MASK(pos, mask) if (mask) { auto mask##Copy = mask; const int loop_count = std::popcount(mask); int loop_iter = 0; do { const int pos = std::countr_zero(mask##Copy);
#define END_FOR_EACH_POS_IN_CONST_MASK(pos, mask)  mask##Copy &= mask##Copy - 1; ++loop_iter; } while (loop_iter != loop_count); }
// Version to be used when the bitmask is already known to be non-zero:
#define BEGIN_DOWHILE_POS_IN_MASK(pos, mask) { assert(mask); do { const int pos = std::countr_zero(mask);
#define END_DOWHILE_POS_IN_MASK(pos, mask)  mask &= mask - 1; } while (mask); }
// Version to be used when the bitmask is likely to be non-zero:
#define BEGIN_FOR_EACH_POS_IN_MASK__LIKELY(pos, mask) {const int loop_count = std::popcount(mask); int loop_iter = 0; while(loop_iter != loop_count) { const int pos = std::countr_zero(mask);
#define END_FOR_EACH_POS_IN_MASK__LIKELY(pos, mask) mask &= mask - 1; ++ loop_iter;}}


ALWAYS_INLINE constexpr bool SameDiagonalOrLine(const int pos1, const int pos2)
{
	assert(IsValidPos(pos1));
	assert(IsValidPos(pos2));
	assert(pos1 != pos2);

	return (Queen_Attacks[pos1] & sq_to_bb(pos2)) != 0; 
}
ALWAYS_INLINE constexpr bool SameDiag(const int sqr1, const int sqr2)
{
	assert(IsValidPos(sqr1));
	assert(IsValidPos(sqr2));
	assert(sqr1 != sqr2);

	return (Bishop_Attacks[sqr1] & sq_to_bb(sqr2)) != 0;
}
ALWAYS_INLINE constexpr bool SameLine(const int pos1, const int pos2)
{
	assert(IsValidPos(pos1));
	assert(IsValidPos(pos2));
	assert(pos1 != pos2);

	return (Rook_Attacks[pos1] & sq_to_bb(pos2)) != 0;
}

ALWAYS_INLINE constexpr bool SameFile(const int sq1, const int sq2) 
{
	assert(IsValidPos(sq1));
	assert(IsValidPos(sq2));

	return ((sq1 - sq2) & 7) == 0;
}

ALWAYS_INLINE constexpr bool IsKnightDiff(const int pos1, const int pos2)
{
	assert(IsValidPos(pos1));
	assert(IsValidPos(pos2));
	
	return (Knight_Attacks[pos1] & sq_to_bb(pos2)) != 0; // this version wins in PerfTests (Knight_Attacks is quite "hot" so in L1 cache always)
}

ALWAYS_INLINE constexpr bool AreSquaresAside(const int sqr1, const int sqr2)
{
	return ((sqr1 >> 3) == (sqr2 >> 3)) & (CTABS(sqr1 - sqr2) == 1);
}

ALWAYS_INLINE constexpr BYTE Distance(const int x1, const int y1, const int x2, const int y2)
{
	assert(IsValidPos(x1, y1));
	assert(IsValidPos(x2, y2));

	BYTE param1 = CTABS(x1 - x2);
	BYTE param2 = CTABS(y1 - y2);
	return (std::max)(param1, param2);
}

ALWAYS_INLINE constexpr BYTE Distance(const int sqr1, const int sqr2)
{
	assert(IsValidPos(sqr1));
	assert(IsValidPos(sqr2));

	return Distance(sqr1 & 7, sqr1 >> 3, sqr2 & 7, sqr2 >> 3);
}

// NOTE: Returns true also on the same squares
ALWAYS_INLINE constexpr bool AreSquaresAdjacent(const int sq1, const int sq2)
{
	assert(IsValidPos(sq1));
	assert(IsValidPos(sq2));

	return (King_Attacks_Ext[sq1] & (sq_to_bb(sq2))) != 0; // this version wins PerfTests (hmm..., maybe dependent on L1 cache usage but King_Attacks_Ext is on all hot paths of the engine...)
}
// NOTE: Returns true also on the same squares
ALWAYS_INLINE constexpr bool AreSquaresAdjacentOrKnightDiff(const int sq1, const int sq2)
{
	assert(IsValidPos(sq1));
	assert(IsValidPos(sq2));

	return ((Knight_Attacks[sq1] | King_Attacks_Ext[sq1]) & (sq_to_bb(sq2))) != 0;
}

ALWAYS_INLINE constexpr bool IsSquareAlongTheLineOrDiag(const int sq, const int sq1, const int sq2)
{
	assert(IsValidPos(sq));
	assert(IsValidPos(sq1));
	assert(IsValidPos(sq2));
	assert(SameDiagonalOrLine(sq1, sq2));

	return betweenLookup.IsSquareOnCommonDiagOrLineOf(sq, sq1, sq2);
}

ALWAYS_INLINE constexpr bool IsPosInBitmask(const int sq, const Bitboard mask)
{
	assert(IsValidPos(sq));

	return (mask & (sq_to_bb(sq))) != 0;
}

ALWAYS_INLINE constexpr bool is_edge(int sq)
{
	assert(IsValidPos(sq));

	return Is_Edge[sq] != 0;
}

template<bool tbActive>
ALWAYS_INLINE constexpr bool is_edge_and_not_same_edge(int sq1, int sq2)
{
	assert(IsValidPos(sq1));
	assert(IsValidPos(sq2));

	if constexpr (tbActive)
	{
		uint8_t edges1 = Is_Edge[sq1];
		uint8_t edges2 = Is_Edge[sq2];

		// Remove shared edges from sq1, then check if any edge bits remain
		return (edges1 & ~edges2) != 0;
	}
	else
		return false; // NOTE: for all existing use cases it is enough to 'return false' to make the feature inactive; MAKE SURE it is the same after any other use case added
}

ALWAYS_INLINE constexpr Bitboard GetRayInDir(const int sqr, const int dx, const int dy)
{
	assert(IsValidPos(sqr));
	assert(dx | dy);
	assert(CTABS(dx) <= 1);
	assert(CTABS(dy) <= 1);

	#ifdef __USE_RAYLOOKUP__
	return rayLookup.GetRayInDir(sqr, dx, dy);
	#else
	
	const bool is_diagonal = dx & dy;
	const bool bAntiDiag = (dx + dy == 0);
	const auto bishopDiagMask = bAntiDiag ? anti_diagonal_masks[sqr] : diagonal_masks[sqr];
	const auto rookDirMask = (dx == 0) ? file_masks[sqr] : rank_masks[sqr];
	const auto full_line = is_diagonal ? bishopDiagMask : rookDirMask;

	const bool moves_upward = (dy > 0) | ((dy == 0) & (dx > 0));

	const Bitboard self_mask = sq_to_bb(sqr);
	const Bitboard up_mask = ~((self_mask - 1) | self_mask);
	const Bitboard down_mask = self_mask - 1;

	const Bitboard direction_mask = moves_upward ? up_mask : down_mask;

	return full_line & direction_mask;
	#endif
}

// Ray starts from posRayAfter, while posRayBase is like a sling that only shows direction (e.g. when pinning piece is searched for with own king on posRayBase and potentially pinned piece on posRayAfter)
ALWAYS_INLINE constexpr Bitboard GetRay(const int posRayAfter, const int posRayBase)
{
	assert(IsValidPos(posRayAfter));
	assert(IsValidPos(posRayBase));
	assert(SameDiagonalOrLine(posRayAfter, posRayBase));

	#ifdef __USE_RAYLOOKUP__
	return rayLookup.GetRay(posRayAfter, posRayBase);
	#else
	
	const Bitboard self_mask = 1ULL << posRayAfter;
	
	#if defined(__USE_OPTIMFORGETRAY__)
	const Bitboard maskFullLineOrDiag = betweenLookup.GetCommonDiagOrLine(posRayAfter, posRayBase);
	#else	
	
	const bool bSameDiag = (Bishop_Attacks[posRayBase] & self_mask) != 0;
	const Bitboard maskSameLine = Rook_Attacks[posRayAfter] & Rook_Attacks[posRayBase];
	const Bitboard maskSameDiag = Bishop_Attacks[posRayAfter] & Bishop_Attacks[posRayBase];
	const Bitboard maskFullLineOrDiag = bSameDiag ? maskSameDiag : maskSameLine;
	#endif
	
	const bool moves_upward = posRayAfter > posRayBase;

	const Bitboard up_mask = ~((self_mask - 1) | self_mask);
	const Bitboard down_mask = self_mask - 1;

	const Bitboard direction_mask = moves_upward ? up_mask : down_mask;
	const Bitboard res = maskFullLineOrDiag & direction_mask;

	return res;	
	#endif
}


// This template function is probably a slight overkill - compilers are already quite good at such tricks:
template<unsigned int val2, typename T>
ALWAYS_INLINE std::conditional_t<std::is_same<T, bool>::value, int, T> MUL(const T val1)
{
	static_assert(val2 == 3 || val2 == 5 || val2 == 6 || val2 == 7 || val2 == 9 || val2 == 10 || val2 == 12 || val2 == 20, "Unsupported template parameter for method MUL.");

	if constexpr (val2 == 3)
	{
		return val1 + val1 + val1;
	}
	else if constexpr (val2 == 5)
	{
		return (val1 << 2) + val1;
	}
	else if constexpr (val2 == 6)
	{
		return (val1 << 2) + val1 + val1;
	}
	else if constexpr (val2 == 7)
	{
		return (val1 << 3) - val1;
	}
	else if constexpr (val2 == 9)
	{
		return (val1 << 3) + val1;
	}
	else if constexpr (val2 == 10)
	{
		return (val1 << 3) + val1 + val1;
	}
	else if constexpr (val2 == 12)
	{
		return (val1 << 3) + (val1 << 2);
	}
	else if constexpr (val2 == 20)
	{
		return (val1 << 4) + (val1 << 2);
	}
	else
	{
		throw std::string("Unsupported template parameter for method MUL.");
	}
}

// Constexpr version:
[[nodiscard]] ALWAYS_INLINE constexpr Bitboard ct_bswap64(const Bitboard x) noexcept
{
	return ((x & 0x00000000000000FFULL) << 56) |
		((x & 0x000000000000FF00ULL) << 40) |
		((x & 0x0000000000FF0000ULL) << 24) |
		((x & 0x00000000FF000000ULL) << 8) |
		((x & 0x000000FF00000000ULL) >> 8) |
		((x & 0x0000FF0000000000ULL) >> 24) |
		((x & 0x00FF000000000000ULL) >> 40) |
		((x & 0xFF00000000000000ULL) >> 56);
}

[[nodiscard]] ALWAYS_INLINE Bitboard bswap64(const Bitboard x) noexcept
{
	#if defined(_MSC_VER)
		return _byteswap_uint64(x);
	#elif defined(__GNUC__) || defined(__clang__)
		return __builtin_bswap64(x);
	#else		
		return ct_bswap64(x); // fallback
	#endif
}

[[nodiscard]] ALWAYS_INLINE Bitboard reflect_bits(Bitboard b) noexcept
{
	#if __cplusplus >= 202302L
		return std::bit_reverse(b);
	#else

	#if !defined(_MSC_VER) && defined(__clang__)
		return __builtin_bitreverse64(b);
	#else

		b = bswap64(b);

		b = ((b >> 1) & 0x5555555555555555ULL) | ((b & 0x5555555555555555ULL) << 1);
		b = ((b >> 2) & 0x3333333333333333ULL) | ((b & 0x3333333333333333ULL) << 2);
		b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b & 0x0F0F0F0F0F0F0F0FULL) << 4);

		return b;
	#endif
	#endif
}

// An alternative is to use std::has_single_bit but some compilers are noticed to emit too many branches with it
template<bool tbKnownToBeNonZero = false>
ALWAYS_INLINE bool HasSingleBit(const Bitboard val)
{
	if constexpr (tbKnownToBeNonZero)
	{
		assert(val != 0);
		return (val & (val - 1)) == 0;
	}
	else	
		return (val != 0) & ((val & (val - 1)) == 0);
}
