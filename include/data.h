//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <array>
#include <cassert>
#include "common.h"

#ifdef __USE_SQUARE_BITBOARD__
constexpr std::array<uint64_t, 64> init_square_bitboard()
{
	std::array<uint64_t, 64> res;

	for (int i = 0; i < 64; ++i)
		res[i] = 1ULL << i;

	return res;
}
alignas(64) inline constexpr std::array<std::uint64_t, 64> square_bitboard = init_square_bitboard();
#endif

ALWAYS_INLINE constexpr uint64_t sq_to_bb(const int sq)
{
	assert(((unsigned) sq) < 64);

	#ifdef __USE_SQUARE_BITBOARD__
	return square_bitboard[sq];
	#else
	return 1ULL << sq;
	#endif
}

// ----------- Helper constexpr functions to fill the data (at the end of this file) in compile time ----------

// Compile-time abs (abs() not constexpr yet in C++20)
template<typename T>
ALWAYS_INLINE constexpr T CTABS(const T a)
{
	if (std::is_constant_evaluated())
		return (a < 0) ? -a : a;
	else
		return std::abs(a);
}

constexpr std::array<std::uint64_t, 64> init_black_pawn_attacks()
{
	std::array<std::uint64_t, 64> attacks{};

	constexpr Bitboard NOT_A_FILE = 0xFEFEFEFEFEFEFEFEULL; // All but column A
	constexpr Bitboard NOT_H_FILE = 0x7F7F7F7F7F7F7F7FULL; // All but column H

	for (int sq = 0; sq < 64; ++sq)
	{
		Bitboard pawn = sq_to_bb(sq);
		Bitboard mask = 0ULL;

		mask |= (pawn & NOT_H_FILE) >> 7;

		mask |= (pawn & NOT_A_FILE) >> 9;

		attacks[sq] = mask;
	}

	return attacks;
}

constexpr std::array<uint64_t, 64> init_white_pawn_attacks()
{
	std::array<uint64_t, 64> attacks{};

	constexpr Bitboard NOT_A_FILE = 0xFEFEFEFEFEFEFEFEULL;
	constexpr Bitboard NOT_H_FILE = 0x7F7F7F7F7F7F7F7FULL;

	for (int sq = 0; sq < 64; ++sq) 
	{
		Bitboard pawn = sq_to_bb(sq);
		Bitboard mask = 0ULL;

		mask |= (pawn & NOT_A_FILE) << 7;

		mask |= (pawn & NOT_H_FILE) << 9;

		attacks[sq] = mask;
	}
	return attacks;
}

constexpr uint64_t generate_bishop_mask(int square_index) noexcept
{
	uint64_t mask = 0;
	const int sq_x = square_index & 7;
	const int sq_y = square_index >> 3;

	for (int dx = -1; dx <= 1; dx += 2)
		for (int dy = -1; dy <= 1; dy += 2)
		{
			int target_x = sq_x + dx;
			int target_y = sq_y + dy;

			while (unsigned(target_x) < 8 && unsigned(target_y) < 8)
			{
				int target_index = target_y * 8 + target_x;
				mask |= (1ULL << target_index);

				target_x += dx;
				target_y += dy;
			}
		}

	return mask;
}

constexpr uint64_t generate_rook_mask(int square_index) noexcept
{
	uint64_t mask = 0;
	const int sq_x = square_index & 7;
	const int sq_y = square_index >> 3;

	for (int dx = -1; dx <= 1; ++dx)
		for (int dy = -1; dy <= 1; ++dy)
			if (CTABS(dx) + CTABS(dy) == 1)
			{
				int target_x = sq_x + dx;
				int target_y = sq_y + dy;

				while (unsigned(target_x) < 8 && unsigned(target_y) < 8)
				{
					int target_index = target_y * 8 + target_x;
					mask |= (1ULL << target_index);

					target_x += dx;
					target_y += dy;
				}
			}

	return mask;
}

constexpr uint64_t generate_knight_mask(int square_index) noexcept
{
	uint64_t mask = 0;
	const int sq_x = square_index & 7;
	const int sq_y = square_index >> 3;

	const int dx[8] = { 1,  2,  2,  1, -1, -2, -2, -1 };
	const int dy[8] = { 2,  1, -1, -2, -2, -1,  1,  2 };

	for (int i = 0; i < 8; ++i)
	{
		int target_x = sq_x + dx[i];
		int target_y = sq_y + dy[i];

		if (unsigned(target_x) < 8 && unsigned(target_y) < 8)
		{
			int target_index = target_y * 8 + target_x;
			mask |= (1ULL << target_index);
		}
	}
	return mask;
}


constexpr std::array<std::uint64_t, 64> CalcKnightAttackBitboards() noexcept
{
	std::array<uint64_t, 64> table{};
	for (int i = 0; i < 64; ++i) {
		table[i] = generate_knight_mask(i);
	}
	return table;
}
constexpr std::array<std::uint64_t, 64> CalcBishopAttackBitboards() noexcept
{
	std::array<uint64_t, 64> table{};
	for (int i = 0; i < 64; ++i) {
		table[i] = generate_bishop_mask(i);
	}
	return table;
}
constexpr std::array<std::uint64_t, 64> CalcRookAttackBitboards() noexcept
{
	std::array<uint64_t, 64> table{};
	for (int i = 0; i < 64; ++i) {
		table[i] = generate_rook_mask(i);
	}
	return table;
}

constexpr std::array<std::uint64_t, 64> InitQueenAttackMasks() noexcept
{
	std::array<std::uint64_t, 64> res;
	for (int sq = 0; sq < 64; ++sq)
	{
		uint64_t mask = 0;
		int r = sq / 8, c = sq % 8;
		for (int i = 0; i < 64; ++i) {
			int ri = i / 8, ci = i % 8;
			if (i != sq && (r == ri || c == ci || CTABS(r - ri) == CTABS(c - ci))) {
				mask |= (1ULL << i);
			}
		}

		res[sq] = mask;
	}
	return res;
}

template<bool tbInclKingsSquare = false>
constexpr std::array<uint64_t, 64> init_king_attacks()
{
	std::array<uint64_t, 64> king_attacks{};

	constexpr uint64_t NOT_A_FILE = 0xFEFEFEFEFEFEFEFEULL;
	constexpr uint64_t NOT_H_FILE = 0x7F7F7F7F7F7F7F7FULL;

	for (int sq = 0; sq < 64; ++sq)
	{
		uint64_t king = sq_to_bb(sq);
		uint64_t attacks = 0ULL;

		if constexpr (tbInclKingsSquare)
			attacks |= king;

		attacks |= (king << 8); 
		attacks |= (king >> 8); 
		
		attacks |= (king << 1) & NOT_A_FILE; 
		attacks |= (king << 9) & NOT_A_FILE; 
		attacks |= (king >> 7) & NOT_A_FILE; 

		attacks |= (king >> 1) & NOT_H_FILE;
		attacks |= (king << 7) & NOT_H_FILE;
		attacks |= (king >> 9) & NOT_H_FILE;

		king_attacks[sq] = attacks;
	}

	return king_attacks;
}

template<bool tbFileMasks> // otherwise lineMasks
constexpr std::array<uint64_t, 64> init_line_masks()
{
	std::array<uint64_t, 64> masks;

	for (int sq = 0; sq < 64; sq++)
	{
		int file = sq % 8; 
		int rank = sq / 8; 

		if constexpr (tbFileMasks)
		{

			uint64_t f_mask = 0ULL;
			for (int r = 0; r < 8; r++) {
				f_mask |= (1ULL << (r * 8 + file));
			}
			masks[sq] = f_mask;
		}
		else
		{
			uint64_t r_mask = 0ULL;
			for (int f = 0; f < 8; f++) {
				r_mask |= (1ULL << (rank * 8 + f));
			}
			masks[sq] = r_mask;
		}
	}

	return masks;
}

template<bool tbAntiDiagonal = false>
constexpr std::array<uint64_t, 64> init_diagonal_masks()
{
	std::array<uint64_t, 64> masks;

	for (int sq = 0; sq < 64; sq++)
	{
		int file = sq % 8; 
		int rank = sq / 8; 

		uint64_t d_mask = 0ULL;
		uint64_t ad_mask = 0ULL;

		if constexpr (tbAntiDiagonal)
		{
			for (int f = file, r = rank; f >= 0 && r < 8; f--, r++) {
				ad_mask |= (1ULL << (r * 8 + f));
			}
			for (int f = file, r = rank; f < 8 && r >= 0; f++, r--) {
				ad_mask |= (1ULL << (r * 8 + f));
			}
			masks[sq] = ad_mask;

		}
		else
		{
			for (int f = file, r = rank; f >= 0 && r >= 0; f--, r--) {
				d_mask |= (1ULL << (r * 8 + f));
			}
			for (int f = file, r = rank; f < 8 && r < 8; f++, r++) {
				d_mask |= (1ULL << (r * 8 + f));
			}
			masks[sq] = d_mask;
		}
	}

	return masks;
}

// ------------------------------------------------------
// Function that generates attacks on 1st line 
constexpr uint8_t gather_rank_attacks(int square_file, uint8_t occ) 
{
	uint8_t attacks = 0;
	// Move right (positive)
	for (int f = square_file + 1; f < 8; ++f) {
		attacks |= (1 << f);
		if (occ & (1 << f)) 
			break; // Blocker found
	}
	// Move left (negative)
	for (int f = square_file - 1; f >= 0; --f) {
		attacks |= (1 << f);
		if (occ & (1 << f)) 
			break; // blocker found
	}
	return attacks;
}

constexpr std::array<uint8_t, 64 * 8> init_first_rank_attacks()
{
	std::array<uint8_t, 64 * 8> arrFirstRankAttacks64x8;

	for (int occ64 = 0; occ64 < 64; ++occ64) {
		// We take full occupancy (put bits 1-6 on their place, while edge bits 0 and 7 are zeros)
		uint8_t full_occ = occ64 << 1;
		for (int file = 0; file < 8; ++file) {
			arrFirstRankAttacks64x8[4 * (occ64 * 2) + file] = gather_rank_attacks(file, full_occ);
		}
	}

	return arrFirstRankAttacks64x8;
}

// ------------ Constexpr data filled in in compile time (6kB in total) ---------------

alignas(64) inline constexpr std::array<std::uint64_t, 64> Black_Pawn_Attacks = init_black_pawn_attacks();
alignas(64) inline constexpr std::array<std::uint64_t, 64> White_Pawn_Attacks = init_white_pawn_attacks();

alignas(64) inline constexpr std::array<std::uint64_t, 64> Knight_Attacks = CalcKnightAttackBitboards();
alignas(64) inline constexpr std::array<std::uint64_t, 64> Bishop_Attacks = CalcBishopAttackBitboards();
alignas(64) inline constexpr std::array<std::uint64_t, 64> Rook_Attacks = CalcRookAttackBitboards();
alignas(64) inline constexpr std::array<std::uint64_t, 64> Queen_Attacks = InitQueenAttackMasks();

alignas(64) inline constexpr std::array<std::uint64_t, 64> King_Attacks = init_king_attacks();
alignas(64) inline constexpr std::array<std::uint64_t, 64> King_Attacks_Ext = init_king_attacks<1>(); // incl.king's square

alignas(64) inline constexpr std::array<std::uint64_t, 64> file_masks = init_line_masks<1>();
alignas(64) inline constexpr std::array<std::uint64_t, 64> rank_masks = init_line_masks<0>();
alignas(64) inline constexpr std::array<std::uint64_t, 64> diagonal_masks = init_diagonal_masks<0>();
alignas(64) inline constexpr std::array<std::uint64_t, 64> anti_diagonal_masks = init_diagonal_masks<1>();

alignas(64) inline constexpr std::array<uint8_t, 64 * 8> arrFirstRankAttacks64x8 = init_first_rank_attacks();
