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

constexpr std::array<uint64_t, 64> init_white_pawn_check_area()
{
	std::array<uint64_t, 64> res{};

	for (int sq = 0; sq < 64; ++sq)
	{
		uint64_t mask = 0;
		const int x = sq % 8;
		const int y = sq / 8;

		// Direct check:
		if (y >= 3)
		{
			mask |= 1ULL << (sq - 16); // capture
			if (x != 0)
			{
				mask |= 1ULL << (sq - 17); // move forward
				if (x != 1)
					mask |= 1ULL << (sq - 18); // capture
				if (y == 4) // fifth line
					mask |= 1ULL << (sq - 25); // double move forward
			}
			if (x != 7)
			{
				mask |= 1ULL << (sq - 15); // move forward
				if (x != 6)
					mask |= 1ULL << (sq - 14); // capture
				if (y == 4) // fifth line
					mask |= 1ULL << (sq - 23); // double move forward
			}			
		}

		// Discovered attack or promo to long distance figure:
		for (int dx = -1 ; dx <= 1 ; ++ dx)
			for (int dy = -1; dy <= 1; ++dy)
				if (dx | dy)
				{
					int px = x + dx;
					int py = y + dy;
					while (px >= 0 && py > 0 && px < 8 && py < 8)
					{
						if (py != 7)
						{
							if (dx != 0 && (px == 0 || px == 7))
								break; // discovered check impossible
							mask |= 1ULL << (px + py * 8);
						}
						else
						{
							mask |= 1ULL << (px + (py - 1) * 8); // promo forward
							if (px != 0)
								mask |= 1ULL << (px - 1 + (py - 1) * 8); // promo capture
							if (px != 7)
								mask |= 1ULL << (px + 1 + (py - 1) * 8); // promo capture
						}
						px += dx;
						py += dy;
					}
				}

		// Promo to knight:
		if (y == 5)
		{
			mask |= 1ULL << (x + 6 * 8); // promo capture
			if (x != 0)
			{
				mask |= 1ULL << (x - 1 + 6 * 8); // promo forward
				if (x != 1)
					mask |= 1ULL << (x - 2 + 6 * 8); // promo capture
			}
			if (x != 7)
			{
				mask |= 1ULL << (x + 1 + 6 * 8); // promo forward
				if (x != 6)
					mask |= 1ULL << (x + 2 + 6 * 8); // promo capture
			}
		}
		else
			if (y == 6)
			{
				if (x >= 2)
				{
					mask |= 1ULL << (x - 2 + 6 * 8); // promo forward
					if (x >= 3)
						mask |= 1ULL << (x - 3 + 6 * 8); // promo capture
				}
				if (x <= 5)
				{
					mask |= 1ULL << (x + 2 + 6 * 8); // promo forward
					if (x <= 6)
						mask |= 1ULL << (x + 3 + 6 * 8); // promo capture
				}
				// other squares match queen promo ...
			}

		res[sq] = mask;
	}

	return res;
}

constexpr std::array<uint64_t, 64> init_bishops_that_can_check()
{	
	constexpr uint64_t FILE_A_NO_EDGES = 0x0001010101010100ULL;
	constexpr uint64_t LINE_1_NO_EDGES = 126;

	constexpr uint64_t DARK_SQUARES = 0xAA55AA55AA55AA55ULL; // a1, c1 etc.
	constexpr uint64_t LIGHT_SQUARES = ~DARK_SQUARES; // b1, d1 etc.

	std::array<uint64_t, 64> res{};

	for (int sq = 0; sq < 64; ++sq)
	{
		const int x = sq & 7;
		const int y = sq >> 3;

		const bool isDarkSquare = DARK_SQUARES & (1ULL << sq);
		res[sq] = isDarkSquare ? DARK_SQUARES : LIGHT_SQUARES; // direct check		
		res[sq] |= (LINE_1_NO_EDGES << (y * 8)) | (FILE_A_NO_EDGES << x); // discovered check					  		
	}

	return res;
}

constexpr std::array<uint64_t, 64> init_knights_that_can_check()
{	
	constexpr uint64_t FILE_A_NO_EDGES = 0x0001010101010100ULL;
	constexpr uint64_t LINE_1_NO_EDGES = 126;

	std::array<uint64_t, 64> res{};

	for (int sq = 0; sq < 64; ++sq)
	{
		const int x = sq & 7;
		const int y = sq >> 3;

		// discovered check	from the same line:	
		res[sq] |= (LINE_1_NO_EDGES << (y * 8)) | (FILE_A_NO_EDGES << x); 
		// discovered check	from the same diagonal:
		for (int dx = -1 ; dx <= 1 ; dx += 2)
			for (int dy = -1; dy <= 1; dy += 2)
			{
				int cx = x + dx;
				int cy = y + dy;
				while (cx >= 0 && cy >= 0 && cx < 8 && cy < 8)
				{
					res[sq] |= (1ULL << (cx + cy * 8));
					cx += dx;
					cy += dy;
				}
			}

		// direct check:
		for (int knpos = 0; knpos < 64; ++knpos)
		{
			const int kx = knpos & 7;
			const int ky = knpos >> 3;
			const int diffx = CTABS(kx - x);
			const int diffy = CTABS(ky - y);
			if (diffx + diffy <= 6)
				if (((diffx + diffy) & 1) == 0) // even sum diffs
					if (diffx != 2 || diffy != 2)
						res[sq] |= (1ULL << (kx + ky * 8));
		}
	}

	return res;
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

// ------------------------------------------------------------------------------------

// Bitmasks representing the 4 edges of a chessboard
enum class EdgeMasks : uint8_t {
	NONE = 0,
	FILE_A = 1 << 0, // 1
	FILE_H = 1 << 1, // 2
	RANK_1 = 1 << 2, // 4
	RANK_8 = 1 << 3  // 8
};

// Array mapping each square (0-63) to its edge mask
alignas(64) inline constexpr uint8_t Is_Edge[64] = {
	5, 4, 4, 4, 4, 4, 4, 6,  // Rank 1 (A1=FILE_A|RANK_1 = 1|4 = 5)
	1, 0, 0, 0, 0, 0, 0, 2,  // Rank 2
	1, 0, 0, 0, 0, 0, 0, 2,  // Rank 3
	1, 0, 0, 0, 0, 0, 0, 2,  // Rank 4
	1, 0, 0, 0, 0, 0, 0, 2,  // Rank 5
	1, 0, 0, 0, 0, 0, 0, 2,  // Rank 6
	1, 0, 0, 0, 0, 0, 0, 2,  // Rank 7
	9, 8, 8, 8, 8, 8, 8, 10  // Rank 8
};

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

// Fast filtering pieces that can check:
alignas(64) inline constexpr std::array<std::uint64_t, 64> White_Pawn_Check_Area = init_white_pawn_check_area();
alignas(64) inline constexpr std::array<std::uint64_t, 64> Bishops_That_Can_Check = init_bishops_that_can_check();
alignas(64) inline constexpr std::array<std::uint64_t, 64> Knights_That_Can_Check = init_knights_that_can_check();


