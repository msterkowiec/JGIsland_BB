//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "config.h"

#include <cstdint>
#include <iostream>
#include <random>
#include <algorithm>

namespace DenseFancyMagicBitboards
{

inline uint64_t generate_rook_mask(int sq) {
    uint64_t mask = 0ULL;
    int start_r = sq / 8;
    int start_c = sq % 8;

    // Up/Down (Strictly exclude extreme board edges Row 0 and Row 7)
    for (int r = start_r + 1; r <= 6; ++r) mask |= (1ULL << (r * 8 + start_c));
    for (int r = start_r - 1; r >= 1; --r) mask |= (1ULL << (r * 8 + start_c));

    // Left/Right (Strictly exclude extreme board edges Col 0 and Col 7)
    for (int c = start_c + 1; c <= 6; ++c) mask |= (1ULL << (start_r * 8 + c));
    for (int c = start_c - 1; c >= 1; --c) mask |= (1ULL << (start_r * 8 + c));

    return mask;
}

inline uint64_t generate_bishop_mask(int sq) {
    uint64_t mask = 0ULL;
    int start_r = sq / 8;
    int start_c = sq % 8;

    // Diagonals (Strictly exclude all outermost board edges: row 0/7, col 0/7)
    for (int r = start_r + 1, c = start_c + 1; r <= 6 && c <= 6; ++r, ++c) mask |= (1ULL << (r * 8 + c));
    for (int r = start_r + 1, c = start_c - 1; r <= 6 && c >= 1; ++r, --c) mask |= (1ULL << (r * 8 + c));
    for (int r = start_r - 1, c = start_c + 1; r >= 1 && c <= 6; --r, ++c) mask |= (1ULL << (r * 8 + c));
    for (int r = start_r - 1, c = start_c - 1; r >= 1 && c >= 1; --r, --c) mask |= (1ULL << (r * 8 + c));

    return mask;
}

struct MagicConfig {
    uint64_t magic;
    int bits;
};

inline constexpr MagicConfig RookConfigs[64] = {
    { 0x8080002080144002ULL, 12 }, { 0x80400040200e1000ULL, 11 }, { 0x4080082000821000ULL, 11 }, { 0x8100200408100100ULL, 11 },
    { 0x230003000c080050ULL, 11 }, { 0x20001080c100200ULL, 11 }, { 0x4080020022800100ULL, 11 }, { 0x420000a28100c614ULL, 12 },
    { 0x2082800080400020ULL, 11 }, { 0x200400020100440ULL, 10 }, { 0x210801000882000ULL, 10 }, { 0x1003002200900ULL, 10 },
    { 0x8102002200881004ULL, 10 }, { 0xa042001601940810ULL, 10 }, { 0xc009000600210004ULL, 10 }, { 0x1000800050800100ULL, 11 },
    { 0x8040028000842040ULL, 11 }, { 0x4010094040006002ULL, 10 }, { 0x1081010041e00014ULL, 10 }, { 0x8048008100080ULL, 10 },
    { 0x8e020008204410ULL, 10 }, { 0x1020080140200410ULL, 10 }, { 0x20004000801300aULL, 10 }, { 0x220000941045ULL, 11 },
    { 0x8001400480228000ULL, 11 }, { 0x210500040002008ULL, 10 }, { 0x4200441200220280ULL, 10 }, { 0x464080080100080ULL, 10 },
    { 0x8000180080140080ULL, 10 }, { 0x5024000401081020ULL, 10 }, { 0x240101000200ULL, 10 }, { 0x80001aa00004401ULL, 11 },
    { 0xa284002a0800080ULL, 11 }, { 0x1240400680802000ULL, 10 }, { 0x91500882802000ULL, 10 }, { 0x290210009003000ULL, 10 },
    { 0x481000411004800ULL, 10 }, { 0x40360010160008c4ULL, 10 }, { 0x180408011c005042ULL, 10 }, { 0x442208412000053ULL, 11 },
    { 0x40401020828000ULL, 11 }, { 0x401500020004000ULL, 10 }, { 0x80902001010042ULL, 10 }, { 0x8822001020c20008ULL, 10 },
    { 0x8000080100050010ULL, 10 }, { 0x89200110c020008ULL, 10 }, { 0x28020810040001ULL, 10 }, { 0x3000008700420004ULL, 11 },
    { 0x81004a016200ULL, 11 }, { 0x2004b10040008100ULL, 10 }, { 0x300020028080ULL, 10 }, { 0x8000203000890100ULL, 10 },
    { 0x84100800050100ULL, 10 }, { 0x800040002008080ULL, 10 }, { 0x800988201100400ULL, 10 }, { 0x10004403c1008200ULL, 11 },
    { 0x9a80788000a10041ULL, 12 }, { 0x49014000803021ULL, 11 }, { 0x240a00100413049ULL, 11 }, { 0xa02004004a08892ULL, 11 },
    { 0x5523001008000403ULL, 11 }, { 0x148a000410010802ULL, 11 }, { 0x2208502085004ULL, 11 }, { 0x80003404804102ULL, 12 }
};

inline constexpr MagicConfig BishopConfigs[64] = {
    { 0x850200224152021ULL, 6 }, { 0x1020041095810041ULL, 5 }, { 0x1010012600200005ULL, 5 }, { 0x8704041081082200ULL, 5 },
    { 0x20a1000800000ULL, 5 }, { 0x2002011460001400ULL, 5 }, { 0x100c040202108014ULL, 5 }, { 0x26202021232020cULL, 6 },
    { 0x1083801081220ULL, 5 }, { 0x242d08400840052ULL, 5 }, { 0x101482044010ULL, 5 }, { 0x9c4050800000ULL, 5 },
    { 0x20041044021010ULL, 5 }, { 0x8242008220218420ULL, 5 }, { 0x10004880310120cULL, 5 }, { 0x40002120602220aULL, 5 },
    { 0x40083010810112ULL, 5 }, { 0x1008001035050400ULL, 5 }, { 0x50000802801010ULL, 7 }, { 0x808044401401010ULL, 7 },
    { 0x9500800404a010c0ULL, 7 }, { 0x201040601820105ULL, 7 }, { 0x104400488141018ULL, 5 }, { 0x1a02514010400ULL, 5 },
    { 0x2c04008b00411ULL, 5 }, { 0xb880010060807ULL, 5 }, { 0x100410004200a044ULL, 7 }, { 0x8004180000620040ULL, 9 },
    { 0x4a1040082002101ULL, 9 }, { 0x7000410002028240ULL, 7 }, { 0x23004a04040481ULL, 5 }, { 0x1000410600840500ULL, 5 },
    { 0xa0184828202c2000ULL, 5 }, { 0x28040420102100ULL, 5 }, { 0x4800209800300120ULL, 7 }, { 0x24004808000a0a00ULL, 9 },
    { 0x40900300400c0ULL, 9 }, { 0x8044280020020084ULL, 7 }, { 0x801080902288404ULL, 5 }, { 0x12129204c0120100ULL, 5 },
    { 0x108031050000801ULL, 5 }, { 0x30482404245000ULL, 5 }, { 0x2022003048038408ULL, 7 }, { 0x5001420220b800ULL, 7 },
    { 0x808880900401401ULL, 7 }, { 0x202004204000a0ULL, 7 }, { 0x2003100301028210ULL, 5 }, { 0x10940900206040ULL, 5 },
    { 0x30424820081040ULL, 5 }, { 0x84100880412020cULL, 5 }, { 0x1000084404043281ULL, 5 }, { 0x80018060880000ULL, 5 },
    { 0x4004110c10000ULL, 5 }, { 0x30004c1408121000ULL, 5 }, { 0x240100401c19200ULL, 5 }, { 0x4002d00c01084121ULL, 5 },
    { 0x5000104710101008ULL, 6 }, { 0x485122104700400ULL, 5 }, { 0x42c200420808ULL, 5 }, { 0x10200002008c0421ULL, 5 },
    { 0x802002088210b00ULL, 5 }, { 0x1004004082080ULL, 5 }, { 0x80080208082d00ULL, 5 }, { 0x50020200440100ULL, 6 }
};

constexpr inline size_t calculate_total_pool_size() 
{
    size_t total = 0;
    for (int i = 0; i < 64; ++i) 
        total += (1ULL << RookConfigs[i].bits);
    for (int i = 0; i < 64; ++i) 
        total += (1ULL << BishopConfigs[i].bits);
    return total;
}

inline uint64_t MassiveDenseAttackTable[calculate_total_pool_size()] = { 0 };

struct RuntimeMagic {
    uint64_t* ptr; // Points to the dense, tightly packed lookup table; TODO: might be reduced to uint32_t offset
    uint64_t mask; // Relevant blocker mask
    uint64_t magic;  // Magic multiplier
    int shift; // Dynamically scaled shift value (minimized per square)
};

alignas(32) inline RuntimeMagic RookMagics[64];
alignas(32) inline RuntimeMagic BishopMagics[64];


inline uint64_t calculate_slider_attack_on_the_fly(int sq, uint64_t occupancy, bool is_rook) {
    uint64_t attacks = 0ULL;
    int start_r = sq / 8;
    int start_c = sq % 8;

    if (is_rook) {
        int dr[] = { 1, -1, 0, 0 };
        int dc[] = { 0, 0, 1, -1 };

        for (int i = 0; i < 4; ++i) {
            int r = start_r + dr[i];
            int c = start_c + dc[i];

            while (r >= 0 && r < 8 && c >= 0 && c < 8) {
                uint64_t target_bit = 1ULL << (r * 8 + c);
                attacks |= target_bit;

                // Truncate ONLY if there is a real blocking piece on the board
                if (occupancy & target_bit) {
                    break;
                }

                r += dr[i];
                c += dc[i];
            }
        }
    }
    else {
        int dr[] = { 1, 1, -1, -1 };
        int dc[] = { 1, -1, 1, -1 };

        for (int i = 0; i < 4; ++i) {
            int r = start_r + dr[i];
            int c = start_c + dc[i];

            while (r >= 0 && r < 8 && c >= 0 && c < 8) {
                uint64_t target_bit = 1ULL << (r * 8 + c);
                attacks |= target_bit;

                if (occupancy & target_bit) {
                    break;
                }

                r += dr[i];
                c += dc[i];
            }
        }
    }
    return attacks;
}


// Returns all blocker permutations matching a given bitmask (fills in buf and returns count)
inline size_t generate_occupancy_permutations(uint64_t mask, uint64_t* buf)
{
    size_t count = 0;
    size_t num_bits = 0;
    int bit_indices[64];

    // Extract individual bit positions from the mask
    for (int i = 0; i < 64; ++i) 
        if ((mask >> i) & 1) 
            bit_indices[num_bits++] = i;
    
    const int num_permutations = 1 << num_bits; // 2^n possibilities

    for (int i = 0; i < num_permutations; ++i) {
        uint64_t occ = 0;
        for (int j = 0; j < num_bits; ++j) {
            if ((i >> j) & 1) {
                occ |= (1ULL << bit_indices[j]);
            }
        }
        buf[count++] = occ;
    }
    return count;
}


inline bool initialize_sliding_attacks() 
{
    size_t current_offset = 0;

    [[maybe_unused]] const size_t pool_size = calculate_total_pool_size();
    uint64_t occupancies[4096]; // 32kB on stack

    // 1. Initialize Rooks
    for (int sq = 0; sq < 64; ++sq) {
        RookMagics[sq].mask = generate_rook_mask(sq);
        RookMagics[sq].magic = RookConfigs[sq].magic;
        RookMagics[sq].shift = 64 - RookConfigs[sq].bits; // Uses the exact bit pair!
        RookMagics[sq].ptr = &MassiveDenseAttackTable[current_offset];

        current_offset += (1ULL << RookConfigs[sq].bits); // Steps offset safely

        const auto count_occupancies = generate_occupancy_permutations(RookMagics[sq].mask, occupancies);
        
        for (size_t i = 0 ; i < count_occupancies ; ++ i)
        {            
            const auto occ = occupancies[i];
            uint64_t attack = calculate_slider_attack_on_the_fly(sq, occ, true);
            size_t idx = ((occ & RookMagics[sq].mask) * RookMagics[sq].magic) >> RookMagics[sq].shift;
            RookMagics[sq].ptr[idx] = attack;
        }
    }

    // 2. Initialize Bishops
    for (int sq = 0; sq < 64; ++sq) {
        BishopMagics[sq].mask = generate_bishop_mask(sq);
        BishopMagics[sq].magic = BishopConfigs[sq].magic;
        BishopMagics[sq].shift = 64 - BishopConfigs[sq].bits; // Uses the exact bit pair!
        BishopMagics[sq].ptr = &MassiveDenseAttackTable[current_offset];

        current_offset += (1ULL << BishopConfigs[sq].bits); // Steps offset safely

        const auto count_occupancies = generate_occupancy_permutations(BishopMagics[sq].mask, occupancies);

        for (size_t i = 0; i < count_occupancies; ++i)
        {
            const auto occ = occupancies[i];
            uint64_t attack = calculate_slider_attack_on_the_fly(sq, occ, false);
            size_t idx = ((occ & BishopMagics[sq].mask) * BishopMagics[sq].magic) >> BishopMagics[sq].shift;
            BishopMagics[sq].ptr[idx] = attack;
        }
    }

    assert(current_offset == pool_size);

    return true;
}

const bool init_done = DenseFancyMagicBitboards::initialize_sliding_attacks();

} // namespace DenseFancyMagicBitboards


ALWAYS_INLINE uint64_t get_raw_rook_moves_dfmb(int sq, uint64_t occupancy)
{
    const auto& magicRecord = DenseFancyMagicBitboards::RookMagics[sq];
    uint64_t blockers = occupancy & magicRecord.mask;
    uint64_t idx = (blockers * magicRecord.magic) >> magicRecord.shift;
    return magicRecord.ptr[idx];
}

ALWAYS_INLINE uint64_t get_raw_bishop_moves_dfmb(int sq, uint64_t occupancy)
{
    const auto& magicRecord = DenseFancyMagicBitboards::BishopMagics[sq];
    uint64_t blockers = occupancy & magicRecord.mask;
    uint64_t idx = (blockers * magicRecord.magic) >> magicRecord.shift;
    return magicRecord.ptr[idx];
}
