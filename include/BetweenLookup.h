//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <cstdint>
#include <algorithm>
#include <array>
#include "common.h"
#include "config.h"

//
// This additional lookup takes about 11.5kB or 15.7 kB dependent on temp.param. tbReduceMemUsage
// It allows for significant speed-up (even -20%)
// 
// Its main feature is to provide Between Mask (the bitboard of squares between two given squares)
// Its additional feature is to provide mask of common line or diagonal (the bitboard of the whole diagonal or line on which given two squares lie)
//
// Added template param bool tbReduceMemUsage - then 8kB buf is common for both features only if tbReduceMemUsage==true, but it incurs a very small but measurable impact on performance (mainly on older machines) due to a small additional operation on hot path.
//

//
// 11.5 kB with tbReduceMemUsage == true
// 15.7 kB with tbReduceMemUsage == false
//

template<bool tbReduceMemUsage = false> // reducing mem.usage (using common index_map) seems to incur a very small but measurable impact on performance
struct BetweenLookupExt
{
private:
    
    // 64x64 grid of 16-bit IDs (8,192 bytes = 8 KiB)
    alignas(64) std::array<std::array<uint16_t, 64>, 64> index_map{};

    struct Empty {};
    using AdditionalArrayType = std::conditional_t < tbReduceMemUsage, Empty, std::array<std::array<uint8_t, 64>, 64> >;
    alignas(64) AdditionalArrayType index_map_2{}; // for bitmasks of diagonals or lines 

    // Contiguous array of unique masks
    // Element 0 is reserved for "no mask / empty"      
    // Expanded by 1 to accommodate both special control slots (0 and -1)
    static constexpr size_t maxUniqueMasks = 412;
    alignas(64) std::array<uint64_t, maxUniqueMasks> unique_masks{}; // ~3.5kB

    // Masks of common line or diagonal
    static constexpr size_t maxUniqueDiagOrLines = 43; 
    alignas(64) std::array<uint64_t, maxUniqueDiagOrLines> unique_lines{};
    alignas(64) std::array<uint8_t, maxUniqueDiagOrLines> is_it_line{}; // yet another auxiliary lookup

public:
    ALWAYS_INLINE uint64_t GetBetweenMask(int sq1, int sq2) const
    {
        assert(((unsigned int) sq1) < 64);
        assert(((unsigned int) sq2) < 64);
        assert(sq1 != sq2);
        
        uint16_t mask_id = index_map[sq1][sq2];

        if constexpr(tbReduceMemUsage)
            return unique_masks[mask_id & 511];
        else
            return unique_masks[mask_id];
    }
    ALWAYS_INLINE constexpr uint64_t GetCommonDiagOrLine(int sq1, int sq2) const
    {
        assert(((unsigned int) sq1) < 64);
        assert(((unsigned int) sq2) < 64);
        assert(sq1 != sq2);
        
        if constexpr (tbReduceMemUsage)
        {
            uint16_t mask_id = index_map[sq1][sq2];            
            return unique_lines[mask_id >> 9];
        }
        else
        {
            auto mask_id = index_map_2[sq1][sq2];
            return unique_lines[mask_id];
        }
    }
    ALWAYS_INLINE constexpr bool IsSquareOnCommonDiagOrLineOf(int sq, int sq1, int sq2) const
    {
        assert(((unsigned int) sq) < 64);
        assert(((unsigned int) sq1) < 64);
        assert(((unsigned int) sq2) < 64);        
        assert(sq1 != sq2);

        return (1ULL << sq) & GetCommonDiagOrLine(sq1, sq2);
    }
    ALWAYS_INLINE constexpr uint64_t MatchOnCommonDiagOrLine(int sq1, int sq2, uint64_t lineMask, uint64_t diagMask) const
    {
        assert(((unsigned int) sq1) < 64);
        assert(((unsigned int) sq2) < 64);        
        assert(sq1 != sq2);
        
        if constexpr (tbReduceMemUsage)
        {
            uint16_t mask_id = index_map[sq1][sq2];
            auto line = is_it_line[mask_id];
            auto match = line ? lineMask : diagMask;
            return unique_lines[mask_id >> 9] & match;
        }
        else
        {
            auto mask_id = index_map_2[sq1][sq2];
            auto line = is_it_line[mask_id];
            auto match = line ? lineMask : diagMask;
            return unique_lines[mask_id] & match;
        }
    }
    
    ALWAYS_INLINE constexpr uint64_t MatchOnCommonDiagOrLineIfAllBetweenEmpty(int sq1, int sq2, uint64_t lineMask, uint64_t diagMask, uint64_t occ) const
    {
        assert(((unsigned int) sq1) < 64);
        assert(((unsigned int) sq2) < 64);        
        assert(sq1 != sq2);
        
        if constexpr (tbReduceMemUsage)
        {
            uint16_t mask_id = index_map[sq1][sq2];
            auto line = is_it_line[mask_id];
            auto match = line ? lineMask : diagMask;    
            bool bAllBetweenEmpty = (unique_masks[mask_id & 511] & occ) == 0;
            return unique_lines[mask_id >> 9] & match & (0ULL - static_cast<uint64_t>(bAllBetweenEmpty));
        }
        else
        {
            auto mask_id = index_map_2[sq1][sq2];
            auto line = is_it_line[mask_id];
            auto match = line ? lineMask : diagMask;
            bool bAllBetweenEmpty = (unique_masks[index_map[sq1][sq2]] & occ) == 0;
            return unique_lines[mask_id] & match & (0ULL - static_cast<uint64_t>(bAllBetweenEmpty));
        }
    }    

    // Implementation:
private:
    constexpr static int ct_abs(int x) { return x < 0 ? -x : x; }

    // Initializes for additional feature:
    template<typename T>
    constexpr void InitCommonDiagOrLineLookup(std::array<std::array<T, 64>, 64>& idxMap)
    {
        constexpr auto tbShift = tbReduceMemUsage ? 9 : 0;

        int next_line_id = 0;
        unique_lines[next_line_id++] = 0; // zero mask for unaligned squares (it is different than in the main between lookup that returns full mask (-1) for unaligned squares)        

        for (int sq1 = 0; sq1 < 64; ++sq1)
        {
            int r1 = sq1 / 8;
            int f1 = sq1 % 8;

            for (int sq2 = sq1 + 1; sq2 < 64; ++sq2)
            {

                int r2 = sq2 / 8;
                int f2 = sq2 % 8;
                int dr = r2 - r1;
                int df = f2 - f1;

                if (dr == 0 || df == 0 || ct_abs(dr) == ct_abs(df))
                {
                    // Raycast line generation
                    int step_r = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
                    int step_c = (df == 0) ? 0 : (df > 0 ? 1 : -1);

                    uint64_t mask = 0ULL;

                    // Raycast forward
                    int r = r1, c = f1;
                    while (r >= 0 && r < 8 && c >= 0 && c < 8) {
                        mask |= (1ULL << (r * 8 + c));
                        r += step_r; c += step_c;
                    }

                    // Raycast backward
                    r = r1 - step_r; c = f1 - step_c;
                    while (r >= 0 && r < 8 && c >= 0 && c < 8)
                    {
                        mask |= (1ULL << (r * 8 + c));
                        r -= step_r; c -= step_c;
                    }

                    // Explicitly scan already discovered unique lines for a duplicate mask
                    int existing_id = -1;
                    for (int i = 0; i < next_line_id; ++i) {
                        if (unique_lines[i] == mask) {
                            existing_id = i;
                            break;
                        }
                    }

                    if (existing_id != -1) {
                        // Reuse the existing unique line ID
                        idxMap[sq1][sq2] |= existing_id << tbShift;
                        idxMap[sq2][sq1] |= existing_id << tbShift;
                    }
                    else {
                        // Brand new line mask found, store it safely
                        assert(next_line_id < maxUniqueDiagOrLines);
                        unique_lines[next_line_id] = mask;
                        is_it_line[next_line_id] = (dr == 0 || df == 0);
                        idxMap[sq1][sq2] |= next_line_id << tbShift;
                        idxMap[sq2][sq1] |= next_line_id << tbShift;
                        next_line_id++;
                    }
                }
            }
        }
        return;
    }

public:
    static constexpr BetweenLookupExt CreateBetweenLookupExt()
    {
        BetweenLookupExt table{};

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
                bool shares_line = (dr == 0 || dc == 0 || ct_abs(dr) == ct_abs(dc));

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

        // Init also additional structures for additional feature
        if constexpr(tbReduceMemUsage)
            table.InitCommonDiagOrLineLookup(table.index_map); 
        else
            table.InitCommonDiagOrLineLookup(table.index_map_2); 

        return table;
    }
};

// Instantiate globally at compile time. This lands straight into the read-only (.rodata) segment of the binary
inline constexpr BetweenLookupExt<tbMemUsageOptimInBetweenLookup> betweenLookup = BetweenLookupExt<tbMemUsageOptimInBetweenLookup>::CreateBetweenLookupExt();

