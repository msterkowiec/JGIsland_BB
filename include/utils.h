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

#include <stdint.h>
#include <cstdint>
#include <cassert>
#include <bit>

// Some legacy/mailbox coding that will occasionally be used. 
inline constexpr int CLR_WHITE = 64;
inline constexpr int CLR_BLACK = 128;

inline constexpr int FGR_EMPTY = 0;
inline constexpr int FGR_KING = 1;
inline constexpr int FGR_PAWN = 2;
inline constexpr int FGR_BISHOP = 4;
inline constexpr int FGR_ROOK = 8;
inline constexpr int FGR_QUEEN = 12;
inline constexpr int FGR_KNIGHT = 16;

inline constexpr int WHITE_KING = (CLR_WHITE + FGR_KING);
inline constexpr int WHITE_PAWN = (CLR_WHITE + FGR_PAWN);
inline constexpr int WHITE_KNIGHT = (CLR_WHITE + FGR_KNIGHT);
inline constexpr int WHITE_BISHOP = (CLR_WHITE + FGR_BISHOP);
inline constexpr int WHITE_ROOK = (CLR_WHITE + FGR_ROOK);
inline constexpr int WHITE_QUEEN = (CLR_WHITE + FGR_QUEEN);

inline constexpr int BLACK_KING = (CLR_BLACK + FGR_KING);
inline constexpr int BLACK_PAWN = (CLR_BLACK + FGR_PAWN);
inline constexpr int BLACK_KNIGHT = (CLR_BLACK + FGR_KNIGHT);
inline constexpr int BLACK_BISHOP = (CLR_BLACK + FGR_BISHOP);
inline constexpr int BLACK_ROOK = (CLR_BLACK + FGR_ROOK);
inline constexpr int BLACK_QUEEN = (CLR_BLACK + FGR_QUEEN);

enum HCOORD { _A_ = 0, _B_, _C_, _D_, _E_, _F_, _G_, _H_ };
enum VCOORD { _1_ = 0, _2_, _3_, _4_, _5_, _6_, _7_, _8_ };
inline constexpr int _A1_ = 0;
inline constexpr int _B1_ = 1;
inline constexpr int _C1_ = 2;
inline constexpr int _D1_ = 3;
inline constexpr int _E1_ = 4;
inline constexpr int _F1_ = 5;
inline constexpr int _G1_ = 6;
inline constexpr int _H1_ = 7;
inline constexpr int _A2_ = 8;
inline constexpr int _B2_ = 9;
inline constexpr int _C2_ = 10;
inline constexpr int _D2_ = 11;
inline constexpr int _E2_ = 12;
inline constexpr int _F2_ = 13;
inline constexpr int _G2_ = 14;
inline constexpr int _H2_ = 15;
inline constexpr int _A3_ = 16;
inline constexpr int _B3_ = 17;
inline constexpr int _C3_ = 18;
inline constexpr int _D3_ = 19;
inline constexpr int _E3_ = 20;
inline constexpr int _F3_ = 21;
inline constexpr int _G3_ = 22;
inline constexpr int _H3_ = 23;
inline constexpr int _A4_ = 24;
inline constexpr int _B4_ = 25;
inline constexpr int _C4_ = 26;
inline constexpr int _D4_ = 27;
inline constexpr int _E4_ = 28;
inline constexpr int _F4_ = 29;
inline constexpr int _G4_ = 30;
inline constexpr int _H4_ = 31;
inline constexpr int _A5_ = 32;
inline constexpr int _B5_ = 33;
inline constexpr int _C5_ = 34;
inline constexpr int _D5_ = 35;
inline constexpr int _E5_ = 36;
inline constexpr int _F5_ = 37;
inline constexpr int _G5_ = 38;
inline constexpr int _H5_ = 39;
inline constexpr int _A6_ = 40;
inline constexpr int _B6_ = 41;
inline constexpr int _C6_ = 42;
inline constexpr int _D6_ = 43;
inline constexpr int _E6_ = 44;
inline constexpr int _F6_ = 45;
inline constexpr int _G6_ = 46;
inline constexpr int _H6_ = 47;
inline constexpr int _A7_ = 48;
inline constexpr int _B7_ = 49;
inline constexpr int _C7_ = 50;
inline constexpr int _D7_ = 51;
inline constexpr int _E7_ = 52;
inline constexpr int _F7_ = 53;
inline constexpr int _G7_ = 54;
inline constexpr int _H7_ = 55;
inline constexpr int _A8_ = 56;
inline constexpr int _B8_ = 57;
inline constexpr int _C8_ = 58;
inline constexpr int _D8_ = 59;
inline constexpr int _E8_ = 60;
inline constexpr int _F8_ = 61;
inline constexpr int _G8_ = 62;
inline constexpr int _H8_ = 63;

inline constexpr int DBL_CHECKED = 64;
inline constexpr bool EXCL_KING = 0;
inline constexpr bool SKIP_KING = 0;
inline constexpr bool INCL_KING = 1;
inline constexpr bool EXCL_PINNED = 0;
inline constexpr bool INCL_PINNED = 1;
inline constexpr bool FIND_ALL = 0;
inline constexpr bool FIND_ONE = 1;

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

// Perf.test show this kind of looping to be the fastest (keeping loop count makes loop branching more predictable for CPU)
// Note that the value of the second parameter will always be zero after the loop - use "const" version, BEGIN_FOR_EACH_POS_IN_CONST_MASK, to prevent it:
#define BEGIN_FOR_EACH_POS_IN_MASK(pos, mask) if (mask) { const int loop_count = std::popcount(mask); int loop_iter = 0; do { const int pos = std::countr_zero(mask);
#define END_FOR_EACH_POS_IN_MASK(pos, mask)  mask &= mask - 1; ++loop_iter; } while (loop_iter != loop_count); }
// Version that leaves mask intact (minimal overhead to make a copy of uint64_t)
#define BEGIN_FOR_EACH_POS_IN_CONST_MASK(pos, mask) if (mask) { auto mask##Copy = mask; const int loop_count = std::popcount(mask); int loop_iter = 0; do { const int pos = std::countr_zero(mask##Copy);
#define END_FOR_EACH_POS_IN_CONST_MASK(pos, mask)  mask##Copy &= mask##Copy - 1; ++loop_iter; } while (loop_iter != loop_count); }
// Version to be used when mask already known to be non-zero:
#define BEGIN_DOWHILE_POS_IN_MASK(pos, mask) { assert(mask); const int loop_count = std::popcount(mask); int loop_iter = 0; do { const int pos = std::countr_zero(mask);
#define END_DOWHILE_POS_IN_MASK(pos, mask)  mask &= mask - 1; ++loop_iter; } while (loop_iter != loop_count); }


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

// Returns true also on the same squares!
ALWAYS_INLINE constexpr bool AreSquaresAdjacent(const int sq1, const int sq2)
{
	assert(IsValidPos(sq1));
	assert(IsValidPos(sq2));

	return (King_Attacks_Ext[sq1] & (sq_to_bb(sq2))) != 0; // this version wins PerfTests (hmm..., maybe dependent on L1 cache usage but King_Attacks_Ext is on all hot paths of the engine...)
}

ALWAYS_INLINE constexpr bool IsSquareAlongTheLineOrDiag(const int sq, const int sq1, const int sq2)
{
	assert(IsValidPos(sq));
	assert(IsValidPos(sq1));
	assert(IsValidPos(sq2));
	assert(SameDiagonalOrLine(sq1, sq2));

	const bool sameLine = SameLine(sq1, sq2);
	const uint64_t* pBitboard = sameLine ? Rook_Attacks.data() : Bishop_Attacks.data();
	const auto mask = (pBitboard[sq1] & pBitboard[sq2]) | (sq_to_bb(sq1)) | (sq_to_bb(sq2));
	return (mask & (sq_to_bb(sq))) != 0;
}

ALWAYS_INLINE constexpr bool IsPosInBitmask(const int sq, const uint64_t mask)
{
	assert(IsValidPos(sq));

	return (mask & (sq_to_bb(sq))) != 0;
}

ALWAYS_INLINE constexpr uint64_t GetRayInDir(const int sqr, const int dx, const int dy)
{
	assert(IsValidPos(sqr));
	assert(dx | dy);
	assert(CTABS(dx) <= 1);
	assert(CTABS(dy) <= 1);

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
}

// Ray starts from posRayAfter, while posRayBase is like a sling that only shows direction (e.g. when pinning piece is searched for with own king on posRayBase and potentially pinned piece on posRayAfter)
ALWAYS_INLINE constexpr uint64_t GetRay(const int posRayAfter, const int posRayBase)
{
	assert(IsValidPos(posRayAfter));
	assert(IsValidPos(posRayBase));
	assert(SameDiagonalOrLine(posRayAfter, posRayBase));

	const Bitboard self_mask = 1ULL << posRayAfter;
	const bool bSameDiag = (Bishop_Attacks[posRayBase] & self_mask) != 0;
	const Bitboard maskSameLine = Rook_Attacks[posRayAfter] & Rook_Attacks[posRayBase];
	const Bitboard maskSameDiag = Bishop_Attacks[posRayAfter] & Bishop_Attacks[posRayBase];
	const Bitboard maskFullLineOrDiag = bSameDiag ? maskSameDiag : maskSameLine;

	const bool moves_upward = posRayAfter > posRayBase;

	const Bitboard up_mask = ~((self_mask - 1) | self_mask);
	const Bitboard down_mask = self_mask - 1;

	const Bitboard direction_mask = moves_upward ? up_mask : down_mask;
	const Bitboard res = maskFullLineOrDiag & direction_mask;

	return res;
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
[[nodiscard]] ALWAYS_INLINE constexpr uint64_t ct_bswap64(const uint64_t x) noexcept
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

[[nodiscard]] ALWAYS_INLINE uint64_t bswap64(const uint64_t x) noexcept
{
	#if defined(_MSC_VER)
		return _byteswap_uint64(x);
	#elif defined(__GNUC__) || defined(__clang__)
		return __builtin_bswap64(x);
	#else		
		return ct_bswap64(x); // fallback
	#endif
}

[[nodiscard]] ALWAYS_INLINE uint64_t reflect_bits(uint64_t b) noexcept
{
	#if __cplusplus >= 202302L
		return std::bit_reverse(b);
	#else

	#if !defined(_MSC_VER) && !defined(__GNUC__)
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


// --------------------------------------------------------
// Hyperbola Quintessence

template<char Dir> //Dir 0 = any, 1/-1 == horizontal, 8/-8 == vertical
ALWAYS_INLINE uint64_t get_raw_rook_moves_hq(const int square, const uint64_t occupancy)
{
	static_assert(Dir == 0 || Dir == 1 || Dir == -1 || Dir == 8 || Dir == -8, "");
	assert(IsValidPos(square));

	uint64_t raw_moves = 0ULL;

	const uint64_t slider = sq_to_bb(square);	
	const uint64_t slider2 = slider * 2;

	if constexpr (Dir == 0 || CTABS(Dir) == 1)
	{
		#ifdef __USE_FIRSTRANKATTACKSLOOKUP__
		const unsigned int file = square & 7;       // file
		const unsigned int rkx8 = square & 56;      // row * 8

		// We reduce occupancy of one line to one byte and isolate 6 inner bits (skipping edges)
		const unsigned int rankOccX2 = (occupancy >> rkx8) & 126;
		
		const Bitboard attacks = arrFirstRankAttacks64x8[4 * rankOccX2 + file];

		// Shift the result back to the proper line of the chessboard:
		raw_moves |= (attacks << rkx8);

		#else
		const uint64_t mask_r = rank_masks[square];
		const uint64_t forward_r = (occupancy & mask_r) - slider2;
		
		const uint64_t reverse_r = reflect_bits(reflect_bits(occupancy & mask_r) - (2ULL << (63 - square))); // optimized -(2 * reflect_bits(slider))

		raw_moves |= (forward_r ^ reverse_r) & mask_r;
		#endif
	}

	if constexpr (Dir == 0 || CTABS(Dir) == 8)
	{
		const uint64_t mask_f = file_masks[square];
		const uint64_t forward_f = (occupancy & mask_f) - slider2;
		const uint64_t reverse_f = bswap64(bswap64(occupancy & mask_f) - (2 * bswap64(slider)));
		raw_moves |= (forward_f ^ reverse_f) & mask_f;
	}

	return raw_moves;
}

//Dir 0 = any, 1/-1 == horizontal, 8/-8 == vertical
template<char Dir = 0, bool tbCapturesOnly = false>
ALWAYS_INLINE uint64_t get_rook_moves_hq(const int square, const uint64_t occupancy, const uint64_t pieces_of_same_color)
{
	assert(IsValidPos(square));

	uint64_t raw_moves = get_raw_rook_moves_hq<Dir>(square, occupancy);

	if constexpr (tbCapturesOnly)
		return raw_moves & (occupancy & ~pieces_of_same_color); 
	else
		// Filter own pieces:
		return raw_moves & ~pieces_of_same_color;
}

// These are not moves yet, since blocking piece is not considered properly
template<char Dir = 0>
ALWAYS_INLINE auto get_raw_bishop_moves_hq(const int square, const uint64_t occupancy)
{
	assert(IsValidPos(square));

	const uint64_t slider = sq_to_bb(square);	
	const uint64_t helper = 2ULL << (square ^ 56); // 2 * bswap64(slider);
	const uint64_t slider2 = 2 * slider;
	uint64_t diagonal_moves, anti_diagonal_moves;

	// Diagonal
	if constexpr (Dir >= 0)
	{
		const uint64_t mask_d = diagonal_masks[square];
		const uint64_t forward_d = (occupancy & mask_d) - slider2;
		const uint64_t reverse_d = bswap64(bswap64(occupancy & mask_d) - helper);
		diagonal_moves = (forward_d ^ reverse_d) & mask_d;
	}
	else
		diagonal_moves = 0;

	// Anti-Diagonal
	if constexpr (Dir <= 0)
	{
		const uint64_t mask_ad = anti_diagonal_masks[square];
		const uint64_t forward_ad = (occupancy & mask_ad) - slider2;
		const uint64_t reverse_ad = bswap64(bswap64(occupancy & mask_ad) - helper);
		anti_diagonal_moves = (forward_ad ^ reverse_ad) & mask_ad;
	}
	else
		anti_diagonal_moves = 0;

	return diagonal_moves | anti_diagonal_moves;
}

// Dir == 0 == any, 1==main diagonal, -1==anti diagonal
// For tbStable see get_raw_bishop_moves_hq (it's unused - experimental...)
template<bool tbCapturesOnly = false, char Dir = 0>
ALWAYS_INLINE uint64_t get_bishop_moves_hq(const int square, const uint64_t occupancy, const uint64_t own_pieces) // own_pieces should be white_pieces for white bishop and vice versa
{
	static_assert(Dir == 0 || Dir == 1 || Dir == -1, "");
	assert(IsValidPos(square));

	uint64_t raw_moves = get_raw_bishop_moves_hq<Dir>(square, occupancy);
	if constexpr (tbCapturesOnly)
		raw_moves &= (occupancy & ~own_pieces);
	else
		raw_moves &= ~own_pieces;
	return raw_moves;	
}

