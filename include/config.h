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


#define __USE_FIRSTRANKATTACKSLOOKUP__ // if on, it speeds up about 1%; it is a slight extention of pure Hyperbola Quintessence that requires only 512B of additional lookup; for details see https://www.chessprogramming.org/First_Rank_Attacks


#define __PREEMPTIVE_WHITEPINNEDPIECES__ // Most probably this macro should be on - it seems responsible for about 2% speed-up


// #define __PREEMPTIVE_BLACKPINNEDPIECES__ // It should be off (not defined), since statistically already the first Black move found is a refutation, so we should not make this preemptive check (this macro is for IsImmediateMateAfterAnyBlackResponse)

#define __USE_OPTIM_FOR_NON_CAPTURE__ // should rather be on - the observed speed-up is about 1-2%
#define __USE_OPTIM_FOR_NON_CAPTURE_BY_KING__ // should rather be on; here the difference is that we have to pay one branch for this feature but as an additional advantage we have black king's moves sorted (captures analyzed first) - perf.tests indicate a tiny improvement


#define __USE_OPTIM_FOR_SAMEDIAGORLINE__ // seems to cause a tiny performance speed-up, ~0.5%

// Set of the most recent micro-optimizations that speed up from almost 45 to 46 two-movers per millisecond on Intel i7-14700 (single thread, all sol.)
#define __USE_ISEDGE_FORDISCOVEREDCHECK__ // see also tbUseIsEdgeForDiscoveredCheck below
#define __USE_ISEDGE_FORISPINNED__ // see also tbUseIsEdgeForIsPinned
#define __USE_WHITEPAWNCHECKOPTIM__ // see also tbUseWhitePawnCheckOptim below 
#define __USE_OPTIMFORGETRAY__
#define __USE_STDBITLOOPING__ // should rather be on - this way of looping wins in integrated tests
// #define __USE_MEMUSAGEOPTIM_IN_BETWEENLOOKUP__ // should rather be off - causes a very slight but measurable slowdown (although on some newest machines a small speed-up observed...)

#define __VERIFY_PINNING_PREREQUISITE__   // should rather be on - seems to speed up from 46 to 47 two-movers per millisecond on Intel i7-14700 (single thread, all sol.)
// #define __VERIFY_PINNING_WITHMOVEGEN__ // should rather be off - no performance gain

#define __USE_WHITEKNIGHTATTACKMASK__  // should rather be on - seems to further speed up to almost 52 two-movers per millisecond on Intel i7-14700 (single thread, all sol.)

#define __USE_MOVEGENINCANWHITEQUEENCHECK__ // should be ON
#define __USE_MOVEGENINCANWHITEROOKCHECK__  // should be ON
#define __USE_MOVEGENINCANWHITEBISHOPCHECK__ // should rather be ON (speeds up with Dence Fancy Magic Bitboards), although causes a slight slow-down with Hyperbola Quintessence

// This new set of optimizations should be ON. They let speed up test suite up to 59 two-movers per millisecond on Intel i7-14700 (single thread, all sol.)
#define __USE_RAYLOOKUP__
#define __USE_FASTANDLARGEDIRLOOKUP__ // uses relatively large 4kB lookup, but allows to preserve maximum simplicity of code (avoiding risk of register spilling)
#define __USE_MOVEGENINCANBLACKCAPTURE__
#define __USE_MOVEGENINCANBLACKMOVEON__
#define __USE_MOVEGENINCANBLACKMOVEINBETWEEN__

// -------------------------------------------------------------------------------------------------------------
// Additional constexpr boolean values to simplify code based on config macros (while config values above can be alterned, the code below should stay intact)
// (tb prefix stands for Template Boolean but in this context it means just Compile-Time Boolean)

#ifdef __USE_WHITEPAWNCHECKOPTIM__
inline constexpr bool tbUseWhitePawnCheckOptim = true;
#else
inline constexpr bool tbUseWhitePawnCheckOptim = false;
#endif

#ifdef __USE_ISEDGE_FORDISCOVEREDCHECK__
inline constexpr bool tbUseIsEdgeForDiscoveredCheck = true;
#else
inline constexpr bool tbUseIsEdgeForDiscoveredCheck = false;
#endif;

#ifdef __USE_ISEDGE_FORISPINNED__
inline constexpr bool tbUseIsEdgeForIsPinned = true;
#else
inline constexpr bool tbUseIsEdgeForIsPinned = false;
#endif

#ifdef __USE_MEMUSAGEOPTIM_IN_BETWEENLOOKUP__
inline constexpr bool tbMemUsageOptimInBetweenLookup = true;
#else
inline constexpr bool tbMemUsageOptimInBetweenLookup = false;
#endif

