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
using solely bitboard representation of chessboard and **Hyperbola Quintessence*** in order to reduce memory usage. Tt is, dependent on configuration (config.h), only **6kB-20kB**, so it entirely fits into L1 cache of modern CPUs (a small, branchless calculation on data in CPU registers and/or L1 cache is often much better then fetching a precalculated value from a large buffer in memory, even if in L2/L3 cache\*\*).
**More than 35 two-movers per millisecond** can be solved in all solutions mode (without stopping after finding a solution) as measured on Intel i7-14700 (single thread).
You can freely reuse this code inside your chess engine(s) - see LICENCE file for details.

**JGIsland_BB is a greenfield part of J.G.Island - Chess Moremovers** (https://jgisland.pl) with its source code, contrary to the main product, made public.
JGIsland_BB was added to J.G.Island - Chess Moremovers in its version 11.0 and it decreased total times on the test suite (https://jgisland.pl/download/reports/testsuite.php) by about -10%.

As already mentioned, one of the assumptions of this project was to minimize memory usage and reduce latencies keeping all the data in L1 cache of CPU.
That's why Magic Bitboards were not used but Hyperbola Quintessence** (super small calculations using data in CPU registers and L1 cache).
Castling and en passant information is passed as template parameters, so it doesn't have any physical representation (except for the possible en passant square). 
A dispatcher method selects the proper template version. This is sort of paradigm of the library to make this information "weightless", like a ray of light.

**C++ 20** makes quite a lot of work in compile-time (see data.h for generation of constexpr data).
C++ 20 uses fast uniform bit manipulation instructions like std::popcount or std::countr_zero. Based on this, a utility macro was created to be able to efficiently iterate through bits:
```
#define BEGIN_FOR_EACH_POS_IN_MASK(pos, mask) if (mask) { const int loop_count = std::popcount(mask); int loop_iter = 0; do { const int pos = std::countr_zero(mask);
#define END_FOR_EACH_POS_IN_MASK(pos, mask)  mask &= mask - 1; ++loop_iter; } while (loop_iter != loop_count); }
```
This way of looping, though with slightly more instructions, proved to be the fastest in isolated performance tests. This is because loop end condition is easily predictable for CPU and no CPU cycles are lost for branching (reload of instruction cache). Note also that there is a separate macro for const-iterating (leaving mask intact). Originally this macro was used in JGIsland_BB until it turned out in integrated tests that the simplest way of looping on bits wins:
```
#define BEGIN_FOR_EACH_POS_IN_MASK(pos, mask) while (mask) { const int pos = std::countr_zero(mask);
#define END_FOR_EACH_POS_IN_MASK(pos, mask)  mask &= mask - 1; }
```
Most probably the reason is register spilling - frequently inlined utility code should be as concise as possible (leaving complexity - and CPU registers - for surrounding code) and simplicity wins.
<!-- -->
JGIsland_BB was created in about 3 weeks (including bug fixing) mainly thanks to AI, which provided efficient implementations e.g. for Hyperbola Quintessence functions get_raw_rook_moves_hq and get_raw_bishop_moves_hq (although it was an iterative process and further manual optimizations were added later on this code). I intended to "commit and forget" but soon it turned out that many things requires tuning. It took about a month to speed up the code about 50% - from more than 30 two-movers per millisecond (on Intel i7-14700, single thread, all sol.) up to 46. I had to reconsider some original assumptions: originally I was determined to minimize the data, so that it all fits in L1 cache. During this time I realized how big an enemy is register spilling occuring when a small inlined utility function on hot path is slightly overcomplicated (e.g. uses more variables than absolute minimum). Reaching for immediate data in an array that is likely to be in L2 cache may be much better than sticking to strict L1 cache friendliness (see also Update 2 below). 

Checkmate search is ultrafast thanks to almost branchless operations on bitmasks. 
For example CanBlackMoveInBetween first calculates branchless (sometimes cmov) the bitmask of all the squares between the two given squares (GetBetweenMask), then filters out Black pieces that cannot possibly reach any of these squares, then every remaining Black piece is matched agains this bitmask, AllBetweenEmpty (branchless) is called on every candidate, then pinning is verified (IsBlackPinned; BTW: there is also a config macro \_\_PREEMPTIVE_BLACKPINNEDPIECES\_\_, but it is better off for Black moves).

The C++20 code is maybe not super-clean (e.g. name conventions mixed, Clang warns about 'dangling else') but should be considered clean enough. I have a weakness for a prefix "t" for template parameter names and for some remnants of Hungarian notation (e.g. tbInclKing stands for template boolean parameter that specifies if a method includes king or not). 
Macros are avoided, although BEGIN_FOR_EACH_POS_IN_MASK may be considered useful focusing on logic and hiding the implementation details, at the same time providing maximum performance.

The main idea of this piece of code is simplicity and conciseness (buffers using only from 6kB to 20kB) - CPUs really like it. 
As already said, performing a small, branchless calculation should be preferred over fetching data from large buffers for maximum speed.
<!-- -->
An interesting example of L1 cache-friendliness combined with performance is [betweenLookup](include/BetweenLookup.h). It uses only 11.5kB on hot path and its main method GetBetweenMask (called very frequently with forced inlining) does nothing but immediately indexes twice two small arrays (8kB + 3.5kB) with no risk of register spilling (no calculations at all). Typically this array occupies 32kB (64 x 64 x 8 bytes) but from overall 64*64 = 4096 line/diagonal bitmasks only 412 are unique bitmasks that can lie between two squares, including two special slots for zero bitmask (for adjacent squares) and full bitmask (for unaligned sqares, i.e. not on the same diagonal or line).
<!-- -->
I admit with remorse that one of the last things was writing several UTs... However my situation was specific: 
1) I already had a test suite of more than 10k two-movers on [https://jgisland.pl/download/reports/testsuite.php](https://jgisland.pl/download/reports/testsuite.php?page=6&m=0&sort=2) (two last subpages) and converted it into [integration test suite](tests/integration_tests_suite.h) of this project
2) I added a cross-check versus legacy methods inside a debug version (well, actually ReleaseWithAsserts to complete it faster) of the main product J.G.Island - Chess Moremovers 11.0 and ran it on the whole test suite (including all moremovers).
   
-----------------------------------------------------------------------------------------
**UPDATE**\
*For comparison there are also added - created in close co-operation with AI - methods of: 
* __Fancy Magic Bitboards__ (get_raw_rook_moves_fmb and get_raw_bishop_moves_fmb), data used on hot paths is more than 800kB
* __Dense Fancy Magic Bitboards__ (get_raw_rook_moves_dfmb and get_raw_bishop_moves_dfmb), data used on hot paths is less than **110kB**

but they are not used by default. However in order to activate them it's enough to use another value of template parameter MoveGenMethod of class FullBitboards: MoveGenMethodT::FancyMagics or MoveGenMethodT::DenseFancyMagics.

* Fancy Magic Bitboards use more than 800kB on hot path and in isolated tests cause a speed-up about 10-12% (44 two-movers per millisecond vs. 39 with Hyperbola Quintessence).\
* Dense Fancy Magic Bitboards use less than 110kB on hot path and in isolated tests cause a speed up to about 45 two-movers per millisecond.

This example shows quite well how cache-friendliness and smaller buffers directly translate into performance.
I will later provide some more information about results of integrations tests with Dense Fancy Magic Bitboards (using full test suite of J.G.Island - Chess Moremovers)

-----------------------------------------------------------------------------------------
**UPDATE 2**\
As mentioned above, a small, branchless calculation on data in CPU registers and/or L1 cache is often much better then fetching a precalculated value from a large buffer in memory, even if in L2/L3 cache.
However after the more thorough analysis of performance of Hyperbola Quintessence finally I came to a conclusion that it suffers from another bottleneck: __register spilling__. This is because this small calculation uses quite many variables and compiler is not able to put them all in CPU registers, so it is forced to put them on stack.
<!-- -->
Another finding was the following: Originally I considered this two-mover performance test, an "isolated" test. In a way it is true: in this test memory usage is very low, transposition table is not used, simplicity is at its maximum. However recently I decided to make a fully isolated performance test of Hyperbola Quintessence vs (Dense) Fancy Magic Bitboards. And, to my surprise, I observed performance reversal: Hyperbola Quintessence proved to be 15% faster in this fully isolated test. At first I was really confused, I suspected some error in my performance test (admittedly, it is very easy to create a wrong performance test). However it turned out that most probably all is OK and the results, although unintuitive to me, are fully explainable: in fully isolated test Hyperbola Quintessence does not suffer from register spilling. CPU does nothing but move calculations and does not need registers for anything else. Dense Fancy Magic Bitboards advantage is total simplicity: it does not need any registers for any calculations and this advantage reveals in a more complex context. Need to reach for L2 cache does not seem to be a matter.
<!-- -->
First, tentative conclusions: __Although Hyperbola Quitenssence was my first choice and looked L1 cache-friendly and flawless, register spilling seems to be an important obstacle that may encourage to turn to Dense Fancy Magic Bitboards__ (i.e. using FullBitboards_DFMB). This is confirmed by the results of performance test (single thread) on two-movers on Intel i7-14700: almost 45 two-movers per millisecond can be solved using Dense Fancy Magic Bitboards, while only 39 with Hyperbola Quintessence. Integrated tests of J.G.Island - Chess Moremovers with Dense Fancy Magic Bitboards also confirm this conclusion so far. However as soon as CPUs in future (10-20 years) have more registers, Hyperbola Quintessence may outperform its competitors (but it will require recompilation for the target platform, while taking advantage of larger CPU cache by Magics is smooth - without any recompilation).
