// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

inline constexpr int _A_ = 0; 
inline constexpr int _B_ = 1;
inline constexpr int _C_ = 2;
inline constexpr int _D_ = 3;
inline constexpr int _E_ = 4; 
inline constexpr int _F_ = 5; 
inline constexpr int _G_ = 6; 
inline constexpr int _H_ = 7;

inline constexpr int _1_ = 0; 
inline constexpr int _2_ = 1;
inline constexpr int _3_ = 2; 
inline constexpr int _4_ = 3; 
inline constexpr int _5_ = 4; 
inline constexpr int _6_ = 5;
inline constexpr int _7_ = 6; 
inline constexpr int _8_ = 7;

inline constexpr int _A1_ = 0;
inline constexpr int _B1_ = 1;
inline constexpr int _C1_ = 2;
inline constexpr int _D1_ = 3;
inline constexpr int _E1_ = 4;
inline constexpr int _F1_ = 5;
inline constexpr int _G1_ = 6;
inline constexpr int _H1_ = 7;
inline constexpr int _A2_ = 8;
inline constexpr int _B2_ = 9;
inline constexpr int _C2_ = 10;
inline constexpr int _D2_ = 11;
inline constexpr int _E2_ = 12;
inline constexpr int _F2_ = 13;
inline constexpr int _G2_ = 14;
inline constexpr int _H2_ = 15;
inline constexpr int _A3_ = 16;
inline constexpr int _B3_ = 17;
inline constexpr int _C3_ = 18;
inline constexpr int _D3_ = 19;
inline constexpr int _E3_ = 20;
inline constexpr int _F3_ = 21;
inline constexpr int _G3_ = 22;
inline constexpr int _H3_ = 23;
inline constexpr int _A4_ = 24;
inline constexpr int _B4_ = 25;
inline constexpr int _C4_ = 26;
inline constexpr int _D4_ = 27;
inline constexpr int _E4_ = 28;
inline constexpr int _F4_ = 29;
inline constexpr int _G4_ = 30;
inline constexpr int _H4_ = 31;
inline constexpr int _A5_ = 32;
inline constexpr int _B5_ = 33;
inline constexpr int _C5_ = 34;
inline constexpr int _D5_ = 35;
inline constexpr int _E5_ = 36;
inline constexpr int _F5_ = 37;
inline constexpr int _G5_ = 38;
inline constexpr int _H5_ = 39;
inline constexpr int _A6_ = 40;
inline constexpr int _B6_ = 41;
inline constexpr int _C6_ = 42;
inline constexpr int _D6_ = 43;
inline constexpr int _E6_ = 44;
inline constexpr int _F6_ = 45;
inline constexpr int _G6_ = 46;
inline constexpr int _H6_ = 47;
inline constexpr int _A7_ = 48;
inline constexpr int _B7_ = 49;
inline constexpr int _C7_ = 50;
inline constexpr int _D7_ = 51;
inline constexpr int _E7_ = 52;
inline constexpr int _F7_ = 53;
inline constexpr int _G7_ = 54;
inline constexpr int _H7_ = 55;
inline constexpr int _A8_ = 56;
inline constexpr int _B8_ = 57;
inline constexpr int _C8_ = 58;
inline constexpr int _D8_ = 59;
inline constexpr int _E8_ = 60;
inline constexpr int _F8_ = 61;
inline constexpr int _G8_ = 62;
inline constexpr int _H8_ = 63;

inline constexpr int DBL_CHECKED = 64;
inline constexpr bool EXCL_KING = 0;
inline constexpr bool SKIP_KING = 0;
inline constexpr bool INCL_KING = 1;
inline constexpr bool EXCL_PINNED = 0;
inline constexpr bool INCL_PINNED = 1;
inline constexpr bool FIND_ALL = 0;
inline constexpr bool FIND_ONE = 1;

// Some legacy/mailbox coding that will occasionally be used. 
inline constexpr int CLR_WHITE = 64;
inline constexpr int CLR_BLACK = 128;

inline constexpr int FGR_EMPTY = 0;
inline constexpr int FGR_KING = 1;
inline constexpr int FGR_PAWN = 2;
inline constexpr int FGR_BISHOP = 4;
inline constexpr int FGR_ROOK = 8;
inline constexpr int FGR_QUEEN = 12;
inline constexpr int FGR_KNIGHT = 16;

inline constexpr int WHITE_KING = (CLR_WHITE + FGR_KING);
inline constexpr int WHITE_PAWN = (CLR_WHITE + FGR_PAWN);
inline constexpr int WHITE_KNIGHT = (CLR_WHITE + FGR_KNIGHT);
inline constexpr int WHITE_BISHOP = (CLR_WHITE + FGR_BISHOP);
inline constexpr int WHITE_ROOK = (CLR_WHITE + FGR_ROOK);
inline constexpr int WHITE_QUEEN = (CLR_WHITE + FGR_QUEEN);

inline constexpr int BLACK_KING = (CLR_BLACK + FGR_KING);
inline constexpr int BLACK_PAWN = (CLR_BLACK + FGR_PAWN);
inline constexpr int BLACK_KNIGHT = (CLR_BLACK + FGR_KNIGHT);
inline constexpr int BLACK_BISHOP = (CLR_BLACK + FGR_BISHOP);
inline constexpr int BLACK_ROOK = (CLR_BLACK + FGR_ROOK);
inline constexpr int BLACK_QUEEN = (CLR_BLACK + FGR_QUEEN);
