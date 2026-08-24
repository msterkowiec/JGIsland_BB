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
// This additional lookup takes about 11.5kB
// It allows for significant speed-up (even -20%)
//

#include <array>

// The 11.5 KiB Zero-CMOV database structure
struct ZeroCmovBetween
{
private:
    // 64x64 grid of 16-bit IDs (8,192 bytes = 8 KiB)
    alignas(64) std::array<std::array<uint16_t, 64>, 64> index_map{};

    // Contiguous array of unique masks
    // Element 0 is reserved for "no mask / empty"      
    // Expanded by 1 to accommodate both special control slots (0 and -1)
    static constexpr size_t maxUniqueMasks = 412;
    alignas(64) std::array<uint64_t, maxUniqueMasks> unique_masks{}; // ~3.5kB

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
                        int found_id = -1;
                        for (int id = 0; id < next_unique_id; ++id)
                            if (table.unique_masks[id] == mask)
                            {
                                table.index_map[sq1][sq2] = id;
                                table.index_map[sq2][sq1] = id;
                                found_id = id;
                                break;
                            }

                        // Shared line with a gap: Allocate a unique mask entry
                        if (found_id == -1)
                        {
                            table.unique_masks[next_unique_id] = mask;
                            table.index_map[sq1][sq2] = next_unique_id;
                            table.index_map[sq2][sq1] = next_unique_id;
                            next_unique_id++;
                            assert(next_unique_id <= maxUniqueMasks);
                        }
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

