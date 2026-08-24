//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

//
// This file contains some compilation options
// These options mainly depict how some more or less interesting possibilities were verified during development.
// Some of these options require additional small lookup tables (mentioned below)
//

//#define __JGI_BB_PEDANTIC__ // this should be off - some redundant parts of code are wrapped around with this macro

//#define __USE_SQUARE_BITBOARD__ // should rather be off (shifting usually faster than taking from L1 cache); requires 512B of additional lookup; performance tests didn't show significant difference


#define __USE_BETWEEN_LOOKUP__ // it should be on, since it speeds up by ~20%; additional 11.5kB buffer used on the hot path; it's both small and super-fast - branchless, even no cmov; smaller than 32kB thanks to using a separate buffer with all bitmasks than can possibly be "between"


#define __USE_FIRSTRANKATTACKSLOOKUP__ // if on, it speeds up about 1%; it is a slight extention of pure Hyperbola Quintessence that requires only 512B of additional lookup; for details see https://www.chessprogramming.org/First_Rank_Attacks


#define __PREEMPTIVE_WHITEPINNEDPIECES__ // Most probably this macro should be on - it seems responsible for about 2% speed-up


// #define __PREEMPTIVE_BLACKPINNEDPIECES__ // It should be off (not defined), since statistically already the first Black move found is a refutation, so we should not make this preemptive check (this macro is for IsImmediateMateAfterAnyBlackResponse)

// #define __VERIFY_PINNING_PREREQUISITE__ // an older attempt; although a few branchless operations, no performance gain when using it

#define __USE_OPTIM_FOR_NON_CAPTURE__ // should rather be on - the observed speed-up is about 1-2%
#define __USE_OPTIM_FOR_NON_CAPTURE_BY_KING__ // should rather be on; here the difference is that we have to pay one branch for this feature but as an additional advantage we have black king's moves sorted (captures analyzed first) - perf.tests indicate a tiny improvement

#define __USE_OPTIM_FOR_SAMEDIAGORLINE__ // seems to cause a tiny performance speed-up, ~0.5%
