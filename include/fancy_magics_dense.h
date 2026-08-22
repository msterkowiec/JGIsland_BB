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
    { 0x80048090400420ULL, 12 }, { 0x340001000402002ULL, 11 }, { 0x80100020008208ULL, 11 }, { 0x80080280100004ULL, 11 },
    { 0x480221800800400ULL, 11 }, { 0x1a001004452e0008ULL, 11 }, { 0x5100020000930004ULL, 11 }, { 0x2884009084204100ULL, 13 },
    { 0x20a0801040008220ULL, 11 }, { 0x4091004000210080ULL, 10 }, { 0x284806001100082ULL, 10 }, { 0x1110400840100064ULL, 11 },
    { 0x80008014000280c8ULL, 11 }, { 0x2008054000e0080ULL, 10 }, { 0x8165005100060094ULL, 10 }, { 0x220004820c4104ULL, 11 },
    { 0x4208000804000ULL, 11 }, { 0x10004020004000ULL, 10 }, { 0x2104084010010042ULL, 11 }, { 0x1008401000110500ULL, 11 },
    { 0x4020404002084448ULL, 11 }, { 0x4000041002004400ULL, 11 }, { 0x1640002581001ULL, 10 }, { 0x9000020004006091ULL, 11 },
    { 0x9210400080066088ULL, 11 }, { 0x10e4500040002002ULL, 10 }, { 0xc00800c0106000ULL, 11 }, { 0x10121000a3000ULL, 11 },
    { 0xc20041408000802ULL, 11 }, { 0x2000600442801ULL, 11 }, { 0x1ccc02040008e110ULL, 10 }, { 0x410805200040489ULL, 11 },
    { 0x480082001400440ULL, 11 }, { 0x1200840401000ULL, 10 }, { 0x8086002801000ULL, 11 }, { 0x3028108040602004ULL, 11 }, 
    { 0x13200044e000220ULL, 11 }, { 0x204002414400241ULL, 11 }, { 0x2002005702008408ULL, 10 }, { 0x8001105482000104ULL, 11 },
    { 0x880002001444004ULL, 11 }, { 0x30042008404000ULL, 10 }, { 0x89080202000ULL, 11 }, { 0x2241000c0402800ULL, 11 },
    { 0x884030500100ULL, 11 }, { 0x29000a04110028ULL, 11 }, { 0x124a100a18040005ULL, 10 }, { 0x8000040282420009ULL, 11 },
    { 0x240020c000800080ULL, 11 }, { 0x1200410082a200ULL, 10 }, { 0x8240802008900080ULL, 10 }, { 0x413a8004050a030ULL, 11 },
    { 0xc008880010242401ULL, 11 }, { 0x204024300801ULL, 10 }, { 0x4001000600040100ULL, 10 }, { 0xc0240a144010200ULL, 11 },
    { 0x41002040120082ULL, 12 }, { 0x2300802040030011ULL, 11 }, { 0x2002004008241082ULL, 11 }, { 0x2005002010000805ULL, 11 },
    { 0x8001000410421801ULL, 11 }, { 0x840200302d880402ULL, 11 }, { 0x800500106008804ULL, 11 }, { 0xa400002488450402ULL, 12 }
};

inline constexpr MagicConfig BishopConfigs[64] = {
    { 0xa08102082700a0ULL, 6 }, { 0x4080800408004ULL, 5 }, { 0x2004480208400208ULL, 5 }, { 0x424104200004040ULL, 5 },
    { 0x204042000610021ULL, 5 }, { 0x1002120a20000040ULL, 5 }, { 0x880410440700ULL, 5 }, { 0x280110410040406ULL, 6 },
    { 0x1001480a90040900ULL, 5 }, { 0x801020084280a086ULL, 5 }, { 0x1040104301410000ULL, 5 }, { 0x8200090413005a00ULL, 5 },
    { 0x81040c2060f010ULL, 5 }, { 0x215880c402400ULL, 5 }, { 0x148004130103000ULL, 5 }, { 0x208026202422002ULL, 5 },
    { 0x22401010100240a0ULL, 5 }, { 0x40028101404d0ULL, 5 }, { 0x408080928010030ULL, 7 }, { 0x40800008203c104ULL, 7 },
    { 0x800824c00a00401ULL, 7 }, { 0x2034100820100ULL, 7 }, { 0x1804202500e02ULL, 5 }, { 0x2101010084238200ULL, 5 },
    { 0xd50500a0085008ULL, 5 }, { 0xa504202002020400ULL, 5 }, { 0x1028000c005400ULL, 7 }, { 0x4212808008020002ULL, 9 },
    { 0x20104000600e102ULL, 9 }, { 0x802000820a400ULL, 7 }, { 0xc8404408404a0ULL, 5 }, { 0x400b090002094900ULL, 5 },
    { 0x208024021900482ULL, 5 }, { 0x4002111080a0021cULL, 5 }, { 0xa8020a0200890800ULL, 7 }, { 0x100b20080080480ULL, 9 },
    { 0x80400081200200a0ULL, 9 }, { 0x408086a200010108ULL, 7 }, { 0x2040920084802ULL, 5 }, { 0x2013104684070400ULL, 5 },
    { 0x11108081402c008ULL, 5 }, { 0x4009a300008a0ULL, 5 }, { 0x20a010409003200ULL, 7 }, { 0x10162018088100ULL, 7 },
    { 0x200414000040ULL, 7 }, { 0x2020022840400a00ULL, 7 }, { 0x4080483011c00ULL, 5 }, { 0x2001440102011040ULL, 5 },
    { 0x82308020a200000ULL, 5 }, { 0x44c10401200000ULL, 5 }, { 0x6890e02114400c0ULL, 5 }, { 0x40000c46080180ULL, 5 },
    { 0x4000440450440210ULL, 5 }, { 0x1000200401020000ULL, 5 }, { 0x4200882029405ULL, 5 }, { 0x8110130104008140ULL, 5 },
    { 0x12500a092084020ULL, 6 }, { 0x258121a8041000ULL, 5 }, { 0x420000104010408ULL, 5 }, { 0xa010080100208806ULL, 5 },
    { 0x840800010020602ULL, 5 }, { 0xe0840d6850010200ULL, 5 }, { 0x2008251410040908ULL, 5 }, { 0x8011004005080ULL, 6 }
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
