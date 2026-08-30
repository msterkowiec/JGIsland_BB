//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "DirLookup.h"
#include "common.h"

class RayLookup
{	
public:
	constexpr RayLookup()
	{
		FillLookup();
	}
	constexpr ALWAYS_INLINE uint64_t GetRay(const int pos, const int posBase) const
	{
		const auto dir = dirLookup.GetDir(posBase, pos);
		const auto idx = idxLookup[(uint32_t)dir][pos];
		return maskLookup[idx];
	}
	constexpr ALWAYS_INLINE uint64_t GetRayInDir(const int pos, const int dx, const int dy) const
	{
		const auto dir = dirLookup.DirFromDxDy(dx, dy);
		const auto idx = idxLookup[(uint32_t)dir][pos];
		return maskLookup[idx];
	}
	constexpr ALWAYS_INLINE uint64_t MatchOnRay(const int pos, const int posBase, const uint64_t rookLikes, const uint64_t bishopLikes) const
	{
		const auto dir = dirLookup.GetDir(posBase, pos);
		const auto idx = idxLookup[(uint32_t)dir][pos];
		const bool line = ((uint32_t) dir) & 1;
		const auto matchMask = line ? rookLikes : bishopLikes;
		const auto mask = maskLookup[idx];
		return mask & matchMask;
	}
	constexpr ALWAYS_INLINE std::pair<uint64_t, uint64_t> GetRayAndMatchOnRay(const int pos, const int posBase, const uint64_t rookLikes, const uint64_t bishopLikes) const
	{
		std::pair<uint64_t, uint64_t> res;
		const auto dir = dirLookup.GetDir(posBase, pos);
		const auto idx = idxLookup[(uint32_t)dir][pos];
		const bool line = ((uint32_t)dir) & 1;
		const auto matchMask = line ? rookLikes : bishopLikes;
		res.first = maskLookup[idx];
		res.second = res.first & matchMask;
		return res;
	}

private:

	alignas(64) std::array<std::array<uint16_t, 64>, 9> idxLookup{}; // first indexing by DirLookup::Direction [0...7,8] (8==DIR_NONE is for unaligned), then by by square [0...63] - this way we avoid multiplication by 9
	alignas(64) std::array<uint64_t, 369> maskLookup{};

	static constexpr int ct_abs(int x)
	{
		return (x < 0) ? -x : x;
	}
	static constexpr int ct_sgn(int x)
	{
		return (x > 0) - (x < 0);
	}
	constexpr void FillLookup()
	{
		size_t numMasks = 0;
		maskLookup[numMasks++] = 0;
		
		for (int sq = 0; sq < 64; ++sq)
			for (int dir = 0 ; dir <= 8 ; ++ dir)
				{
					if (dir == (int) DirLookup::Direction::DIR_NONE)
					{
						idxLookup[dir][sq] = 0; // unaligned (not that the same mask is also for aligned, but on edge - and dir not along the edge)
						continue;
					}
					const int x = sq & 7;
					const int y = sq >> 3;
					const auto [dx, dy] = DirLookup::DirDiffs[dir];
					
					uint64_t mask = 0;
					int cx = x + dx;
					int cy = y + dy;					

					while (cx >= 0 && cy >= 0 && cx < 8 && cy < 8)
					{
						mask |= (1ULL << (cx + cy * 8));
						cx += dx;
						cy += dy;
					}

					bool maskAlreadyExists = false;
					for (int i = 0; i < numMasks; ++i)
						if (mask == maskLookup[i])
						{
							idxLookup[dir][sq] = i;							
							maskAlreadyExists = true;
						}

					if (!maskAlreadyExists)
					{
						idxLookup[dir][sq] = static_cast<uint16_t>(numMasks);
						maskLookup[numMasks++] = mask;						
					}
					
				}

		return;
	}

};

inline constexpr RayLookup rayLookup;

