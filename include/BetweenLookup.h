//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <cstdint>
#include <algorithm>
#include "common.h"

//
// This additional lookup takes about 16kB of memory
// It is used if macro __USE_BETWEENLOOKUP__ is on
// It proved to provide slightly better performance (15%) than pure Hyperbola Quintessence in isolated test, the full trade-off unnkown yet - qutie much data in L1 cache
//

class BetweenLookup 
{
public:
    // ANY two squares are accepted: for squares that are not on the same diagonal or line, -1 (all bits on) is returned
    ALWAYS_INLINE uint64_t GetBetweenMask(int s1, int s2) const
    {
        // Compiles directly to branchless CMOV hardware instructions
        int low = (std::min)(s1, s2);
        int high = (std::max)(s1, s2);

        // When low == high, this naturally evaluates to: row_offsets[low] - 1
        // If low == 0, it hits masks[0], which is initialized to 0ULL.
        int idx = row_offsets[low] + (high - low - 1);
        return masks[idx];
    }

    BetweenLookup() 
    {
        init();
    }

private:

    alignas(64) uint64_t masks[2017];

    // Shifted by +1 to safely absorb the (-1) index when low == high
    alignas(64) static constexpr uint16_t row_offsets[65] = {
        1, 64, 126, 187, 247, 306, 364, 421, 477, 532, 586, 639, 691, 742, 792, 841,
        889, 936, 982, 1027, 1071, 1114, 1156, 1197, 1237, 1276, 1314, 1351, 1387, 1422, 1456, 1489,
        1521, 1552, 1582, 1611, 1639, 1666, 1692, 1717, 1741, 1764, 1786, 1807, 1827, 1846, 1864, 1881,
        1897, 1912, 1926, 1939, 1951, 1962, 1972, 1981, 1989, 1996, 2002, 2007, 2011, 2014, 2016, 2017,
        2017 // Extra boundary guard
    };

    void init() {
        // 1. Explicitly fill the entire array with -1 (all bits on)
        // This handles the dummy index 0 and all non-aligned pairs automatically.
        for (int i = 0; i < 2017; ++i) {
            masks[i] = ~0ULL; // Equivalent to -1 or 0xFFFFFFFFFFFFFFFFULL
        }

        // 2. Loop through every unique pair of squares
        for (int s1 = 0; s1 < 64; ++s1) {
            for (int s2 = 0; s2 < 64; ++s2) {
                if (s1 == s2) continue;

                int f1 = s1 % 8, r1 = s1 / 8;
                int f2 = s2 % 8, r2 = s2 / 8;

                int df = f2 - f1;
                int dr = r2 - r1;

                // Check if they are on the same file, rank, or diagonal
                bool same_file = (df == 0);
                bool same_rank = (dr == 0);
                bool same_diag = (std::abs(df) == std::abs(dr));

                if (same_file || same_rank || same_diag) {
                    uint64_t path_mask = 0ULL;

                    // Normalize direction steps (-1, 0, or 1)
                    int step_f = (df == 0) ? 0 : (df > 0 ? 1 : -1);
                    int step_r = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);

                    // Step through squares strictly BETWEEN s1 and s2
                    int curr_f = f1 + step_f;
                    int curr_r = r1 + step_r;

                    while (curr_f != f2 || curr_r != r2) {
                        int square = curr_r * 8 + curr_f;
                        path_mask |= (1ULL << square);

                        curr_f += step_f;
                        curr_r += step_r;
                    }

                    // Map this calculated mask to our 1D upper-triangular array
                    int low = std::min(s1, s2);
                    int high = std::max(s1, s2);
                    int idx = row_offsets[low] + (high - low - 1);

                    // Overwrite the default -1 with the valid, clear path mask
                    masks[idx] = path_mask;
                }
            }
        }
    }
   
};

inline const BetweenLookup betweenLookup;
