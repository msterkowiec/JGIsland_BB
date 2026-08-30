//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "common.h"
#include <array>

class DirLookup
{

public:
	enum class Direction : int {
		DIR_NONE = 0, // 0, none, unaligned
		// Numbering clock-wise:
		DIR_NORTH = 1,
		DIR_NORTHEAST = 2,
		DIR_EAST = 3,
		DIR_SOUTHEAST = 4,
		DIR_SOUTH = 5,
		DIR_SOUTHWEST = 6,
		DIR_WEST = 7,
		DIR_NORTHWEST = 8		
	};

	alignas(16) static constexpr std::pair<char, char> DirDiffs[9] = { {0, 0}, { 0,1 }, {1,1}, {1, 0}, {1,-1},{0,-1},{-1,-1},{-1,0}, {-1,1} };
	alignas(16) static constexpr std::array<uint8_t, 19> ReindexArray = { (uint8_t)Direction::DIR_SOUTHWEST, (uint8_t)Direction::DIR_SOUTH, (uint8_t)Direction::DIR_SOUTHEAST, 0, 0, 0, 0, 0, (uint8_t)Direction::DIR_WEST,
																		  0, (uint8_t)Direction::DIR_EAST, 0, 0, 0, 0, 0, (uint8_t)Direction::DIR_NORTHWEST, (uint8_t)Direction::DIR_NORTH, (uint8_t)Direction::DIR_NORTHEAST };

	ALWAYS_INLINE constexpr Direction GetDir(int sqFrom, int sqTo) const
	{
		assert(((unsigned int)sqFrom) < 64);
		assert(((unsigned int)sqTo) < 64);
		assert(sqFrom != sqTo);

		#ifdef __USE_FASTANDLARGEDIRLOOKUP__
		return (Direction)DirectionArray[sqFrom + sqTo * 64];
		#else

		// Convert normal 0..63 squares to 0x88 format
		int sq1_88 = sqFrom + (sqFrom & ~7);
		int sq2_88 = sqTo + (sqTo & ~7);

		return (Direction) DirectionArray[sq2_88 - sq1_88 + 119];
		#endif
	}
	ALWAYS_INLINE constexpr Direction DirFromDxDy(int dx, int dy) const
	{
		assert(dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1 && (dx || dy));

		int dd = dx + dy * 8 + 9;
		return (Direction) ReindexArray[dd];
	}

	constexpr DirLookup()
	{
		init_lookup();
	}

private:

	#ifdef __USE_FASTANDLARGEDIRLOOKUP__
	alignas(64) std::array<uint8_t, 4096> DirectionArray = { (uint8_t)Direction::DIR_NONE };
	#else
	// 0x88 coordinates range from 0 to 119. 
	// The maximum possible difference is 119 - 0 = 119. 
	// The minimum is 0 - 119 = -119.
	// Offset by 119 to handle negative indices safely inside an array of size 240.
	alignas(64) std::array<uint8_t, 240> DirectionArray = { (uint8_t)Direction::DIR_NONE };
	#endif

	constexpr static int ct_sgn(int x)
	{
		return (x > 0) - (x < 0);
	}

	constexpr static Direction calc_dir(const int sq1, const int sq2)
	{
		const int x1 = sq1 & 7;
		const int y1 = sq1 >> 3;
		const int x2 = sq2 & 7;
		const int y2 = sq2 >> 3;

		const int diff = ct_sgn(x2 - x1) + ct_sgn(y2 - y1) * 8;
		switch (diff)
		{
			case 8: return Direction::DIR_NORTH;
			case 9: return Direction::DIR_NORTHEAST;
			case 1: return Direction::DIR_EAST;
			case -7: return Direction::DIR_SOUTHEAST;
			case -8: return Direction::DIR_SOUTH;
			case -9: return Direction::DIR_SOUTHWEST;
			case -1: return Direction::DIR_WEST;
			case 7: return Direction::DIR_NORTHWEST;
		}

		return Direction::DIR_NONE;
	}
	constexpr void init_lookup() 
	{
		for (int sq1 = 0; sq1 < 64; ++sq1) {
			for (int sq2 = 0; sq2 < 64; ++sq2) {
				#ifdef __USE_FASTANDLARGEDIRLOOKUP__
				DirectionArray[sq1 + sq2 * 64] = (uint8_t)calc_dir(sq1, sq2);
				#else
				int sq1_88 = sq1 + (sq1 & ~7);
				int sq2_88 = sq2 + (sq2 & ~7);
				int diff = sq2_88 - sq1_88 + 119;

				// Re-use logic from Option 1 to determine direction
				DirectionArray[diff] = (uint8_t) calc_dir(sq1, sq2);
				#endif
			}
		}
	}

}; // class DirLookup

constexpr inline DirLookup dirLookup;
