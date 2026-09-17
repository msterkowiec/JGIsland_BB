//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "config.h"
#include "utils.h"

template<char Dir = 0> //Dir 0 = any, 1/-1 == horizontal, 8/-8 == vertical
ALWAYS_INLINE Bitboard get_raw_rook_moves_hq(const int square, const Bitboard occupancy)
{
	static_assert(Dir == 0 || Dir == 1 || Dir == -1 || Dir == 8 || Dir == -8, "");
	assert(IsValidPos(square));

	Bitboard raw_moves = 0ULL;

	const Bitboard slider = sq_to_bb(square);
	const Bitboard slider2 = slider + slider;

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
		const Bitboard mask_r = rank_masks[square];
		const Bitboard forward_r = (occupancy & mask_r) - slider2;

		const Bitboard reverse_r = reflect_bits(reflect_bits(occupancy & mask_r) - (2ULL << (63 - square))); // optimized -(2 * reflect_bits(slider))

		raw_moves |= (forward_r ^ reverse_r) & mask_r;
		#endif
	}

	if constexpr (Dir == 0 || CTABS(Dir) == 8)
	{
		const Bitboard mask_f = file_masks[square];
		const Bitboard forward_f = (occupancy & mask_f) - slider2;
		const Bitboard reverse_f = bswap64(bswap64(occupancy & mask_f) - (2 * bswap64(slider)));
		raw_moves |= (forward_f ^ reverse_f) & mask_f;
	}

	return raw_moves;
}

// These are not moves yet, since blocking piece is not considered properly
template<char Dir = 0>
ALWAYS_INLINE static  auto get_raw_bishop_moves_hq(const int square, const Bitboard occupancy)
{
	assert(IsValidPos(square));

	// Hyperbola Quintessence
	const Bitboard helper = 2ULL << (square ^ 56); // 2 * bswap64(slider);	
	const Bitboard slider = sq_to_bb(square);

	#ifdef __USE_OPTIMIZED_HQ__
	if constexpr (Dir == 0) // both diag and anti-diag - perf.tests don't show any speed-up, so for now __USE_OPTIMIZED_HQ__ commented out
	{
		// let's try to make instructions sort of interlaced hoping to get boost from CPU ILP:
		const Bitboard mask_d = diagonal_masks[square];
		const Bitboard mask_ad = anti_diagonal_masks[square];
		const Bitboard forward_d = (occupancy & mask_d) - slider - slider;
		const Bitboard forward_ad = (occupancy & mask_ad) - slider - slider;
		const Bitboard reverse_d = bswap64(bswap64(occupancy & mask_d) - helper);
		const Bitboard reverse_ad = bswap64(bswap64(occupancy & mask_ad) - helper);
		return ((forward_d ^ reverse_d) & mask_d) | ((forward_ad ^ reverse_ad) & mask_ad); // return diagonal_moves | anti_diagonal_moves
	}
	else
	#endif
	{
		Bitboard res;

		// Diagonal
		if constexpr (Dir >= 0)
		{
			const Bitboard mask_d = diagonal_masks[square];
			const Bitboard forward_d = (occupancy & mask_d) - slider - slider;
			const Bitboard reverse_d = bswap64(bswap64(occupancy & mask_d) - helper);
			res = (forward_d ^ reverse_d) & mask_d;
		}
		else
			res = 0;

		// Anti-Diagonal
		if constexpr (Dir <= 0)
		{
			const Bitboard mask_ad = anti_diagonal_masks[square];
			const Bitboard forward_ad = (occupancy & mask_ad) - slider - slider;
			const Bitboard reverse_ad = bswap64(bswap64(occupancy & mask_ad) - helper);
			res |= ((forward_ad ^ reverse_ad) & mask_ad);
		}

		return res;
	}
}
