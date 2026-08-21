__JGIsland_BB__
<!-- -->
Copyright Marcin Sterkowiec, 2026. Use, modification and\
distribution is subject to license (see accompanying file license.txt)

[![CMake-MSVC-2022](https://github.com/msterkowiec/JGIsland_BB/actions/workflows/cmake-msvc-2022.yml/badge.svg)](https://github.com/msterkowiec/JGIsland_BB/actions/workflows/cmake-msvc-2022.yml)
[![CMake-GCC](https://github.com/msterkowiec/JGIsland_BB/actions/workflows/cmake-gcc.yml/badge.svg)](https://github.com/msterkowiec/JGIsland_BB/actions/workflows/cmake-gcc.yml)
[![CMake-Clang](https://github.com/msterkowiec/JGIsland_BB/actions/workflows/cmake-clang.yml/badge.svg)](https://github.com/msterkowiec/JGIsland_BB/actions/workflows/cmake-clang.yml)

JGIsland_BB contains ultrafast methods of:
1) finding immediate checkmate,
2) solving chess two-movers
<!-- -->
using solely bitboard representation of chessboard and **Hyperbola Quintessence*** in order to reduce memory usage. Tt is, dependent on configuration (config.h), only **6kB-31kB**, so it entirely fits into L1 cache of modern CPUs (usually a small, branchless calculation on data in CPU registers and/or L1 cache is much better then fetching a precalculated value from a large buffer in memory, even if in L3 cache).
**More than 35 two-movers per millisecond** can be solved in all solutions mode (without stopping after finding a solution) as measured on Intel i7-14700 (single thread).
You can freely reuse this code inside your chess engine(s) - see LICENCE file for details.

**JGIsland_BB is a greenfield part of J.G.Island - Chess Moremovers** (https://jgisland.pl) with its source code, contrary to the main product, made public.
JGIsland_BB was added to J.G.Island - Chess Moremovers in its version 11.0 and it decreased total times on the test suite (https://jgisland.pl/download/reports/testsuite.php) by about -10%.

As already mentioned, one of the assumptions of this project was to minimize memory usage and reduce latencies keeping all the data in L1 cache of CPU.
That's why Magic Bitboards were not used but Hyperbola Quintessence (super small calculations using data in CPU registers and L1 cache).
Castling and en passant information is passed as template parameters, so it doesn't have any physical representation (except for the possible en passant square). 
A dispatcher method selects the proper template version. This is sort of paradigm of the library to make this information "weightless", like a ray of light.

**C++ 20** makes quite a lot of work in compile-time (see data.h for generation of constexpr data).
C++ 20 uses fast uniform bit manipulation instructions like std::popcount or std::countr_zero. Based on this, a utility macro was created to be able to efficiently iterate through bits:
```
#define BEGIN_FOR_EACH_POS_IN_MASK(pos, mask) if (mask) { const int loop_count = std::popcount(mask); int loop_iter = 0; do { const int pos = std::countr_zero(mask);
#define END_FOR_EACH_POS_IN_MASK(pos, mask)  mask &= mask - 1; ++loop_iter; } while (loop_iter != loop_count); }
```
This way of looping, though with slightly more instructions, proved to be the fastest in performance tests. This is because loop end condition is easily predictable for CPU and no CPU cycles are lost for branching (reload of instruction cache). Note also that there is a separate macro for const-iterating (leaving mask intact).
<!-- -->
JGIsland_BB was created in about 3 weeks (including bug fixing) mainly thanks to AI, which provided efficient implementations e.g. for Hyperbola Quintessence functions get_raw_rook_moves_hq and get_raw_bishop_moves_hq (although it was an iterative process and further manual optimizations were added later on this code).

Checkmate search is ultrafast thanks to almost branchless operations on bitmasks. 
For example CanBlackMoveInBetween first calculates branchless (sometimes cmov) the bitmask of all the squares between the two given squares (GetBetweenMask), then filters out Black pieces that cannot possibly reach any of these squares, then every remaining Black piece is matched agains this bitmask, AllBetweenEmpty (branchless) is called on every candidate, then pinning is verified (IsBlackPinned; BTW: there is also a config macro \_\_PREEMPTIVE_BLACKPINNEDPIECES\_\_, but it is better off for Black moves).

The C++20 code is maybe not super-clean (e.g. name conventions mixed, Clang warns about 'dangling else') but should be considered clean enough. I have a weakness for a prefix "t" for template parameter names and for some remnants of Hungarian notation (e.g. tbInclKing stands for template boolean parameter that specifies if a method includes king or not). 
Macros are avoided, although BEGIN_FOR_EACH_POS_IN_MASK may be considered useful focusing on logic and hiding the implementation details, at the same time providing maximum performance.

The main idea of this piece of code is simplicity and conciseness (buffers using only from 6kB to 31kB) - CPUs really like it. 
As already said, performing a small, branchless calculation should be preferred over fetching data from large buffers for maximum speed.

I admit with remorse that one of the last things was writing several UTs... However my situation was specific: 
1) I already had a test suite of more than 10k two-movers on [https://jgisland.pl/download/reports/testsuite.php](https://jgisland.pl/download/reports/testsuite.php?page=6&m=0&sort=2) (two last subpages) and converted it into [integration test suite](tests/integration_tests_suite.h) of this project
2) I added a cross-check versus legacy methods inside a debug version (well, actually ReleaseWithAsserts to complete it faster) of the main product J.G.Island - Chess Moremovers 11.0 and ran it on the whole test suite (including all moremovers).
   
-----------------------------------------------------------------------------------------
**UPDATE**\
*For comparison there are also added - created in close co-operation with AI - methods of: 
* __Fancy Magic Bitboards__ (get_raw_rook_moves_fmb and get_raw_bishop_moves_fmb), data used on hot paths is more than 800kB
* __Dense Fancy Magic Bitboards__ (get_raw_rook_moves_dfmb and get_raw_bishop_moves_dfmb), data used on hot paths is more than 160kB

but they are not used by default. Their usage can be activated using macros in config.h: \_\_USE_FANCY_MAGIC_BITBOARDS_INSTEAD_OF_HQ\_\_ or \_\_USE_DENSE_FANCY_MAGIC_BITBOARDS_INSTEAD_OF_HQ\_\_ respectively

* Fancy Magic Bitboards use more than 800kB on hot path and in isolated tests cause a speed-up about 10-12% (44 two-movers per millisecond vs. 39 with Hyperbola Quintessence).\
* Dense Fancy Magic Bitboards use more than 160kB on hot path and in isolated tests cause a speed up to about 45 two-movers per millisecond.

This example shows quite well how cache-friendliness and smaller buffers directly translate into performance.
I will later provide some more information about results of integrations tests with Dense Fancy Magic Bitboards (using full test suite of J.G.Island - Chess Moremovers)

