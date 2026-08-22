
#include <chrono>
#include <gtest/gtest.h>
#include "integration_tests_suite.h"

#define private public // dirty trick for UTs only 
#include "utils.h"
#include "../include/JGIsland_BB.h"


bool IsSolutionAsExpected(int count, TMove* aMoves, const std::vector<TMove>& expectedSolutions)
{
	if (count != expectedSolutions.size())
		return false;

	for (const auto& expectedMove : expectedSolutions)
	{
		bool found = false;
		for (int j = 0; j < count; ++j)
			if (aMoves[j] == expectedMove)
			{
				found = true;
				break;
			}
		if (!found)
			return false;
	}

	return true;
}

TEST(JGIsland_BB_Tests, TestAllBetweenEmpty)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");
	EXPECT_EQ(bb.AllBetweenEmpty(_B1_, _B6_), true);
	EXPECT_EQ(bb.AllBetweenEmpty(_H2_, _E5_), true);
	EXPECT_EQ(bb.AllBetweenEmpty(_D2_, _D6_), true);
	EXPECT_EQ(bb.AllBetweenEmpty(_E4_, _A8_), true);

	EXPECT_EQ(bb.AllBetweenEmpty(_D2_, _D8_), false);
	EXPECT_EQ(bb.AllBetweenEmpty(_B1_, _G6_), false);
	EXPECT_EQ(bb.AllBetweenEmpty(_C4_, _H4_), false);
	EXPECT_EQ(bb.AllBetweenEmpty(_F3_, _A8_), false);
}

TEST(JGIsland_BB_Tests, TestAllBetweenEmptyIfTakeOffWhitePawn)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.AllBetweenEmptyIfTakeOffWhitePawn(_B1_, _B8_, _B6_), true);
	EXPECT_EQ(bb.AllBetweenEmptyIfTakeOffWhitePawn(_D2_, _G2_, _E2_), true);

	EXPECT_EQ(bb.AllBetweenEmptyIfTakeOffWhitePawn(_A2_, _G2_, _E2_), false);
}

TEST(JGIsland_BB_Tests, TestGetBetweenMask)
{
	EXPECT_EQ(FullBitboards_HQ::GetBetweenMask(_A1_, _H1_), 255 - 1 - 128);
	EXPECT_EQ(FullBitboards_HQ::GetBetweenMask<1>(_A1_, _H1_), 255);
	EXPECT_EQ(FullBitboards_HQ::GetBetweenMask(_A5_, _H5_), (255ULL - 1 - 128) << 32);
	EXPECT_EQ(FullBitboards_HQ::GetBetweenMask(_A1_, _A3_), 256);
	EXPECT_EQ(FullBitboards_HQ::GetBetweenMask(_A1_, _A4_), 256 + (256<<8));
	EXPECT_EQ(FullBitboards_HQ::GetBetweenMask(_C1_, _C4_), (256 + (256 << 8)) << 2);
	EXPECT_EQ(FullBitboards_HQ::GetBetweenMask(_A1_, _C3_), 512);
}

TEST(JGIsland_BB_Tests, TestIsSquareBetween)
{
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween<1>(_E1_, _A1_, _H1_), true);
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween<1>(_G1_, _A1_, _E1_), false);
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween(_C2_, _E4_, _B1_), true);
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween(_E8_, _E4_, _B1_), false);
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween(_E7_, _E8_, _E1_), true);
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween(_E8_, _E7_, _E1_), false);
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween<1>(_F7_, _E8_, _E1_), false);
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween<1>(_H1_, _H1_, _A8_), false);
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween<1>(_F3_, _H1_, _A8_), true);
	EXPECT_EQ(FullBitboards_HQ::IsSquareBetween(_F4_, _H1_, _A8_), false);
	#ifdef __USE_BETWEENLOOKUP__ 
	EXPECT_EQ((FullBitboards_HQ::IsSquareBetween<1,0>(_H1_, _H1_, _A8_)), true);// incl.ends
	#endif
}

TEST(JGIsland_BB_Tests, TestGetRay)
{
	EXPECT_EQ(GetRay(_B2_, _H8_), 1);
	EXPECT_EQ(GetRay(_G7_, _A1_), 1ULL << 63);
	EXPECT_EQ(GetRay(_G1_, _H1_), 63);
	EXPECT_EQ(GetRay(_A3_, _A8_), 1 + 256);
}

TEST(JGIsland_BB_Tests, TestSameDiagonalOrLineAndAllBetweenEmpty)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmpty(_B1_, _B6_), 1);
	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmpty(_B1_, _B8_), 0);
	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmpty(_B1_, _C6_), 0);

	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmpty(_B1_, _E4_), 1);
	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmpty(_B1_, _G6_), 0);
	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmpty(_B1_, _D4_), 0);
}

TEST(JGIsland_BB_Tests, TestSameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(_B1_, _B8_, _B6_), 1);
	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(_B1_, _C8_, _B6_), 0);

	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(_H7_, _E4_, _G6_), 1);
	EXPECT_EQ(bb.SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(_H8_, _E4_, _G6_), 0);
}

TEST(JGIsland_BB_Tests, TestWhiteLongDistanceFigureInDir)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir(_D2_, _D1_), 1); // wh.Rd6
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir<1>(_D2_, _D1_), _D6_);
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir(_D2_, 0, 1), 1);
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir<1>(_D2_, 0, 1), _D6_);
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir(_G4_, _H3_), 0); // wh.Re6 but not matching the direction
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir<1>(_G4_, _H3_), -1);
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir<1>(_G4_, -1, 1), -1);

	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir(_B6_, _B8_), 0); // bl.Bb1
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir<1>(_B6_, _B8_), -1);
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir(_B6_, 0, -1), 0);
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir<1>(_B6_, 0, -1), -1);

	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir(_A4_, 1, 1), 1); // wh.Be8
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir<1>(_A4_, 1, 1), _E8_); 
	
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir(_E4_, -1, 1), 0); // bl.Ba8
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir(_E4_, _F3_), 0); 
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir<1>(_E4_, -1, 1), -1); 
	EXPECT_EQ(bb.WhiteLongDistanceFigureInDir<1>(_E4_, _F3_), -1);
}

TEST(JGIsland_BB_Tests, TestBlackLongDistanceFigureInDir)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.BlackLongDistanceFigureInDir(_D2_, _D1_), 0); // wh.Rd6
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir<1>(_D2_, _D1_), -1);

	EXPECT_EQ(bb.BlackLongDistanceFigureInDir(_D2_, 0, 1), 0);
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir<1>(_D2_, 0, 1), -1);

	EXPECT_EQ(bb.BlackLongDistanceFigureInDir(_E4_, _F5_), 1); // bl.Bb1
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir<1>(_E4_, _F5_), _B1_);
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir(_E4_, -1, -1), 1);
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir<1>(_E4_, -1, -1), _B1_);
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir(_B6_, _B1_), 0); // bl.Bb1 but not matching the direction
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir<1>(_B6_, _B1_), -1);

	EXPECT_EQ(bb.BlackLongDistanceFigureInDir(_A4_, 1, 1), 0); // wh.Be8
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir<1>(_A4_, 1, 1), -1);

	EXPECT_EQ(bb.BlackLongDistanceFigureInDir(_E4_, -1, 1), 1); // bl.Ba8
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir(_E4_, _F3_), 1);
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir<1>(_E4_, -1, 1), _A8_);
	EXPECT_EQ(bb.BlackLongDistanceFigureInDir<1>(_E4_, _F3_), _A8_);
}

TEST(JGIsland_BB_Tests, TestIsSquareAttackedByWhite)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.IsSquareAttackedByWhite(_D1_), true);
	EXPECT_EQ(bb.IsSquareAttackedByWhite(_F1_), false);
	EXPECT_EQ(bb.IsSquareAttackedByWhite(_F3_), true);
	EXPECT_EQ(bb.IsSquareAttackedByWhite(_F4_), false);
	EXPECT_EQ(bb.IsSquareAttackedByWhite(_F5_), true);
	EXPECT_EQ(bb.IsSquareAttackedByWhite(_H3_), false);
	EXPECT_EQ(bb.IsSquareAttackedByWhite(_B4_), true);
	EXPECT_EQ(bb.IsSquareAttackedByWhite(_B7_), false);
	EXPECT_EQ(bb.IsSquareAttackedByWhite(_E6_), true);
}

TEST(JGIsland_BB_Tests, TestIsSquareAttackedByBlack)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.IsSquareAttackedByBlack(_D1_), true);
	EXPECT_EQ(bb.IsSquareAttackedByBlack(_B2_), false);
	EXPECT_EQ(bb.IsSquareAttackedByBlack(_C4_), true);
	EXPECT_EQ(bb.IsSquareAttackedByBlack(_E3_), false);
	EXPECT_EQ(bb.IsSquareAttackedByBlack(_A8_), true);
	EXPECT_EQ(bb.IsSquareAttackedByBlack(_E5_), false);
	EXPECT_EQ(bb.IsSquareAttackedByBlack(_G7_), true);
	EXPECT_EQ(bb.IsSquareAttackedByBlack(_H8_), false);
	EXPECT_EQ(bb.IsSquareAttackedByBlack(_F2_), true);
	EXPECT_EQ(bb.IsSquareAttackedByBlack(_H7_), false);
}

TEST(JGIsland_BB_Tests, TestIsSquareAttackedByWhiteIfTakeOffBlackKing)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1BRPP1/p1k3Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.IsSquareAttackedByWhite(_A3_), false);
	EXPECT_EQ(bb.IsSquareAttackedByWhiteIfTakeOffBlackKing(_A3_), true);
	EXPECT_EQ(bb.IsSquareAttackedByWhite(_A5_), false);
	EXPECT_EQ(bb.IsSquareAttackedByWhiteIfTakeOffBlackKing(_A5_), true);

}

TEST(JGIsland_BB_Tests, TestIsSquareAttackedByBlackIfTakeOffWhiteKing)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.IsSquareAttackedByBlack(_B2_), false);
	EXPECT_EQ(bb.IsSquareAttackedByBlackIfTakeOffWhiteKing(_B2_), true);
}

TEST(JGIsland_BB_Tests, TestIsWhitePinned)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b1r1BN1n/b3npb1/pPR3P1/p3B2n/B3b2p/k1K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.IsWhitePinned(_E5_, _D4_), false);
	EXPECT_EQ(bb.IsWhitePinned(_E5_, _F4_), true);
	EXPECT_EQ(bb.IsWhitePinned(_C6_, _C7_), false);
	EXPECT_EQ(bb.IsWhitePinned(_C6_, _C8_), false);
	EXPECT_EQ(bb.IsWhitePinned(_C6_, _D6_), true);
	EXPECT_EQ(bb.IsWhitePinned(_D2_, _E3_), false);
	EXPECT_EQ(bb.IsWhitePinned(_F8_, _H7_), false);
}

TEST(JGIsland_BB_Tests, TestIsWhitePinnedIfTakeOffBlackPawn)
{
	FullBitboards_HQ bb;
	bb.fromFEN("8/2k5/8/r2Pp2K/2P5/1P4B1/8/8 w - e6 0 2");

	EXPECT_EQ((bb.IsWhitePinnedIfTakeOffBlackPawn<1>(_D5_, _E6_, _E5_)), true);
	EXPECT_EQ(bb.IsWhitePinnedIfTakeOffBlackPawn(_C4_, _C5_, _E5_), false);

	bb.fromFEN("8/2k5/8/rP1Pp2K/2P5/6B1/8/8 w - e6 0 2");
	EXPECT_EQ((bb.IsWhitePinnedIfTakeOffBlackPawn<1>(_D5_, _E6_, _E5_)), false);
}

TEST(JGIsland_BB_Tests, TestIsBlackPinned)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b1R1BN1n/b1r1npP1/pP2RPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	EXPECT_EQ(bb.IsBlackPinned(_E5_, _D4_), 1);
	EXPECT_EQ(bb.IsBlackPinned(_C7_, _D7_), 1);
	EXPECT_EQ(bb.IsBlackPinned(_C7_, _C6_), 0);
	EXPECT_EQ(bb.IsBlackPinned(_H5_, _F6_), 0);
	EXPECT_EQ(bb.IsBlackPinned(_E3_, _D2_), 0);
}

TEST(JGIsland_BB_Tests, TestIsBlackPinnedIfTakeOffWhitePawn)
{
	FullBitboards_HQ bb;
	bb.fromFEN("8/8/8/2p5/k2pP1R1/8/5K2/8 b - e3 0 1");

	EXPECT_EQ(bb.IsBlackPinnedIfTakeOffWhitePawn(_D4_, _E3_, _E4_), true);
	EXPECT_EQ(bb.IsBlackPinnedIfTakeOffWhitePawn(_C5_, _C4_, _E4_), false);

	bb.fromFEN("8/8/8/8/kp1pP1R1/8/5K2/8 b - e3 0 1");
	EXPECT_EQ(bb.IsBlackPinnedIfTakeOffWhitePawn(_D4_, _E3_, _E4_), false);
}

TEST(JGIsland_BB_Tests, TestGetWhitePinnedPieces)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b1r1BN1n/b3npb1/pPR3P1/p3B2n/B3b2p/k1K1pp1p/3PP1R1/1b2r2b");

	const auto mask = bb.GetWhitePinnedPieces();
	EXPECT_EQ(mask, (1ULL << _C6_) + (1ULL <<_E5_));
}

TEST(JGIsland_BB_Tests, TestGetBlackPinnedPieces)
{
	FullBitboards_HQ bb;
	bb.fromFEN("b1R1BN1n/b1r1npP1/pP2RPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b");

	const auto mask = bb.GetBlackPinnedPieces();
	EXPECT_EQ(mask, (1ULL << _C7_) + (1ULL << _E5_));
}

TEST(JGIsland_BB_Tests, TestCanWhiteKingCheckMate)
{
	FullBitboards_HQ bb;
	bb.fromFEN("6bk/7p/8/4K3/8/2B5/8/8");
	EXPECT_EQ(bb.CanWhiteKingCheckMate(_E5_), true);

	bb.fromFEN("6bk/2p4p/6p1/4K1p1/4P3/2B5/8/8");
	EXPECT_EQ(bb.CanWhiteKingCheckMate(_E5_), false);
}

#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
#define NOT_PINNED ,false
#else
#define NOT_PINNED
#endif

TEST(JGIsland_BB_Tests, TestCanWhiteKnightCheckMate)
{
	FullBitboards_HQ bb;
	bb.fromFEN("3N2rk/6pp/8/8/8/8/8/4K3");
	EXPECT_EQ(bb.CanWhiteKnightCheckMate(_D8_), true);

	#ifndef __PREEMPTIVE_WHITEPINNEDPIECES__ // when preemptive pinning verification is on, this call should not be made - it would just raise assertion failure
	bb.fromFEN("2KN2rk/6pp/8/8/8/8/8/8");
	EXPECT_EQ(bb.CanWhiteKnightCheckMate(_D8_), false);
	#endif

	bb.fromFEN("2K3bk/7p/8/8/8/2N5/8/B7");
	EXPECT_EQ(bb.CanWhiteKnightCheckMate(_C3_), true);
}

TEST(JGIsland_BB_Tests, TestCanWhiteBishopCheckMate)
{
	FullBitboards_HQ bb;
	bb.fromFEN("3K4/5p2/5kp1/3P1p2/7P/8/8/6B1");	
	EXPECT_EQ(bb.CanWhiteBishopCheckMate(_G1_ NOT_PINNED), true);
		
	bb.fromFEN("3K4/5p2/5kp1/3P4/7P/8/5B2/8");
	EXPECT_EQ(bb.CanWhiteBishopCheckMate(_F2_ NOT_PINNED), false);

	bb.fromFEN("3K4/5p2/5kp1/3P4/7P/8/5B2/5R2");
	EXPECT_EQ(bb.CanWhiteBishopCheckMate(_F2_ NOT_PINNED), true);
}

TEST(JGIsland_BB_Tests, TestCanWhiteRookCheckMate)
{
	FullBitboards_HQ bb;
	bb.fromFEN("3K4/5p2/5kpP/3P4/3P3P/8/1R6/8");
	EXPECT_EQ(bb.CanWhiteRookCheckMate(_B2_ NOT_PINNED), true);

	bb.fromFEN("3K4/5p2/5kp1/8/7P/8/1R6/B7");
	EXPECT_EQ(bb.CanWhiteRookCheckMate(_B2_ NOT_PINNED), false);

	bb.fromFEN("3K4/5p2/1P3kp1/5p2/7P/8/1R6/B7");
	EXPECT_EQ(bb.CanWhiteRookCheckMate(_B2_ NOT_PINNED), true);
}

TEST(JGIsland_BB_Tests, TestCanWhiteQueenCheckMate)
{
	FullBitboards_HQ bb;
	bb.fromFEN("8/8/5k2/8/7K/1B6/8/2Q5");
	EXPECT_EQ(bb.CanWhiteQueenCheckMate(_C1_ NOT_PINNED), true);

	bb.fromFEN("8/8/4pk2/8/7K/1B6/8/2Q5");
	EXPECT_EQ(bb.CanWhiteQueenCheckMate(_C1_ NOT_PINNED), false);
}

TEST(JGIsland_BB_Tests, TestCanWhitePawnCheckMate)
{
	FullBitboards_HQ bb;
	bb.fromFEN("8/6p1/3K1kp1/4p1p1/4bPp1/5R2/8/8");
	EXPECT_EQ(bb.CanWhitePawnCheckMate<0>(_F4_, -1 NOT_PINNED), true);

	bb.fromFEN("6K1/6p1/5pkp/6pp/5Pp1/5R2/8/8");
	EXPECT_EQ(bb.CanWhitePawnCheckMate<0>(_F4_, -1 NOT_PINNED), true);

	bb.fromFEN("6K1/6p1/5pkp/6pp/5Pp1/5b2/5R2/8");
	EXPECT_EQ(bb.CanWhitePawnCheckMate<0>(_F4_, -1 NOT_PINNED), false);

	bb.fromFEN("8/6K1/8/5pkp/6pp/8/5PRB/8");
	EXPECT_EQ(bb.CanWhitePawnCheckMate<0>(_F2_, -1 NOT_PINNED), true);

	bb.fromFEN("8/6K1/8/5pkp/6pp/8/5PBB/8");
	EXPECT_EQ(bb.CanWhitePawnCheckMate<0>(_F2_, -1 NOT_PINNED), false);

	bb.fromFEN("r5k1/RP6/8/8/4pp2/8/5K2/8");
	EXPECT_EQ(bb.CanWhitePawnCheckMate(_B7_, -1 NOT_PINNED), true);
}

TEST(JGIsland_BB_Integration, BasicIntegrationTest)
{
	FullBitboards_HQ bb;	

	// Immediate checkmate:
	EXPECT_EQ(bb.IsImmediateCheckMate("1B6/2P2BKp/3k3P/4N3/1P6/8/8/8"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/7B/6R1/3ppb2/4k3/7K/3PP3/8"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("4rk2/4ppR1/7B/5b2/8/7K/8/8"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/8/4N3/4pb2/4k3/7K/2PP2P1/7Q"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("1R3Bk1/2K3pr/5pP1/3p1N2/6P1/4bPp1/4p3/2q5"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("1K1k4/4p3/2P5/1B1p2q1/3ppp2/1P3b2/7b/1q6"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("1r6/1n3Bp1/5k2/8/R4N1p/2n1BR2/6p1/5bK1"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("5n2/2p1P2p/5k2/5P2/1KP2P2/2pPp1Pp/q5b1/3n1r1r"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("7B/1p4PR/1P2k2P/KPP5/6PP/8/8/8"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("7B/1p4PR/1P3k1P/KPP5/6P1/7P/8/8"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("7B/1p4PR/1P4kP/KPP5/6P1/7P/8/8"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("6B1/8/8/3R4/ppb5/1k6/pppp4/5K2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("3B2B1/pKNR4/1p5r/2k1pRb1/P3p2q/P5n1/1r1P4/4nb2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("2K5/k7/8/PpP5/8/8/R7/8 w - b6 0 2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("k1K5/8/8/PpP5/8/8/R7/8 w - b6 0 2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/P4k1b/R4N1P/4pPPP/6K1/B7/8/8 w - e6 0 2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/3B4/ppp5/1k2PpR1/8/1K6/8/8 w - f6 0 2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("6N1/2p3N1/2pk4/2pPp3/2p5/3R2B1/2K5/8 w - e6 0 2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/8/8/8/2N5/8/2p1PP2/2k1K2R w K - 0 1"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/Qb1Rp2r/4B1p1/R1pP2Np/N4p2/3k2P1/P7/3KB3 w - c6 0 2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("4K3/b2NPQ1p/1p1pP3/1P1k2p1/1R2rp2/P2P1n1b/6q1/8"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/2n5/8/7K/6R1/5Bpk/8/7q"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("rN2n1K1/b1kPQ1P1/Qn2Qpq1/pRBp2n1/P1P4N/1PpN2R1/4PR2/5b2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/8/p1p5/P1k2K1R/p7/2PP4/8/8"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/8/8/8/pk6/pp4K1/3P4/4BB2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("1N6/8/1p6/pk3b1R/pp1r4/3K4/8/5B2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/6N1/3k1P2/1K1Pp3/7p/2p3B1/3R4/8 w - e6 0 2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("2qn4/4P3/3P1kPP/3P3P/3P2PP/8/8/4K3"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("2qnn3/3P4/3P1kPP/3P3P/3P2PP/8/8/4K3"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("4bk2/3np3/P1N2PRP/B1NbPRq1/PB1KQ1N1/1P2BPRR/pN1RR2R/1n1RN2Q"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("2N2b2/3n1pp1/Rr3kp1/4pPR1/PN1n3P/K4Q2/B7/B7 w - e6 0 2"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/1bQ5/7B/K1p1pPr1/p4k2/2P1p1N1/4Bp1p/5rb1 w - e6 0 3"), 1);
	EXPECT_EQ(bb.IsImmediateCheckMate("3rRB1r/1n2PP1b/1Rn1k1B1/2p1p3/6p1/p3Pp2/3K4/3N4"), 1);


	// No immediate checkmate:
	EXPECT_EQ(bb.IsImmediateCheckMate("b3BN1n/b3npP1/pP1RRPP1/p1k1b1Rn/B1p1b2p/2K1pp1p/3PP1R1/1b2r2b"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("B2B4/3p1p2/1R1pkp1p/5r2/p2P1P2/Pr4KB/B2PP3/4Q3"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("5B2/3p1p2/3pkp1p/3r1r2/3P1P2/1B5B/3PPK2/4Q3"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("RR4K1/8/8/8/8/8/1pp5/bkrb3Q"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("3n4/bpPnR2q/2p1pr1p/Q1PkP2R/N2N1p2/1P3P2/2p5/B1K5"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("2rK4/B2PNbR1/BP2k1p1/P3P1Pp/1p1PpQ1b/Pp6/nn4pp/2rRN2q"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/8/8/4pp2/2P1k3/2P3K1/2P2P2/8"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("N2B4/2b5/p7/k3PpR1/8/1PP5/7K/8 w - f6 0 2"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/3B4/ppp5/1k2PpR1/8/1K6/8/8"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("3B2Bb/pKNR4/1p5r/2k1pRb1/P3p2q/P5n1/1r1P4/4nb2"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/P4k1b/R4N1P/4pPPP/4K3/B7/8/8 w - e6 0 2"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("8/5k2/R4N1P/1r2pPKR/8/B7/8/8 w - e6 0 2"), 0);
	EXPECT_EQ(bb.IsImmediateCheckMate("4rr2/3k1P2/2R5/N3pP2/3p2QB/8/8/R3K3 w Q e6 0 2"), 0);

	// A few simple twomovers:
	EXPECT_EQ(bb.SolveTwoMover("2n5/3p1p1R/pp1Pk1p1/b2qP1K1/3pNP1p/n1pb2P1/QP6/2r2B2"), 0);
	EXPECT_EQ(bb.SolveTwoMover("R7/P2PP2k/R1P2p2/PP3N2/1B1P3N/4R2P/P1K3P1/3N4"), 1);
	EXPECT_EQ(bb.SolveTwoMover("6Nq/5p2/2P1k1P1/2P1P3/2P1R2P/2K5/8/5B2"), 1);
}

template<MoveGenMethodT MoveGenMethod>
size_t RunTestFindMoveThatMatesInTwoMoves()
{
	FullBitboards<MoveGenMethod> bb;
	std::array<TMove, 256> aMoves;

	auto start = std::chrono::steady_clock::now();

	constexpr auto num = sizeof(test_suite) / sizeof(test_suite[0]);
	size_t numSuccessful = 0;
	size_t numFailed = 0;
	for (size_t i = 0; i < num; ++i)
	{
		const auto szFEN = test_suite[i].first.c_str();
		const auto& sExpectedSolutions = test_suite[i].second;


		auto res = bb.SolveTwoMover<1>(szFEN, aMoves.data());
		if (res < 0)
			std::cout << "Error parsing FEN: " << szFEN << "\n";
		else
		{
			const auto expectedSolutions = bb.StringToMoves(sExpectedSolutions);
			if (!IsSolutionAsExpected(res, aMoves.data(), expectedSolutions))
			{
				std::string additionalInfo = (res != expectedSolutions.size()) ? " (" + std::to_string(res) + " solutions found)" : "";
				std::cout << "Test failed for FEN=" << szFEN << additionalInfo << "\n";
				++numFailed;
			}
			else
				++numSuccessful;
		}
	}

	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	std::cout << "[   INFO   ] Successful: " << std::to_string(numSuccessful) << std::endl;
	std::cout << "[   INFO   ] Failed: " << std::to_string(numFailed) << std::endl;
	std::cout << "[   INFO   ] Elapsed time: " << duration.count() << " ms" << std::endl;
	std::cout << "[   INFO   ] Twomovers per millisecond: " << std::to_string(((double)(numSuccessful + numFailed)) / duration.count()) << "\n";

	return numFailed;
}

TEST(JGIsland_BB_Integration, TestFindMoveThatMatesInTwoMoves_HQ)
{
	auto numFailed = RunTestFindMoveThatMatesInTwoMoves<MoveGenMethodT::HyperbolaQuintessence>();
	EXPECT_EQ(numFailed, 0);
}

TEST(JGIsland_BB_Integration, TestFindMoveThatMatesInTwoMoves_FMB)
{
	auto numFailed = RunTestFindMoveThatMatesInTwoMoves<MoveGenMethodT::FancyMagics>();
	EXPECT_EQ(numFailed, 0);
}

TEST(JGIsland_BB_Integration, TestFindMoveThatMatesInTwoMoves_DFMB)
{
	auto numFailed = RunTestFindMoveThatMatesInTwoMoves<MoveGenMethodT::DenseFancyMagics>();
	EXPECT_EQ(numFailed, 0);
}
