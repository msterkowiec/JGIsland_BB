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
// This additional lookup takes about 16kB (__USE_SMALL_BETWEEN_LOOKUP__) or 24kB (__USE_FAST_BETWEEN_LOOKUP__) of memory
// It allows for significant speed-up (-15% or even -20%, with __USE_FAST_BETWEEN_LOOKUP__)
//

#ifdef __USE_FAST_BETWEEN_LOOKUP__ // 24kB vs 16kB

#include <array>

// The 24 KiB Zero-CMOV database structure
struct ZeroCmovBetween
{
private:
    // 64x64 grid of 16-bit IDs (8,192 bytes = 8 KiB)
    std::array<std::array<uint16_t, 64>, 64> index_map{};

    // Contiguous array of unique masks (2017 elements * 8 bytes = 16,136 bytes ~15.75 KiB)
    // Element 0 is reserved for "no mask / empty"      
    // Expanded by 1 to accommodate both special control slots (0 and -1)
    std::array<uint64_t, 2018> unique_masks{};

public:
    ALWAYS_INLINE uint64_t GetBetweenMask(int sq1, int sq2) const
    {
        // No "if (sq1 == sq2)" check required! The index_map handles it automatically.
        // No std::min or std::max required! 

        // Step 1: Direct 2D array read to grab the 16-bit payload ID
        uint16_t mask_id = index_map[sq1][sq2];

        // Step 2: Grab the actual 64-bit bitboard mask
        return unique_masks[mask_id];
    }

    constexpr static int ct_abs(int x) { return x < 0 ? -x : x; }

    static constexpr ZeroCmovBetween create_zero_cmov_table()
    {
        ZeroCmovBetween table{};

        // Slot 0: Fallback for squares that do NOT share a line/diagonal
        table.unique_masks[0] = static_cast<uint64_t>(-1LL);

        // Slot 1: Fallback for squares that DO share a line/diagonal but have 0 squares between
        table.unique_masks[1] = 0ULL;

        // Dynamic unique IDs now start at index 2
        uint16_t next_unique_id = 2;

        for (int sq1 = 0; sq1 < 64; ++sq1) {
            for (int sq2 = sq1 + 1; sq2 < 64; ++sq2) {
                int r1 = sq1 / 8, c1 = sq1 % 8;
                int r2 = sq2 / 8, c2 = sq2 % 8;

                int dr = r2 - r1;
                int dc = c2 - c1;

                // Check alignment (rank, file, or main diagonals)
                bool shares_line = (dr == 0 || dc == 0 || ZeroCmovBetween::ct_abs(dr) == ZeroCmovBetween::ct_abs(dc));

                if (!shares_line) {
                    // Not aligned: Map to Index 0 (Returns -1)
                    table.index_map[sq1][sq2] = 0;
                    table.index_map[sq2][sq1] = 0;
                }
                else {
                    // They share a line. Let's generate the mask of squares strictly between them
                    int step_r = (dr > 0) ? 1 : (dr < 0 ? -1 : 0);
                    int step_c = (dc > 0) ? 1 : (dc < 0 ? -1 : 0);

                    uint64_t mask = 0ULL;
                    int curr_r = r1 + step_r;
                    int curr_c = c1 + step_c;

                    while (curr_r != r2 || curr_c != c2) {
                        mask |= (1ULL << (curr_r * 8 + curr_c));
                        curr_r += step_r;
                        curr_c += step_c;
                    }

                    if (mask == 0ULL) {
                        // Shared line but adjacent (like d2 and c3): Map to Index 1 (Returns 0)
                        table.index_map[sq1][sq2] = 1;
                        table.index_map[sq2][sq1] = 1;
                    }
                    else {
                        // Shared line with a gap: Allocate a unique mask entry
                        table.unique_masks[next_unique_id] = mask;
                        table.index_map[sq1][sq2] = next_unique_id;
                        table.index_map[sq2][sq1] = next_unique_id;
                        next_unique_id++;
                    }
                }
            }
            // A square to itself is technically aligned on its own line/diagonal with 0 gap
            table.index_map[sq1][sq1] = 1;
        }

        return table;
    }

};

// Instantiate globally at compile time. 
// This lands straight into the read-only (.rodata) segment of the binary
inline constexpr ZeroCmovBetween betweenLookup = ZeroCmovBetween::create_zero_cmov_table();


#else // ---------------------------------------------------------------------------------------------------------
// __USE_SMALL_BETWEEN_LOOKUP__

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

#endif // __USE_SMALL_BETWEEN_LOOKUP__

