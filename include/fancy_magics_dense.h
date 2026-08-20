//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "config.h"

#ifdef __USE_DENSE_FANCY_MAGIC_BITBOARDS_INSTEAD_OF_HQ__

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
    { 0x5080012040021180ULL, 12 }, { 0x840001008426040ULL, 12 }, { 0x410102000045080ULL, 12 }, { 0x280080030002284ULL, 12 },
    { 0x100180100020450ULL, 12 }, { 0x4080080200040081ULL, 12 }, { 0x800080044a0003ULL, 12 }, { 0x8e00001600210084ULL, 13 },
    { 0x40a0800040008020ULL, 11 }, { 0x8201004000210480ULL, 10 }, { 0x406002a00108022ULL, 11 }, { 0x8008008800c0050ULL, 11 },
    { 0x2802000a00042002ULL, 11 }, { 0x4c20011a8020014ULL, 10 }, { 0x1414009208411004ULL, 10 }, { 0x108a800480205500ULL, 11 },
    { 0x4022000840108081ULL, 12 }, { 0x1010004006200048ULL, 10 }, { 0x800820022064010ULL, 10 }, { 0xe020040092010ULL, 10 },
    { 0x80000a0044020014ULL, 11 }, { 0x4820a0001902408ULL, 11 }, { 0x140804000a080110ULL, 10 }, { 0x1000020000810144ULL, 11 },
    { 0x1846280004000ULL, 11 }, { 0x11088200422200ULL, 10 }, { 0x444041a0200800ULL, 11 }, { 0x610080040140008ULL, 11 }, 
    { 0x2100c0080080082ULL, 10 }, { 0xa28a004200081004ULL, 10 }, { 0x441048100440202ULL, 11 }, { 0x320800880004100ULL, 11 },
    { 0x400880800020ULL, 11 }, { 0x1420020245680104ULL, 11 }, { 0x24032800808010e0ULL, 11 }, { 0x8e140020200a00ULL, 11 },
    { 0x8410081080800402ULL, 11 }, { 0x8440010010400882ULL, 11 }, { 0x800082081400301bULL, 10 }, { 0x2c0008200403500ULL, 12 },
    { 0x802400284a08008ULL, 11 }, { 0x4084401020000800ULL, 11 }, { 0x40200812002000ULL, 11 }, { 0x42300042440ac004ULL, 11 },
    { 0x20a4000800828004ULL, 10 }, { 0x800080b10020004ULL, 11 }, { 0x20130080b8201100ULL, 11 }, { 0x8c0301a24c020009ULL, 11 },
    { 0x1280205080010100ULL, 11 }, { 0x4004408022050200ULL, 10 }, { 0x2043100020008080ULL, 10 }, { 0x50001209a0420200ULL, 10 },
    { 0x2b0042a01480200ULL, 11 }, { 0x10247040a0040801ULL, 10 }, { 0x5100081002010400ULL, 10 }, { 0x2006800045000280ULL, 11 },
    { 0x820c1108001ULL, 13 }, { 0x4c830040013180abULL, 11 }, { 0x4000c1200100900bULL, 11 }, { 0x4286002040191006ULL, 11 },
    { 0x8100304200882ULL, 12 }, { 0x4080a40100881006ULL, 12 }, { 0x8012620281081004ULL, 11 }, { 0xa08040011704022ULL, 13 }
};

inline constexpr MagicConfig BishopConfigs[64] = {
    { 0x4081000408900ULL, 6 }, { 0xe08890800810002ULL, 5 }, { 0x122080100204898ULL, 5 }, { 0x640810004a100ULL, 5 },
    { 0x14246000040400ULL, 5 }, { 0x4182080404420820ULL, 5 }, { 0x301821820042060ULL, 5 }, { 0x80a10b104010ULL, 6 },
    { 0x500051004290400ULL, 5 }, { 0x40100103240380ULL, 5 }, { 0x222500092094110ULL, 5 }, { 0x202080941089050ULL, 5 },
    { 0x2000240ca0008220ULL, 5 }, { 0x10220804142200ULL, 5 }, { 0x820040908080401ULL, 5 }, { 0x310620101011020ULL, 5 },
    { 0x830308a20032400ULL, 5 }, { 0xcc205204142408ULL, 5 }, { 0x50000210220020ULL, 7 }, { 0xc00080c101031ULL, 7 },
    { 0x40002020a0102ULL, 7 }, { 0xb00020080a444ULL, 7 }, { 0x3102000088048281ULL, 5 }, { 0x20068200440a8840ULL, 5 },
    { 0x84e0420208200ULL, 5 }, { 0x2228a2030040802ULL, 5 }, { 0x2044828080022ULL, 7 }, { 0x2000c050a0606020ULL, 10 },
    { 0x6040802012020042ULL, 9 }, { 0x1470022010140ULL, 7 }, { 0x2005002111001ULL, 5 }, { 0x688002008c02ULL, 5 },
    { 0x1050280402083080ULL, 5 }, { 0x8008294848042800ULL, 5 }, { 0x800140408860803ULL, 7 }, { 0x2000c0100082002ULL, 10 },
    { 0x210400021002ULL, 10 }, { 0x201030200010040ULL, 7 }, { 0x40a2040408b84a10ULL, 5 }, { 0xa004040024008898ULL, 5 },
    { 0x14101290011800ULL, 5 }, { 0x90040088044148c3ULL, 5 }, { 0x408208020801000ULL, 7 }, { 0xc080005148002400ULL, 7 },
    { 0x102001a0800400ULL, 7 }, { 0xc404108482041501ULL, 7 }, { 0x10040861c00280ULL, 5 }, { 0x2101080080800110ULL, 5 },
    { 0x100a20812400060ULL, 5 }, { 0x8032a10808041800ULL, 5 }, { 0x400490401040000ULL, 5 }, { 0x8000600084041000ULL, 5 },
    { 0x3801001002022000ULL, 5 }, { 0x2c08841010000ULL, 5 }, { 0x620040420840100ULL, 5 }, { 0x822680a002002ULL, 5 },
    { 0x211040202430c00ULL, 6 }, { 0x8000068044109408ULL, 5 },{ 0x1001020841014ULL, 5 }, { 0x200021101840400ULL, 5 },
    { 0x6004000110060a02ULL, 5 }, { 0x50082040920c1100ULL, 5 }, { 0x1000420202020200ULL, 5 }, { 0x840240c00c04900ULL, 6 }
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
    uint64_t* ptr; // Points to the dense, tightly packed lookup table
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

    const size_t pool_size = calculate_total_pool_size();
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
    uint64_t blockers = occupancy & DenseFancyMagicBitboards::RookMagics[sq].mask;
    uint64_t idx = (blockers * DenseFancyMagicBitboards::RookMagics[sq].magic) >> DenseFancyMagicBitboards::RookMagics[sq].shift;
    return DenseFancyMagicBitboards::RookMagics[sq].ptr[idx];
}

ALWAYS_INLINE uint64_t get_raw_bishop_moves_dfmb(int sq, uint64_t occupancy)
{
    uint64_t blockers = occupancy & DenseFancyMagicBitboards::BishopMagics[sq].mask;
    uint64_t idx = (blockers * DenseFancyMagicBitboards::BishopMagics[sq].magic) >> DenseFancyMagicBitboards::BishopMagics[sq].shift;
    return DenseFancyMagicBitboards::BishopMagics[sq].ptr[idx];
}

#endif // __USE_DENSE_FANCY_MAGIC_BITBOARDS_INSTEAD_OF_HQ__
