//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include "config.h"

#ifdef __USE_FANCY_MAGIC_BITBOARDS_INSTEAD_OF_HQ__ 

#include <cstdint>
#include <array>
#include <random>
#include <bit>

namespace FancyMagicBitboards
{

    constexpr inline std::array<uint64_t, 64> BishopMagicsSeeds = {
        0x0002020080080080ULL, 0x0040020100400040ULL, 0x0001011001002400ULL, 0x0000820040011000ULL,
        0x0004000800800100ULL, 0x0000100402004200ULL, 0x0001000210080200ULL, 0x0080081040040001ULL,
        0x0080400010002000ULL, 0x0001002000401040ULL, 0x004100104000a002ULL, 0x0004400100802008ULL,
        0x0001004002002004ULL, 0x0040200800400100ULL, 0x0001010100420100ULL, 0x0000a00410008010ULL,
        0x0001000408020020ULL, 0x0004040011020200ULL, 0x0020084100400040ULL, 0x0040200040010010ULL,
        0x0010002008004080ULL, 0x0001040081004002ULL, 0x0001010041002004ULL, 0x0080204000800400ULL,
        0x0020410040240080ULL, 0x0002008104000800ULL, 0x0020004810200800ULL, 0x0080020020400080ULL,
        0x0084000200040100ULL, 0x0020010802001000ULL, 0x0020400080800200ULL, 0x0000100204004020ULL,
        0x0008021002040001ULL, 0x0081002200080040ULL, 0x0002040200011000ULL, 0x0010008024000802ULL,
        0x0004200401040020ULL, 0x0002004001080400ULL, 0x0001040200400081ULL, 0x0000820040011000ULL,
        0x0001000408020020ULL, 0x0040020100400040ULL, 0x0020410040240080ULL, 0x0001040081004002ULL,
        0x0080400010002000ULL, 0x0001002000401040ULL, 0x0020004810200800ULL, 0x0080020020400080ULL,
        0x0002020080080080ULL, 0x0000100402004200ULL, 0x0001011001002400ULL, 0x0004400100802008ULL,
        0x0004000800800100ULL, 0x0040200800400100ULL, 0x0001000210080200ULL, 0x0080081040040001ULL,
        0x0008021002040001ULL, 0x0081002200080040ULL, 0x0002040200011000ULL, 0x0010008024000802ULL,
        0x0004200401040020ULL, 0x0002004001080400ULL, 0x0001040200400081ULL, 0x0000820040011000ULL
    };

    constexpr inline std::array<uint64_t, 64> FixedRookMagics = {
        0x0080001000200040ULL, 0x0080010000400020ULL, 0x0080008000400100ULL, 0x0080008000200200ULL,
        0x0080400000100200ULL, 0x0001000024000800ULL, 0x0001000048001000ULL, 0x0001000090002000ULL,
        0x0010008000200040ULL, 0x0020010000400020ULL, 0x0020008000400100ULL, 0x0020008000200200ULL,
        0x0020400000100200ULL, 0x0001000024000800ULL, 0x0001000048001000ULL, 0x0001000090002000ULL,
        0x0010008000400080ULL, 0x0020010000800040ULL, 0x0020008000800200ULL, 0x0020008000400400ULL,
        0x0020400000200400ULL, 0x0001000048001000ULL, 0x0001000090002000ULL, 0x0001000120004000ULL,
        0x0010008000400080ULL, 0x0020010000800040ULL, 0x0020008000800200ULL, 0x0020008000400400ULL,
        0x0020400000200400ULL, 0x0001000048001000ULL, 0x0001000090002000ULL, 0x0001000120004000ULL,
        0x0010008000400080ULL, 0x0020010000800040ULL, 0x0020008000800200ULL, 0x0020008000400400ULL,
        0x0020400000200400ULL, 0x0001000048001000ULL, 0x0001000090002000ULL, 0x0001000120004000ULL,
        0x0010008000400080ULL, 0x0020010000800040ULL, 0x0020008000800200ULL, 0x0020008000400400ULL,
        0x0020400000200400ULL, 0x0001000048001000ULL, 0x0001000090002000ULL, 0x0001000120004000ULL,
        0x0080001000200040ULL, 0x0080010000400020ULL, 0x0080008000400100ULL, 0x0080008000200200ULL,
        0x0080400000100200ULL, 0x0001000024000800ULL, 0x0001000048001000ULL, 0x0001000090002000ULL,
        0x0000400020001001ULL, 0x0000400040002001ULL, 0x0000800080004001ULL, 0x0000800100008001ULL,
        0x0000a000a0004001ULL, 0x0000200048001001ULL, 0x0000200090002001ULL, 0x0000200120004001ULL
    };

    struct MagicRecord {
        uint64_t mask;
        uint64_t magic;
        uint32_t offset;
        uint8_t  shift;
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Bishop
    // ---------------------------------------------------------------------------------------------------------------

    constexpr inline uint64_t generate_bishop_mask_fmb(int sq)
    {
        uint64_t mask = 0ULL;
        int r = sq / 8, c = sq % 8;
        for (int dr : {-1, 1}) {
            for (int dc : {-1, 1}) {
                int cur_r = r + dr, cur_c = c + dc;
                while (cur_r > 0 && cur_r < 7 && cur_c > 0 && cur_c < 7) {
                    mask |= (1ULL << (cur_r * 8 + cur_c));
                    cur_r += dr; cur_c += dc;
                }
            }
        }
        return mask;
    }

    constexpr inline uint64_t generate_bishop_attacks_on_the_fly(int sq, uint64_t occ)
    {
        uint64_t attacks = 0ULL;
        int r = sq / 8, c = sq % 8;
        for (int dr : {-1, 1}) {
            for (int dc : {-1, 1}) {
                int cur_r = r + dr, cur_c = c + dc;
                while (cur_r >= 0 && cur_r < 8 && cur_c >= 0 && cur_c < 8) {
                    uint64_t target_bit = (1ULL << (cur_r * 8 + cur_c));
                    attacks |= target_bit;
                    if (occ & target_bit) break; // Clips after tracking the blocker square
                    cur_r += dr; cur_c += dc;
                }
            }
        }
        return attacks;
    }


    constexpr inline uint64_t get_occupancy_permutation(int index, uint64_t mask) {
        uint64_t occ = 0ULL;
        int bit_index = 0;
        for (int i = 0; i < 64; ++i) {
            if (mask & (1ULL << i)) {
                if (index & (1 << bit_index)) {
                    occ |= (1ULL << i);
                }
                bit_index++;
            }
        }
        return occ;
    }

    struct FancyBishops {
        MagicRecord magics[64];
        uint64_t    shared_pool[5248];
        uint32_t    total_pool_size;
    };


    #if !defined(NDEBUG)
        #if defined(_MSC_VER)
            #pragma runtime_checks("", off) // Disables /RTC1 flag checks locally
            #pragma optimize("g", on)       // Forces global speed optimization on MSVC
        #elif defined(__clang__)
            #pragma clang optimize on       
            __attribute__((optimize("O3")))
        #elif defined(__GNUC__)
            #pragma GCC optimize ("O3")     
        #endif
    #endif

    inline FancyBishops generate_fancy_bishops_runtime()
    {
        FancyBishops table = {}; // Created locally on the stack
        std::mt19937_64 prng(987654321ULL); // High-quality random engine

        uint32_t current_global_offset = 0;

        // Temporary workspace arrays
        uint64_t temp_blockers[4096];
        uint64_t temp_attacks[4096];
        uint64_t local_table[4096];
        bool     occupied[4096];

        for (int sq = 0; sq < 64; ++sq) {
            uint64_t mask = generate_bishop_mask_fmb(sq);
            int bits = static_cast<int>(std::popcount(mask)); // Dynamic popcount
            int num_permutations = 1 << bits;
            uint8_t shift = 64 - bits;

            // Populate subsets using the carry-ripple sequence
            uint64_t b = 0ULL;
            int idx = 0;
            do {
                temp_blockers[idx] = b;
                temp_attacks[idx] = generate_bishop_attacks_on_the_fly(sq, b);
                idx++;
                b = (b - mask) & mask;
            } while (b != 0ULL);

            // Search for a perfect magic seed
            uint64_t magic = 0;
            bool magic_found = false;

            while (!magic_found) {
                // Generate a random candidate with a low bit population (ideal for magics)
                magic = prng() & prng() & prng();

                // Reset local collision verification tables
                for (int i = 0; i < num_permutations; ++i) {
                    local_table[i] = 0ULL;
                    occupied[i] = false;
                }

                magic_found = true;
                for (int i = 0; i < num_permutations; ++i) {
                    uint32_t hash = static_cast<uint32_t>((temp_blockers[i] * magic) >> shift);

                    // If a collision maps to an incorrect attack bitboard, reject this magic seed
                    if (occupied[hash] && local_table[hash] != temp_attacks[i]) {
                        magic_found = false;
                        break;
                    }
                    local_table[hash] = temp_attacks[i];
                    occupied[hash] = true;
                }

                if (magic_found) {
                    table.magics[sq] = { mask, magic, current_global_offset, shift };
                    for (int i = 0; i < num_permutations; ++i) {
                        table.shared_pool[current_global_offset + i] = local_table[i];
                    }
                    current_global_offset += num_permutations;
                }
            }
        }
        table.total_pool_size = current_global_offset;
        return table; // Returned cleanly by value (modern compilers optimize this via NRVO)
    }

    // RESTORE ORIGINAL COMPILER SETTINGS (OFF)
    #if !defined(NDEBUG)
        #if defined(_MSC_VER)
            #pragma runtime_checks("", restore) // Safely re-enables /RTC1 tracking
            #pragma optimize("", on)            // FIX C4085: Resets back to global defaults
        #elif defined(__clang__)
            #pragma clang optimize off
        #elif defined(__GNUC__)
            #pragma GCC reset_options
        #endif
    #endif

// ---------------------------------------------------------------------------------------------------------------
// Rook
// ---------------------------------------------------------------------------------------------------------------

    inline uint64_t generate_rook_mask_fmb(int sq) {
        uint64_t mask = 0ULL;
        int r = sq / 8, c = sq % 8;
        for (int cur_r = r + 1; cur_r < 7; ++cur_r) mask |= (1ULL << (cur_r * 8 + c));
        for (int cur_r = r - 1; cur_r > 0; --cur_r) mask |= (1ULL << (cur_r * 8 + c));
        for (int cur_c = c + 1; cur_c < 7; ++cur_c) mask |= (1ULL << (r * 8 + cur_c));
        for (int cur_c = c - 1; cur_c > 0; --cur_c) mask |= (1ULL << (r * 8 + cur_c));
        return mask;
    }

    // Fixed Attack Generator (Accurately records the blocking square first!)
    inline uint64_t generate_rook_attacks_on_the_fly_fmb(int sq, uint64_t occ) {
        uint64_t attacks = 0ULL;
        int r = sq / 8, c = sq % 8;

        for (int cur_r = r + 1; cur_r < 8; ++cur_r) {
            attacks |= (1ULL << (cur_r * 8 + c));
            if (occ & (1ULL << (cur_r * 8 + c))) break;
        }
        for (int cur_r = r - 1; cur_r >= 0; --cur_r) {
            attacks |= (1ULL << (cur_r * 8 + c));
            if (occ & (1ULL << (cur_r * 8 + c))) break;
        }
        for (int cur_c = c + 1; cur_c < 8; ++cur_c) {
            attacks |= (1ULL << (r * 8 + cur_c));
            if (occ & (1ULL << (r * 8 + cur_c))) break;
        }
        for (int cur_c = c - 1; cur_c >= 0; --cur_c) {
            attacks |= (1ULL << (r * 8 + cur_c));
            if (occ & (1ULL << (r * 8 + cur_c))) break;
        }
        return attacks;
    }

    struct FancyRooks
    {
        MagicRecord magics[64];
        uint64_t    shared_pool[102400]; // Precise raw array sizing for all rook blocker states
        uint32_t    total_pool_size;
    };

    // CROSS-COMPILER OPTIMIZATION ENFORCEMENT (ON)
    #if !defined(NDEBUG)
        #if defined(_MSC_VER)
            #pragma runtime_checks("", off) 
            #pragma optimize("g", on)       
        #elif defined(__clang__)
            #pragma clang optimize on       
            __attribute__((optimize("O3")))
        #elif defined(__GNUC__)
            #pragma GCC optimize ("O3")     
        #endif
    #endif

    inline std::unique_ptr<FancyRooks> generate_fancy_rooks_runtime() {
        auto table = std::make_unique<FancyRooks>();

        // High-quality MT19937 random stream
        std::mt19937_64 prng(987654321ULL);

        uint32_t current_global_offset = 0;

        // Sized safely to 8192 elements to absorb dynamic 12-bit configurations cleanly
        uint64_t* temp_blockers = new uint64_t[8192];
        uint64_t* temp_attacks = new uint64_t[8192];
        uint64_t* local_table = new uint64_t[8192];
        bool* occupied = new bool[8192];

        for (int sq = 0; sq < 64; ++sq) {
            uint64_t mask = generate_rook_mask_fmb(sq);
            int bits = static_cast<int>(std::popcount(mask));
            int num_permutations = 1 << bits;
            uint8_t shift = 64 - bits;

            // Step A: Populate configurations using your exact carry-ripple loop
            uint64_t b = 0ULL;
            int idx = 0;
            do {
                temp_blockers[idx] = b;
                temp_attacks[idx] = generate_rook_attacks_on_the_fly_fmb(sq, b);
                idx++;
                b = (b - mask) & mask;
            } while (b != 0ULL);

            // Step B: Hunt for a perfect custom seed for THIS square
            uint64_t magic = 0;
            bool magic_found = false;

            while (!magic_found) {
                // Generate standard candidates
                magic = prng() & prng() & prng();

                // THE CRITICAL ROOK PRE-FILTER:
                // A valid Rook magic multiplier requires high bit density in the upper 8 bits
                // of the product. Throwing away bad candidates here takes 1 cycle, preventing 
                // the loop from locking up on square 0
                if (std::popcount((mask * magic) & 0xFF00000000000000ULL) < 6) {
                    continue;
                }

                // Clear mapping tables up to current permutation ceiling
                for (int i = 0; i < num_permutations; ++i) {
                    local_table[i] = 0ULL;
                    occupied[i] = false;
                }

                magic_found = true;
                for (int i = 0; i < num_permutations; ++i) {
                    uint32_t hash = static_cast<uint32_t>((temp_blockers[i] * magic) >> shift);

                    if (occupied[hash] && local_table[hash] != temp_attacks[i]) {
                        magic_found = false; // Collision found. Discard multiplier.
                        break;
                    }
                    local_table[hash] = temp_attacks[i];
                    occupied[hash] = true;
                }

                if (magic_found) {
                    table->magics[sq] = { mask, magic, current_global_offset, shift };
                    for (int i = 0; i < num_permutations; ++i) {
                        table->shared_pool[current_global_offset + i] = local_table[i];
                    }
                    current_global_offset += num_permutations;
                }
            }
        }

        table->total_pool_size = current_global_offset;

        delete[] temp_blockers;
        delete[] temp_attacks;
        delete[] local_table;
        delete[] occupied;

        return table;
    }

} // namespace FancyMagicBitboards

// ===========================================================================================================

const inline FancyMagicBitboards::FancyBishops GlobalBishops = FancyMagicBitboards::generate_fancy_bishops_runtime();
const inline std::unique_ptr<FancyMagicBitboards::FancyRooks> GlobalRooks = FancyMagicBitboards::generate_fancy_rooks_runtime();

ALWAYS_INLINE uint64_t get_raw_bishop_moves_fmb(int sq, uint64_t occupancy)
{
    const FancyMagicBitboards::MagicRecord& m = GlobalBishops.magics[sq];

    // 1. Isolate relevant blocker squares
    uint64_t blockers = occupancy & m.mask;

    // 2. Perform Magic hashing shift
    uint64_t hash = (blockers * m.magic) >> m.shift;

    // 3. Multi-Indexed step: Direct retrieval from the shared cache block
    return GlobalBishops.shared_pool[m.offset + hash];
}

ALWAYS_INLINE uint64_t get_raw_rook_moves_fmb(int sq, uint64_t occupancy)
{
    const FancyMagicBitboards::MagicRecord& m = GlobalRooks->magics[sq];
    uint64_t blockers = occupancy & m.mask;
    uint64_t hash = (blockers * m.magic) >> m.shift;
    return GlobalRooks->shared_pool[m.offset + hash];
}

#endif // #ifdef __USE_FANCY_MAGIC_BITBOARDS_INSTEAD_OF_HQ__ 

