//
// JGIsland_BB
//
//  Copyright Marcin Sterkowiec, 2026. Use, modification and
//  distribution is subject to license (see accompanying file license.txt)
//

#pragma once

#include <vector>
#include <string>
#include <cstring>
#include "config.h"
#include "data.h"
#include "utils.h"
#include "BetweenLookup.h"
#include "RayLookup.h"

#include "hyperbola.h"
#include "fancy_magics.h"
#include "fancy_magics_dense.h"

enum class MoveGenMethodT { HyperbolaQuintessence, FancyMagics, DenseFancyMagics };

template<MoveGenMethodT MoveGenMethod = MoveGenMethodT::HyperbolaQuintessence, bool tbBlackHaveRookLikes = true, bool tbBlackHaveBishopLikes = true, bool tbAnyBlackKnights = true>
struct alignas(64) FullBitboards
{
	// SolveTwoMover(FEN, pBufOutputMoves)
	// Returns:
	//   -1 on invalid FEN:
	//   0 on #2 checkmate not found 
	//   1 or number of solutions on #2 checkmate found, dependent on template parameter FindAllSolutionsAndFillBuf (if true, passed pointer pBufOutputMoves must point to the buffer that is long enough, ideally 256 elements)
	template<bool FindAllSolutionsAndFillBuf = false>
	int SolveTwoMover(const char* szFEN, TMove* pBufOutputMoves = nullptr)
	{
		const auto res = fromFEN(szFEN);
		if (res.first)
		{
			const auto info = res.second;
			const auto castlingFlags = info.whiteCastlingLongPossible * 8 + info.whiteCastlingShortPossible * 4 + info.blackCastlingLongPossible * 2 + info.blackCastlingShortPossible;			
			
			return SolveTwoMoverDispatcher<FindAllSolutionsAndFillBuf>(IsWhiteKingChecked(), info.enPassantSquare, castlingFlags, pBufOutputMoves);
		}
		else
			return -1;
	}
	// Added two wrappers for SolveTwoMover to avoid special syntax for calling a template method of a template class:
	ALWAYS_INLINE int SolveTwoMover_OneSolution(const char* szFEN)
	{
		return SolveTwoMover<0>(szFEN);
	}
	ALWAYS_INLINE int SolveTwoMover_AllSolutions(const char* szFEN, TMove* pBufOutputMoves = nullptr)
	{
		return SolveTwoMover<1>(szFEN, pBufOutputMoves);
	}

	// Returns -1 on invalid FEN or 1/0.
	ALWAYS_INLINE char IsImmediateCheckMate(const char* szFEN)
	{
		const auto res = fromFEN(szFEN);
		if (res.first)
		{
			const auto info = res.second;
			return IsImmediateCheckMateDispatcher(IsWhiteKingChecked(), info.enPassantSquare, info.whiteCastlingShortPossible, info.whiteCastlingLongPossible);
		}
		else
			return -1;
	}

private:
	Bitboard white;
	Bitboard black;

	Bitboard kings;
	Bitboard pawns;
	Bitboard qbishops; // NOTE: there are no separate bishops, rooks and queens bitboards but only a small methods bishops(), rooks() and queens()
	Bitboard qrooks;   // This approach proved to be beneficial - as a side effect there's an additional space in the struct to fit important reduntant fields posWhiteKing and posBlackKing while keeping data concise (64 bytes) 
	Bitboard knights;

	// Redundancies vs. kings bitboard:
	int posWhiteKing; 
	int posBlackKing; // NOTE: these two fields can be reduced to short or char if needed in order to fit more data within 64 bytes of FullBitboards

	[[nodiscard]] ALWAYS_INLINE Bitboard rooks() CONST_RESTRICT
	{ 
		return qrooks & ~qbishops; 
	} 
	[[nodiscard]] ALWAYS_INLINE Bitboard bishops() CONST_RESTRICT
	{ 
		return qbishops & ~qrooks; 
	}
	[[nodiscard]] ALWAYS_INLINE Bitboard queens() CONST_RESTRICT
	{
		return qbishops & qrooks;
	}

	bool operator == (const FullBitboards&) CONST_RESTRICT = default;

	[[nodiscard]] ALWAYS_INLINE Bitboard occ() CONST_RESTRICT noexcept {
		return white | black;
	}
	void clear()
	{
		white = black = kings = pawns = qbishops = qrooks = knights = 0ULL;
	}
	struct CastlingAndEnPassantPossibilityT
	{
		char enPassantSquare; // -1 if none
		char whiteCastlingShortPossible : 2;
		char whiteCastlingLongPossible : 2;
		char blackCastlingShortPossible : 2;
		char blackCastlingLongPossible : 2;		
	};
	std::pair<bool,CastlingAndEnPassantPossibilityT> fromFEN(const char* szFEN)
	{
		std::pair<bool, CastlingAndEnPassantPossibilityT> res;
		res.first = false; // init status false

		clear();
		int i = 0;
		int line = _8_;
		int file = _1_;
		int pos;
		Bitboard mask;
		int whiteKings = 0;
		int blackKings = 0;

		while (szFEN[i] != 0)
		{
			switch (szFEN[i])
			{
				case ' ': goto labelEndLooping;
				case '/': 
					++i;
					goto labelNextIter;
				case '1': [[fallthrough]];
				case '2': [[fallthrough]];
				case '3': [[fallthrough]];
				case '4': [[fallthrough]];
				case '5': [[fallthrough]];
				case '6': [[fallthrough]];
				case '7': [[fallthrough]];
				case '8':
					file += szFEN[i] - '1';					
					break;
				case 'K':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					white |= mask;
					kings |= mask;
					++whiteKings;					
					break;
				case 'k':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					black |= mask;
					kings |= mask;
					++blackKings;
					break;
				case 'P':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					white |= mask;
					pawns |= mask;
					break;
				case 'p':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					black |= mask;
					pawns |= mask;
					break;
				case 'N':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					white |= mask;
					knights |= mask;
					break;
				case 'n':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					black |= mask;
					knights |= mask;
					break;
				case 'B':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					white |= mask;
					qbishops |= mask;
					break;
				case 'b':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					black |= mask;
					qbishops |= mask;
					break;
				case 'R':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					white |= mask;
					qrooks |= mask;
					break;
				case 'r':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					black |= mask;
					qrooks |= mask;
					break;
				case 'Q':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					white |= mask;
					qrooks |= mask;
					qbishops |= mask;
					break;
				case 'q':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					black |= mask;
					qrooks |= mask;
					qbishops |= mask;
					break;
				default:
					return res;
			}
			++file;
			++i;
			if (file >= 8)
			{
				if (file > 8)
					return res;				
				if (line == 0)
					goto labelEndLooping;
				file = 0;
				--line;
			}
		labelNextIter:;
		}

	labelEndLooping:
		if ((whiteKings != 1) | (blackKings != 1))
			return res;

		posWhiteKing = std::countr_zero(white & kings);
		posBlackKing = std::countr_zero(black & kings);

		while (((szFEN[i] == ' ') | (szFEN[i] == '\n')) & (szFEN[i] != 0))
			++i;
		
		if (szFEN[i] == 0)
		{
			// short FEN (no castling/en passant info) ? If so, then assume castlings available and no en passant
			res.first = true;
			res.second.whiteCastlingLongPossible = (posWhiteKing == _E1_) & (((1ULL << _A1_) & white & rooks()) != 0);
			res.second.whiteCastlingShortPossible = (posWhiteKing == _E1_) & (((1ULL << _H1_) & white & rooks()) != 0);
			res.second.blackCastlingLongPossible = (posBlackKing == _E8_) & (((1ULL << _A8_) & black & rooks()) != 0);
			res.second.blackCastlingShortPossible = (posBlackKing == _E8_) & (((1ULL << _H8_) & black & rooks()) != 0);
			res.second.enPassantSquare = -1;			
		}
		else
		{
			// Castling + en passant:

			if (szFEN[i] != 'w')
				return res; // white should be on move
			++i;
			while (((szFEN[i] == ' ') | (szFEN[i] == '\n')) & (szFEN[i] != 0))
				++i;
			if (szFEN[i] == 0)
				return res; // complete FEN expected

			res.second.whiteCastlingLongPossible = false;
			res.second.whiteCastlingShortPossible = false;
			res.second.blackCastlingLongPossible = false;
			res.second.blackCastlingShortPossible = false;
			while ((szFEN[i] != ' ') & (szFEN[i] != 0))
			{
				switch (szFEN[i])
				{
					case '-': ++i;  goto labelEndCastlingCheck;
					case 'K': res.second.whiteCastlingShortPossible = true; break;
					case 'Q': res.second.whiteCastlingLongPossible = true; break;
					case 'k': res.second.blackCastlingShortPossible = true; break;
					case 'q': res.second.blackCastlingLongPossible = true; break;
					default: return res;
				}
				++i;
			}
		labelEndCastlingCheck:
			res.second.enPassantSquare = -1;
			while (((szFEN[i] == ' ') | (szFEN[i] == '\n')) & (szFEN[i] != 0))
				++i;
			if (szFEN[i] == 0)
				return res; // complete FEN expected
			if (szFEN[i] != '-')
			{
				if ((szFEN[i + 1] < '1') | (szFEN[i + 1] > '8') | (szFEN[i] < 'a') | (szFEN[i] > 'h'))
					return res; // en passant square expected				
				res.second.enPassantSquare = (szFEN[i] - 'a') + (szFEN[i + 1] - '1') * 8 - 8;
				assert(res.second.enPassantSquare >= _A5_ && res.second.enPassantSquare <= _H5_);
			}

			res.first = true; // status ok
		}

		return res;
	}

	template<bool tbSkipKing = false>
	ALWAYS_INLINE FIGURE GetFigureAt(const int pos) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		const auto mask = sq_to_bb(pos);
		assert(mask & occ());

		if constexpr (tbSkipKing)
			return ((pawns & mask) != 0) * FGR_PAWN + ((qbishops & mask) != 0) * FGR_BISHOP + ((qrooks & mask) != 0) * FGR_ROOK + ((knights & mask) != 0) * FGR_KNIGHT;
		else
			return ((kings & mask) != 0) * FGR_KING + ((pawns & mask) != 0) * FGR_PAWN + ((qbishops & mask) != 0) * FGR_BISHOP + ((qrooks & mask) != 0) * FGR_ROOK + ((knights & mask) != 0) * FGR_KNIGHT;
	}
	ALWAYS_INLINE FIGURE GetLongDistanceFigureAt(const int pos) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		const auto mask = sq_to_bb(pos);
		assert(mask & occ());

		const bool bBishopLike = qbishops & mask;
		const bool bRookLike = qrooks & mask;

		return bBishopLike * FGR_BISHOP + bRookLike * FGR_ROOK; // bit shifting twice
	}
	// A version that returns 1 for bishop, 2 for rook and 3 for queen (slightly optimized, since it avoids bit shifting at all)
	ALWAYS_INLINE int GetLongDistanceFigureAtExt(const int pos) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		const auto mask = sq_to_bb(pos);
		assert(mask & occ());

		const bool bBishopLike = qbishops & mask;
		const bool bRookLike = qrooks & mask;

		return bBishopLike + bRookLike + bRookLike;
	}

	ALWAYS_INLINE Bitboard WhitePawnAttacks() CONST_RESTRICT
	{
		constexpr Bitboard NOT_A_FILE = 0xFEFEFEFEFEFEFEFEULL;
		constexpr Bitboard NOT_H_FILE = 0x7F7F7F7F7F7F7F7FULL;

		const Bitboard attack_left = ((white & pawns) << 7) & NOT_H_FILE;
		const Bitboard attack_right = ((white & pawns) << 9) & NOT_A_FILE;
		return attack_left | attack_right;
	}
	ALWAYS_INLINE Bitboard BlackPawnAttacks() CONST_RESTRICT
	{
		constexpr Bitboard NOT_A_FILE = 0xFEFEFEFEFEFEFEFEULL;
		constexpr Bitboard NOT_H_FILE = 0x7F7F7F7F7F7F7F7FULL;

		const Bitboard attack_right = ((black & pawns) >> 7) & NOT_A_FILE;
		const Bitboard attack_left = ((black & pawns) >> 9) & NOT_H_FILE;
		return attack_left | attack_right;
	}
	template<bool tbWhite>
	ALWAYS_INLINE Bitboard KnightAttacks() CONST_RESTRICT
	{
		if constexpr (!tbAnyBlackKnights && !tbWhite)
			return 0ULL;

		constexpr Bitboard NOT_A_FILE = 0xFEFEFEFEFEFEFEFEULL;
		constexpr Bitboard NOT_AB_FILE = 0xFCFCFCFCFCFCFCFCULL;
		constexpr Bitboard NOT_H_FILE = 0x7F7F7F7F7F7F7F7FULL;
		constexpr Bitboard NOT_GH_FILE = 0x3F3F3F3F3F3F3F3FULL;

		const Bitboard white_knights = (tbWhite ? white : black) & knights;// or black_knights, if tbWhite == false
		Bitboard attacks = 0;

		// Grouping by actual column dependencies
		attacks |= ((white_knights & NOT_H_FILE) << 17) | ((white_knights & NOT_H_FILE) >> 15); // Right 1
		attacks |= ((white_knights & NOT_GH_FILE) << 10) | ((white_knights & NOT_GH_FILE) >> 6);  // Right 2
		attacks |= ((white_knights & NOT_A_FILE) << 15) | ((white_knights & NOT_A_FILE) >> 17); // Left 1
		attacks |= ((white_knights & NOT_AB_FILE) << 6) | ((white_knights & NOT_AB_FILE) >> 10); // Left 2

		return attacks;
	}
	ALWAYS_INLINE Bitboard WhiteKnightAttacks() CONST_RESTRICT
	{
		return KnightAttacks<1>();
	}
	ALWAYS_INLINE Bitboard BlackKnightAttacks() CONST_RESTRICT
	{
		return KnightAttacks<0>();
	}
	// Pinning is not verified by this method:
	template<bool tbInclDiscoveredCheck = true>
	ALWAYS_INLINE Bitboard BlackPawnsThatCanCaptureWithCheck(const Bitboard blackDiscoveredCheckers = 0ULL) CONST_RESTRICT
	{
		auto auxMask = (White_Pawn_Attacks[posWhiteKing] & white) << 8;
		constexpr Bitboard NOT_A_FILE = 0xFEFEFEFEFEFEFEFEULL;
		constexpr Bitboard NOT_H_FILE = 0x7F7F7F7F7F7F7F7FULL;
		const auto maskForBlackPawnsThatCanCaptureWithCheck = ((auxMask & NOT_A_FILE) >> 1) | ((auxMask & NOT_H_FILE) << 1);

		if constexpr (tbInclDiscoveredCheck)
		{
			auxMask = white << 8;
			const auto maskForBlackPawnsThanCanCapture = ((auxMask & NOT_A_FILE) >> 1) | ((auxMask & NOT_H_FILE) << 1);		

			const auto blackPawnsThatCanCaptureWithCheck = (maskForBlackPawnsThatCanCaptureWithCheck | (maskForBlackPawnsThanCanCapture & blackDiscoveredCheckers)) & black & pawns;
			return blackPawnsThatCanCaptureWithCheck;
		}
		else
		{
			const auto blackPawnsThatCanCaptureWithCheck = maskForBlackPawnsThatCanCaptureWithCheck & black & pawns;
			return blackPawnsThatCanCaptureWithCheck;
		}
	}

	ALWAYS_INLINE bool AllBetweenEmpty(const int pos1, const int pos2) CONST_RESTRICT
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(pos1 != pos2);

		return (GetBetweenMask(pos1, pos2) & (white | black)) == 0;
	}
	ALWAYS_INLINE bool AllBetweenEmptyIfTakeOffWhitePawn(const int pos1, const int pos2, const int posWhitePawnToTakeOff) CONST_RESTRICT
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(pos1 != pos2);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		const auto mask = (sq_to_bb(posWhitePawnToTakeOff));
		const_cast<FullBitboards*>(this)->white ^= mask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= mask;
		#endif

		const auto res = (GetBetweenMask(pos1, pos2) & (white | black)) == 0;

		const_cast<FullBitboards*>(this)->white ^= mask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= mask;
		#endif

		return res;
	}

	template<bool tbSkipAssertionIfNotSameDiagOrLine = false>
	ALWAYS_INLINE bool AllBetweenEmptyIfTakeOffBlackPawn(const int pos1, const int pos2, const int posBlackPawnToTakeOff) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(IsValidPos(posBlackPawnToTakeOff));
		assert(pos1 != pos2);
		assert((1ULL << posBlackPawnToTakeOff) & black & pawns);
		assert(SameDiagonalOrLine(pos1, pos2) || tbSkipAssertionIfNotSameDiagOrLine);

		const auto mask = (1ULL << posBlackPawnToTakeOff);
		const_cast<FullBitboards*>(this)->black ^= mask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= mask;
		#endif

		const auto res = (GetBetweenMask(pos1, pos2) & (white | black)) == 0;

		const_cast<FullBitboards*>(this)->black ^= mask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= mask; // __JGI_BB_PEDANTIC__
		#endif

		return res;
	}


	template<bool tbIncludeEnds = false>
	ALWAYS_INLINE static constexpr Bitboard GetBetweenMask(const char sq1, const char sq2)
	{
		assert(IsValidPos(sq1));
		assert(IsValidPos(sq2));
		assert(sq1 != sq2);

		const auto res = betweenLookup.GetBetweenMask(sq1, sq2);
		if constexpr (tbIncludeEnds)
			return res | (sq_to_bb(sq1)) | (sq_to_bb(sq2));
		else
			return res;		
	}

	ALWAYS_INLINE static constexpr Bitboard GetCommonDiagOrLine(int sq1, int sq2)
	{
		assert(IsValidPos(sq1));
		assert(IsValidPos(sq2));
		assert(sq1 != sq2);
		const auto res = betweenLookup.GetCommonDiagOrLine(sq1, sq2);
		return res;
	}
	ALWAYS_INLINE static constexpr bool IsSquareOnCommonDiagOrLineOf(int sq, int sq1, int sq2)
	{
		assert(IsValidPos(sq));
		assert(IsValidPos(sq1));
		assert(IsValidPos(sq2));
		assert(sq1 != sq2);
		return betweenLookup.IsSquareOnCommonDiagOrLineOf(sq, sq1, sq2);
	}

	template<bool tbSquaresS1S2KnownToBeOnSameDiagonalOrLine = false, bool tbExcludingEnds = true, bool tbOptim = true>
	ALWAYS_INLINE static constexpr bool IsSquareBetween(const char square, const char s1, const char s2)
	{
		assert(IsValidPos(square));
		assert(IsValidPos(s1));
		assert(IsValidPos(s2));
		assert(s1 != s2); // let's assume that square can be the same as s1 or s2
		assert(!tbSquaresS1S2KnownToBeOnSameDiagonalOrLine || SameDiagonalOrLine(s1, s2));

		if constexpr(tbSquaresS1S2KnownToBeOnSameDiagonalOrLine)
			return GetBetweenMask<!tbExcludingEnds>(s1, s2) & (sq_to_bb(square));
		else
		{
			const auto mask = GetBetweenMask<!tbExcludingEnds>(s1, s2);
			const bool match = (mask & (sq_to_bb(square))) != 0;
			const bool res = (mask != ~0ULL) ? match : false;
			return res;
		}
	}

	ALWAYS_INLINE bool SameLineAndAllBetweenEmpty(const int pos1, const int pos2) CONST_RESTRICT
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(pos1 != pos2);
		
		return AllBetweenEmpty(pos1, pos2) & (SameLine(pos1, pos2));
	}
	ALWAYS_INLINE bool SameDiagAndAllBetweenEmpty(const int pos1, const int pos2) CONST_RESTRICT
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(pos1 != pos2);

		return AllBetweenEmpty(pos1, pos2) & (SameDiag(pos1, pos2));
	}
	template<bool tbUseFlagsOfBlackLongDistanceFigures = false>
	ALWAYS_INLINE bool SameDiagonalOrLineAndAllBetweenEmpty(const int pos1, const int pos2) CONST_RESTRICT
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(pos1 != pos2);

		if constexpr (tbUseFlagsOfBlackLongDistanceFigures)
		{
			if constexpr (!tbBlackHaveBishopLikes && !tbBlackHaveRookLikes)
				return false;
			if constexpr (!tbBlackHaveBishopLikes)
				return SameLineAndAllBetweenEmpty(pos1, pos2);
			if constexpr (!tbBlackHaveRookLikes)
				return SameDiagAndAllBetweenEmpty(pos1, pos2);
		}

		return AllBetweenEmpty(pos1, pos2);
	}
	ALWAYS_INLINE bool SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(const int pos1, const int pos2, const int posWhitePawnToTakeOff) CONST_RESTRICT
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(pos1 != pos2);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		return AllBetweenEmptyIfTakeOffWhitePawn(pos1, pos2, posWhitePawnToTakeOff);
	}
	ALWAYS_INLINE bool SameDiagAndAllBetweenEmptyIfTakeOffWhitePawn(const int pos1, const int pos2, const int posWhitePawnToTakeOff) CONST_RESTRICT
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(pos1 != pos2);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		return AllBetweenEmptyIfTakeOffWhitePawn(pos1, pos2, posWhitePawnToTakeOff) & (SameDiag(pos1, pos2));
	}
	ALWAYS_INLINE bool SameLineAndAllBetweenEmptyIfTakeOffWhitePawn(const int pos1, const int pos2, const int posWhitePawnToTakeOff) CONST_RESTRICT
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(pos1 != pos2);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		return AllBetweenEmptyIfTakeOffWhitePawn(pos1, pos2, posWhitePawnToTakeOff) & (SameLine(pos1, pos2));
	}	
	
	template<bool tbBlack = true, char tbKnownGeneralDir = -1> // -1==unknown, 0 for diagonals, 1 for rows/columns
	ALWAYS_INLINE Bitboard GetCandidatesForLongDistanceFigureInDir(const int pos, const int posBase) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posBase));
		assert(SameDiagonalOrLine(pos, posBase));
		
		if constexpr (tbBlack && !tbBlackHaveRookLikes && !tbBlackHaveBishopLikes)
			return 0ULL;

		if constexpr (tbKnownGeneralDir < 0) // unknown
		{
			const auto [rayMask, matchingPieceBitboard] = rayLookup.GetRayAndMaskForDir(pos, posBase, qrooks, qbishops);
			return rayMask & (tbBlack ? black : white) & matchingPieceBitboard;
		}
		else
		{
			const auto rayMask = GetRay(pos, posBase);
			if constexpr (tbKnownGeneralDir == 0) // diagonal		
				return rayMask & (tbBlack ? black : white) & qbishops;
			else // rookLike dir
				return rayMask & (tbBlack ? black : white) & qrooks;
		}
	}	
	template<char tbKnownGeneralDir = -1> // -1==unknown, 0 for diagonals, 1 for rows/columns
	ALWAYS_INLINE Bitboard GetCandidatesForWhiteLongDistanceFigureInDir(const int pos, const int posBase) CONST_RESTRICT
	{
		return GetCandidatesForLongDistanceFigureInDir<0, tbKnownGeneralDir>(pos, posBase);
	}
	template<char tbKnownGeneralDir = -1> // -1==unknown, 0 for diagonals, 1 for rows/columns
	ALWAYS_INLINE Bitboard GetCandidatesForBlackLongDistanceFigureInDir(const int pos, const int posBase) CONST_RESTRICT
	{
		return GetCandidatesForLongDistanceFigureInDir<1, tbKnownGeneralDir>(pos, posBase);
	}
	// This version returns just a boolean:
	ALWAYS_INLINE bool IsCandidateForLongDistanceFigureInDirValid(const Bitboard mask, const int pos, const int posBase) CONST_RESTRICT
	{
		const bool bRayUpward = pos > posBase;
		const int firstPosMatching = std::countr_zero(mask);
		const int lastPosMatching = 63 - std::countl_zero(mask);
		const int posPiece = bRayUpward ? firstPosMatching : lastPosMatching;
		const bool bAllBetweenEmpty = AllBetweenEmpty(pos, posPiece);
		return bAllBetweenEmpty;
	}
	// This version returns piece position or -1
	ALWAYS_INLINE int ValidateCandidateForLongDistanceFigureInDir(const Bitboard mask, const int pos, const int posBase) CONST_RESTRICT
	{
		const bool bRayUpward = pos > posBase;
		const int firstPosMatching = std::countr_zero(mask);
		const int lastPosMatching = 63 - std::countl_zero(mask);
		const int posPiece = bRayUpward ? firstPosMatching : lastPosMatching;
		const bool bAllBetweenEmpty = AllBetweenEmpty(pos, posPiece);
		return bAllBetweenEmpty ? posPiece : -1;
	}

	template<bool tbGetPos = false, bool tbBlack = true, bool tbKnownToExist = false>
	ALWAYS_INLINE int LongDistanceFigureInDir(const int pos, const int posBase) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posBase));
		assert(SameDiagonalOrLine(pos, posBase));
		static_assert(!tbKnownToExist || tbGetPos); // no need to call with tbKnownToExist==true and tbGetPos==false (guaranteed 1 to be returned then)

		if constexpr (tbBlack && !tbBlackHaveRookLikes && !tbBlackHaveBishopLikes)
			return 0ULL;

		const auto mask = GetCandidatesForLongDistanceFigureInDir<tbBlack>(pos, posBase);
		if (tbKnownToExist || mask)
		{
			const bool bRayUpward = pos > posBase;
			const int posPiece = bRayUpward ? std::countr_zero(mask) : (63 - std::countl_zero(mask));
			if constexpr (tbKnownToExist)
			{
				assert(AllBetweenEmpty(pos, posPiece));
				if constexpr (tbGetPos)
				{
					assert(IsValidPos(posPiece));
					return posPiece;
				}
				else
					return 1;
			}
			else
			{
				const bool bAllBetweenEmpty = AllBetweenEmpty(pos, posPiece);
				if constexpr (tbGetPos)
					return bAllBetweenEmpty ? posPiece : -1;
				else
					return bAllBetweenEmpty;
			}
		}
		if constexpr (tbGetPos)
			return -1;
		else
			return false;
	}

	template<bool tbGetPos = false, bool tbBlack = true, bool tbKnownToExist = false>
	ALWAYS_INLINE int LongDistanceFigureInDir(const int pos, const int dx, const int dy) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(abs(dx) <= 1 && abs(dy) <= 1 && (dx | dy));
		static_assert(!tbKnownToExist || tbGetPos); // no need to call with tbKnownToExist==true and tbGetPos==false (guaranteed 1 to be returned then)

		if constexpr (tbBlack && !tbBlackHaveRookLikes && !tbBlackHaveBishopLikes)
			return 0ULL;

		const auto dir = dirLookup.DirFromDxDy(dx, dy);
		const auto rayMask = rayLookup.GetRayInDir(pos, dir);
		const auto mask = rayMask & (tbBlack ? black : white) & (DirLookup::IsLineDir(dir) ? qrooks : qbishops);
		if (tbKnownToExist || mask)
		{
			assert(mask);
			const bool bRayUpward = DirLookup::IsUpwardDir(dir); // (dy > 0) | ((dy == 0) & (dx > 0));
			const int posPiece = bRayUpward ? std::countr_zero(mask) : (63 - std::countl_zero(mask));
			if constexpr (tbKnownToExist)
			{
				assert(AllBetweenEmpty(pos, posPiece));
				if constexpr (tbGetPos)
				{
					assert(IsValidPos(posPiece));
					return posPiece;
				}
				else
					return 1;
			}
			else
			{			
				const bool bAllBetweenEmpty = AllBetweenEmpty(pos, posPiece);
				if constexpr (tbGetPos)
					return bAllBetweenEmpty ? posPiece : -1;
				else
					return bAllBetweenEmpty;
			}
		}
		
		if constexpr (tbGetPos)
			return -1;
		else
			return false;
	}
	template<bool tbGetPos = false, bool tbKnownToExist = false>
	ALWAYS_INLINE int WhiteLongDistanceFigureInDir(const int pos, const int posBase) CONST_RESTRICT
	{
		return LongDistanceFigureInDir<tbGetPos, 0, tbKnownToExist>(pos, posBase);
	}
	template<bool tbGetPos = false>
	ALWAYS_INLINE int WhiteLongDistanceFigureInDir(const int pos, const int dx, const int dy) CONST_RESTRICT
	{
		return LongDistanceFigureInDir<tbGetPos, 0>(pos, dx, dy);
	}
	template<bool tbGetPos = false, bool tbKnownToExist = false>
	ALWAYS_INLINE int BlackLongDistanceFigureInDir(const int pos, const int posBase) CONST_RESTRICT
	{
		return LongDistanceFigureInDir<tbGetPos, 1, tbKnownToExist>(pos, posBase);
	}
	template<bool tbGetPos = false>
	ALWAYS_INLINE int BlackLongDistanceFigureInDir(const int pos, const int dx, const int dy) CONST_RESTRICT
	{
		return LongDistanceFigureInDir<tbGetPos, 1>(pos, dx, dy);
	}
	template<bool tbGetPos = false>
	ALWAYS_INLINE int WhiteLongDistanceFigureInDirIfTakeOffWhitePawn(const int pos, const int posBase, const int posWhitePawnToTakeOff)
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posBase));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(SameDiagonalOrLine(pos, posBase));
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		const auto mask = (sq_to_bb(posWhitePawnToTakeOff));
		#ifdef __JGI_BB_PEDANTIC__
		pawns ^= mask;
		#endif
		white ^= mask;

		const auto res = WhiteLongDistanceFigureInDir<tbGetPos>(pos, posBase);

		#ifdef __JGI_BB_PEDANTIC__
		pawns ^= mask;
		#endif
		white ^= mask;

		return res;
	}

	template<bool tbGetPos = false>
	ALWAYS_INLINE int WhiteLongDistanceFigureInDirIfTakeOffWhitePawn(const int pos, const int dx, const int dy, const int posWhitePawnToTakeOff)
	{
		assert(IsValidPos(pos));
		assert(abs(dx) <= 1 && abs(dy) <= 1 && (dx | dy));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(posWhitePawnToTakeOff != pos);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		const auto mask = (sq_to_bb(posWhitePawnToTakeOff));
		#ifdef __JGI_BB_PEDANTIC__
		pawns ^= mask;
		#endif
		white ^= mask;

		const auto res = WhiteLongDistanceFigureInDir<tbGetPos>(pos, dx, dy);

		#ifdef __JGI_BB_PEDANTIC__
		pawns ^= mask;
		#endif
		white ^= mask;

		return res;
	}
	template<bool tbGetPos = false>
	ALWAYS_INLINE int BlackLongDistanceFigureInDirIfTakeOffWhitePawn(const int pos, const int posBase, const int posWhitePawnToTakeOff)
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posBase));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(SameDiagonalOrLine(pos, posBase));
		assert(posWhitePawnToTakeOff != pos);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		if constexpr (!tbBlackHaveRookLikes && !tbBlackHaveBishopLikes)
			return 0ULL;

		const auto mask = (sq_to_bb(posWhitePawnToTakeOff));
		#ifdef __JGI_BB_PEDANTIC__
		pawns ^= mask;
		#endif
		white ^= mask;

		const auto res = BlackLongDistanceFigureInDir<tbGetPos>(pos, posBase);

		#ifdef __JGI_BB_PEDANTIC__
		pawns ^= mask;
		#endif
		white ^= mask;

		return res;
	}

	template<bool tbGetPos = false>
	ALWAYS_INLINE int BlackLongDistanceFigureInDirIfTakeOffWhitePawn(const int pos, const int dx, const int dy, const int posWhitePawnToTakeOff)
	{
		assert(IsValidPos(pos));
		assert(abs(dx) <= 1 && abs(dy) <= 1 && (dx | dy));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(posWhitePawnToTakeOff != pos);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		if constexpr (!tbBlackHaveRookLikes && !tbBlackHaveBishopLikes)
			return 0ULL;

		const auto mask = (sq_to_bb(posWhitePawnToTakeOff));
		#ifdef __JGI_BB_PEDANTIC__
		pawns ^= mask;
		#endif
		white ^= mask;

		const auto res = BlackLongDistanceFigureInDir<tbGetPos>(pos, dx, dy);

		#ifdef __JGI_BB_PEDANTIC__
		pawns ^= mask;
		#endif
		white ^= mask;

		return res;
	}

	// Fast verification if there is white potential attacker somewhere on common diag or line. 
	// Returns non-zerp only if boh AllBetweenEmpty and the attacker matches direction (bishop-like for diagonals or rook-like for file/rank)	
	ALWAYS_INLINE Bitboard MatchOnCommonDiagOrLineIfAllBetweenEmpty(const int sq1, const int sq2) CONST_RESTRICT
	{
		assert(IsValidPos(sq1));
		assert(IsValidPos(sq2));
		assert(sq1 != sq2);

		return betweenLookup.MatchOnCommonDiagOrLineIfAllBetweenEmpty(sq1, sq2, qrooks, qbishops, white | black);
	}
	
	ALWAYS_INLINE int WhiteMatchOnRayIfAllBetweenEmpty(const int posBase, const int pos) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posBase));
		assert(pos != posBase);
		
		const auto mask = white & rayLookup.MatchOnRay(pos, posBase, qrooks, qbishops);
		if ((mask != 0) & SameDiagonalOrLineAndAllBetweenEmpty(pos, posBase))
		{
			const bool upRay = pos > posBase;
			const auto posFirst = std::countr_zero(mask);
			const auto posLast = 63 - std::countl_zero(mask);
			const auto posInRay = upRay ? posFirst : posLast;
			const bool bAllBetweenEmpty = AllBetweenEmpty(posInRay, pos);
			return bAllBetweenEmpty ? posInRay : -1;
		}
		return -1;
	}
	ALWAYS_INLINE int BlackMatchOnRayIfAllBetweenEmpty(const int posBase, const int pos) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posBase));
		assert(pos != posBase);

		if constexpr (!tbBlackHaveRookLikes && !tbBlackHaveBishopLikes)
			return 0ULL;

		const auto mask = black & rayLookup.MatchOnRay(pos, posBase, qrooks, qbishops);
		if ((mask != 0) & SameDiagonalOrLineAndAllBetweenEmpty(pos, posBase))
		{
			const bool upRay = pos > posBase;
			const auto posFirst = std::countr_zero(mask);
			const auto posLast = 63 - std::countl_zero(mask);
			const auto posInRay = upRay ? posFirst : posLast;
			const bool bAllBetweenEmpty = AllBetweenEmpty(posInRay, pos);
			return bAllBetweenEmpty ? posInRay : -1;
		}
		return -1;
	}

	template<char tbInclKing = true>
	ALWAYS_INLINE bool IsSquareAttackedByWhiteIfTakeOffBlackKing(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));

		const auto blackKing = black & kings;
		const_cast<FullBitboards*>(this)->black ^= blackKing;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->kings ^= blackKing;
		#endif

		const auto res = IsSquareAttackedByWhite<tbInclKing>(sq);

		const_cast<FullBitboards*>(this)->black ^= blackKing;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->kings ^= blackKing;
		#endif

		return res;
	}
	template<char tbInclKing = true>
	ALWAYS_INLINE bool IsSquareAttackedByBlackIfTakeOffWhiteKing(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));

		const auto whiteKing = white & kings;
		const_cast<FullBitboards*>(this)->white ^= whiteKing;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->kings ^= whiteKing;
		#endif

		const auto res = IsSquareAttackedByBlack<tbInclKing>(sq);

		const_cast<FullBitboards*>(this)->white ^= whiteKing;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->kings ^= whiteKing;
		#endif

		return res;
	}

	// Improved implementations using move generation methods (get_raw_bishop_moves and get_raw_rook_moves)
	// NOTE: There's a trick with tbInclKing<0 - in this case king, pawn and knight attacks are not verified (only long distance attackers)
	template<char tbInclKing = true, bool tbFindAll = true>
	ALWAYS_INLINE Bitboard IsSquareAttackedByWhite_GenMoves(const int target_sq) CONST_RESTRICT
	{
		constexpr bool tbLongDistanceAttackersOnly = tbInclKing < 0; // special value to verify only long distance attackers (see also comments above)
		
		assert(IsValidPos(target_sq));
		
		Bitboard direct_attackers;
		
		if constexpr (!tbLongDistanceAttackersOnly) 
		{
			if constexpr (tbInclKing)
				direct_attackers = ((Knight_Attacks[target_sq] & knights) | (King_Attacks[target_sq] & kings) | (Black_Pawn_Attacks[target_sq] & pawns)) & white;
			else
				direct_attackers = ((Knight_Attacks[target_sq] & knights) | (Black_Pawn_Attacks[target_sq] & pawns)) & white;

			if constexpr (!tbFindAll)
				if (direct_attackers)
					return direct_attackers;
		}		

		const Bitboard occ = white | black;
		const Bitboard direct_b_moves = get_raw_bishop_moves(target_sq, occ);
		const Bitboard direct_r_moves = get_raw_rook_moves(target_sq, occ);

		const Bitboard direct_b_pieces = direct_b_moves & qbishops;
		const Bitboard direct_r_pieces = direct_r_moves & qrooks;

		if constexpr (tbLongDistanceAttackersOnly)
			direct_attackers = direct_b_pieces | direct_r_pieces;	
		else
			direct_attackers |= direct_b_pieces | direct_r_pieces;
		
		return direct_attackers & white;
	}

	template<char tbInclKing = true, bool tbFindAll = true>
	ALWAYS_INLINE Bitboard IsSquareAttackedByBlack_GenMoves(const int target_sq) CONST_RESTRICT
	{
		constexpr bool tbLongDistanceAttackersOnly = tbInclKing < 0; // special value to verify only long distance attackers (see also comments above)
		
		assert(IsValidPos(target_sq));
		
		Bitboard direct_attackers; 
		
		if constexpr (!tbLongDistanceAttackersOnly)
		{				
			if constexpr(!tbAnyBlackKnights)
				if constexpr (tbInclKing)
					direct_attackers = ((King_Attacks[target_sq] & kings) | (White_Pawn_Attacks[target_sq] & pawns)) & black;
				else
					direct_attackers = White_Pawn_Attacks[target_sq] & pawns & black;
			else
				if constexpr (tbInclKing)
					direct_attackers = ((Knight_Attacks[target_sq] & knights) | (King_Attacks[target_sq] & kings) | (White_Pawn_Attacks[target_sq] & pawns)) & black;
				else
					direct_attackers = ((Knight_Attacks[target_sq] & knights) | (White_Pawn_Attacks[target_sq] & pawns)) & black;
	
			if constexpr (!tbBlackHaveBishopLikes && !tbBlackHaveRookLikes)
				return direct_attackers;

			if constexpr (!tbFindAll)
				if (direct_attackers)
					return direct_attackers;
		}

		if constexpr (!tbBlackHaveBishopLikes && !tbBlackHaveRookLikes)
			return 0ULL;

		Bitboard occ = white | black;
		Bitboard direct_b_moves = tbBlackHaveBishopLikes ? get_raw_bishop_moves(target_sq, occ) : 0ULL;
		Bitboard direct_r_moves = tbBlackHaveRookLikes ? get_raw_rook_moves(target_sq, occ) : 0ULL;

		Bitboard direct_b_pieces = direct_b_moves & qbishops;
		Bitboard direct_r_pieces = direct_r_moves & qrooks;

		if constexpr (tbLongDistanceAttackersOnly)
			direct_attackers = direct_b_pieces | direct_r_pieces;
		else
			direct_attackers |= direct_b_pieces | direct_r_pieces;
		
		return direct_attackers & black;
	}

	// bitmask of attackers is returned, even if tbOneIsEnough = false provided that tbOneIsEnough > 1 (see tbReturnBitmaskEvenIfOneIsEnough)
	template<char tbInclKing = true, bool tbInclPinned = true, char tbOneIsEnough = true>
	ALWAYS_INLINE Bitboard IsSquareAttackedByWhite(const int sq) CONST_RESTRICT
	{
		static_assert(tbInclKing >= 0 || tbInclPinned, "Not implemented"); // special value tbInclKing < 0 for long distance attackers only is implemented only for tbInclPinned == true
		
		constexpr bool tbReturnBitmaskEvenIfOneIsEnough = tbOneIsEnough > 1;
		assert(IsValidPos(sq));

		if constexpr (tbInclPinned) // TODO: use IsSquareAttackedByWhite_GenMoves also when pinned attackers must be excluded
		{
			constexpr bool tbFindAll = !tbOneIsEnough;
			const auto mask = IsSquareAttackedByWhite_GenMoves<tbInclKing, tbFindAll>(sq);
			if constexpr (tbOneIsEnough == 1)
				return mask != 0;
			else
				return mask;
		}
		
		Bitboard mask;		
		if constexpr (tbInclKing > 0)
			mask = ((Black_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights) | (King_Attacks[sq] & kings)) & white;
		else
			mask = ((Black_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights)) & white;

		Bitboard res = 0;

		if constexpr (tbInclPinned && tbOneIsEnough)
		{
			if (mask)
				if constexpr (tbReturnBitmaskEvenIfOneIsEnough)
					return mask;
				else
					return 1;
		}
		else
		{
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				if (tbInclPinned || (tbInclKing && IsKingAt(pos)) || !IsWhitePinned(pos, sq))
					if constexpr (tbOneIsEnough)
					{
						if constexpr (tbReturnBitmaskEvenIfOneIsEnough)
							return sq_to_bb(pos);
						else
							return 1;
					}
					else
						res |= sq_to_bb(pos);
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		auto maskLongDist = ((Rook_Attacks[sq] & qrooks) | (Bishop_Attacks[sq] & qbishops)) & white;

		BEGIN_FOR_EACH_POS_IN_MASK(pos, maskLongDist)
		{
			if constexpr (tbOneIsEnough && !tbReturnBitmaskEvenIfOneIsEnough && tbInclPinned)
				res |= (uint64_t)AllBetweenEmpty(pos, sq); // TODO: is it better than branching? make PerfTest
			else
				if (AllBetweenEmpty(pos, sq))
					if (tbInclPinned || !IsWhitePinned(pos, sq))
						if constexpr (tbOneIsEnough)
							if constexpr (tbReturnBitmaskEvenIfOneIsEnough)
								return sq_to_bb(pos);
							else
								return 1;
						else
							res |= sq_to_bb(pos);
		}
		END_FOR_EACH_POS_IN_MASK(pos, maskLongDist);

		return res;
	}

	// Bitmask of attackers is returned - even if tbOneIsEnough = false provided that tbOneIsEnough > 1 (see tbReturnBitmaskEvenIfOneIsEnough)
	// For consistency with legacy methods, the square occupied by black king is considered attacked (when tbInclKing) - that's why bitboards King_Attacks_Ext are used, instead of King_Attacks
	template<char tbInclKing = true, bool tbInclPinned = true, char tbOneIsEnough = true>
	ALWAYS_INLINE Bitboard IsSquareAttackedByBlack(const int sq) CONST_RESTRICT
	{
		static_assert(tbInclKing >= 0 || tbInclPinned, "Not implemented"); // special value tbInclKing < 0 for long distance attackers only is implemented only for tbInclPinned == true
		
		constexpr bool tbReturnBitmaskEvenIfOneIsEnough = tbOneIsEnough > 1;
		assert(IsValidPos(sq));

		if constexpr (tbInclPinned) // TODO: use IsSquareAttackedByBlack_GenMoves also when pinned attackers must be excluded
		{
			constexpr bool tbFindAll = !tbOneIsEnough;
			const auto mask = IsSquareAttackedByBlack_GenMoves<tbInclKing, tbFindAll>(sq);			
			if constexpr (tbOneIsEnough == 1)
				return mask != 0;
			else
				return mask;						
		}
		
		Bitboard mask;
		if constexpr (!tbAnyBlackKnights)
			if constexpr (tbInclKing > 0)
				mask = ((White_Pawn_Attacks[sq] & pawns) | (King_Attacks_Ext[sq] & kings)) & black;
			else
				mask = White_Pawn_Attacks[sq] & pawns & black;
		else
			if constexpr (tbInclKing > 0)
				mask = ((White_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights) | (King_Attacks_Ext[sq] & kings)) & black;
			else
				mask = ((White_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights)) & black;

		Bitboard res = 0;

		if constexpr (tbInclPinned && tbOneIsEnough)
		{
			if (mask)
				if constexpr (tbReturnBitmaskEvenIfOneIsEnough)
					return mask;
				else
					return 1;
		}
		else
		{
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				if (tbInclPinned || (tbInclKing && IsKingAt(pos)) || !IsBlackPinned(pos, sq))
					if constexpr (tbOneIsEnough)
					{
						if constexpr (tbReturnBitmaskEvenIfOneIsEnough)
							return sq_to_bb(pos);
						else
							return 1;
					}
					else
						res |= sq_to_bb(pos);
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		auto maskLongDist = ((Rook_Attacks[sq] & qrooks) | (Bishop_Attacks[sq] & qbishops)) & black;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, maskLongDist)
		{
			if constexpr (tbOneIsEnough && !tbReturnBitmaskEvenIfOneIsEnough && tbInclPinned)
				res |= (uint64_t)AllBetweenEmpty(pos, sq); // TODO: is it better than branching? make PerfTest
			else
				if (AllBetweenEmpty(pos, sq))
					if (tbInclPinned || !IsBlackPinned(pos, sq))
						if constexpr (tbOneIsEnough)
						{
							if constexpr (tbReturnBitmaskEvenIfOneIsEnough)
								return sq_to_bb(pos);
							else
								return 1;
						}
						else
							res |= sq_to_bb(pos);
		}
		END_FOR_EACH_POS_IN_MASK(pos, maskLongDist);

		return res;
	}

	ALWAYS_INLINE bool IsEmptyAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((white | black) & (sq_to_bb(sq))) == 0;
	}
	ALWAYS_INLINE bool IsBlackAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & black) != 0;
	}
	ALWAYS_INLINE bool IsWhiteAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white) != 0;
	}
	ALWAYS_INLINE bool IsWhitePawnAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & pawns) != 0;
	}
	ALWAYS_INLINE bool IsBlackPawnAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & black & pawns) != 0;
	}
	ALWAYS_INLINE bool IsKingAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & kings) != 0;
	}
	ALWAYS_INLINE bool IsWhiteKingAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & kings) != 0;
	}
	ALWAYS_INLINE bool IsWhiteRookAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & rooks()) != 0;
	}
	ALWAYS_INLINE bool IsWhiteBishopAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & bishops()) != 0;
	}
	ALWAYS_INLINE bool IsWhiteKnightAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & knights) != 0;
	}
	ALWAYS_INLINE bool IsBlackBishopAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & black & bishops()) != 0;
	}
	ALWAYS_INLINE bool IsBlackRookAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & black & rooks()) != 0;
	}
	ALWAYS_INLINE bool IsWhiteQueenAt(const int sq) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & queens()) != 0;
	}
	ALWAYS_INLINE bool IsBlackAbsolutelyPinned(const int pos) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(black & kings);
		assert((sq_to_bb(pos)) & black);

		if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, pos))		
			if (const auto mask = GetCandidatesForWhiteLongDistanceFigureInDir(pos, posBlackKing))
				return IsCandidateForLongDistanceFigureInDirValid(mask, pos, posBlackKing);		

		return false;
	}
	ALWAYS_INLINE bool IsWhiteAbsolutelyPinned(const int pos) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(white & kings);
		assert((sq_to_bb(pos)) & white);

		if constexpr (tbBlackHaveBishopLikes || tbBlackHaveRookLikes)		
			if (SameDiagonalOrLineAndAllBetweenEmpty<1>(posWhiteKing, pos))
			{
				constexpr char tbGeneralDir = (!tbBlackHaveBishopLikes) ? 1 : ((!tbBlackHaveRookLikes) ? 0 : -1); // let's use any prior/compile-time knowledge we have
				if (const auto mask = GetCandidatesForBlackLongDistanceFigureInDir<tbGeneralDir>(pos, posWhiteKing))
					return IsCandidateForLongDistanceFigureInDirValid(mask, pos, posWhiteKing);
			}
				
		return false;
	}

	// En passant is not verified here (use IsBlackPawnPinned in such case)
	ALWAYS_INLINE bool IsBlackPinned(const int pos, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posTo));
		assert(pos != posTo);
		assert(black & kings);
		assert((sq_to_bb(pos)) & black);
				
		#if defined(__VERIFY_PINNING_WITHMOVEGEN__) 
		if constexpr (MoveGenMethod == MoveGenMethodT::DenseFancyMagics)
		{
			const auto occu = occ();
			const auto maskWhiteLongDistAttackers = ((get_raw_bishop_moves(pos, occu) & white & qbishops) | (get_raw_rook_moves(pos, occu) & white & qrooks)) & GetCommonDiagOrLine(pos, posBlackKing);
			if ((maskWhiteLongDistAttackers != 0) & SameDiagonalOrLineAndAllBetweenEmpty(pos, posBlackKing))
				return !IsSquareOnCommonDiagOrLineOf(posTo, pos, posBlackKing);
			else
				return false;
		}
		#endif

		int posLongDistanceAttacker;

		#ifdef __VERIFY_PINNING_PREREQUISITE__		
		if ((posLongDistanceAttacker = WhiteMatchOnRayIfAllBetweenEmpty(posBlackKing, pos)) >= 0)
		#else		
		if (!is_edge_and_not_same_edge<tbUseIsEdgeForIsPinned>(pos, posBlackKing) & SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, pos))		
			if ((posLongDistanceAttacker = WhiteLongDistanceFigureInDir<1>(pos, posBlackKing)) >= 0)		
		#endif	
				return !IsSquareOnCommonDiagOrLineOf(posTo, pos, posBlackKing);


		return false;
	}
	// En passant is not verified here (use IsWhitePawnPinned in such case or better IsWhitePinnedIfTakeOffBlackPawn)
	template<bool tbSkipAssertionForEnPassant = false>
	ALWAYS_INLINE bool IsWhitePinned(const int pos, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posTo));
		assert(pos != posTo);
		assert(white & kings);
		assert((sq_to_bb(pos)) & white);
		assert(tbSkipAssertionForEnPassant || ((sq_to_bb(pos)) & white & pawns) == 0 || (pos & 7) == (posTo & 7) || IsBlackAt(posTo)); // do not call this method for en passant! (see IsWhitePawnPinned)
		
		if constexpr (!tbBlackHaveBishopLikes && !tbBlackHaveRookLikes)
			return false;

		#if defined(__VERIFY_PINNING_WITHMOVEGEN__)
		if constexpr (MoveGenMethod == MoveGenMethodT::DenseFancyMagics)
		{
			const auto occu = occ();
			const auto maskBlackLongDistAttackers = ((get_raw_bishop_moves(pos, occu) & black & qbishops) | (get_raw_rook_moves(pos, occu) & black & qrooks)) & GetCommonDiagOrLine(pos, posWhiteKing);
			if ((maskBlackLongDistAttackers != 0) & SameDiagonalOrLineAndAllBetweenEmpty(pos, posWhiteKing))
				return !IsSquareOnCommonDiagOrLineOf(posTo, pos, posWhiteKing);
			else
				return false;
		}
		#endif
		
		return BlackMatchOnRayIfAllBetweenEmpty(posWhiteKing, pos) >= 0 && !IsSquareOnCommonDiagOrLineOf(posTo, pos, posWhiteKing);
	}
	// Mainly for en passant:
	bool IsWhitePawnPinned(const int pos, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posTo));
		assert(pos != posTo);
		assert((sq_to_bb(pos)) & white & pawns);

		if constexpr (!tbBlackHaveBishopLikes && !tbBlackHaveRookLikes)
			return false;

		const bool bEnPassant = (!SameFile(pos, posTo)) & IsEmptyAt(posTo);
		if (bEnPassant)		
			return IsWhitePinnedIfTakeOffBlackPawn(pos, posTo, posTo - 8);
		else
			return IsWhitePinned(pos, posTo);
	}

	ALWAYS_INLINE bool IsBlackPinnedIfTakeOffWhitePawn(const int pos, const int posTo, const int posWhitePawnToTakeOff) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posTo));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(pos != posTo);
		assert(black & kings);
		assert((sq_to_bb(pos)) & black);
		assert(posWhitePawnToTakeOff != pos && posWhitePawnToTakeOff != posTo);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		const auto whitePawnMask = sq_to_bb(posWhitePawnToTakeOff);
		const_cast<FullBitboards*>(this)->white ^= whitePawnMask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= whitePawnMask;
		#endif

		const auto res = IsBlackPinned(pos, posTo);

		const_cast<FullBitboards*>(this)->white ^= whitePawnMask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= whitePawnMask;
		#endif

		return res;
	}
	template<bool tbSkipAssertionForEnPassant = false>
	ALWAYS_INLINE bool IsWhitePinnedIfTakeOffBlackPawn(const int pos, const int posTo, const int posBlackPawnToTakeOff) CONST_RESTRICT
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posTo));
		assert(IsValidPos(posBlackPawnToTakeOff));
		assert(pos != posTo);
		assert(black & kings);
		assert((sq_to_bb(pos)) & white);
		assert(posBlackPawnToTakeOff != pos && posBlackPawnToTakeOff != posTo);
		assert((sq_to_bb(posBlackPawnToTakeOff)) & black & pawns);

		const auto blackPawnMask = sq_to_bb(posBlackPawnToTakeOff);
		const_cast<FullBitboards*>(this)->black ^= blackPawnMask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= blackPawnMask;
		#endif

		const auto res = IsWhitePinned<tbSkipAssertionForEnPassant>(pos, posTo);

		const_cast<FullBitboards*>(this)->black ^= blackPawnMask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= blackPawnMask;
		#endif

		return res;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackLongDistFigure(const int from, const int to) CONST_RESTRICT
	{
		assert(IsValidPos(from));
		assert(IsValidPos(to));
		assert(to != from);
		
		if constexpr (!tbBlackHaveRookLikes)
		{
			assert(IsBlackBishopAt(from));
			return IsImmediateMateAfterMoveByBlackBishop<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible, tbKnownThatItIsNotACapture>(from, to);
		}
		if constexpr (!tbBlackHaveBishopLikes)
		{
			assert(IsBlackRookAt(from));
			return IsImmediateMateAfterMoveByBlackRook<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible, tbKnownThatItIsNotACapture>(from, to);
		}

		const auto f = GetLongDistanceFigureAtExt(from);
		switch (f)
		{
			case 1: // bishop
				return IsImmediateMateAfterMoveByBlackBishop<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible, tbKnownThatItIsNotACapture>(from, to);
			case 2: // rook
				return IsImmediateMateAfterMoveByBlackRook<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible, tbKnownThatItIsNotACapture>(from, to);
			case 3: // queen
				return IsImmediateMateAfterMoveByBlackQueen<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible, tbKnownThatItIsNotACapture>(from, to);
		}
		
		assert(false);
		return false;
	}

	ALWAYS_INLINE bool IsDirectCheckByBlackQueen(const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(toPos));
		
		const auto res = SameDiagonalOrLineAndAllBetweenEmpty(toPos, posWhiteKing);
		return res;
	}
	// Returns -1 if no check after a move by black rook. Otherwise returns checker position or DBL_CHECKED.
	ALWAYS_INLINE int IsCheckByBlackRook(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		
		const bool bDirectCheck = SameLineAndAllBetweenEmpty(toPos, posWhiteKing);
		if constexpr(tbBlackHaveBishopLikes)
			if (SameDiagAndAllBetweenEmpty(fromPos, posWhiteKing)) // prerequisite for discovered check by rook - a highly predictable branch (likely false), so it rather shouldn't be removed		
				if (const auto mask = get_raw_bishop_moves(posWhiteKing, occ()) & black & qbishops)
				{
					assert(std::popcount(mask) == 1);
					const auto posDiscoveredChecker = std::countr_zero(mask);
					return bDirectCheck ? DBL_CHECKED : posDiscoveredChecker;
				}		

		return bDirectCheck ? toPos : -1;
	}
	// Returns -1 if no check after a move by black bishop. Otherwise returns checker position or DBL_CHECKED.
	ALWAYS_INLINE int IsCheckByBlackBishop(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		
		const bool bDirectCheck = SameDiagAndAllBetweenEmpty(toPos, posWhiteKing);
		if constexpr(tbBlackHaveRookLikes)
			if (SameLineAndAllBetweenEmpty(fromPos, posWhiteKing)) // prerequisite for discovered check by bishop - a highly predictable branch (likely false), so it rather shouldn't be removed		
				if (const auto mask = get_raw_rook_moves(posWhiteKing, occ()) & black & qrooks)
				{
					assert(std::popcount(mask) == 1);
					const auto posDiscoveredChecker = std::countr_zero(mask);
					return bDirectCheck ? DBL_CHECKED : posDiscoveredChecker;
				}		

		return bDirectCheck ? toPos : -1;
	}
	ALWAYS_INLINE int IsCheckByBlackKnight(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		
		const bool bDirectCheck = IsKnightDiff(posWhiteKing, toPos);

		if constexpr (tbBlackHaveBishopLikes | tbBlackHaveRookLikes) // prerequisite for discovered check
		{			
			const auto dir = dirLookup.GetDir(posWhiteKing, fromPos);
			const auto ray = rayLookup.GetRayInDir(fromPos, dir);
			const auto match = ray & (DirLookup::IsLineDir(dir) ? (tbBlackHaveRookLikes ? black & qrooks : 0) : (tbBlackHaveBishopLikes ? black & qbishops : 0));

			if (SameDiagonalOrLineAndAllBetweenEmpty<1>(fromPos, posWhiteKing) & (match != 0))
			{
				const bool upDir = fromPos > posWhiteKing;
				const auto posDiscoveredChecker = upDir ? std::countr_zero(match) : (63 - std::countl_zero(match));
				if (AllBetweenEmpty(posDiscoveredChecker, fromPos))
					return bDirectCheck ? DBL_CHECKED : posDiscoveredChecker;				
			}
		}

		return bDirectCheck ? toPos : -1;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackQueen(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(!IsBlackAt(toPos));

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownThatItIsNotACapture)
		{		
			const auto captureMask = white & toMask;
	
			const auto bbSaved = *this; // save
	
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_EMPTY>(captureMask);
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->qrooks ^= moveMask;
			const_cast<FullBitboards*>(this)->qbishops ^= moveMask;
			const_cast<FullBitboards*>(this)->white ^= captureMask;
	
			// Verify if white king checked and dispatch to proper template version:
			const bool bDirectCheck = IsDirectCheckByBlackQueen(toPos);		
			if (bDirectCheck)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(toPos);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
	
			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert(IsEmptyAt(toPos));
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->qrooks ^= moveMask;
			const_cast<FullBitboards*>(this)->qbishops ^= moveMask;

			// Verify if white king checked and dispatch to proper template version:
			const bool bDirectCheck = IsDirectCheckByBlackQueen(toPos);
			if (bDirectCheck)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(toPos);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->qrooks ^= moveMask;
			const_cast<FullBitboards*>(this)->qbishops ^= moveMask;
		}
		
		return res;
	}
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackRook(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(!IsBlackAt(toPos));

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownThatItIsNotACapture)
		{		
			const auto captureMask = white & toMask;
	
			const auto bbSaved = *this; // save
	
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_EMPTY>(captureMask);
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->qrooks ^= moveMask;
			const_cast<FullBitboards*>(this)->white ^= captureMask;
	
			// Find checker(s) and dispatch to proper template version:
			const auto posWhiteKingChecker = IsCheckByBlackRook(fromPos, toPos);
			if (posWhiteKingChecker >= 0)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
	
			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert(IsEmptyAt(toPos));
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->qrooks ^= moveMask;

			// Find checker(s) and dispatch to proper template version:
			const auto posWhiteKingChecker = IsCheckByBlackRook(fromPos, toPos);
			if (posWhiteKingChecker >= 0)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->qrooks ^= moveMask;			
		}
		
		return res;
	}
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackBishop(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(!IsBlackAt(toPos));

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownThatItIsNotACapture)
		{		
			const auto captureMask = white & toMask;
	
			const auto bbSaved = *this; // save
	
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_EMPTY>(captureMask);
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->qbishops ^= moveMask;
			const_cast<FullBitboards*>(this)->white ^= captureMask;
	
			// First find checker(s) and dispatch to proper template version:
			const auto posWhiteKingChecker = IsCheckByBlackBishop(fromPos, toPos);		
			if (posWhiteKingChecker >= 0)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
	
			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert(IsEmptyAt(toPos));
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->qbishops ^= moveMask;

			// First find checker(s) and dispatch to proper template version:
			const auto posWhiteKingChecker = IsCheckByBlackBishop(fromPos, toPos);			
			if (posWhiteKingChecker >= 0)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->qbishops ^= moveMask;
		}
		
		return res;
	}
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackKnight(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(!IsBlackAt(toPos));

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownThatItIsNotACapture)
		{
			const auto captureMask = white & toMask;
	
			const auto bbSaved = *this; // save
	
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_EMPTY>(captureMask);
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->knights ^= moveMask;
			const_cast<FullBitboards*>(this)->white ^= captureMask;
	
			// First find checker(s) and dispatch to proper template version:
			const auto posWhiteKingChecker = IsCheckByBlackKnight(fromPos, toPos);
			if (posWhiteKingChecker >= 0)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
	
			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert(IsEmptyAt(toPos));
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->knights ^= moveMask;

			// First find checker(s) and dispatch to proper template version:
			const auto posWhiteKingChecker = IsCheckByBlackKnight(fromPos, toPos);			
			if (posWhiteKingChecker >= 0)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->knights ^= moveMask;			
		}
		
		return res;
	}

	// NOTE: It verifies if it is a promo move and in such case up to 4 attempts are made to prevent checkmate
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterCaptureByBlackPawn(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert((fromPos >> 3) - (toPos >> 3) == 1); // call IsImmediateMateAfterLongMoveByBlackPawn for a long move by black pawn
		assert((fromPos & 7) != (toPos & 7));

		if (fromPos <= _H2_)
			return IsImmediateMateAfterCaptureWithPromo<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(fromPos, toPos);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos)); // captureMask at the same time
		const auto moveMask = fromMask | toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_EMPTY>(toMask);
		const_cast<FullBitboards*>(this)->black ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;
		const_cast<FullBitboards*>(this)->white ^= toMask;

		// TODO: maybe find checker(s) and dispatch to proper template version?
		const auto res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterCaptureWithPromo(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert((fromPos >> 3) - (toPos >> 3) == 1);
		assert((fromPos & 7) != (toPos & 7));
		assert(fromPos <= _H2_);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->black ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->white ^= toMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_PAWN>(toMask);

		// 1) Promo to queen
		const_cast<FullBitboards*>(this)->qrooks ^= toMask;
		const_cast<FullBitboards*>(this)->qbishops ^= toMask;
		bool res;		
		#ifdef __USE_OPTIMFORMISSINGBLACKLONGDISTANCEFIGURES__
		if constexpr(!tbBlackHaveBishopLikes || !tbBlackHaveRookLikes)
			res = reinterpret_cast<const FullBitboards<MoveGenMethod,1,1>*>(this)->template FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
		else
		#endif
		// TODO: maybe find checker(s) and dispatch to proper template version?
			res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

		if (res)
		{
			// 2) Try promo to knight (no need to check bishop and rook for immediate checkmate)
			const_cast<FullBitboards*>(this)->qrooks ^= toMask;
			const_cast<FullBitboards*>(this)->qbishops ^= toMask;
			const_cast<FullBitboards*>(this)->knights ^= toMask;			
			#ifdef __USE_OPTIMFORMISSINGBLACKLONGDISTANCEFIGURES__
			if constexpr(!tbAnyBlackKnights)
				res = reinterpret_cast<const FullBitboards<MoveGenMethod, tbBlackHaveRookLikes, tbBlackHaveBishopLikes, 1>*>(this)->template FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
			else
			#endif
				// TODO: maybe find checker(s) and dispatch to proper template version?
				res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
		}

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterPromoMoveForwardByBlackPawn(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert((fromPos >> 3) - (toPos >> 3) == 1); // call IsImmediateMateAfterLongMoveByBlackPawn for a long move by black pawn
		assert(fromPos <= _H2_);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;

		const_cast<FullBitboards*>(this)->black ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;

		// 1) Promo to queen
		const_cast<FullBitboards*>(this)->qrooks |= toMask;
		const_cast<FullBitboards*>(this)->qbishops |= toMask;
		bool res;
		#ifdef __USE_OPTIMFORMISSINGBLACKLONGDISTANCEFIGURES__
		if constexpr(!tbBlackHaveBishopLikes || !tbBlackHaveRookLikes)
			res = reinterpret_cast<const FullBitboards<MoveGenMethod,1,1>*>(this)->template FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
		else
		#endif
			// TODO: maybe find checker(s) and dispatch to proper template version?
			res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(); // TODO: maybe find checker(s) and dispatch to proper template version?

		const_cast<FullBitboards*>(this)->qrooks ^= toMask;
		const_cast<FullBitboards*>(this)->qbishops ^= toMask;
		if (res)
		{
			// 2) Try promo to knight (no need to check bishop and rook for immediate checkmate)

			const_cast<FullBitboards*>(this)->knights ^= toMask;			
			#ifdef __USE_OPTIMFORMISSINGBLACKLONGDISTANCEFIGURES__
			if constexpr(!tbAnyBlackKnights)
				res = reinterpret_cast<const FullBitboards<MoveGenMethod, tbBlackHaveRookLikes, tbBlackHaveBishopLikes, 1>*>(this)->template FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
			else
			#endif
				// TODO: maybe find checker(s) and dispatch to proper template version?
				res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
			const_cast<FullBitboards*>(this)->knights ^= toMask;
		}

		const_cast<FullBitboards*>(this)->black ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;

		return res;
	}

	ALWAYS_INLINE int GetWhiteKingCheckerAfterBlackPawnMoveForward(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert((fromPos & 7) == (toPos & 7));
		assert((black & pawns & (sq_to_bb(fromPos))) == 0); // after the move
		assert((black & pawns & (sq_to_bb(toPos))) != 0); // after the move

		const bool bDirectCheck = (white & kings & Black_Pawn_Attacks[toPos]) != 0;

		if constexpr (tbBlackHaveBishopLikes || tbBlackHaveRookLikes)
		{
			assert(SameDiagonalOrLineAndAllBetweenEmpty(fromPos, posWhiteKing) + bDirectCheck < 2);

			bool bPrerequisiteForDiscoveredCheck;
			if constexpr (!tbBlackHaveBishopLikes)
				bPrerequisiteForDiscoveredCheck = ((fromPos >> 3) == (posWhiteKing >> 3)) & AllBetweenEmpty(fromPos, posWhiteKing);
			else
				bPrerequisiteForDiscoveredCheck = SameDiagonalOrLineAndAllBetweenEmpty<1>(fromPos, posWhiteKing);

			if (bPrerequisiteForDiscoveredCheck)
			{
				const auto posDiscoveredChecker = BlackLongDistanceFigureInDir<1>(fromPos, posWhiteKing); // verification is AFTER making move on bitboard, so no need to bother with a move along the line			
				const int posWhiteKingChecker = bDirectCheck ? toPos : posDiscoveredChecker; // no chance for double check in case of a pawn move forward
				return posWhiteKingChecker;
			}
		}
		
		return bDirectCheck ? toPos : -1;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbVerifyIfPromo = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveForwardByBlackPawn(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert((fromPos >> 3) - (toPos >> 3) == 1); // call IsImmediateMateAfterLongMoveByBlackPawn for a long move by black pawn
		assert((fromPos & 7) == (toPos & 7));

		if constexpr (tbVerifyIfPromo)
			if (toPos <= _H1_)
				return IsImmediateMateAfterPromoMoveForwardByBlackPawn< tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(fromPos, toPos);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;

		const_cast<FullBitboards*>(this)->black ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;

		// First find potential checker and dispatch to proper template version:
		const auto posWhiteKingChecker = GetWhiteKingCheckerAfterBlackPawnMoveForward(fromPos, toPos);
		bool res;

		if (posWhiteKingChecker >= 0)
			res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
		else
			res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

		const_cast<FullBitboards*>(this)->black ^= moveMask; // restore
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;

		return res;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterLongMoveByBlackPawn(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(fromPos - toPos == 16 && fromPos >= _A7_);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;

		const_cast<FullBitboards*>(this)->black ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;

		// First find potential checker and dispatch to proper template version:
		const auto posWhiteKingChecker = GetWhiteKingCheckerAfterBlackPawnMoveForward(fromPos, toPos);
		bool res;
		if (posWhiteKingChecker >= 0)
			res = FindMoveThatMates<1, 1, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker, toPos);
		else
			res = FindMoveThatMates<0, 1, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(-1, toPos);

		const_cast<FullBitboards*>(this)->black ^= moveMask; // restore
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;

		return res;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterBlackEnPassant(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert((fromPos >> 3) - (toPos >> 3) == 1); // call IsImmediateMateAfterLongMoveByBlackPawn for a long move by black pawn
		assert((fromPos & 7) != (toPos & 7));

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos)); // captureMask at the same time
		const auto moveMask = fromMask | toMask;
		const auto captureMask = sq_to_bb((toPos & 7) + (fromPos >> 3) * 8);
		assert(white & pawns & captureMask);

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->black ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;
		const_cast<FullBitboards*>(this)->white ^= captureMask;
		const_cast<FullBitboards*>(this)->pawns ^= captureMask;

		// TODO: maybe find checker(s) and dispatch to proper template version?
		const auto res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible & 1, tbWhiteShortCastlingPossible & 2>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterBlackCastlingShort() CONST_RESTRICT
	{
		assert(posBlackKing == _E8_);
		assert(IsBlackRookAt(_H8_));
		
		constexpr auto fromMask = (1ULL << _E8_);
		constexpr auto toMask = (1ULL << _G8_);
		constexpr auto kingMoveMask = fromMask | toMask;

		constexpr auto fromMaskRook = (1ULL << _H8_);
		constexpr auto toMaskRook = (1ULL << _F8_);
		constexpr auto rookMoveMask = fromMaskRook | toMaskRook;

		const_cast<FullBitboards*>(this)->black ^= (kingMoveMask | rookMoveMask);
		const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= rookMoveMask;
		const_cast<FullBitboards*>(this)->posBlackKing = _G8_;

		// Verify if wh.king checked and dispatch template version:		
		const bool bCheck = SameLineAndAllBetweenEmpty(posWhiteKing, _F8_);
		bool res;
		if (bCheck)
			res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(_F8_);
		else
			res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

		const_cast<FullBitboards*>(this)->black ^= (kingMoveMask | rookMoveMask);
		const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= rookMoveMask;
		const_cast<FullBitboards*>(this)->posBlackKing = _E8_;

		return res;
	}
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterBlackCastlingLong() CONST_RESTRICT
	{
		assert(posBlackKing == _E8_);
		assert(IsBlackRookAt(_A8_));
		
		constexpr auto fromMask = (1ULL << _E8_);
		constexpr auto toMask = (1ULL << _C8_);
		constexpr auto kingMoveMask = fromMask | toMask;

		constexpr auto fromMaskRook = (1ULL << _A8_);
		constexpr auto toMaskRook = (1ULL << _D8_);
		constexpr auto rookMoveMask = fromMaskRook | toMaskRook;

		const_cast<FullBitboards*>(this)->black ^= (kingMoveMask | rookMoveMask);
		const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= rookMoveMask;
		const_cast<FullBitboards*>(this)->posBlackKing = _C8_;

		// Verify if wh.king checked and dispatch template version:		
		const bool bCheck = SameLineAndAllBetweenEmpty(posWhiteKing, _D8_);
		bool res;
		if (bCheck)
			res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(_D8_);
		else
			res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

		const_cast<FullBitboards*>(this)->black ^= (kingMoveMask | rookMoveMask);
		const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= rookMoveMask;
		const_cast<FullBitboards*>(this)->posBlackKing = _E8_;

		return res;
	}

	// Alias: FindMoveThatMatesAfterMoveByBlackKing
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackKing(const int toPos) CONST_RESTRICT
	{		
		constexpr bool tbDiscoveredCheckPossible = tbBlackHaveBishopLikes || tbBlackHaveRookLikes;

		const int fromPos = posBlackKing;
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(!IsBlackAt(toPos));
		assert(AreSquaresAdjacent(posBlackKing, toPos));
		
		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownThatItIsNotACapture)
		{		
			const auto captureMask = white & toMask;
	
			const auto bbSaved = *this; // save
	
			const_cast<FullBitboards*>(this)->posBlackKing = toPos;
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->kings ^= moveMask;
			const_cast<FullBitboards*>(this)->white ^= captureMask;
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KING>(captureMask);
	
			// Find potential discovered checker and dispatch to proper template version:
			Bitboard maskCandidatesForDiscoveredChecker;
			int posWhiteKingChecker;
			if (!tbDiscoveredCheckPossible || !SameDiagonalOrLineAndAllBetweenEmpty<1>(fromPos, posWhiteKing) || (maskCandidatesForDiscoveredChecker = GetCandidatesForBlackLongDistanceFigureInDir(fromPos, posWhiteKing)) == 0 || (posWhiteKingChecker = ValidateCandidateForLongDistanceFigureInDir(maskCandidatesForDiscoveredChecker, fromPos, posWhiteKing)) < 0)
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
			else
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker); // no need to verify !IsSquareBetween(to, posDiscoveredChecker, posWhiteKing) since AllBetweenEmpty and BlackLongDistanceFigureInDir were called AFTER moving bl.king
	
			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->kings ^= moveMask;
			const_cast<FullBitboards*>(this)->posBlackKing = toPos;

			// Find potential discovered checker and dispatch to proper template version:
			Bitboard maskCandidatesForDiscoveredChecker;
			int posWhiteKingChecker;
			if (!tbDiscoveredCheckPossible || !SameDiagonalOrLineAndAllBetweenEmpty<1>(fromPos, posWhiteKing) || (maskCandidatesForDiscoveredChecker = GetCandidatesForBlackLongDistanceFigureInDir(fromPos, posWhiteKing)) == 0 || (posWhiteKingChecker = ValidateCandidateForLongDistanceFigureInDir(maskCandidatesForDiscoveredChecker, fromPos, posWhiteKing)) < 0)
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
			else
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);  // no need to verify !IsSquareBetween(to, posDiscoveredChecker, posWhiteKing) since AllBetweenEmpty and BlackLongDistanceFigureInDir were called AFTER moving bl.king

			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->kings ^= moveMask;	
			const_cast<FullBitboards*>(this)->posBlackKing = fromPos;
		}
		
		return res;
	}
	
	// Wraps up the main method CanBlackCapture_AlwaysInline (dependent on context, one or the other version may be selected)	
	// This quite ugly "conditional ALWAYS_INLINE" on CanBlackCapture and CanBlackMoveInBetween speeds up code about 1.5%
	template<bool tbEnPassantPossible = false, bool tbInclKing = true, bool tbFindAll = false, char tbOnlyIfPreventsImmediateMateAndFlags = false>
	Bitboard CanBlackCapture(const int sq) CONST_RESTRICT
	{
		return CanBlackCapture_AlwaysInline<tbEnPassantPossible, tbInclKing, tbFindAll, tbOnlyIfPreventsImmediateMateAndFlags>(sq);
	}

	// It's assummed that bl.king is not checked or 'sq' is the only checker (no double check)
	// If tbFindAll == true, then bitmask of pieces that can capture sq is returned
	// NOTE!!! If tbOnlyIfPreventsImmediateMate is on, together white castling flags must be passed in tbOnlyIfPreventsImmediateMateAndFlags
	template<bool tbEnPassantPossible = false, bool tbInclKing = true, bool tbFindAll = false, char tbOnlyIfPreventsImmediateMateAndFlags = false>
	ALWAYS_INLINE Bitboard CanBlackCapture_AlwaysInline(const int sq) CONST_RESTRICT
	{
		constexpr bool tbOneIsEnough = !tbFindAll;
		constexpr bool tbOnlyIfPreventsImmediateMate = (tbOnlyIfPreventsImmediateMateAndFlags & 1) != 0;
		constexpr bool tbWhiteShortCastlingPossible = (tbOnlyIfPreventsImmediateMateAndFlags & 2) != 0;
		constexpr bool tbWhiteLongCastlingPossible = (tbOnlyIfPreventsImmediateMateAndFlags & 4) != 0;

		assert(IsValidPos(sq));
		assert(white & (sq_to_bb(sq)));
		assert(IsBlackKingChecked() < 0 || IsBlackKingChecked() == sq);

		Bitboard res;
		if constexpr (!tbOneIsEnough)
			res = 0;

		Bitboard mask;
		if constexpr (tbBlackHaveBishopLikes || tbBlackHaveRookLikes)
		{
			#ifdef __USE_MOVEGENINCANBLACKCAPTURE__		
			const auto rawBishopMoves = tbBlackHaveBishopLikes ? get_raw_bishop_moves(sq, occ()) : 0ULL;
			const auto rawRookMoves = tbBlackHaveRookLikes ? get_raw_rook_moves(sq, occ()) : 0ULL;
			mask = black & ((qbishops & rawBishopMoves) | (qrooks & rawRookMoves));
			#else
			mask = black & ((qrooks & Rook_Attacks[sq]) | (qbishops & Bishop_Attacks[sq]));
			#endif

			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				#ifndef __USE_MOVEGENINCANBLACKCAPTURE__
				if (AllBetweenEmpty(pos, sq))
				#endif
					if (!IsBlackPinned(pos, sq))
						if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackLongDistFigure<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, sq))
							if constexpr (tbOneIsEnough)
								return true;
							else
								res |= (sq_to_bb(pos));
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		if constexpr (tbAnyBlackKnights)
		{
			mask = black & knights & Knight_Attacks[sq];
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				if (!IsBlackAbsolutelyPinned(pos))
					if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackKnight<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, sq))
						if constexpr (tbOneIsEnough)
							return true;
						else
							res |= (sq_to_bb(pos));
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		mask = black & pawns & White_Pawn_Attacks[sq];
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (!IsBlackPinned(pos, sq))
				if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterCaptureByBlackPawn<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, sq))
					if constexpr (tbOneIsEnough)
						return true;
					else
						res |= (sq_to_bb(pos));
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		if constexpr (tbInclKing)
		{			
			mask = sq_to_bb(sq) & King_Attacks[posBlackKing] & ~King_Attacks[posWhiteKing] & (((Black_Pawn_Attacks[sq] & white & pawns) | (Knight_Attacks[sq] & white & knights)) ? 0ULL : ~0ULL);

			if (mask)
				if (!IsSquareAttackedByWhiteIfTakeOffBlackKing<-1>(sq)) // -1 == verify only long distance attackers; all other already filtered out above
					if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackKing<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(sq))
						if constexpr (tbOneIsEnough)
							return true;
						else
							res |= mask;
		}

		if constexpr (tbEnPassantPossible)
		{
			assert(sq >= _A4_ && sq <= _H4_);
			assert(pawns & white & (sq_to_bb(sq)));

			auto mask = black & pawns & White_Pawn_Attacks[sq - 8];
			BEGIN_FOR_EACH_POS_IN_MASK(bppos, mask)
			{
				if (!IsBlackPinnedIfTakeOffWhitePawn(bppos, sq - 8, sq))
				{
					assert(!SameDiag(sq, posBlackKing) || !AllBetweenEmpty(sq, posBlackKing) || !WhiteLongDistanceFigureInDir(sq, posBlackKing));

					if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterBlackEnPassant<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(bppos, sq - 8))
						if constexpr (tbOneIsEnough)
							return true;
						else
							res |= (sq_to_bb(bppos));
				}
			}
			END_FOR_EACH_POS_IN_MASK(bppos, mask);
		}

		if constexpr (tbOneIsEnough)
			return false;
		else
			return res;
	}

	template<bool tbEnPassantPossible = false, bool tbInclKing = true, bool tbFindAll = false>
	ALWAYS_INLINE Bitboard CanWhiteCaptureWithCheckMate(const int sq, const Bitboard whitePiecesWithDiscoveredCheck) CONST_RESTRICT
	{
		return CanWhiteCapture<tbEnPassantPossible, tbInclKing, tbFindAll, 2>(sq, whitePiecesWithDiscoveredCheck);
	}

	template<bool tbCheckMateOnly, bool tbAlreadyKnownToBeCheck = false>
	ALWAYS_INLINE bool WillWhiteQueenMoveBeCheck(const int posFrom, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteQueenAt(posFrom));

		if constexpr(tbAlreadyKnownToBeCheck && tbCheckMateOnly)
			return IsCheckMateAfterQueenCheck(posFrom, posTo);
		else
		{
			if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, posTo))
				if (!tbCheckMateOnly)
					return true;
				else
					return IsCheckMateAfterQueenCheck(posFrom, posTo);

			return false;
		}
	}

	template<bool tbCheckMateOnly, bool tbAlreadyKnownToBeCheck = false>
	ALWAYS_INLINE bool WillWhiteRookMoveBeCheck(const int posFrom, const int posTo, bool isDiscoveredCheck) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteRookAt(posFrom));

		if constexpr (tbAlreadyKnownToBeCheck && !tbCheckMateOnly)
			return true;

		if (isDiscoveredCheck)
			if constexpr (tbCheckMateOnly)
				return IsCheckMateAfterRookDiscoveredCheck(posFrom, posTo, WhiteLongDistanceFigureInDir<1, 1>(posFrom, posBlackKing));
			else
				return true;

		if constexpr (tbAlreadyKnownToBeCheck & tbCheckMateOnly)
			return IsCheckMateAfterRookDirectCheck(posFrom, posTo);
		else
		{
			const bool bDirectCheck = SameLineAndAllBetweenEmpty(posBlackKing, posTo);

			if constexpr (!tbCheckMateOnly)
				return bDirectCheck;
			else
				if (bDirectCheck)
					return IsCheckMateAfterRookDirectCheck(posFrom, posTo);
				else
					return false;
		}
	}

	template<bool tbCheckMateOnly, bool tbAlreadyKnownToBeCheck = false>
	ALWAYS_INLINE bool WillWhiteBishopMoveBeCheck(const int posFrom, const int posTo, const bool isDiscoveredCheck) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteBishopAt(posFrom));

		if constexpr (tbAlreadyKnownToBeCheck && !tbCheckMateOnly)
			return true;

		if (isDiscoveredCheck)
			if constexpr (tbCheckMateOnly)
				return IsCheckMateAfterBishopDiscoveredCheck(posFrom, posTo, WhiteLongDistanceFigureInDir<1, 1>(posFrom, posBlackKing));
			else
				return true;

		if constexpr (tbAlreadyKnownToBeCheck & tbCheckMateOnly)
			return IsCheckMateAfterBishopDirectCheck(posFrom, posTo);
		else
		{
			const bool bDirectCheck = SameDiagAndAllBetweenEmpty(posBlackKing, posTo);

			if constexpr (!tbCheckMateOnly)
				return bDirectCheck;
			else
				if (bDirectCheck)
					return IsCheckMateAfterBishopDirectCheck(posFrom, posTo);
				else
					return false;
		}
	}

	template<bool tbCheckMateOnly = false, bool tbAlreadyKnownToBeCheck = false>
	ALWAYS_INLINE bool WillWhiteLongDistanceFigureMoveBeCheck(const int posFrom, const int posTo, const bool isDiscoveredCheck) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteAt(posFrom));
		
		const auto f = GetLongDistanceFigureAtExt(posFrom);

		switch (f)
		{
			case 1: // bishop
				return WillWhiteBishopMoveBeCheck<tbCheckMateOnly, tbAlreadyKnownToBeCheck>(posFrom, posTo, isDiscoveredCheck);
			
			case 2: // rook
				return WillWhiteRookMoveBeCheck<tbCheckMateOnly, tbAlreadyKnownToBeCheck>(posFrom, posTo, isDiscoveredCheck);

			case 3: // queen
			{
				assert(!isDiscoveredCheck);
				return WillWhiteQueenMoveBeCheck<tbCheckMateOnly, tbAlreadyKnownToBeCheck>(posFrom, posTo);
			}			
		}

		assert(false);
		return false;
	}


	template<bool tbCheckMateOnly = false, bool tbAlreadyKnownToBeCheck = false, bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool WillWhiteKnightMoveBeCheck(const int posFrom, const int posTo, const bool bDiscoveredCheck) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteAt(posFrom));

		if constexpr (tbAlreadyKnownToBeCheck && !tbCheckMateOnly)
			return true;

		if (bDiscoveredCheck)
		{
			assert(SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, posFrom));
			
			return IsCheckMateAfterKnightDiscoveredCheck<tbKnownToBeNotACapture>(posFrom, posTo, WhiteLongDistanceFigureInDir<1, 1>(posFrom, posBlackKing));
		}

		if constexpr (tbAlreadyKnownToBeCheck)
		{
			assert(IsKnightDiff(posBlackKing, posTo));
			return IsCheckMateAfterKnightDirectCheck<tbKnownToBeNotACapture>(posFrom, posTo);
		}
		else
		{
			const bool bDirectCheck = IsKnightDiff(posBlackKing, posTo);
			if constexpr (!tbCheckMateOnly)
				return bDirectCheck;

			if (bDirectCheck)
				return IsCheckMateAfterKnightDirectCheck<tbKnownToBeNotACapture>(posFrom, posTo);
			else
				return false;
		}
	}

	// En passant not handled by this method
	template<bool tbCheckMateOnly = false, FIGURE fPromo = 0, bool tbKnownToBeNotACapture = false> // when fPromo == 0 (FGR_EMPTY), and the move is a promo, both promotions to queen and knight are verified
	ALWAYS_INLINE bool WillMoveByWhitePawnBeCheck(const int posFrom, const int posTo) CONST_RESTRICT
	{
		static_assert(fPromo == 0 || fPromo == FGR_QUEEN || fPromo == FGR_ROOK || fPromo == FGR_BISHOP || fPromo == FGR_KNIGHT, "");
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteAt(posFrom));
		assert((posFrom & 7) == (posTo & 7) || IsBlackAt(posTo)); // en passant not handled by this method

		if (posTo >= _A8_)
		{
			if constexpr (fPromo != 0) // analyze specific promo only or any one?
			{
				bool bDirectCheck;
				if constexpr (fPromo == FGR_KNIGHT)
					bDirectCheck = IsKnightDiff(posBlackKing, posTo);
				else if constexpr (fPromo == FGR_QUEEN)
					bDirectCheck = SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(posBlackKing, posTo, posFrom);
				else if constexpr (fPromo == FGR_ROOK)
					bDirectCheck = SameLineAndAllBetweenEmptyIfTakeOffWhitePawn(posBlackKing, posTo, posFrom);
				else if constexpr (fPromo == FGR_ROOK)
					bDirectCheck = SameDiagAndAllBetweenEmptyIfTakeOffWhitePawn(posBlackKing, posTo, posFrom);

				if (bDirectCheck)
				{
					if constexpr (!tbCheckMateOnly)
						return true;
					const bool bDiscoveredCheck = SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, posFrom) && WhiteLongDistanceFigureInDir(posFrom, posBlackKing);
					if constexpr (fPromo == FGR_KNIGHT)
						return IsCheckMateAfterPromoToKnightDirectCheck<tbKnownToBeNotACapture>(posFrom, posTo, bDiscoveredCheck);
					else if constexpr (fPromo == FGR_QUEEN)
						return IsCheckMateAfterPromoToQueenDirectCheck<tbKnownToBeNotACapture>(posFrom, posTo, bDiscoveredCheck);
					else if constexpr (fPromo == FGR_ROOK)
						return IsCheckMateAfterPromoToRookDirectCheck<tbKnownToBeNotACapture>(posFrom, posTo, bDiscoveredCheck);
					else if constexpr (fPromo == FGR_BISHOP)
						return IsCheckMateAfterPromoToBishopDirectCheck<tbKnownToBeNotACapture>(posFrom, posTo, bDiscoveredCheck);
				}
				else
				{
					const auto posDiscoveredChecker = (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, posFrom)) ? WhiteLongDistanceFigureInDir<1>(posFrom, posBlackKing) : -1;
					if (posDiscoveredChecker >= 0)
					{
						if constexpr (!tbCheckMateOnly)
							return true;
						if constexpr (fPromo == FGR_KNIGHT)
							return IsCheckMateAfterPromoToKnightDiscoveredCheck<tbKnownToBeNotACapture>(posFrom, posTo, posDiscoveredChecker);
						else if constexpr (fPromo == FGR_QUEEN)
							return IsCheckMateAfterPromoToQueenDiscoveredCheck<tbKnownToBeNotACapture>(posFrom, posTo, posDiscoveredChecker);
						else if constexpr (fPromo == FGR_ROOK)
							return IsCheckMateAfterPromoToRookDiscoveredCheck<tbKnownToBeNotACapture>(posFrom, posTo, posDiscoveredChecker);
						else if constexpr (fPromo == FGR_BISHOP)
							return IsCheckMateAfterPromoToBishopDiscoveredCheck<tbKnownToBeNotACapture>(posFrom, posTo, posDiscoveredChecker);
					}
				}
			}
			else
			{
				// Any promo should be considered, so we can limit to knight and queen:
				const bool bPromoToKnightDirectCheck = IsKnightDiff(posBlackKing, posTo);
				const bool bPromoToQueenDirectCheck = SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(posBlackKing, posTo, posFrom);

				if constexpr (!tbCheckMateOnly)
					if (bPromoToKnightDirectCheck | bPromoToQueenDirectCheck)
						return true;

				const auto posDiscoveredChecker = (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, posFrom)) ? WhiteLongDistanceFigureInDir<1>(posFrom, posBlackKing) : -1;
				if constexpr (!tbCheckMateOnly)
					if (posDiscoveredChecker >= 0)
						return true;

				if constexpr (tbCheckMateOnly)
				{
					if (bPromoToKnightDirectCheck | bPromoToQueenDirectCheck)
					{
						const bool bDiscoveredCheck = posDiscoveredChecker >= 0;
						if (bPromoToKnightDirectCheck)
						{
							if (IsCheckMateAfterPromoToKnightDirectCheck<tbKnownToBeNotACapture>(posFrom, posTo, bDiscoveredCheck))
								return true;
						}
						else
							if (bPromoToQueenDirectCheck)
								if (IsCheckMateAfterPromoToQueenDirectCheck<tbKnownToBeNotACapture>(posFrom, posTo, bDiscoveredCheck))
									return true;
					}

					if (posDiscoveredChecker >= 0)
					{
						if (!bPromoToQueenDirectCheck && IsCheckMateAfterPromoToQueenDiscoveredCheck<tbKnownToBeNotACapture>(posFrom, posTo, posDiscoveredChecker))
							return true;
						else
							return !bPromoToKnightDirectCheck && IsCheckMateAfterPromoToKnightDiscoveredCheck<tbKnownToBeNotACapture>(posFrom, posTo, posDiscoveredChecker);
					}
				}
			}
		}
		else
		{
			const bool bDirectCheck = (White_Pawn_Attacks[posTo] & black & kings) != 0;
			if (bDirectCheck)
				if constexpr (!tbCheckMateOnly)
					return true;
				else
					if (posBlackKing == posFrom + 16 && IsEmptyAt(posFrom + 8) && WhiteLongDistanceFigureInDir(posFrom, 0, -1))
						return IsCheckMateAfterPawnDirectCheck(posFrom, posTo, true); // double check
					else
						if (posTo == posFrom + 16)
							return IsCheckMateAfterPawnDirectCheck<1, 1>(posFrom, posTo);
						else
							return IsCheckMateAfterPawnDirectCheck<0, tbKnownToBeNotACapture>(posFrom, posTo);

			if (SameDiagonalOrLineAndAllBetweenEmpty(posFrom, posBlackKing))
			{
				const auto posDiscoveredChecker = WhiteLongDistanceFigureInDir<1>(posFrom, posBlackKing);
				if (posDiscoveredChecker >= 0)
				{
					const bool bMoveForward = SameFile(posTo, posFrom);
					if (!bMoveForward || !SameFile(posDiscoveredChecker, posFrom)) // not a move forward while long distance attacker on the same file?
						if constexpr (!tbCheckMateOnly)
							return true;
						else
							return IsCheckMateAfterPawnDiscoveredCheck<tbKnownToBeNotACapture>(posFrom, posTo, posDiscoveredChecker);
				}
			}
		}

		return false;
	}

	template<bool tbCheckMateOnly = false>
	ALWAYS_INLINE bool WillWhiteKingMoveBeCheck(const int posTo) CONST_RESTRICT
	{
		const int posFrom = posWhiteKing;
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteAt(posFrom));
		assert(Distance(posFrom, posTo) == 1);

		if (SameDiagonalOrLineAndAllBetweenEmpty(posFrom, posBlackKing))		
			if (const auto mask = GetCandidatesForWhiteLongDistanceFigureInDir(posFrom, posBlackKing))
			{
				const auto posDiscoveredChecker = ValidateCandidateForLongDistanceFigureInDir(mask, posFrom, posBlackKing);
				if (posDiscoveredChecker >= 0)
					if (!IsSquareBetween<1>(posTo, posDiscoveredChecker, posBlackKing)) // not a move along the diagonal/line ?
						if constexpr (!tbCheckMateOnly)
							return true;
						else
							return IsCheckMateAfterKingDiscoveredCheck(posTo, posDiscoveredChecker);
			}		

		return false;
	}

	
	template<bool tbCheckMateOnly = false>
	bool WillWhiteEnPassantBeCheck(const int posFrom, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhitePawnAt(posFrom));
		assert(IsEmptyAt(posTo));
		assert(posFrom >= _A5_ && posFrom <= _H5_);

		const auto posBlackPawn = _A5_ + (posTo & 7);
		assert(IsBlackPawnAt(posBlackPawn));

		const bool bDirectCheck = (White_Pawn_Attacks[posTo] & black & kings) != 0;
		if (bDirectCheck)
			if (!tbCheckMateOnly)
				return true;
			else
			{
				const bool bDiscoveredCheck = posBlackKing == posFrom + 16 && IsEmptyAt(posFrom + 8) && WhiteLongDistanceFigureInDir(posFrom, 0, -1);
				return IsCheckMateAfterEnPassantDirectCheck(posFrom, posTo, bDiscoveredCheck);
			}

		const auto posBlackPawnDiscoveredChecker = (SameDiagAndAllBetweenEmpty(posBlackKing, posBlackPawn)) ? WhiteLongDistanceFigureInDir<1>(posBlackPawn, posBlackKing) : -1;
		if constexpr (!tbCheckMateOnly)
			if (posBlackPawnDiscoveredChecker >= 0)
				return true;

		const bool bBlackKingOnFifthLine = (posBlackKing >> 3) == _5_;
		int posWhitePawnDiscoveredChecker = -1;
		if (!bBlackKingOnFifthLine)
		{
			if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, posFrom))
			{
				posWhitePawnDiscoveredChecker = WhiteLongDistanceFigureInDir<1>(posFrom, posBlackKing);
				if (posWhitePawnDiscoveredChecker >= 0)
					if (IsSquareBetween<1>(posTo, posBlackKing, posWhitePawnDiscoveredChecker))
						posWhitePawnDiscoveredChecker = -1;
				if constexpr (!tbCheckMateOnly)
					if (posWhitePawnDiscoveredChecker >= 0)
						return true;
			}
		}
		else
		{
			assert(posWhitePawnDiscoveredChecker == -1 && posBlackPawnDiscoveredChecker == -1);
			posWhitePawnDiscoveredChecker = AllBetweenEmptyIfTakeOffWhitePawn(posBlackKing, posBlackPawn, posFrom) ? const_cast<FullBitboards*>(this)->WhiteLongDistanceFigureInDirIfTakeOffWhitePawn<1>(posBlackPawn, posBlackKing, posFrom) : -1;
			if constexpr (!tbCheckMateOnly)
				if (posWhitePawnDiscoveredChecker >= 0)
					return true;
		}

		if constexpr (tbCheckMateOnly)
			if ((posWhitePawnDiscoveredChecker >= 0) | (posBlackPawnDiscoveredChecker >= 0))
			{
				const auto posDiscoveredAttacker = (posWhitePawnDiscoveredChecker >= 0) ? ((posBlackPawnDiscoveredChecker >= 0) ? DBL_CHECKED : posWhitePawnDiscoveredChecker) : posBlackPawnDiscoveredChecker;
				if (IsCheckMateAfterEnPassantDiscoveredCheck(posFrom, posTo, posDiscoveredAttacker))
					return true;
			}

		return false;
	}
	
	// It's assummed that wh.king is not checked or 'sq' is the only checker (no double check)
	// If tbFindAll == true, then bitmask of pieces that can capture sq is returned
	// If tbOnlyCheckingMoves > 1, then immediate checkmate will be searched for
	// Parameter whitePiecesWithDiscoveredCheck is meaningful only if tbOnlyCheckingMoves
	template<bool tbEnPassantPossible = false, bool tbInclKing = true, bool tbFindAll = false, char tbOnlyCheckingMoves = false>
	Bitboard CanWhiteCapture(const int sq, const Bitboard whitePiecesWithDiscoveredCheck = 0) CONST_RESTRICT
	{
		constexpr bool tbOneIsEnough = !tbFindAll;
		constexpr bool tbOnlyMatingMoves = tbOnlyCheckingMoves > 1;

		assert(IsValidPos(sq));
		assert(black & (sq_to_bb(sq)));
		assert(IsWhiteKingChecked() < 0 || IsWhiteKingChecked() == sq);

		Bitboard res;
		if constexpr (!tbOneIsEnough)
			res = 0;

		Bitboard mask;
		#ifdef __USE_MOVEGENINCANWHITECAPTURE__				
		const auto occ = this->occ();
		const auto rawBishopMoves = get_raw_bishop_moves(sq, occ);
		const auto rawRookMoves = get_raw_rook_moves(sq, occ);
		if constexpr (tbOnlyCheckingMoves)
		{
			const auto allBetweenEmpty = AllBetweenEmpty(posBlackKing, sq);
			const bool diagAttack = allBetweenEmpty & SameDiag(posBlackKing, sq);
			const bool lineAttack = allBetweenEmpty & SameLine(posBlackKing, sq);
			const auto maskForRooks = BOOL_EXTEND64(lineAttack) | whitePiecesWithDiscoveredCheck;
			const auto maskForBishops = BOOL_EXTEND64(diagAttack) | whitePiecesWithDiscoveredCheck;
			const auto maskForQueens = BOOL_EXTEND64(diagAttack | lineAttack);
			mask = white & ((rawBishopMoves & bishops() & maskForBishops) | (rawRookMoves & rooks() & maskForRooks) | ((rawBishopMoves | rawRookMoves) & queens() & maskForQueens));
		}
		else
			mask = white & ((rawBishopMoves & bishops()) | (rawRookMoves & rooks()) | ((rawBishopMoves | rawRookMoves) & queens()));
		#else
		{
			if constexpr (tbOnlyCheckingMoves)
			{
				const auto maskForQueens = BOOL_EXTEND64(SameDiagonalOrLineAndAllBetweenEmpty(sq, posBlackKing)); // a queen cannot make a discovered check
				const auto maskForRooks = BOOL_EXTEND64(SameLineAndAllBetweenEmpty(posBlackKing, sq)) | whitePiecesWithDiscoveredCheck;
				const auto maskForBishops = BOOL_EXTEND64(SameDiagAndAllBetweenEmpty(posBlackKing, sq))) | whitePiecesWithDiscoveredCheck;
			
				mask = white & ((rooks() & Rook_Attacks[sq] & (tbOnlyCheckingMoves ? maskForRooks : ~0ULL)) | 
								(queens() & Queen_Attacks[sq] & (tbOnlyCheckingMoves ? maskForQueens : ~0ULL)) |
								(bishops() & Bishop_Attacks[sq] & (tbOnlyCheckingMoves ? maskForBishops : ~0ULL)));
			}
			else
				mask = white & ((qrooks & Rook_Attacks[sq]) | (qbishops & Bishop_Attacks[sq]));
		}
		#endif
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			#ifndef __USE_MOVEGENINCANWHITECAPTURE__
			if (AllBetweenEmpty(pos, sq))
			#endif
				if (!IsWhitePinned(pos, sq))
					if (!tbOnlyMatingMoves || WillWhiteLongDistanceFigureMoveBeCheck<tbOnlyMatingMoves,1>(pos, sq, whitePiecesWithDiscoveredCheck & (1ULL << pos)))
						if constexpr (tbOneIsEnough)
							return true;
						else
							res |= (sq_to_bb(pos));
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		const bool bKnightDiff = IsKnightDiff(sq, posBlackKing);
		mask = white & knights & Knight_Attacks[sq] & (tbOnlyCheckingMoves ? ((BOOL_EXTEND64(bKnightDiff) & Knights_That_Can_Directly_Check[posBlackKing]) | whitePiecesWithDiscoveredCheck) : ~0ULL);
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (!IsWhiteAbsolutelyPinned(pos))
				if (!tbOnlyMatingMoves || WillWhiteKnightMoveBeCheck<tbOnlyMatingMoves,1>(pos, sq, whitePiecesWithDiscoveredCheck & (1ULL << pos)))
					if constexpr (tbOneIsEnough)
						return true;
					else
						res |= (sq_to_bb(pos));
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		mask = white & pawns & Black_Pawn_Attacks[sq] & (tbOnlyCheckingMoves ? (White_Pawn_Direct_Check_Area[posBlackKing] | whitePiecesWithDiscoveredCheck) : ~0ULL);
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (!IsWhitePinned(pos, sq))
				if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves>(pos, sq))
					if constexpr (tbOneIsEnough)
						return true;
					else
						res |= (sq_to_bb(pos));
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		if constexpr (tbInclKing)
		{
			mask = white & kings & King_Attacks[sq] & (tbOnlyCheckingMoves ? whitePiecesWithDiscoveredCheck : ~0ULL);
			if (mask)
				if (!IsSquareAttackedByBlackIfTakeOffWhiteKing(sq))
					if (!tbOnlyCheckingMoves || WillWhiteKingMoveBeCheck<tbOnlyMatingMoves>(sq))
						if constexpr (tbOneIsEnough)
							return true;
						else
							res |= mask;
		}

		if constexpr (tbEnPassantPossible)
		{
			assert(sq >= _A5_ && sq <= _H5_);
			assert(pawns & black & (sq_to_bb(sq)));

			auto mask = Black_Pawn_Attacks[sq + 8] & white & pawns;
			BEGIN_FOR_EACH_POS_IN_MASK(wppos, mask)
			{
				if (!IsWhitePinnedIfTakeOffBlackPawn<1>(wppos, sq + 8, sq))
				{
					assert(!SameDiag(sq, posWhiteKing) || !AllBetweenEmpty(sq, posWhiteKing) || !BlackLongDistanceFigureInDir(sq, posWhiteKing));

					if (!tbOnlyCheckingMoves || WillWhiteEnPassantBeCheck<tbOnlyMatingMoves>(wppos, sq + 8))
						if constexpr (tbOneIsEnough)
							return true;
						else
							res |= (sq_to_bb(wppos));
				}
			}
			END_FOR_EACH_POS_IN_MASK(wppos, mask);
		}

		if constexpr (tbOneIsEnough)
			return false;
		else
			return res;
	}

	// Wraps up the main method CanBlackMoveInBetween_AlwaysInline (dependent on context, one or the other version may be selected)
	// This quite ugly "conditional ALWAYS_INLINE" on CanBlackCapture and CanBlackMoveInBetween speeds up code about 1.5%
	template<bool tbFindAllAndFillBuf = false, char tbOnlyIfPreventsImmediateMateAndFlags = false> // if tbFindAllAndFillBuf == false, then aMoves will not be filled in
	int CanBlackMoveInBetween(const int sq1, const int sq2, TMove* aMoves = nullptr) CONST_RESTRICT
	{
		return CanBlackMoveInBetween_AlwaysInline<tbFindAllAndFillBuf, tbOnlyIfPreventsImmediateMateAndFlags>(sq1, sq2, aMoves);
	}

	// !!! Method does not take into account en passant nor castling (en passant can never prevent a discovered check by a long distance black attacker)
	// Method assumes that either bl.king is not checked, or is checked so that moving in between can prevent it
	// NOTE!!! If tbOnlyIfPreventsImmediateMate is on, together white castling flags must be passed in tbOnlyIfPreventsImmediateMateAndFlags
	template<bool tbFindAllAndFillBuf = false, char tbOnlyIfPreventsImmediateMateAndFlags = false> // if tbFindAllAndFillBuf == false, then aMoves will not be filled in
	ALWAYS_INLINE int CanBlackMoveInBetween_AlwaysInline(const int sq1, const int sq2, TMove* aMoves = nullptr) CONST_RESTRICT
	{
		constexpr bool tbOnlyIfPreventsImmediateMate = (tbOnlyIfPreventsImmediateMateAndFlags & 1) != 0;
		constexpr bool tbWhiteShortCastlingPossible = (tbOnlyIfPreventsImmediateMateAndFlags & 2) != 0;
		constexpr bool tbWhiteLongCastlingPossible = (tbOnlyIfPreventsImmediateMateAndFlags & 4) != 0;

		assert(IsValidPos(sq1));
		assert(IsValidPos(sq2));
		assert(sq1 != sq2);
		assert(SameDiagonalOrLine(sq1, sq2));
		assert(!tbFindAllAndFillBuf || aMoves != nullptr);

		constexpr bool tbInclKing = false;
		constexpr bool tbIncludingEnds = false;
		constexpr bool tbOneIsEnough = !tbFindAllAndFillBuf;
		constexpr bool tbVerifyPinning = true;
		#ifdef __USE_OPTIM_FOR_NON_CAPTURE__
		constexpr bool tbKnownThatItIsNotACapture = true; // (maskBetween & occ()) == 0 is a prerequisite (see assert below)
		#else
		constexpr bool tbKnownThatItIsNotACapture = false;
		#endif		

		const auto maskBetween = GetBetweenMask<tbIncludingEnds>(sq1, sq2);
		assert((maskBetween & occ()) == 0); // prerequisite
		int count;
		if constexpr (!tbOneIsEnough)
			count = 0;
		
		if constexpr (tbAnyBlackKnights || tbBlackHaveBishopLikes || tbBlackHaveRookLikes)
		{
			auto tmpMaskBetween = maskBetween;
			BEGIN_DOWHILE_POS_IN_MASK(pos, tmpMaskBetween)
			{
				if constexpr (tbAnyBlackKnights)
				{
					auto knightBitboard = Knight_Attacks[pos] & black & knights;
					BEGIN_FOR_EACH_POS_IN_MASK(kpos, knightBitboard)
					{
						if (!tbVerifyPinning || !IsBlackAbsolutelyPinned(kpos))
							if constexpr (tbOneIsEnough && !tbOnlyIfPreventsImmediateMate)
								return 1;
							else
								if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackKnight<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible, tbKnownThatItIsNotACapture>(kpos, pos))
									if constexpr (tbOneIsEnough)
										return 1;
									else
										aMoves[count++].set(kpos, pos);
					}
					END_FOR_EACH_POS_IN_MASK(kpos, knightBitboard);
				}

				if constexpr (tbBlackHaveBishopLikes || tbBlackHaveRookLikes)
				{
					const auto rawBishopMoves = tbBlackHaveBishopLikes ? get_raw_bishop_moves(pos, occ()) : 0ULL;
					const auto rawRookMoves = tbBlackHaveRookLikes ? get_raw_rook_moves(pos, occ()) : 0ULL;
					auto blackLongDistAttackers = ((rawBishopMoves & qbishops) | (rawRookMoves & qrooks)) & black;
					BEGIN_FOR_EACH_POS_IN_MASK(rbpos, blackLongDistAttackers)
					{
						if (!tbVerifyPinning || !IsBlackPinned(rbpos, pos))
							if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackLongDistFigure<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible, tbKnownThatItIsNotACapture>(rbpos, pos))
								if constexpr (tbOneIsEnough)
									return 1;
								else
									aMoves[count++].set(rbpos, pos);

					}
					END_FOR_EACH_POS_IN_MASK(rbpos, blackLongDistAttackers);
				}
			}
			END_DOWHILE_POS_IN_MASK(pos, tmpMaskBetween);
		}

		// King (for now unused code, thus - not optimized)
		if constexpr (tbInclKing)
		{
			auto matchMask = King_Attacks[posBlackKing] & maskBetween;
			BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
			{
				if (!tbVerifyPinning || !IsSquareAttackedByWhiteIfTakeOffBlackKing(posBetween))
					if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackKing<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posBetween))
					{
						if constexpr (tbOneIsEnough)
							return 1;
						else
							aMoves[count++].set(posBlackKing, posBetween);
					}
			}
			END_FOR_EACH_POS_IN_MASK(posBetween, matchMask);
		}

		// Pawns: 
		auto mask = (black & pawns) >> 8;
		auto matchMask = mask & maskBetween;
		while (matchMask) // no easy way to use BEGIN_FOR_EACH_POS_IN_MASK
		{
			const int pos = std::countr_zero(matchMask) + 8;
			if (!tbVerifyPinning || !IsBlackPinned(pos, pos - 8))
				if (pos <= _H2_)
				{
					if constexpr (tbOneIsEnough && !tbOnlyIfPreventsImmediateMate)
						return 1;
					else
					{
						static_assert(!(tbOnlyIfPreventsImmediateMate && !tbOneIsEnough), ""); // combination not implemented yet
						if (tbOnlyIfPreventsImmediateMate && tbOneIsEnough)
						{
							if (!IsImmediateMateAfterPromoMoveForwardByBlackPawn<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, pos - 8))
								return 1;
						}
						else
						{
							aMoves[count++].set(pos, pos - 8, FGR_QUEEN);
							aMoves[count++].set(pos, pos - 8, FGR_ROOK);
							aMoves[count++].set(pos, pos - 8, FGR_BISHOP);
							aMoves[count++].set(pos, pos - 8, FGR_KNIGHT);
						}
					}
				}
				else
				{
					if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveForwardByBlackPawn<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, pos - 8))
						if constexpr (tbOneIsEnough)
							return 1;
						else
							aMoves[count++].set(pos, pos - 8);
				}
			matchMask &= matchMask - 1;
		}

		// Long move by a pawn: 
		mask >>= 8;
		matchMask = mask & maskBetween;
		matchMask &= (255ULL << _A5_); // or GetBetweenMask<1>(_A5_,_H5_), which is contexpr
		while (matchMask) // no easy way to use BEGIN_FOR_EACH_POS_IN_MASK
		{
			const int pos = std::countr_zero(matchMask) + 16;
			if (IsEmptyAt(pos - 8))
				if (!tbVerifyPinning || !IsBlackPinned(pos, pos - 16))
					if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterLongMoveByBlackPawn<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, pos - 16))
					{
						if constexpr (tbOneIsEnough)
							return 1;
						else
							aMoves[count++].set(pos, pos - 16);
					}
			matchMask &= matchMask - 1;
		}

		if constexpr (!tbOneIsEnough)
			return count;
		else
			return 0;
	}

	template<bool tbFindAllAndFillBuf = false>
	ALWAYS_INLINE int CanWhiteMoveInBetweenWithCheckMate(const int sq1, const int sq2, const Bitboard whitePiecesWithDiscoveredCheck, TMove* aMoves = nullptr) CONST_RESTRICT
	{
		return CanWhiteMoveInBetween<tbFindAllAndFillBuf, 2>(sq1, sq2, whitePiecesWithDiscoveredCheck, aMoves);
	}

	// !!! Method does not take into account en passant nor castling (en passant can never prevent a discovered check by a long distance attacker)
	// Method assumes that either wh.king is not checked, or is checked so that moving in between can prevent it
	// Parameter whitePiecesWithDiscoveredCheck is meaningful only with tbOnlyCheckingMoves > 0
	template<bool tbFindAllAndFillBuf = false, char tbOnlyCheckingMoves = false> //  !!! if tbFindAllAndFillBuf == false, aMoves will NOT be filled in
	int CanWhiteMoveInBetween(const int sq1, const int sq2, const Bitboard whitePiecesWithDiscoveredCheck, TMove* aMoves = nullptr) CONST_RESTRICT
	{
		assert(IsValidPos(sq1));
		assert(IsValidPos(sq2));
		assert(sq1 != sq2);
		assert(SameDiagonalOrLine(sq1, sq2));
		assert(!tbFindAllAndFillBuf || aMoves != nullptr);

		constexpr bool tbOnlyMatingMoves = tbOnlyCheckingMoves > 1;
		constexpr bool tbInclKing = false;
		constexpr bool tbIncludingEnds = false;
		constexpr bool tbOneIsEnough = !tbFindAllAndFillBuf;
		constexpr bool tbVerifyPinning = true;
		static_assert(!tbOnlyMatingMoves || tbVerifyPinning, "");

		const auto maskBetween = GetBetweenMask<tbIncludingEnds>(sq1, sq2);
		assert((maskBetween & occ()) == 0); // prerequisite
		int count;
		if constexpr (!tbOneIsEnough)
			count = 0;

		const auto occ = this->occ();
		auto tmpMaskBetween = maskBetween;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, tmpMaskBetween)
		{
			const bool bKnightDiff = IsKnightDiff(posBlackKing, pos);
			auto knightBitboard = Knight_Attacks[pos] & white & knights & (tbOnlyCheckingMoves ? (BOOL_EXTEND64(bKnightDiff) & Knights_That_Can_Directly_Check[posBlackKing]) | whitePiecesWithDiscoveredCheck : ~0ULL);
			BEGIN_FOR_EACH_POS_IN_MASK(kpos, knightBitboard)
			{
				if (!tbVerifyPinning || !IsWhiteAbsolutelyPinned(kpos))
					if (!tbOnlyMatingMoves || WillWhiteKnightMoveBeCheck<tbOnlyMatingMoves, 1, 1>(kpos, pos, whitePiecesWithDiscoveredCheck & (1ULL << kpos)))
						if constexpr (tbOneIsEnough)
							return 1;
						else
							aMoves[count++].set(kpos, pos);
			}
			END_FOR_EACH_POS_IN_MASK(kpos, knightBitboard);
			
			const auto rawBishopMoves = get_raw_bishop_moves(pos, occ);
			const auto rawRookMoves = get_raw_rook_moves(pos, occ);
			Bitboard blackLongDistAttackers; 
			if constexpr (tbOnlyCheckingMoves)
			{
				const bool allBetweenEmpty = AllBetweenEmpty(posBlackKing, pos);
				const bool diagAttack = allBetweenEmpty & SameDiag(posBlackKing, pos);
				const bool lineAttack = allBetweenEmpty & SameLine(posBlackKing, pos);
				const auto maskForQueens = BOOL_EXTEND64(diagAttack|lineAttack);
				const auto maskForRooks = BOOL_EXTEND64(lineAttack) | whitePiecesWithDiscoveredCheck;
				const auto maskForBishops = BOOL_EXTEND64(diagAttack) | whitePiecesWithDiscoveredCheck;
				blackLongDistAttackers = (((rawBishopMoves | rawRookMoves) & queens() & maskForQueens) | (rawBishopMoves & bishops() & maskForBishops) | (rawRookMoves & rooks() & maskForRooks)) & white;
			}
			else
				blackLongDistAttackers = ((rawBishopMoves & qbishops) | (rawRookMoves & qrooks)) & white;

			BEGIN_FOR_EACH_POS_IN_MASK(rbpos, blackLongDistAttackers)
			{
				if (!tbVerifyPinning || !IsWhitePinned(rbpos, pos))
					if (!tbOnlyMatingMoves || WillWhiteLongDistanceFigureMoveBeCheck<tbOnlyMatingMoves,1>(rbpos, pos, whitePiecesWithDiscoveredCheck & (1ULL << rbpos)))
						if constexpr (tbOneIsEnough)
							return 1;
						else
							aMoves[count++].set(rbpos, pos);

			}
			END_FOR_EACH_POS_IN_MASK(rbpos, blackLongDistAttackers);			
		}
		END_FOR_EACH_POS_IN_MASK(pos, tmpMaskBetween);
		
		// King:
		if constexpr (tbInclKing)
		{
			if (!tbOnlyCheckingMoves || (whitePiecesWithDiscoveredCheck & white & kings))
			{
				auto matchMask = King_Attacks[posWhiteKing] & maskBetween;
				BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
				{
					if (!tbVerifyPinning || !IsSquareAttackedByBlackIfTakeOffWhiteKing(posBetween))
						if (!tbOnlyCheckingMoves || WillWhiteKingMoveBeCheck<tbOnlyMatingMoves>(posBetween))
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(posWhiteKing, posBetween);
						}
				}
				END_FOR_EACH_POS_IN_MASK(posBetween, matchMask);
			}
		}

		// Pawns: 			
		auto mask = (white & pawns & (tbOnlyCheckingMoves ? White_Pawn_Direct_Check_Area[posBlackKing] | whitePiecesWithDiscoveredCheck : ~0ULL)) << 8;
		auto matchMask = mask & maskBetween;
		while (matchMask) // no easy way to use BEGIN_FOR_EACH_POS_IN_MASK
		{
			const int pos = std::countr_zero(matchMask) - 8;
			if (!tbVerifyPinning || !IsWhitePinned(pos, pos + 8))
				if (pos >= _A2_)
				{
					if constexpr (tbOneIsEnough && !tbOnlyCheckingMoves)
						return 1;
					else
					{
						if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, FGR_QUEEN, true>(pos, pos + 8))
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, pos + 8, FGR_QUEEN);
						}
						if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, FGR_KNIGHT, true>(pos, pos + 8))
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, pos + 8, FGR_KNIGHT);
						}
						if constexpr (!tbOneIsEnough) // if tbOneIsEnough, then it is enough to analyze promo to queen and promo to knight only
						{
							if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, FGR_BISHOP, true>(pos, pos + 8))
								aMoves[count++].set(pos, pos + 8, FGR_BISHOP);
							if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, FGR_ROOK, true>(pos, pos + 8))
								aMoves[count++].set(pos, pos + 8, FGR_ROOK);
						}
					}
				}
				else
				{
					if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, 0, true>(pos, pos + 8))
					{
						if constexpr (tbOneIsEnough)
							return 1;
						else
							aMoves[count++].set(pos, pos + 8);
					}
				}
			matchMask &= matchMask - 1;
		}

		// Long move by a pawn: 
		mask <<= 8;
		matchMask = mask & maskBetween;
		matchMask &= (255ULL << _A4_); //GetBetweenMask<1>(_A4_,_H4_);
		while (matchMask) // no easy way to use BEGIN_FOR_EACH_POS_IN_MASK
		{
			const int pos = std::countr_zero(matchMask) - 16;
			if (IsEmptyAt(pos + 8))
				if (!tbVerifyPinning || !IsWhitePinned(pos, pos + 16))
					if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, 0, true>(pos, pos + 16))
					{
						if constexpr (tbOneIsEnough)
							return 1;
						else
							aMoves[count++].set(pos, pos + 16);
					}
			matchMask &= matchMask - 1;
		}

		if constexpr (!tbOneIsEnough)
			return count;
		else
			return 0;
	}

	// tbLongDistanceChecker>1 means DLB_CHECKED
	#ifdef __USE_WHITEKNIGHTATTACKMASK__
	template<char tbLongDistanceChecker = false>
	ALWAYS_INLINE bool FindOneValidMove4BlackKingWhenChecked(const char posChecker) CONST_RESTRICT
	{		
		assert((tbLongDistanceChecker > 1) ^ (posChecker != DBL_CHECKED));

		const auto blackKing = black & kings;
				
		// as soon as tbLongDistanceChecker is always correct, taking king off is not needed
		if constexpr(tbLongDistanceChecker > 1) // dblcheck?
		{
			const_cast<FullBitboards*>(this)->black ^= blackKing;
			#ifdef __JGI_BB_PEDANTIC__
			const_cast<FullBitboards*>(this)->kings ^= blackKing;
			#endif
		}

		const auto whitePawnAttacks = WhitePawnAttacks();
		const auto whiteKnightAttacks = WhiteKnightAttacks();
		
		auto mask = King_Attacks[posBlackKing] & ~black & ~King_Attacks[posWhiteKing] & ~whitePawnAttacks & ~whiteKnightAttacks;
		if constexpr (tbLongDistanceChecker == 1)
			mask &= ~(GetCommonDiagOrLine(posBlackKing, posChecker) & ~(1ULL << posChecker));

		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{			
			if (!IsSquareAttackedByWhite_GenMoves<-1,0>(pos))  // -1 == only long distance attackers (king, pawns and knights already verified)
			{
				if constexpr (tbLongDistanceChecker > 1) // dblcheck?
				{
					const_cast<FullBitboards*>(this)->black ^= blackKing;
					#ifdef __JGI_BB_PEDANTIC__
					const_cast<FullBitboards*>(this)->kings ^= blackKing;
					#endif
				}
				return true;
			}
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		if constexpr (tbLongDistanceChecker > 1) // dblcheck?
		{
			// as soon as tbLongDistanceChecker is always correct, taking king off is not needed
			const_cast<FullBitboards*>(this)->black ^= blackKing;
			#ifdef __JGI_BB_PEDANTIC__
			const_cast<FullBitboards*>(this)->kings ^= blackKing;
			#endif
		}

		return false;
	}
	#else
	template<char tbLongDistanceChecker = false>
	ALWAYS_INLINE bool FindOneValidMove4BlackKingWhenChecked(const char posChecker) CONST_RESTRICT
	{		
		const auto blackKing = black & kings;
		const_cast<FullBitboards*>(this)->black ^= blackKing;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->kings ^= blackKing;
		#endif

		const auto whitePawnAttacks = WhitePawnAttacks();
		
		auto mask = King_Attacks[posBlackKing] & ~black & ~King_Attacks[posWhiteKing] & ~whitePawnAttacks;					
		if constexpr (tbLongDistanceChecker == 1)
			mask &= ~(GetCommonDiagOrLine(posBlackKing, posChecker) & ~(1ULL << posChecker));

		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (!IsSquareAttackedByWhite_GenMoves<0,0>(pos))
			{
				const_cast<FullBitboards*>(this)->black ^= blackKing;
				#ifdef __JGI_BB_PEDANTIC__
				const_cast<FullBitboards*>(this)->kings ^= blackKing;
				#endif
				return true;
			}
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		const_cast<FullBitboards*>(this)->black ^= blackKing;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->kings ^= blackKing;
		#endif

		return false;
	}
	#endif

	// Method assumes that either black king is not checked, or moving on this square will block check
	template<bool tbInclKing = false, bool tbSquareKnownToBeNotOccupied = false>
	ALWAYS_INLINE bool CanBlackMoveOn(const int sq) CONST_RESTRICT
	{
		static_assert(!tbInclKing, "TODO");
		assert(IsValidPos(sq));
		assert(!IsBlackAt(sq));

		Bitboard mask;

		if constexpr (tbAnyBlackKnights)
		{
			mask = Knight_Attacks[sq] & black & knights;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				if (!IsBlackAbsolutelyPinned(pos))
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		if constexpr(tbBlackHaveRookLikes || tbBlackHaveBishopLikes)		
		{
			#ifdef __USE_MOVEGENINCANBLACKMOVEON__
			const auto rawBishopMoves = tbBlackHaveBishopLikes ? get_raw_bishop_moves(sq, occ()) : 0ULL;
			const auto rawRookMoves = tbBlackHaveRookLikes ? get_raw_rook_moves(sq, occ()) : 0ULL;
			mask = black & ((qbishops & rawBishopMoves) | (qrooks & rawRookMoves));
			#else
			mask = ((Rook_Attacks[sq] & qrooks) | (Bishop_Attacks[sq] & qbishops)) & black;
			#endif

			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				#ifndef __USE_MOVEGENINCANBLACKMOVEON__
				if (AllBetweenEmpty(pos, sq))
				#endif
					if (!IsBlackPinned(pos, sq))
						return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		if (!tbSquareKnownToBeNotOccupied && IsWhiteAt(sq))
		{
			mask = White_Pawn_Attacks[sq] & black & pawns;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				if (!IsBlackPinned(pos, sq))
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}
		else
		{
			mask = (sq_to_bb(sq)) & ((black & pawns) >> 8);
			if (mask)
				if (!IsBlackPinned(sq + 8, sq))
					return true;
			if ((sq >> 3) == _5_)
			{
				mask = (sq_to_bb(sq)) & ((black & pawns) >> 16);
				if (mask)
					if (IsEmptyAt(sq + 8))
						if (!IsBlackPinned(sq + 16, sq))
							return true;
			}
		}

		return false;
	}

	// The method itself is not inlined, but all the called methods are intended to be inlined, which speeds up the code about 1.5%
	template<bool tbEnPassantPossible = false>
	bool FindOneValidMove4OtherBlackPieceWhenChecked(const int posChecker) CONST_RESTRICT
	{
		assert(IsValidPos(posChecker));
		assert(IsBlackKingChecked() >= 0);
		
		if (CanBlackCapture_AlwaysInline<tbEnPassantPossible, 0>(posChecker)) // excl king (bl. king's neighborhood already verified in a call to FindOneValidMove4BlackKingWhenChecked)
			return true;

		const auto dist = Distance(posBlackKing, posChecker);

		switch (dist)
		{
			case 1:
				break;
			case 2:
				if (!IsKnightDiff(posBlackKing, posChecker))
					if (CanBlackMoveOn<0, 1>((posBlackKing + posChecker) / 2))
						return true;
				break;
			default:
				return CanBlackMoveInBetween_AlwaysInline(posBlackKing, posChecker);
		}

		return false;
	}

	template<bool tbEnPassantPossible = false, char tbLongDistanceChecker = false>
	ALWAYS_INLINE bool FindOneValidMove4BlackWhenChecked(const int posChecker) CONST_RESTRICT
	{
		assert(IsValidPos(posChecker) || posChecker == DBL_CHECKED);
		assert(IsBlackKingChecked() >= 0);

		if (FindOneValidMove4BlackKingWhenChecked<tbLongDistanceChecker>(posChecker))
			return true;

		if (posChecker != DBL_CHECKED)
			return FindOneValidMove4OtherBlackPieceWhenChecked<tbEnPassantPossible>(posChecker);

		return false;
	}
	ALWAYS_INLINE bool IsCheckMateAfterQueenCheck(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & queens() & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->qbishops |= toMask; // TODO: with __JGI_BB_PEDANTIC__ maybe take off from fromMask? (anyway it does not matter before FindOneValidMove4BlackWhenChecked since no white or black at this pos)
		const_cast<FullBitboards*>(this)->qrooks |= toMask; // ditto
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_QUEEN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked<0,1>(toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterRookDirectCheck(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & rooks() & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;
		
		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		if constexpr (!tbBlackHaveRookLikes)
			const_cast<FullBitboards*>(this)->qrooks ^= moveMask;
		else
			const_cast<FullBitboards*>(this)->qrooks |= toMask; // TODO: with __JGI_BB_PEDANTIC__ maybe take off from fromMask? (anyway it does not matter before FindOneValidMove4BlackWhenChecked since no white or black at this pos)
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked<0, 1>(toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore				

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterBishopDirectCheck(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & bishops() & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		if constexpr (!tbBlackHaveBishopLikes)
			const_cast<FullBitboards*>(this)->qbishops ^= moveMask;
		else
			const_cast<FullBitboards*>(this)->qbishops |= toMask; // TODO: with __JGI_BB_PEDANTIC__ maybe take off from fromMask? (anyway it does not matter before FindOneValidMove4BlackWhenChecked since no white or black at this pos)
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked<0,1>(toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	template<FIGURE fToSkip, FIGURE fAnotherTypeToSkip = 0>
	ALWAYS_INLINE void ClearOnPieceBitboardsExcept(const Bitboard maskBitsToClear)
	{
		// kings are not affected by this method
		if constexpr (fToSkip != FGR_PAWN && fAnotherTypeToSkip != FGR_PAWN)
			pawns &= ~maskBitsToClear;
		if constexpr (fToSkip != FGR_BISHOP && fToSkip != FGR_QUEEN && fAnotherTypeToSkip != FGR_BISHOP && fAnotherTypeToSkip != FGR_QUEEN)
			qbishops &= ~maskBitsToClear;
		if constexpr (fToSkip != FGR_ROOK && fToSkip != FGR_QUEEN && fAnotherTypeToSkip != FGR_ROOK && fAnotherTypeToSkip != FGR_QUEEN)
			qrooks &= ~maskBitsToClear;
		if constexpr (fToSkip != FGR_KNIGHT && fAnotherTypeToSkip != FGR_KNIGHT)
			knights &= ~maskBitsToClear;
		return;
	}
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterKnightDirectCheck(const int fromPos, const int toPos) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & knights & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownToBeNotACapture)
		{
			const auto captureMask = black & toMask;

			const auto bbSaved = *this; // save

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->knights |= toMask;
			const_cast<FullBitboards*>(this)->black ^= captureMask;
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KNIGHT>(captureMask);

			res = !FindOneValidMove4BlackWhenChecked<0, 0>(toPos);

			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert((toMask & occ()) == 0);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->knights ^= moveMask;

			res = !FindOneValidMove4BlackWhenChecked<0, 0>(toPos);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->knights ^= moveMask;
		}

		return res;
	}
	template<bool tbBlackEnPassantPossible = false, bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPawnDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert(!tbBlackEnPassantPossible || (toPos - fromPos == 16 && fromPos >= _A2_ && fromPos <= _H2_));

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownToBeNotACapture)
		{
			const auto captureMask = black & toMask;

			const auto bbSaved = *this; // save

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->pawns |= toMask;
			const_cast<FullBitboards*>(this)->black ^= captureMask;
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_PAWN>(captureMask);
			
			if (bDoubleCheck)
				res = !FindOneValidMove4BlackWhenChecked<tbBlackEnPassantPossible, DBL_CHECKED>(DBL_CHECKED);
			else
				res = !FindOneValidMove4BlackWhenChecked<tbBlackEnPassantPossible, 0>(toPos);

			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert((toMask & occ()) == 0);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= moveMask;

			if (bDoubleCheck)
				res = !FindOneValidMove4BlackWhenChecked<tbBlackEnPassantPossible, DBL_CHECKED>(DBL_CHECKED);
			else
				res = !FindOneValidMove4BlackWhenChecked<tbBlackEnPassantPossible, 0>(toPos);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= moveMask;
		}

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterEnPassantDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto posBlackPawn = (toPos & 7) + (fromPos >> 3) * 8;
		assert((sq_to_bb(posBlackPawn)) & black & pawns);
		const auto captureMask = sq_to_bb(posBlackPawn);

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= (moveMask | captureMask);
		const_cast<FullBitboards*>(this)->black ^= captureMask;

		bool res; 
		if (bDoubleCheck)
			res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
		else
			res = !FindOneValidMove4BlackWhenChecked<0,0>(toPos);

		// Restore
		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= (moveMask | captureMask);
		const_cast<FullBitboards*>(this)->black ^= captureMask;

		return res;
	}
	// posWhiteLongDistAttackerInEnPassant can be DBL_CHECKED, e.g. 8/6N1/3k1P2/1K1Pp3/7p/2p3B1/3R4/8 d5:e6++
	ALWAYS_INLINE bool IsCheckMateAfterEnPassantDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttackerInEnPassant) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto posBlackPawn = (toPos & 7) + (fromPos >> 3) * 8;
		assert((sq_to_bb(posBlackPawn)) & black & pawns);
		const auto captureMask = sq_to_bb(posBlackPawn);

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= (moveMask | captureMask);
		const_cast<FullBitboards*>(this)->black ^= captureMask;

		bool res;
		if (posWhiteLongDistAttackerInEnPassant == DBL_CHECKED)
			res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
		else
			res = !FindOneValidMove4BlackWhenChecked<0,1>(posWhiteLongDistAttackerInEnPassant);

		// Restore:
		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= (moveMask | captureMask);
		const_cast<FullBitboards*>(this)->black ^= captureMask;

		return res;
	}
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPromoToKnightDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((fromPos >> 3) == _7_);
		assert(toPos >= _A8_);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownToBeNotACapture)
		{
			const auto captureMask = black & toMask;

			const auto bbSaved = *this; // save

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->knights |= toMask;
			const_cast<FullBitboards*>(this)->black ^= captureMask;
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KNIGHT, FGR_PAWN>(captureMask);
			
			if (bDoubleCheck)
				res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
			else
				res = !FindOneValidMove4BlackWhenChecked<0, 0>(toPos);

			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert((toMask & occ()) == 0);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->knights |= toMask;

			if (bDoubleCheck)
				res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
			else
				res = !FindOneValidMove4BlackWhenChecked<0, 0>(toPos);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->knights &= ~toMask;
		}

		return res;
	}
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPromoToQueenDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((fromPos >> 3) == _7_);
		assert(toPos >= _A8_);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownToBeNotACapture)
		{
			const auto captureMask = black & toMask;

			const auto bbSaved = *this; // save

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->qrooks |= toMask;
			const_cast<FullBitboards*>(this)->qbishops |= toMask;
			const_cast<FullBitboards*>(this)->black ^= captureMask;
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_QUEEN, FGR_PAWN>(captureMask);
			
			if (bDoubleCheck)
				res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
			else
				res = !FindOneValidMove4BlackWhenChecked<0, 1>(toPos);

			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert((toMask & occ()) == 0);
			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->qrooks |= toMask;
			const_cast<FullBitboards*>(this)->qbishops |= toMask;

			if (bDoubleCheck)
				res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
			else
				res = !FindOneValidMove4BlackWhenChecked<0, 1>(toPos);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->qrooks &= ~toMask;
			const_cast<FullBitboards*>(this)->qbishops &= ~toMask;
		}

		return res;
	}
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPromoToRookDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) CONST_RESTRICT
	{
		assert(!tbKnownToBeNotACapture); // TODO
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((fromPos >> 3) == _7_);
		assert(toPos >= _A8_);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->qrooks |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK, FGR_PAWN>(captureMask);

		bool res; 
		if (bDoubleCheck)
			res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
		else
			res = !FindOneValidMove4BlackWhenChecked<0,1>(toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPromoToBishopDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) CONST_RESTRICT
	{
		assert(!tbKnownToBeNotACapture); // TODO
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((fromPos >> 3) == _7_);
		assert(toPos >= _A8_);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->qbishops |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP, FGR_PAWN>(captureMask);

		bool res; 
		if (bDoubleCheck)
			res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
		else
			res = !FindOneValidMove4BlackWhenChecked<0,1>(toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	ALWAYS_INLINE bool IsCheckMateAfterRookDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & rooks() & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= fromMask;
		const_cast<FullBitboards*>(this)->qrooks |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK>(captureMask);

		assert(AllBetweenEmpty(posWhiteLongDistAttacker, posBlackKing));
		const bool doubleCheck = SameLineAndAllBetweenEmpty(posBlackKing, toPos);
		bool res;
		if (doubleCheck)
			res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
		else
			res = !FindOneValidMove4BlackWhenChecked<0,1>(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterBishopDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & bishops() & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->qbishops ^= fromMask;
		const_cast<FullBitboards*>(this)->qbishops |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP>(captureMask);

		assert(AllBetweenEmpty(posWhiteLongDistAttacker, posBlackKing));
		const bool doubleCheck = SameDiagAndAllBetweenEmpty(posBlackKing, toPos);
		bool res; 
		if (doubleCheck)
			res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
		else
			res = !FindOneValidMove4BlackWhenChecked<0,1>(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterKnightDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & knights & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownToBeNotACapture)
		{
			const auto captureMask = black & toMask;

			const auto bbSaved = *this; // save

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->knights ^= fromMask;
			const_cast<FullBitboards*>(this)->knights |= toMask;
			const_cast<FullBitboards*>(this)->black ^= captureMask;
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KNIGHT>(captureMask);

			assert(AllBetweenEmpty(posWhiteLongDistAttacker, posBlackKing));
			const bool doubleCheck = IsKnightDiff(posBlackKing, toPos);			
			if (doubleCheck)
				res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
			else
				res = !FindOneValidMove4BlackWhenChecked<0, 1>(posWhiteLongDistAttacker);

			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert((toMask & occ()) == 0);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->knights ^= moveMask;

			assert(AllBetweenEmpty(posWhiteLongDistAttacker, posBlackKing));
			const bool doubleCheck = IsKnightDiff(posBlackKing, toPos);
			if (doubleCheck)
				res = !FindOneValidMove4BlackWhenChecked<0, DBL_CHECKED>(DBL_CHECKED);
			else
				res = !FindOneValidMove4BlackWhenChecked<0, 1>(posWhiteLongDistAttacker);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->knights ^= moveMask;
		}

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterKingDiscoveredCheck(const int toPos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	{
		const auto fromPos = posWhiteKing;
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & kings & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->posWhiteKing = toPos;
		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KING>(captureMask);

		assert(AllBetweenEmpty(posWhiteLongDistAttacker, posBlackKing));
		const auto res = !FindOneValidMove4BlackWhenChecked<0,1>(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	// This method should not be called on direct check together with discovered check (assertion inside)
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPromoToQueenDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((fromPos >> 3) == _7_);
		assert(toPos >= _A8_);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownToBeNotACapture)
		{
			const auto captureMask = black & toMask;

			const auto bbSaved = *this; // save

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->qrooks |= toMask;
			const_cast<FullBitboards*>(this)->qbishops |= toMask;
			const_cast<FullBitboards*>(this)->black ^= captureMask;
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_QUEEN, FGR_PAWN>(captureMask);

			assert(!SameDiagonalOrLineAndAllBetweenEmpty(toPos, posBlackKing));
			res = !FindOneValidMove4BlackWhenChecked<0, 1>(posWhiteLongDistAttacker);

			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert((toMask & occ()) == 0);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->qrooks |= toMask;
			const_cast<FullBitboards*>(this)->qbishops |= toMask;

			assert(!SameDiagonalOrLineAndAllBetweenEmpty(toPos, posBlackKing));
			res = !FindOneValidMove4BlackWhenChecked<0, 1>(posWhiteLongDistAttacker);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->qrooks &= ~toMask;
			const_cast<FullBitboards*>(this)->qbishops &= ~toMask;
		}

		return res;
	}
	// This method should not be called on direct check together with discovered check (assertion inside)
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPromoToRookDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	{
		assert(!tbKnownToBeNotACapture); // TODO
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((fromPos >> 3) == _7_);
		assert(toPos >= _A8_);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->qrooks |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK, FGR_PAWN>(captureMask);
		
		assert(!SameLineAndAllBetweenEmpty(toPos, posBlackKing));
		const auto res = !FindOneValidMove4BlackWhenChecked<0,1>(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	// This method should not be called on direct check together with discovered check (assertion inside)
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPromoToBishopDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	{
		assert(!tbKnownToBeNotACapture); // TODO
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((fromPos >> 3) == _7_);
		assert(toPos >= _A8_);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->qbishops |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP, FGR_PAWN>(captureMask);

		assert(!SameDiagAndAllBetweenEmpty(toPos, posBlackKing));
		const auto res = !FindOneValidMove4BlackWhenChecked<0,1>(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	// This method should not be called on direct check together with discovered check (assertion inside)
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPromoToKnightDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((fromPos >> 3) == _7_);
		assert(toPos >= _A8_);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);
		assert(!IsKnightDiff(toPos, posBlackKing));

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownToBeNotACapture)
		{
			const auto captureMask = black & toMask;

			const auto bbSaved = *this; // save

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->knights |= toMask;
			const_cast<FullBitboards*>(this)->black ^= captureMask;
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KNIGHT, FGR_PAWN>(captureMask);
			
			res = !FindOneValidMove4BlackWhenChecked<0, 1>(posWhiteLongDistAttacker);

			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert((toMask & occ()) == 0);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->knights |= toMask;

			res = !FindOneValidMove4BlackWhenChecked<0, 1>(posWhiteLongDistAttacker);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->knights &= ~toMask;
		}

		return res;
	}
	template<bool tbKnownToBeNotACapture = false>
	ALWAYS_INLINE bool IsCheckMateAfterPawnDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & pawns & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		bool res;

		if constexpr (!tbKnownToBeNotACapture)
		{
			const auto captureMask = black & toMask;

			const auto bbSaved = *this; // save

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= fromMask;
			const_cast<FullBitboards*>(this)->pawns |= toMask;
			const_cast<FullBitboards*>(this)->black ^= captureMask;
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_PAWN>(captureMask);

			res = !FindOneValidMove4BlackWhenChecked<0, 1>(posWhiteLongDistAttacker);

			*(const_cast<FullBitboards*>(this)) = bbSaved; // restore
		}
		else
		{
			assert((toMask & occ()) == 0);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= moveMask;

			res = !FindOneValidMove4BlackWhenChecked<0, 1>(posWhiteLongDistAttacker);

			const_cast<FullBitboards*>(this)->white ^= moveMask;
			const_cast<FullBitboards*>(this)->pawns ^= moveMask;
		}

		return res;
	}

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	bool IsPinnedFlagOK(const char sq, const bool pinned) CONST_RESTRICT
	{
		assert(IsValidPos(sq));
		assert(!IsEmptyAt(sq));

		if (IsWhiteAt(sq))
		{			
			const auto pinningPieceFound = SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, sq) && BlackLongDistanceFigureInDir(sq, posWhiteKing);
			return pinningPieceFound == pinned;
		}
		else
		{
			const auto pinningPieceFound = SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, sq) && WhiteLongDistanceFigureInDir(sq, posBlackKing);
			return pinningPieceFound == pinned;
		}
	}
	#endif

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	ALWAYS_INLINE bool CanWhiteQueenCheckMate(const int qpos, const bool pinned) CONST_RESTRICT
	#else
	ALWAYS_INLINE bool CanWhiteQueenCheckMate(const int qpos) CONST_RESTRICT
	#endif
	{
		constexpr bool tbCanBePinned = (tbBlackHaveRookLikes || tbBlackHaveBishopLikes);

		assert(!IsSquareAttackedByBlack(posWhiteKing));
		assert(IsValidPos(qpos));
		assert(white & queens() & (1ULL << qpos));
				
		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(qpos, pinned));
		if (tbCanBePinned && pinned)
		{
			const auto posBlackPinner = BlackLongDistanceFigureInDir<1, 1>(qpos, posWhiteKing);
			assert(IsValidPos(posBlackPinner));

			// Can White Queen capture Black pinner with check?
			if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackPinner, posBlackKing)) 
				if (!IsSquareSureToBeAttackedByNotPinnedBlackPiece(posBlackPinner))
					if (IsCheckMateAfterQueenCheck(qpos, posBlackPinner))
						return true;

			const bool isPinnerPinnedItself = IsBlackPinned(posBlackPinner, qpos);
			if (isPinnerPinnedItself)
			{
				const auto movesFromKing = get_raw_bishop_moves(posBlackKing, occ()) | get_raw_rook_moves(posBlackKing, occ());
				auto mask = GetBetweenMask(posBlackPinner, posWhiteKing) & movesFromKing;
				BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
				{
					if (!IsSquareSureToBeAttackedByNotPinnedBlackPiece(pos))
						if (IsCheckMateAfterQueenCheck(qpos, pos))
							return true;			
				}
				END_FOR_EACH_POS_IN_MASK(pos, mask);
			}
		}
		else
		#endif
		{
			#ifdef __USE_MOVEGENINCANWHITEQUEENCHECK__
			const auto occ = this->occ();
			const auto movesByQueen = get_raw_bishop_moves(qpos, occ) | get_raw_rook_moves(qpos, occ);		
			const auto movesFromKing = get_raw_bishop_moves(posBlackKing, occ) | get_raw_rook_moves(posBlackKing, occ);
			auto mask = movesByQueen & movesFromKing & (~white);
			#else
			auto mask = Queen_Attacks[posBlackKing] & Queen_Attacks[qpos] & (~white);
			#endif

			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				#ifndef __USE_MOVEGENINCANWHITEQUEENCHECK__
				if (AllBetweenEmpty(qpos, pos) & AllBetweenEmpty(pos, posBlackKing))			
				#endif
					#ifndef __PREEMPTIVE_WHITEPINNEDPIECES__
					if (!tbCanBePinned || !IsWhitePinned(qpos, pos))
					#endif
						if (!IsSquareSureToBeAttackedByNotPinnedBlackPiece(pos))
							if (IsCheckMateAfterQueenCheck(qpos, pos))
								return true;			
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}
		return false;
	}

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	bool CanWhiteRookMakeDiscoveredCheckMate(const int rpos, const int posWhiteLongDistAttacker, const bool pinned) CONST_RESTRICT
	#else
	bool CanWhiteRookMakeDiscoveredCheckMate(const int rpos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	#endif
	{
		assert(IsValidPos(rpos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(posWhiteLongDistAttacker != rpos && SameDiag(posWhiteLongDistAttacker, rpos));

		constexpr bool tbCanBePinned = (tbBlackHaveRookLikes || tbBlackHaveBishopLikes);

		// Discovered check (and direct check maybe)
		auto trgtBitboard = get_rook_moves(rpos, occ(), white);

		#if !defined(__PREEMPTIVE_WHITEPINNEDPIECES__)
		const bool pinned = tbCanBePinned && SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, rpos) && BlackLongDistanceFigureInDir(rpos, posWhiteKing);
		#endif

		if (tbCanBePinned && pinned)
		{				
			if (SameLine(posWhiteKing, rpos)) // the only chance for legal move when pinned
			{
				BEGIN_FOR_EACH_POS_IN_MASK(pos, trgtBitboard)
				{
					if (IsSquareAlongTheLineOrDiag(pos, rpos, posWhiteKing))
						if (IsCheckMateAfterRookDiscoveredCheck(rpos, pos, posWhiteLongDistAttacker))
							return true;
				}
				END_FOR_EACH_POS_IN_MASK(pos, trgtBitboard);
			}
		}
		else
		{
			BEGIN_FOR_EACH_POS_IN_MASK(pos, trgtBitboard)
			{
				if (IsCheckMateAfterRookDiscoveredCheck(rpos, pos, posWhiteLongDistAttacker))
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, trgtBitboard);
		}

		return false;
	}

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	ALWAYS_INLINE bool CanWhiteRookCheckMate(const int rpos, const bool discoveredCheckPossible, const bool pinned) CONST_RESTRICT
	#else
	ALWAYS_INLINE bool CanWhiteRookCheckMate(const int rpos) CONST_RESTRICT
	#endif
	{
		constexpr bool tbCanBePinned = (tbBlackHaveRookLikes || tbBlackHaveBishopLikes);

		assert(!IsSquareAttackedByBlack(posWhiteKing));
		assert(IsValidPos(rpos));
		assert(white & rooks() & (1ULL << rpos));

		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(rpos, pinned));
		if (discoveredCheckPossible)
		{ 
			const int posWhiteLongDistAttacker = WhiteLongDistanceFigureInDir<1,1>(rpos, posBlackKing);
			assert(posWhiteLongDistAttacker >= 0);
		#else
			if (!is_edge(rpos) & SameDiagAndAllBetweenEmpty(posBlackKing, rpos))		
				if (const auto mask = GetCandidatesForWhiteLongDistanceFigureInDir<0>(rpos, posBlackKing))		
				{
					const int posWhiteLongDistAttacker = ValidateCandidateForLongDistanceFigureInDir(mask, rpos, posBlackKing);
					if (posWhiteLongDistAttacker >= 0)
		#endif
				{
					#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
					return CanWhiteRookMakeDiscoveredCheckMate(rpos, posWhiteLongDistAttacker, pinned);
					#else
					return CanWhiteRookMakeDiscoveredCheckMate(rpos, posWhiteLongDistAttacker);
					#endif
				}
			}		

		// Direct check:
		#ifdef __USE_MOVEGENINCANWHITEROOKCHECK__
			const auto occ = this->occ();
			auto mask = get_raw_rook_moves(rpos, occ) & get_raw_rook_moves(posBlackKing, occ) & ~white;
		#else
			auto mask = Rook_Attacks[posBlackKing] & Rook_Attacks[rpos] & (~white);
			#ifdef __USE_OPTIM_FOR_SAMEDIAGORLINE__
			const bool sameLine = SameLine(posBlackKing, rpos);
			const auto maskCapture = mask & black;
			mask = sameLine ? maskCapture : mask; // cmov; only a capture can be a checkmate when on the same line with black king
			#endif
		#endif
			
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			#ifndef __USE_MOVEGENINCANWHITEROOKCHECK__
			if (AllBetweenEmpty(rpos, pos) & AllBetweenEmpty(pos, posBlackKing))
			#endif
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!tbCanBePinned || !pinned || IsSquareAlongTheLineOrDiag(pos, rpos, posWhiteKing))
				#else
				if (!tbCanBePinned || !IsWhitePinned(rpos, pos))
				#endif
					if (!IsSquareSureToBeAttackedByNotPinnedBlackPiece(pos))
						if (IsCheckMateAfterRookDirectCheck(rpos, pos))
							return true;
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);
		

		return false;
	}

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	bool CanWhiteBishopMakeDiscoveredCheckMate(const int bpos, const int posWhiteLongDistAttacker, const bool pinned) CONST_RESTRICT
	#else
	bool CanWhiteBishopMakeDiscoveredCheckMate(const int bpos, const int posWhiteLongDistAttacker) CONST_RESTRICT
	#endif
	{
		constexpr bool tbCanBePinned = (tbBlackHaveRookLikes || tbBlackHaveBishopLikes);

		assert(IsValidPos(bpos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(bpos != posWhiteLongDistAttacker && SameLine(bpos, posWhiteLongDistAttacker));

		// Discovered check (and direct check maybe)
		auto trgtBitboard = get_bishop_moves(bpos, occ(), white);

		#if !defined(__PREEMPTIVE_WHITEPINNEDPIECES__)
		const bool pinned = tbCanBePinned && SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, bpos) && BlackLongDistanceFigureInDir(bpos, posWhiteKing);
		#endif

		if (tbCanBePinned && pinned)
		{
			if (SameDiag(posWhiteKing, bpos)) // the only chance for legal move when pinned
			{
				BEGIN_FOR_EACH_POS_IN_MASK(pos, trgtBitboard)
				{
					if (IsSquareAlongTheLineOrDiag(pos, bpos, posWhiteKing))
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterBishopDiscoveredCheck(bpos, pos, posWhiteLongDistAttacker))
							return true;
				}
				END_FOR_EACH_POS_IN_MASK(pos, trgtBitboard);
			}
		}
		else
		{
			BEGIN_FOR_EACH_POS_IN_MASK(pos, trgtBitboard)
			{
				if (const_cast<FullBitboards*>(this)->IsCheckMateAfterBishopDiscoveredCheck(bpos, pos, posWhiteLongDistAttacker))
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, trgtBitboard);
		}

		return false;
	}

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	ALWAYS_INLINE bool CanWhiteBishopCheckMate(const int bpos, const bool discoveredCheckPossible, const bool pinned) CONST_RESTRICT
	#else
	ALWAYS_INLINE bool CanWhiteBishopCheckMate(const int bpos) CONST_RESTRICT
	#endif
	{
		constexpr bool tbCanBePinned = (tbBlackHaveRookLikes || tbBlackHaveBishopLikes);

		assert(!IsSquareAttackedByBlack(posWhiteKing));
		assert(IsValidPos(bpos));
		assert(white & bishops() & (1ULL << bpos));

		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(bpos, pinned));
		if (discoveredCheckPossible)
		{
			const int posWhiteLongDistAttacker = WhiteLongDistanceFigureInDir<1,1>(bpos, posBlackKing);
			assert(posWhiteLongDistAttacker >= 0);
		#else
		const int posWhiteLongDistAttacker = WhiteMatchOnRayIfAllBetweenEmpty(posBlackKing, bpos);
		if (posWhiteLongDistAttacker >= 0)
		{
		#endif		
			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			return CanWhiteBishopMakeDiscoveredCheckMate(bpos, posWhiteLongDistAttacker, pinned);
			#else
			return CanWhiteBishopMakeDiscoveredCheckMate(bpos, posWhiteLongDistAttacker);
			#endif
		}
		else
		{
			// Direct check:
			#ifdef __USE_MOVEGENINCANWHITEBISHOPCHECK__
				const auto occ = this->occ();
				auto mask = get_raw_bishop_moves(bpos, occ) & get_raw_bishop_moves(posBlackKing, occ) & ~white;
			#else
				auto mask = Bishop_Attacks[posBlackKing] & Bishop_Attacks[bpos] & (~white);			
				#ifdef __USE_OPTIM_FOR_SAMEDIAGORLINE__
				const bool sameDiag = SameDiag(posBlackKing, bpos);
				const auto maskCapture = mask & black;
				mask = sameDiag ? maskCapture : mask; // cmov; only a capture can be a checkmate when on the same diagonal with black king
				#endif			
			#endif	

			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				#ifndef __USE_MOVEGENINCANWHITEBISHOPCHECK__
				if (AllBetweenEmpty(bpos, pos) & AllBetweenEmpty(pos, posBlackKing))
				#endif
					#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
					if (!tbCanBePinned || !pinned || IsSquareAlongTheLineOrDiag(pos, bpos, posWhiteKing))
					#else
					if (!tbCanBePinned || !IsWhitePinned(bpos, pos))
					#endif
						if (!IsSquareSureToBeAttackedByNotPinnedBlackPiece(pos))
							if (IsCheckMateAfterBishopDirectCheck(bpos, pos))
								return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		return false;
	}

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	ALWAYS_INLINE bool CanWhiteKnightCheckMate(const int kpos, const bool discoveredCheckPossible) CONST_RESTRICT
	#else
	ALWAYS_INLINE bool CanWhiteKnightCheckMate(const int kpos) CONST_RESTRICT
	#endif
	{
		assert(!IsSquareAttackedByBlack(posWhiteKing));
		assert(IsValidPos(kpos));
		assert(white & knights & (1ULL << kpos));
		
		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(kpos, false));
		constexpr bool pinned = false; // already verified before the call; compiler will optimize out the below 'if' condition
		if constexpr (!pinned)
		#else
		constexpr bool tbCanBePinned = (tbBlackHaveRookLikes || tbBlackHaveBishopLikes);
		const bool pinned = tbCanBePinned && SameDiagonalOrLineAndAllBetweenEmpty<1>(posWhiteKing, kpos) && BlackLongDistanceFigureInDir(kpos, posWhiteKing);
		if (!tbCanBePinned || !pinned)
		#endif		
		{			
			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			if (discoveredCheckPossible)
			{
				const int posWhiteLongDistAttacker = WhiteLongDistanceFigureInDir<1, 1>(kpos, posBlackKing);
			#else
			const int posWhiteLongDistAttacker = WhiteMatchOnRayIfAllBetweenEmpty(posBlackKing, kpos);
			if (posWhiteLongDistAttacker >= 0)
			{
			#endif

				// Discovered check (and direct check maybe)
				auto trgtBitboard = Knight_Attacks[kpos] & (~white);

				BEGIN_FOR_EACH_POS_IN_MASK(pos, trgtBitboard)
				{
					if (IsCheckMateAfterKnightDiscoveredCheck(kpos, pos, posWhiteLongDistAttacker))
						return true;
				}
				END_FOR_EACH_POS_IN_MASK(pos, trgtBitboard);
			}
			else
			{
				// Direct check:
				auto mask = Knight_Attacks[posBlackKing] & Knight_Attacks[kpos] & (~white);

				BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
				{
					if (!IsSquareSureToBeAttackedByNotPinnedBlackPiece(pos))
						if (IsCheckMateAfterKnightDirectCheck(kpos, pos))
							return true;
				}
				END_FOR_EACH_POS_IN_MASK(pos, mask);
			}
		}

		return false;
	}

	// Intended for super-fast detection of some non-pinned attackers (not for a conclusive verification of their existence)
	ALWAYS_INLINE bool IsSquareSureToBeAttackedByNotPinnedBlackPiece(const int sq) const
	{
		assert(IsValidPos(sq));

		#if defined(__USE_FASTDETECTIONOFCAPTURABLECHECKER__) || defined(__USE_FASTDETECTIONOFCAPTURABLECHECKEREXT__)
			#if defined(__USE_FASTDETECTIONOFCAPTURABLECHECKEREXT__)
			return (((White_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights) | (((Rook_Attacks[sq] & qrooks) | (Bishop_Attacks[sq] & qbishops)) & King_Attacks[sq])) & ~Queen_Attacks[posBlackKing] & black) != 0;
			#else
			return (((White_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights)) & ~Queen_Attacks[posBlackKing] & black) != 0;
			#endif
		#else
			return false; // the code of the method will be optimized out if feature is off
		#endif
	}

	ALWAYS_INLINE static int GetSquareDiff(const int sqFrom, const int sqTo)
	{
		assert(IsValidPos(sqFrom));
		assert(IsValidPos(sqTo));
		assert(SameDiagonalOrLine(sqFrom, sqTo));

		const int dx = sgn((sqTo & 7) - (sqFrom & 7));
		const int dy = sgn((sqTo >> 3) - (sqFrom >> 3));
		const int diff = dx + dy * 8;

		return diff;
	}

	// Templ.params should be true if a pair king+rook didn't move yet
	template<bool tbShortCastlingPossible = false, bool tbLongCastlingPossible = false>
	ALWAYS_INLINE bool CanWhiteKingCheckMate() CONST_RESTRICT
	{
		assert(IsValidPos(posWhiteKing));
		assert(white & kings & (1ULL << posWhiteKing));
		
		const auto mask = white & rayLookup.MatchOnRay(posWhiteKing, posBlackKing, qrooks, qbishops);
		if ((mask != 0) & SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, posBlackKing))
			if (int posWhiteLongDistAttacker; (posWhiteLongDistAttacker = ValidateCandidateForLongDistanceFigureInDir(mask, posWhiteKing, posBlackKing)) >= 0)
			{
				#ifdef __USE_OPTIMINCANWHITEKINGCHECKMATE__
				const auto blackPawnAttacks = BlackPawnAttacks();
				const auto blackKnightAttacks = BlackKnightAttacks();
				auto mask = King_Attacks[posWhiteKing] & (~white) & ~GetBetweenMask(posWhiteLongDistAttacker, posBlackKing) & ~King_Attacks[posBlackKing] & ~blackPawnAttacks & ~blackKnightAttacks;
				#else			
				auto mask = King_Attacks[posWhiteKing] & (~white) & ~GetBetweenMask(posWhiteLongDistAttacker, posBlackKing);
				#endif

				BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
				{
					#ifdef __USE_OPTIMINCANWHITEKINGCHECKMATE__
					if (!IsSquareAttackedByBlackIfTakeOffWhiteKing<-1>(pos))
					#else				
					if (!IsSquareAttackedByBlackIfTakeOffWhiteKing(pos))
					#endif
						if (IsCheckMateAfterKingDiscoveredCheck(pos, posWhiteLongDistAttacker))
							return true;
				}
				END_FOR_EACH_POS_IN_MASK(pos, mask);
			}			

		if ((tbShortCastlingPossible | tbLongCastlingPossible) && IsWhiteKingAt(_E1_))
		{
			assert(!IsSquareAttackedByBlack(_E1_));
			if constexpr (tbShortCastlingPossible) // it says only that wh.Ke1 didn't move and wh.Rh1 didn't move yet (provided on their initial positions)
				if (IsWhiteRookAt(_H1_))
					if (IsEmptyAt(_F1_) && IsEmptyAt(_G1_))
						if (((posBlackKing & 7) == _F_ && AllBetweenEmpty(_F1_, posBlackKing)) || ((posBlackKing >> 3) == _1_ && AllBetweenEmpty(_E1_, posBlackKing)))
							if (!IsSquareAttackedByBlack(_E1_) && !IsSquareAttackedByBlack(_F1_) && !IsSquareAttackedByBlack(_G1_))
							{
								constexpr auto kingMoveMask = (1ULL << _E1_) | (1ULL << _G1_);
								constexpr auto rookMoveMask = (1ULL << _H1_) | (1ULL << _F1_);
								constexpr auto bothMasks = kingMoveMask | rookMoveMask;
								const_cast<FullBitboards*>(this)->white ^= bothMasks;
								const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
								const_cast<FullBitboards*>(this)->qrooks ^= rookMoveMask;
								const_cast<FullBitboards*>(this)->posWhiteKing = _G1_;

								const auto res = !FindOneValidMove4BlackWhenChecked<0,1>(_F1_);

								const_cast<FullBitboards*>(this)->white ^= bothMasks;
								const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
								const_cast<FullBitboards*>(this)->qrooks ^= rookMoveMask;
								const_cast<FullBitboards*>(this)->posWhiteKing = _E1_;

								if (res)
									return res;
							}

			if constexpr (tbLongCastlingPossible) // it says only that wh.Ke1 didn't move and wh.Ra1 didn't move yet  (provided on their initial positions)
				if (IsWhiteRookAt(_A1_))
					if (IsEmptyAt(_D1_) && IsEmptyAt(_C1_) && IsEmptyAt(_B1_))
						if (((posBlackKing & 7) == _D_ && AllBetweenEmpty(_D1_, posBlackKing)) || ((posBlackKing >> 3) == _1_ && AllBetweenEmpty(_E1_, posBlackKing)))
							if (!IsSquareAttackedByBlack(_E1_) && !IsSquareAttackedByBlack(_D1_) && !IsSquareAttackedByBlack(_C1_))
							{
								constexpr auto kingMoveMask = (1ULL << _E1_) | (1ULL << _C1_);
								constexpr auto rookMoveMask = (1ULL << _A1_) | (1ULL << _D1_);
								constexpr auto bothMasks = kingMoveMask | rookMoveMask;
								const_cast<FullBitboards*>(this)->white ^= bothMasks;
								const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
								const_cast<FullBitboards*>(this)->qrooks ^= rookMoveMask;
								const_cast<FullBitboards*>(this)->posWhiteKing = _C1_;

								const auto res = !FindOneValidMove4BlackWhenChecked<0,1>(_D1_);

								const_cast<FullBitboards*>(this)->white ^= bothMasks;
								const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
								const_cast<FullBitboards*>(this)->qrooks ^= rookMoveMask;
								const_cast<FullBitboards*>(this)->posWhiteKing = _E1_;

								if (res)
									return res;
							}
		}

		return false;
	}

	// tbVerifyEnPassantOnly == true can be useful when bl.pawn is a checker, however some verifications of white self discover are redundant in such case - that's why tbVerifyEnPassantOnly can be >1 - then these checks are skipped 
	// (it is then assumed that bposToCaptureWithEnPassant is the only checker of white king - a double move by pawn cannot be a double check)
	template<bool tbEnPassantPossible = false>
	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	bool CanWhitePawnCheckMate(const int ppos, const int bposToCaptureWithEnPassant, const bool pinned) CONST_RESTRICT
	#else
	bool CanWhitePawnCheckMate(const int ppos, const int bposToCaptureWithEnPassant) CONST_RESTRICT
	#endif
	{
		constexpr bool tbCanBePinned = (tbBlackHaveRookLikes || tbBlackHaveBishopLikes);

		assert(!IsSquareAttackedByBlack(posWhiteKing)); // CanWhiteCheckMateWhenChecked can only be called in case wh.king is under check
		assert(IsValidPos(ppos));
		assert(white & pawns & (1ULL << ppos));
		assert(!tbEnPassantPossible || (bposToCaptureWithEnPassant >= _A5_ && bposToCaptureWithEnPassant <= _H5_));				
		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(ppos, pinned));
		#endif

		const int posWhiteLongDistAttacker = WhiteMatchOnRayIfAllBetweenEmpty(posBlackKing, ppos);
		
		// Capture with direct check?
		auto maskToCapture = White_Pawn_Attacks[ppos] & black & Black_Pawn_Attacks[posBlackKing];
		BEGIN_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture)
		{
			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			if (!tbCanBePinned || !pinned || IsSquareAlongTheLineOrDiag(posToCapture, ppos, posWhiteKing))
			#else
			if (!tbCanBePinned || IsWhitePinned(ppos, posToCapture))
			#endif
			{
				assert((ppos & 7) != (posToCapture & 7));
				const bool bDoubleCheck = posWhiteLongDistAttacker >= 0;
				if (IsCheckMateAfterPawnDirectCheck(ppos, posToCapture, bDoubleCheck))
					return true;
			}
		}
		END_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture);

		// Move forward with direct check?
		const bool bMoveForwardPossible = IsEmptyAt(ppos + 8);
		const bool bKingCheckedAfterMoveForward = ((White_Pawn_Attacks[ppos + 8] & black & kings) != 0) & bMoveForwardPossible;
		if (bKingCheckedAfterMoveForward)
			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			if (!tbCanBePinned || !pinned || IsSquareAlongTheLineOrDiag(ppos + 8, ppos, posWhiteKing))
			#else
			if (!tbCanBePinned || !IsWhitePinned(ppos, ppos + 8))
			#endif
				if (IsCheckMateAfterPawnDirectCheck<0,1>(ppos, ppos + 8))
					return true;

		// Double move forward with direct check?
		constexpr Bitboard secondLine = 255UL << 8;
		const bool bKingCheckedAfterDoubleMoveForward = bMoveForwardPossible & (((Black_Pawn_Attacks[posBlackKing] >> 16) & (1ULL << ppos) & secondLine) != 0); //((ppos >> 3) == _2_) & ((posBlackKing >> 3) == _5_) & bMoveForwardPossible & (abs((ppos & 7) - (posBlackKing & 7)) == 1);
		if (bKingCheckedAfterDoubleMoveForward)
			if (IsEmptyAt(ppos + 16))
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!tbCanBePinned || !pinned || IsSquareAlongTheLineOrDiag(ppos + 16, ppos, posWhiteKing))
				#else
				if (!tbCanBePinned || !IsWhitePinned(ppos, ppos + 16))
				#endif
					if (IsCheckMateAfterPawnDirectCheck<1,1>(ppos, ppos + 16))
						return true;

		// promo:
		if (ppos >= _A7_)
		{
			// promo forward:
			if (bMoveForwardPossible)
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!tbCanBePinned || !pinned) // promo forward cannot be along the pinning line or diagonal || IsSquareAlongTheLineOrDiag(ppos + 8, ppos, posWhiteKing))
				#else
				if (!tbCanBePinned || !IsWhitePinned(ppos, ppos + 8))
				#endif
				{
					bool bPromoToQueenDirectCheck = false;
					const bool bPromoToKnightDirectCheck = IsKnightDiff(ppos + 8, posBlackKing);
					if (bPromoToKnightDirectCheck)
					{
						if (IsCheckMateAfterPromoToKnightDirectCheck<1>(ppos, ppos + 8, posWhiteLongDistAttacker >= 0))
							return true;
					}
					else
						if ((bPromoToQueenDirectCheck = SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(ppos + 8, posBlackKing, ppos)) != 0)						
							if (IsCheckMateAfterPromoToQueenDirectCheck<1>(ppos, ppos + 8, posWhiteLongDistAttacker >= 0))
								return true;						

					if (posWhiteLongDistAttacker >= 0)
					{
						if (!bPromoToQueenDirectCheck)
							if (IsCheckMateAfterPromoToQueenDiscoveredCheck<1>(ppos, ppos + 8, posWhiteLongDistAttacker))
								return true;
						if (!bPromoToKnightDirectCheck)
							if (IsCheckMateAfterPromoToKnightDiscoveredCheck<1>(ppos, ppos + 8, posWhiteLongDistAttacker))
								return true;
					}
				}

			// promo capture:
			auto maskToCapture = White_Pawn_Attacks[ppos] & black;
			#ifdef __USE_FILTERONPROMOCAPTURE__
			const auto maskToCaptureForDirectCheck = maskToCapture & (Queen_Attacks[posBlackKing] | Knight_Attacks[posBlackKing]);
			maskToCapture = (posWhiteLongDistAttacker >= 0) ? maskToCapture : maskToCaptureForDirectCheck;
			#endif
			BEGIN_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture)
			{
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!tbCanBePinned || !pinned || IsSquareAlongTheLineOrDiag(posToCapture, ppos, posWhiteKing))
				#else
				if (!tbCanBePinned || !IsWhitePinned(ppos, posToCapture))
				#endif
				{
					bool bPromoToQueenDirectCheck = false;
					const bool bPromoToKnightDirectCheck = IsKnightDiff(posToCapture, posBlackKing);
					if (bPromoToKnightDirectCheck)
					{
						if (IsCheckMateAfterPromoToKnightDirectCheck(ppos, posToCapture, posWhiteLongDistAttacker >= 0))
							return true;
					}
					else
						if ((bPromoToQueenDirectCheck = SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(posToCapture, posBlackKing, ppos)) != 0)
						{
							if (IsCheckMateAfterPromoToQueenDirectCheck(ppos, posToCapture, posWhiteLongDistAttacker >= 0))
								return true;
						}

					if (posWhiteLongDistAttacker >= 0)
					{
						if (!bPromoToQueenDirectCheck)
							if (IsCheckMateAfterPromoToQueenDiscoveredCheck(ppos, posToCapture, posWhiteLongDistAttacker))
								return true;
						if (!bPromoToKnightDirectCheck)
							if (IsCheckMateAfterPromoToKnightDiscoveredCheck(ppos, posToCapture, posWhiteLongDistAttacker))
								return true;
					}
				}
			}
			END_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture);
		}
		else
		{
			// Discovered check without promo (discovered check with a promo already covered)
			if (posWhiteLongDistAttacker >= 0)
			{
				// Discovered check with a capture (!!note that in the current implementation double check with a capture will be verified twice: here and in the direct check verification !!)
				auto maskToCapture = White_Pawn_Attacks[ppos] & black;

				BEGIN_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture)
				{
					#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
					if (!tbCanBePinned || !pinned || IsSquareAlongTheLineOrDiag(posToCapture, ppos, posWhiteKing))
					#else
					if (!tbCanBePinned || !IsWhitePinned(ppos, posToCapture))
					#endif
						if (IsCheckMateAfterPawnDiscoveredCheck(ppos, posToCapture, posWhiteLongDistAttacker))
							return true;
				}
				END_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture);

				// Discovered check with a move forward:
				if (!SameFile(posBlackKing, ppos) & bMoveForwardPossible)
					#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
					if (!tbCanBePinned || !pinned || IsSquareAlongTheLineOrDiag(ppos + 8, ppos, posWhiteKing))
					#else
					if (!tbCanBePinned || !IsWhitePinned(ppos, ppos + 8))
					#endif
					{
						if (IsCheckMateAfterPawnDiscoveredCheck(ppos, ppos + 8, posWhiteLongDistAttacker))
							return true;

						// Discovered check with a double move forward:
						if ((ppos <= _H2_) & IsEmptyAt(ppos + 16))
							if (IsCheckMateAfterPawnDiscoveredCheck(ppos, ppos + 16, posWhiteLongDistAttacker)) // no need to pass info about possible en passant, since it never prevents discovered check
								return true;
					}
			}
		}		

		// en passant:
		if constexpr (tbEnPassantPossible)
		{
			if ((bposToCaptureWithEnPassant >= _A4_) & AreSquaresAside(ppos, bposToCaptureWithEnPassant))
			{
				const bool bDirectCheckWithEnPassant = (White_Pawn_Attacks[bposToCaptureWithEnPassant + 8] & kings & black) != 0;
				if (bDirectCheckWithEnPassant)
				{
					// Direct check (and maybe double check)
					if (!IsWhitePinnedIfTakeOffBlackPawn<1>(ppos, bposToCaptureWithEnPassant + 8, bposToCaptureWithEnPassant))
					{
						const bool bDoubleCheck = (posWhiteLongDistAttacker >= 0) & ((ppos + posBlackKing) / 2 != bposToCaptureWithEnPassant + 8);
						if (IsCheckMateAfterEnPassantDirectCheck(ppos, bposToCaptureWithEnPassant + 8, bDoubleCheck))
							return true;
					}
				}
				else
				{
					// Discovered check with en passant:
					bool bWhitePawnDisco = posWhiteLongDistAttacker >= 0 && !IsSquareBetween<1>(bposToCaptureWithEnPassant + 8, posBlackKing, posWhiteLongDistAttacker);
					bool bBlackPawnDisco = false;
					int posWhiteLongDistAttackerInEnPassant;
					if (bWhitePawnDisco)
					{
						// Double-discovered check still possible (e.g. wh.p.d5, bl.p.e5, bl.Kd6, wh.Rd1 + wh.Bf4)
						bBlackPawnDisco = (SameDiagAndAllBetweenEmpty(posBlackKing, bposToCaptureWithEnPassant) && (posWhiteLongDistAttackerInEnPassant = WhiteLongDistanceFigureInDir<1>(bposToCaptureWithEnPassant, posBlackKing)) >= 0);
						if (bBlackPawnDisco)
							posWhiteLongDistAttackerInEnPassant = DBL_CHECKED;
						else
							posWhiteLongDistAttackerInEnPassant = posWhiteLongDistAttacker;
					}
					else
					{
						const bool bBlackKingOnFifthLine = (posBlackKing >> 3) == _5_;
						if (bBlackKingOnFifthLine)
						{
							if (AllBetweenEmptyIfTakeOffWhitePawn(bposToCaptureWithEnPassant, posBlackKing, ppos))
							{
								posWhiteLongDistAttackerInEnPassant = const_cast<FullBitboards*>(this)->WhiteLongDistanceFigureInDirIfTakeOffWhitePawn<1>(bposToCaptureWithEnPassant, posBlackKing, ppos);
								bBlackPawnDisco = posWhiteLongDistAttackerInEnPassant >= 0;
							}
						}
						else
							if (SameDiagAndAllBetweenEmpty(posBlackKing, bposToCaptureWithEnPassant))
							{
								posWhiteLongDistAttackerInEnPassant = WhiteLongDistanceFigureInDir<1>(bposToCaptureWithEnPassant, posBlackKing);
								bBlackPawnDisco = posWhiteLongDistAttackerInEnPassant >= 0;
							}
					}

					if (bWhitePawnDisco | bBlackPawnDisco)
						if (!IsWhitePinnedIfTakeOffBlackPawn<1>(ppos, bposToCaptureWithEnPassant + 8, bposToCaptureWithEnPassant))
							if (IsCheckMateAfterEnPassantDiscoveredCheck(ppos, bposToCaptureWithEnPassant + 8, posWhiteLongDistAttackerInEnPassant))
								return true;
				}
			}
		}

		return false;
	}

	// posChecker can be DBL_CHECKED
	template<bool tbEnPassantPossible, bool tbInclKing = false>
	bool CanWhiteCheckMateWhenChecked(const int posChecker) CONST_RESTRICT
	{
		assert(posChecker >= 0 && posChecker <= DBL_CHECKED);

		const auto whitePiecesWithDiscoveredCheck = GetWhitePiecesThatCanMakeDiscoveredCheck();

		if (white & kings & whitePiecesWithDiscoveredCheck)
			if (CanWhiteKingCheckMate<0, 0>())
				return true;

		if (posChecker != DBL_CHECKED)
		{
			constexpr bool tbFindAll = false;
			if (CanWhiteCaptureWithCheckMate<tbEnPassantPossible, tbInclKing, tbFindAll>(posChecker, whitePiecesWithDiscoveredCheck))
				return true;

			if (!AreSquaresAdjacentOrKnightDiff(posChecker, posWhiteKing))
				return CanWhiteMoveInBetweenWithCheckMate(posChecker, posWhiteKing, whitePiecesWithDiscoveredCheck);
		}

		return false;
	}

	ALWAYS_INLINE Bitboard GetAllBlackKingCheckers() CONST_RESTRICT
	{
		return IsSquareAttackedByWhite<EXCL_KING, INCL_PINNED, FIND_ALL>(posBlackKing);
	}
	ALWAYS_INLINE Bitboard GetAllWhiteKingCheckers() CONST_RESTRICT
	{
		return IsSquareAttackedByBlack<EXCL_KING, INCL_PINNED, FIND_ALL>(posWhiteKing);
	}

	// Returns 64 if more than one bit set
	ALWAYS_INLINE static int BitboardToPos(const Bitboard allCheckers)
	{
		assert(allCheckers > 0); // prerequisite
		const bool dblCheck = std::popcount(allCheckers) > 1;
		const auto posCheckingPiece = std::countr_zero(allCheckers); // let's encourage compilers to emit cmov below
		return dblCheck ? DBL_CHECKED : posCheckingPiece;
	}

	// returns DBL_CHECKED (64) on double check; -1 when not checked or checking piece pos.
	ALWAYS_INLINE int IsBlackKingChecked() CONST_RESTRICT
	{
		assert(IsValidPos(posBlackKing));
		assert(black & kings & (1ULL << posBlackKing));

		const auto res = GetAllBlackKingCheckers();
		if (res)
		{
			const bool dblCheck = std::popcount(res) > 1;
			const auto posCheckingPiece = std::countr_zero(res); // let's encourage compilers to emit cmov below
			return dblCheck ? DBL_CHECKED : posCheckingPiece;
		}
		else
			return -1;
	}

	// returns DBL_CHECKED (64) on double check; -1 when not checked or checking piece pos.
	ALWAYS_INLINE int IsWhiteKingChecked() CONST_RESTRICT
	{
		const auto res = GetAllWhiteKingCheckers();
		if (res)
		{
			const bool dblCheck = std::popcount(res) > 1;
			const auto posCheckingPiece = std::countr_zero(res); // let's encourage compilers to emit cmov below
			return dblCheck ? DBL_CHECKED : posCheckingPiece;
		}
		else
			return -1;
	}

	// Alias; returns DBL_CHECKED (64) on double check; -1 when not checked or checking piece pos.
	ALWAYS_INLINE int GeWhiteKingCheckerPos() CONST_RESTRICT
	{
		return IsWhiteKingChecked();
	}

	// tbWhiteKingUnderCheck can be:
	// 0 - not under check
	// 1 - under check; in such case param. posWhiteKingChecker can be filled in (pos or DBL_CHECKED), or left with -1 for the method to find out
	// -1 - unknown, check yourself
	template<char tbWhiteKingUnderCheck = -1, bool tbEnPassantPossible = false, bool tbCastlingShortPossible = true, bool tbCastlingLongPossible = true>
	bool FindMoveThatMates(int posWhiteKingChecker = -1, const int bposToCaptureWithEnPassant = -1) CONST_RESTRICT
	{
		assert(std::popcount(black & kings) == 1);
		assert(std::popcount(white & kings) == 1);
		assert(!IsSquareAttackedByWhite(posBlackKing));

		if (tbWhiteKingUnderCheck > 0 || (tbWhiteKingUnderCheck < 0 && IsSquareAttackedByBlack(posWhiteKing)))
		{
			if constexpr (tbWhiteKingUnderCheck < 0) //if (posWhiteKingChecker < 0)
				posWhiteKingChecker = GeWhiteKingCheckerPos();
			else			
				assert(posWhiteKingChecker == GeWhiteKingCheckerPos());			

			if (tbEnPassantPossible && posWhiteKingChecker != bposToCaptureWithEnPassant)
				return CanWhiteCheckMateWhenChecked<false>(posWhiteKingChecker); // en passant not possible after discovered check with double move by a pawn
			else
				return CanWhiteCheckMateWhenChecked<tbEnPassantPossible>(posWhiteKingChecker);
		}
		else
		{			
			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			constexpr bool tbWhiteKingKnownToBeNotUnderCheck = true;
			const auto whitePinnedPieces = GetWhitePinnedPieces<tbWhiteKingKnownToBeNotUnderCheck>();			
			#endif
			auto mask = queens() & white;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask);
			{
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (CanWhiteQueenCheckMate(pos, IsPosInBitmask(pos, whitePinnedPieces)))
				#else
				if (CanWhiteQueenCheckMate(pos))
				#endif
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			const auto whitePiecesWithDiscoveredCheck = GetWhitePiecesThatCanMakeDiscoveredCheck();
			#endif
			mask = rooks() & white;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask);
			{			
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				const auto posBitmask = 1ULL << pos;
				const bool discoveredCheckPossible = whitePiecesWithDiscoveredCheck & posBitmask;
				const bool pinned = whitePinnedPieces & posBitmask;
				if (CanWhiteRookCheckMate(pos, discoveredCheckPossible, pinned))
				#else
				if (CanWhiteRookCheckMate(pos))
				#endif
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			
			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			mask = bishops() & white & (whitePiecesWithDiscoveredCheck | Bishops_That_Can_Directly_Check[posBlackKing]);
			#else
			mask = bishops() & white & Bishops_That_Can_Check[posBlackKing];
			#endif
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask);
			{
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				const auto posBitmask = 1ULL << pos;
				const bool discoveredCheckPossible = whitePiecesWithDiscoveredCheck & posBitmask;
				const bool pinned = whitePinnedPieces & posBitmask;
				if (CanWhiteBishopCheckMate(pos, discoveredCheckPossible, pinned))
				#else
				if (CanWhiteBishopCheckMate(pos))
				#endif
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			mask = knights & white & (whitePiecesWithDiscoveredCheck | Knights_That_Can_Directly_Check[posBlackKing]);
			#else
			mask = knights & white & Knights_That_Can_Check[posBlackKing];
			#endif
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask);
			{							
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				constexpr bool tbCanBePinned = tbBlackHaveRookLikes || tbBlackHaveBishopLikes;
				if (!tbCanBePinned || !IsPosInBitmask(pos, whitePinnedPieces))
				#endif
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
					if (CanWhiteKnightCheckMate(pos, IsPosInBitmask(pos, whitePiecesWithDiscoveredCheck)))
				#else
					if (CanWhiteKnightCheckMate(pos))
				#endif
						return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			if constexpr(tbEnPassantPossible || !tbUseWhitePawnCheckOptim)
				mask = pawns & white;
			else
				mask = pawns & white & (whitePiecesWithDiscoveredCheck | White_Pawn_Direct_Check_Area[posBlackKing]);
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask);
			{
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (CanWhitePawnCheckMate<tbEnPassantPossible>(pos, bposToCaptureWithEnPassant, IsPosInBitmask(pos, whitePinnedPieces)))
				#else	
				if (CanWhitePawnCheckMate<tbEnPassantPossible>(pos, bposToCaptureWithEnPassant))
				#endif
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			if (tbCastlingShortPossible || tbCastlingLongPossible || IsPosInBitmask(posWhiteKing, whitePiecesWithDiscoveredCheck))
				return CanWhiteKingCheckMate<tbCastlingShortPossible, tbCastlingLongPossible>();
			else
				return false;
			#else
			return CanWhiteKingCheckMate<tbCastlingShortPossible, tbCastlingLongPossible>();
			#endif
		}
	}

	// Here starts the part of code strictly for FindMoveThatMatesInTwoMoves

	// This method is for finding fast refutations - it does not need to be exhaustive
	// Note that this method calls methods from family IsImmediateMateAfter* with template parameters for castling <0,0>, since only checking moves are considered and castling is out of scope anyway
	// NOTE: This method should NOT be ALWAYS_INLINE
	template<char tbBlackCastlingFlags>
	bool IsImmediateMateAfterAnyBlackCheck(const Bitboard blackDiscoveredCheckers, bool& legalMovesFound) const
	{
		assert(!IsSquareAttackedByWhite(posBlackKing)); // prerequisite

		const auto occ = this->occ();

		// Potentially still TODO: 1) discovered check by pawn 2) en passant 3) castling (formally, this method does not have to be exhaustive)

		// Queens:
		if constexpr (tbBlackHaveRookLikes || tbBlackHaveBishopLikes)
		{
			auto blackQueens = black & queens();
			BEGIN_FOR_EACH_POS_IN_MASK(pos, blackQueens)
			{
				auto maskTo = ((get_raw_bishop_moves(pos, occ) | get_raw_rook_moves(pos, occ)) & (get_raw_bishop_moves(posWhiteKing, occ) | get_raw_rook_moves(posWhiteKing, occ))) & ~black;
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskTo)
				{
					if (!IsBlackPinned(pos, posTo))
					{
						if (!IsImmediateMateAfterMoveByBlackQueen<0, 0>(pos, posTo))
							return false;
						legalMovesFound = true;
					}
				}
				END_FOR_EACH_POS_IN_MASK(posTo, maskTo);
			}
			END_FOR_EACH_POS_IN_MASK(pos, blackQueens);
		}

		// Check with promo forward?
		constexpr Bitboard firstLine = 255ULL;
		const auto blackPawns = black & pawns;
		const auto shiftedBlackDiscoveredCheckers = blackDiscoveredCheckers >> 8;
		const auto candidateSquaresForBlackPromo = Queen_Attacks[posWhiteKing] | shiftedBlackDiscoveredCheckers;
		auto blackPawnsPromoForward = (((firstLine & candidateSquaresForBlackPromo & ~occ) << 8)) & blackPawns; // promo to queen only

		BEGIN_FOR_EACH_POS_IN_MASK(pos, blackPawnsPromoForward)
		{
			if (AllBetweenEmptyIfTakeOffBlackPawn<1>(pos - 8, posWhiteKing, pos) | IsPosInBitmask(pos, blackDiscoveredCheckers))
				if (!IsBlackPinned(pos, pos - 8))
				{					
					if (!IsImmediateMateAfterPromoMoveForwardByBlackPawn<0, 0>(pos, pos - 8)) // TODO: maybe add template param. tbVerifyOnlyDirectCheck
						return false;
					legalMovesFound = true;
				}
		}
		END_FOR_EACH_POS_IN_MASK(pos, blackPawnsPromoForward);

		// Check with promo capture?
		constexpr Bitboard firstLineWithoutAColumn = 255ULL - 1;
		constexpr Bitboard firstLineWithoutHColumn = 255ULL - 128;
		auto blackPawnsThatCanPromoCaptureRight = ((firstLineWithoutAColumn & candidateSquaresForBlackPromo & white) << 7) & blackPawns;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanPromoCaptureRight)
		{
			if (AllBetweenEmptyIfTakeOffBlackPawn<1>(pos - 7, posWhiteKing, pos) | IsPosInBitmask(pos, blackDiscoveredCheckers))
				if (!IsBlackPinned(pos, pos - 7))
				{					
					if (!IsImmediateMateAfterCaptureWithPromo<0, 0>(pos, pos - 7))
						return false;
					legalMovesFound = true;
				}
		}
		END_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanPromoCaptureRight);

		auto blackPawnsThatCanPromoCaptureLeft = ((firstLineWithoutHColumn & candidateSquaresForBlackPromo & white) << 9) & blackPawns;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanPromoCaptureLeft)
		{
			if (AllBetweenEmptyIfTakeOffBlackPawn<1>(pos - 9, posWhiteKing, pos) | IsPosInBitmask(pos, blackDiscoveredCheckers))
				if (!IsBlackPinned(pos, pos - 9))
				{					
					if (!IsImmediateMateAfterCaptureWithPromo<0, 0>(pos, pos - 9))
						return false;
					legalMovesFound = true;
				}
		}
		END_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanPromoCaptureLeft);		

		// Black pawn check with a move forward:	
		auto maskForBlackPawnDirectCheck = ((White_Pawn_Attacks[posWhiteKing] | shiftedBlackDiscoveredCheckers) & ~occ) << 8;
		auto blackPawnsThatCanCheckMovingForward = maskForBlackPawnDirectCheck & blackPawns;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanCheckMovingForward)
		{
			if (!IsBlackPinned(pos, pos - 8) & !SameFile(pos, posWhiteKing)) // if black pawn is in blackDiscoveredCheckers, we need to make sure it is on a different file than white king so that discovered check will actually occur
			{
				if (!IsImmediateMateAfterMoveForwardByBlackPawn<0, 0>(pos, pos - 8))
					return false;
				legalMovesFound = true;
			}
		}
		END_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanCheckMovingForward);

		// Black pawn check with a double move forward:		
		constexpr Bitboard seventhLine = 255ULL << _A7_;
		auto blackPawnsThatCanCheckWithDoubleMoveForward = ((maskForBlackPawnDirectCheck & ~occ) << 8) & blackPawns & seventhLine;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanCheckWithDoubleMoveForward)
		{
			if (!IsBlackPinned(pos, pos - 16) & !SameFile(pos, posWhiteKing)) // if black pawn is in blackDiscoveredCheckers, we need to make sure it is on a different file than white king so that discovered check will actually occur
			{
				if (!IsImmediateMateAfterLongMoveByBlackPawn<0, 0>(pos, pos - 16))
					return false;
				legalMovesFound = true;
			}
		}
		END_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanCheckWithDoubleMoveForward);

		// Black pawn check with a capture:
		auto blackPawnsThatCanCaptureWithCheck = BlackPawnsThatCanCaptureWithCheck(blackDiscoveredCheckers); // incl. capture with discovered check
		BEGIN_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanCaptureWithCheck)
		{
			assert((pos >> 3) == (posWhiteKing >> 3) + 2 || IsPosInBitmask(pos, blackDiscoveredCheckers));
			assert((abs((pos & 7) - (posWhiteKing & 7)) <= 2 && abs((pos & 7) - (posWhiteKing & 7)) != 1) || IsPosInBitmask(pos, blackDiscoveredCheckers));
			auto maskPosTo = (BOOL_EXTEND64(IsPosInBitmask(pos, blackDiscoveredCheckers)) | White_Pawn_Attacks[posWhiteKing]) & Black_Pawn_Attacks[pos] & white;
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskPosTo)
			{
				if (!IsBlackPinned(pos, posTo))
				{
					if (!IsImmediateMateAfterCaptureByBlackPawn<0, 0>(pos, posTo))
						return false;
					legalMovesFound = true;
				}
			}
			END_FOR_EACH_POS_IN_MASK(posTo, maskPosTo);
		}
		END_FOR_EACH_POS_IN_MASK(pos, blackPawnsThatCanCaptureWithCheck);

		// Rooks:
		if constexpr (tbBlackHaveRookLikes)
		{
			auto blackRooks = black & rooks();
			BEGIN_FOR_EACH_POS_IN_MASK(pos, blackRooks)
			{
				const bool isDiscoveredChecker = blackDiscoveredCheckers & (1ULL << pos);
				auto maskTo = (get_raw_rook_moves(pos, occ) & (BOOL_EXTEND64(isDiscoveredChecker) | get_raw_rook_moves(posWhiteKing, occ))) & ~black;
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskTo)
				{
					if (!IsBlackPinned(pos, posTo))
					{
						if (!IsImmediateMateAfterMoveByBlackRook<0, 0>(pos, posTo))
							return false;
						legalMovesFound = true;
					}
				}
				END_FOR_EACH_POS_IN_MASK(posTo, maskTo);
			}
			END_FOR_EACH_POS_IN_MASK(pos, blackRooks);
		}

		// Bishops:
		if constexpr (tbBlackHaveBishopLikes)
		{
			auto blackBishops = black & bishops() & Bishops_That_Can_Check[posWhiteKing]; // TODO: here direct check is only searched for, but squares suitable for discovered check are included in Bishops_That_Can_Check
			BEGIN_FOR_EACH_POS_IN_MASK(pos, blackBishops)
			{
				const bool isDiscoveredChecker = blackDiscoveredCheckers & (1ULL << pos);
				auto maskTo = (get_raw_bishop_moves(pos, occ) & (BOOL_EXTEND64(isDiscoveredChecker) | get_raw_bishop_moves(posWhiteKing, occ))) & ~black;
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskTo)
				{
					if (!IsBlackPinned(pos, posTo))
					{
						if (!IsImmediateMateAfterMoveByBlackBishop<0, 0>(pos, posTo))
							return false;
						legalMovesFound = true;
					}
				}
				END_FOR_EACH_POS_IN_MASK(posTo, maskTo);
			}
			END_FOR_EACH_POS_IN_MASK(pos, blackBishops);
		}

		// Knights:		
		auto blackKnights = black & knights & Knights_That_Can_Check[posWhiteKing];
		BEGIN_FOR_EACH_POS_IN_MASK(pos, blackKnights)
		{
			if (!IsBlackAbsolutelyPinned(pos))
			{				
				const bool isDiscoveredChecker = blackDiscoveredCheckers & (1ULL << pos);
				auto maskTo = Knight_Attacks[pos] & (BOOL_EXTEND64(isDiscoveredChecker) | Knight_Attacks[posWhiteKing]) & ~black;
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskTo)
				{					
					if (!IsImmediateMateAfterMoveByBlackKnight<0, 0>(pos, posTo))
						return false;
					legalMovesFound = true;
				}
				END_FOR_EACH_POS_IN_MASK(posTo, maskTo);
			}
		}
		END_FOR_EACH_POS_IN_MASK(pos, blackKnights);

		// Discovered check with black king:
		if (blackDiscoveredCheckers & black & kings)
		{
			auto mask = King_Attacks[posBlackKing] & ~black & ~GetCommonDiagOrLine(posWhiteKing, posBlackKing) & ~WhitePawnAttacks() & ~WhiteKnightAttacks() & ~King_Attacks[posWhiteKing];
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				if (!IsSquareAttackedByWhite<-1>(pos)) // -1==long dist. figures only (squares attacked by white king, pawns or knights already filtered out)
				{
					if (!IsImmediateMateAfterMoveByBlackKing<0, 0>(pos))
						return false;
					legalMovesFound = true;
				}
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		return true;
	}

	template<bool tbEnPassantPossible = false, char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	bool IsImmediateMateAfterAnyBlackResponse(const int bpposForEnPassant = -1) CONST_RESTRICT
	{
		constexpr bool tbWhiteCastlingShortPossible = tbWhiteCastlingFlags & 1;
		constexpr bool tbWhiteCastlingLongPossible = (tbWhiteCastlingFlags & 2) != 0;
		
		const auto allBlackKingCheckers = GetAllBlackKingCheckers();
		
		if (allBlackKingCheckers > 0)
		{						
			const auto whitePawnAttacks = WhitePawnAttacks();
			const auto whiteKnightAttacks = WhiteKnightAttacks();
			const auto posBlackKingChecker = BitboardToPos(allBlackKingCheckers);

			auto mask = King_Attacks[posBlackKing] & ~black & ~King_Attacks[posWhiteKing] & ~whitePawnAttacks & ~whiteKnightAttacks;

			#ifdef __USE_OPTIM_FOR_NON_CAPTURE_BY_KING__
			// First let's analyze capture moves (they have higher probability of being a refutation and additionally it allows to call IsImmediateMateAfterMoveByBlackKing with param. tbKnownThatItIsNotACapture == true in the next loop
			auto maskCaptureMoves = mask & white; 
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskCaptureMoves)
			{
				if (!IsSquareAttackedByWhiteIfTakeOffBlackKing<-1>(posTo))
					if (!IsImmediateMateAfterMoveByBlackKing<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(posTo))
						return false;
			}
			END_FOR_EACH_POS_IN_MASK(posTo, maskCaptureMoves);

			// Now non-capture moves by black king:
			auto maskForNonCapture = mask & ~white;
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskForNonCapture)
			{
				if (!IsSquareAttackedByWhiteIfTakeOffBlackKing<-1>(posTo))
					if (!IsImmediateMateAfterMoveByBlackKing<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible, true>(posTo))
						return false;
			}
			END_FOR_EACH_POS_IN_MASK(posTo, maskForNonCapture);		

			#else
			
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, mask)
			{
				if (!IsSquareAttackedByWhiteIfTakeOffBlackKing<-1>(posTo))
					if (!IsImmediateMateAfterMoveByBlackKing<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(posTo))
						return false;
			}
			END_FOR_EACH_POS_IN_MASK(posTo, mask);

			#endif

			if (posBlackKingChecker != DBL_CHECKED)
			{
				constexpr bool tbOnlyIfPreventsImmediateMateAndFlags = 1;
				constexpr bool tbOneIsEnough = 1;

				Bitboard captureMask;
				constexpr auto tbFlags = tbOnlyIfPreventsImmediateMateAndFlags + 2 * tbWhiteCastlingFlags;
				if (!tbEnPassantPossible || posBlackKingChecker == bpposForEnPassant)
					captureMask = CanBlackCapture<tbEnPassantPossible, 0, tbOneIsEnough, tbFlags>(posBlackKingChecker);
				else
					captureMask = CanBlackCapture<0, 0, tbOneIsEnough, tbFlags>(posBlackKingChecker);

				if (captureMask)
					return false; // tbOnlyIfPreventsImmediateMateAndFlags already verified, so we can return

				if (!AreSquaresAdjacentOrKnightDiff(posBlackKing, posBlackKingChecker))
					if (CanBlackMoveInBetween<0, tbOnlyIfPreventsImmediateMateAndFlags + 2 * tbWhiteCastlingFlags>(posBlackKing, posBlackKingChecker))
						return false; // tbOnlyIfPreventsImmediateMateAndFlags already verified, so we can return
			}

			return true; // black king under check and no defence found - no need to exclude stalemate by verifying, if there were any legal moves
		}
		else
		{
			bool legalMovesFound = false;

			#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
			const auto blackPinnedPieces = GetBlackPinnedPieces();
			#endif

			#ifdef __USE_BLACKCHECKINGMOVESFIRST__
			const auto blackDiscoveredCheckers = GetBlackPiecesThatCanMakeDiscoveredCheck();
			// A method to find fast refutations - after a check White don't have many responses and the analysis is likely to be completed very fast
			if (!IsImmediateMateAfterAnyBlackCheck<tbBlackCastlingFlags>(blackDiscoveredCheckers, legalMovesFound))
				return false;
			#endif
			
			const auto occ = this->occ();
			
			// Queens:
			Bitboard mask;
			if constexpr(tbBlackHaveRookLikes || tbBlackHaveBishopLikes)
			{
				mask = queens() & black;
				BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
				{
					auto movesMask = get_bishop_moves(pos, occ, black) | get_rook_moves(pos, occ, black);
				
					#ifdef __USE_OPTIM_FOR_NON_CAPTURE__
					auto figureCaptureMovesMask = movesMask & white & (~pawns);
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, figureCaptureMovesMask)
					{					
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackQueen<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, figureCaptureMovesMask);
					auto pawnCaptureMovesMask = movesMask & white & pawns;
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, pawnCaptureMovesMask)
					{					
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackQueen<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, pawnCaptureMovesMask);			
					auto nonCaptureMovesMask = movesMask & ~white;
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, nonCaptureMovesMask)
					{					
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackQueen<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible, true>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, nonCaptureMovesMask);
			
					#else				
			
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
					{					
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackQueen<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
					#endif
				}
				END_FOR_EACH_POS_IN_MASK(pos, mask);
			}

			if constexpr (tbBlackHaveRookLikes)
			{
				mask = rooks() & black & ~blackDiscoveredCheckers; // rook discovered checkers already analyzed
				BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
				{
					auto movesMask = get_rook_moves(pos, occ, black);

					#ifdef __USE_OPTIM_FOR_NON_CAPTURE__ 
					auto captureMovesMask = movesMask & white;
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, captureMovesMask)
					{					
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackRook<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, captureMovesMask);
					auto nonCaptureMovesMask = movesMask & ~white;
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, nonCaptureMovesMask)
					{					
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackRook<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible, true>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, nonCaptureMovesMask);

					#else
				
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
					{					
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackRook<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
					#endif
				}
				END_FOR_EACH_POS_IN_MASK(pos, mask);
			}

			if constexpr (tbBlackHaveBishopLikes)
			{
				mask = bishops() & black & ~blackDiscoveredCheckers; // bishops discovered checkers already analyzed
				BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
				{
					auto movesMask = get_bishop_moves(pos, occ, black);

					#ifdef __USE_OPTIM_FOR_NON_CAPTURE__ 
					auto captureMovesMask = movesMask & white;
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, captureMovesMask)
					{
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackBishop<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, captureMovesMask);
					auto nonCaptureMovesMask = movesMask & ~white;
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, nonCaptureMovesMask)
					{
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackBishop<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible, true>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, nonCaptureMovesMask);

					#else
				
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
					{
						#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
						if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posBlackKing))
						#else
						if (!IsBlackPinned(pos, posTo))
						#endif
						{
							if (!IsImmediateMateAfterMoveByBlackBishop<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
								return false;
							legalMovesFound = true;
						}
					}
					END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
					#endif
				}
				END_FOR_EACH_POS_IN_MASK(pos, mask);
			}

			#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
			mask = knights & black & ~blackPinnedPieces;
			#else
			mask = knights & black & ~blackDiscoveredCheckers; // knights discovered checkers already analyzed
			#endif
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				#if !defined(__PREEMPTIVE_BLACKPINNEDPIECES__)
				if (!IsBlackAbsolutelyPinned(pos))
				#endif
				{
					auto movesMask = Knight_Attacks[pos] & ~black;
					
					#ifdef __USE_OPTIM_FOR_NON_CAPTURE__ 
					auto captureMovesMask = movesMask & white;
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, captureMovesMask)
					{
						if (!IsImmediateMateAfterMoveByBlackKnight<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
							return false;
						legalMovesFound = true;
					}
					END_FOR_EACH_POS_IN_MASK(posTo, captureMovesMask);
					auto nonCaptureMovesMask = movesMask & ~white;
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, nonCaptureMovesMask)
					{
						if (!IsImmediateMateAfterMoveByBlackKnight<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible, true>(pos, posTo))
							return false;
						legalMovesFound = true;
					}
					END_FOR_EACH_POS_IN_MASK(posTo, nonCaptureMovesMask);
				
					#else
					
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
					{
						if (!IsImmediateMateAfterMoveByBlackKnight<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
							return false;
						legalMovesFound = true;
					}
					END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
					#endif
				}
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
			
			{ 
				// let's try to minimize the scope of the variables below
				assert(kings & black);
				const auto whitePawnAttacks = WhitePawnAttacks();
				const auto whiteKnightAttacks = WhiteKnightAttacks();

				#ifdef __USE_PEDANTICFILTERINGOUTMOVESALREADYANALYZED__ // tests show it not worth doing (very unlikely to have a discovered check with bl.king that was not a refutation, so no need to lose any time on this filtering)
				const bool bCanMakeDiscoveredCheck = (blackDiscoveredCheckers & black & kings) != 0;
				const Bitboard alongTheDiagOrLine = BOOL_EXTEND64(!bCanMakeDiscoveredCheck) | GetCommonDiagOrLine(posWhiteKing, posBlackKing); // if discovered check by Bl.King is not possible, this mask will be all ones

				mask = King_Attacks[posBlackKing] & ~black & ~King_Attacks[posWhiteKing] & ~whitePawnAttacks & ~whiteKnightAttacks & alongTheDiagOrLine;
				#else
				mask = King_Attacks[posBlackKing] & ~black & ~King_Attacks[posWhiteKing] & ~whitePawnAttacks & ~whiteKnightAttacks;
				#endif

				BEGIN_FOR_EACH_POS_IN_MASK(posTo, mask)
				{
					if (!IsSquareAttackedByWhite<-1>(posTo)) // -1==long dist. figures only (squares attacked by white king, pawns or knights already filtered out)
					{
						if (!IsImmediateMateAfterMoveByBlackKing<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(posTo))
							return false;
						legalMovesFound = true;
					}
				}
				END_FOR_EACH_POS_IN_MASK(pos, mask);
			}


			if constexpr (tbBlackCastlingFlags != 0 && tbBlackHaveRookLikes)
			{
				if (posBlackKing == _E8_) // this is probably redundant (assert might be enough) but a very predictable branch anyway
				{
					if constexpr ((tbBlackCastlingFlags & 1) != 0)
					{
						if (IsBlackRookAt(_H8_) && IsEmptyAt(_F8_) && IsEmptyAt(_G8_))
							if (!IsSquareAttackedByWhite(_F8_) && !IsSquareAttackedByWhite(_G8_))
							{
								legalMovesFound = true; // well, castling is never the only valid move...
								if (!IsImmediateMateAfterBlackCastlingShort<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>())
									return false;
							}
					}
					if constexpr ((tbBlackCastlingFlags & 2) != 0)
					{
						if (IsBlackRookAt(_A8_) && IsEmptyAt(_B8_) && IsEmptyAt(_C8_) && IsEmptyAt(_D8_))
							if (!IsSquareAttackedByWhite(_C8_) && !IsSquareAttackedByWhite(_D8_))
							{
								legalMovesFound = true; // well, castling is never the only valid move...
								if (!IsImmediateMateAfterBlackCastlingLong<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>())
									return false;
							}
					}
				}
			}

			// pawn:
			mask = black & pawns;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{								
				auto captureMask = Black_Pawn_Attacks[pos] & white;
				BEGIN_FOR_EACH_POS_IN_MASK(capturePos, captureMask)
				{
					#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
					if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(capturePos, pos, posBlackKing))
					#else
					if (!IsBlackPinned(pos, capturePos))
					#endif
					{
						if (!IsImmediateMateAfterCaptureByBlackPawn<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, capturePos))
							return false;
						legalMovesFound = true;
					}
				}
				END_FOR_EACH_POS_IN_MASK(capturePos, captureMask);				

				if (IsEmptyAt(pos - 8))
				{
					#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
					if (!IsPosInBitmask(pos, blackPinnedPieces) || IsSquareAlongTheLineOrDiag(pos - 8, pos, posBlackKing))
					#else
					if (!IsBlackPinned(pos, pos - 8))
					#endif
					{
						legalMovesFound = true;
						if (!IsImmediateMateAfterMoveForwardByBlackPawn< tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible, 1>(pos, pos - 8))
							return false;
						if (pos >= _A7_ && IsEmptyAt(pos - 16))
							if (!IsImmediateMateAfterLongMoveByBlackPawn< tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, pos - 16))
								return false;
					}
				}

				if constexpr (tbEnPassantPossible)
					if (AreSquaresAside(pos, bpposForEnPassant))
						if (!IsBlackPinnedIfTakeOffWhitePawn(pos, bpposForEnPassant - 8, bpposForEnPassant))
						{
							legalMovesFound = true;
							if (!IsImmediateMateAfterBlackEnPassant< tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, bpposForEnPassant - 8))
								return false;
						}

			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			return legalMovesFound; // otherwise stalemate
		}
	}

	template<char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteQueenMove(const int posFrom, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteQueenAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= fromMask;
		const_cast<FullBitboards*>(this)->qbishops ^= fromMask;
		const_cast<FullBitboards*>(this)->qrooks |= toMask;
		const_cast<FullBitboards*>(this)->qbishops |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_QUEEN>(captureMask);

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteRookMove(const int posFrom, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteRookAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= fromMask;
		const_cast<FullBitboards*>(this)->qrooks |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK>(captureMask);

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteBishopMove(const int posFrom, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteBishopAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->qbishops ^= fromMask;
		const_cast<FullBitboards*>(this)->qbishops |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP>(captureMask);

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteKnightMove(const int posFrom, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteKnightAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->knights ^= fromMask;
		const_cast<FullBitboards*>(this)->knights |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KNIGHT>(captureMask);

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}


	template<char tbBlackCastlingFlags = 3>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove(const int posTo) CONST_RESTRICT
	{
		const auto posFrom = posWhiteKing;
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteKingAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->posWhiteKing = posTo;
		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KING>(captureMask);

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, 0, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbBlackCastlingFlags = 3>
	bool IsImmediateMateAfterAnyBlackResponseAfterWhiteCastlingShort() CONST_RESTRICT
	{
		constexpr auto fromMask = 1ULL << _E1_;
		constexpr auto toMask = 1ULL << _G1_;
		constexpr auto moveMask = fromMask | toMask;

		constexpr auto fromMaskRook = 1ULL << _H1_;
		constexpr auto toMaskRook = 1ULL << _F1_;
		constexpr auto moveMaskRook = fromMaskRook | toMaskRook;

		constexpr auto moveCastling = moveMask | moveMaskRook;

		const_cast<FullBitboards*>(this)->posWhiteKing = _G1_;
		const_cast<FullBitboards*>(this)->white ^= moveCastling;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= moveMaskRook;

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, 0, tbBlackCastlingFlags>();

		// Restore:
		const_cast<FullBitboards*>(this)->posWhiteKing = _E1_;
		const_cast<FullBitboards*>(this)->white ^= moveCastling;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= moveMaskRook;

		return res;
	}

	template<char tbBlackCastlingFlags = 3>
	bool IsImmediateMateAfterAnyBlackResponseAfterWhiteCastlingLong() CONST_RESTRICT
	{
		constexpr auto fromMask = 1ULL << _E1_;
		constexpr auto toMask = 1ULL << _C1_;
		constexpr auto moveMask = fromMask | toMask;

		constexpr auto fromMaskRook = 1ULL << _A1_;
		constexpr auto toMaskRook = 1ULL << _D1_;
		constexpr auto moveMaskRook = fromMaskRook | toMaskRook;

		constexpr auto moveCastling = moveMask | moveMaskRook;

		const_cast<FullBitboards*>(this)->posWhiteKing = _C1_;
		const_cast<FullBitboards*>(this)->white ^= moveCastling;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= moveMaskRook;

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, 0, tbBlackCastlingFlags>();

		// Restore:
		const_cast<FullBitboards*>(this)->posWhiteKing = _E1_;
		const_cast<FullBitboards*>(this)->white ^= moveCastling;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->qrooks ^= moveMaskRook;

		return res;
	}

	ALWAYS_INLINE bool IsWhitePromoMove(const int posFrom) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));

		return IsWhitePawnAt(posFrom) & (posFrom >= _A7_);
	}

	template<char tbWhiteCastlingFlags, char tbBlackCastlingFlags>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove(const int posFrom, const int posTo, const int promo = FGR_EMPTY) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhitePawnAt(posFrom));
		assert(posFrom >= _A7_);
		assert(posTo >= _A8_);

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;

		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_PAWN>(captureMask);

		bool res;

		if (promo == FGR_EMPTY) // verify all 4 promos?
		{
			const_cast<FullBitboards*>(this)->qrooks |= toMask;
			const_cast<FullBitboards*>(this)->qbishops |= toMask;

			res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();
			if (!res)
			{
				const_cast<FullBitboards*>(this)->qrooks ^= toMask;
				const_cast<FullBitboards*>(this)->qbishops ^= toMask;
				const_cast<FullBitboards*>(this)->knights |= toMask;

				res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();
				if (!res)
				{
					const_cast<FullBitboards*>(this)->knights ^= toMask;
					const_cast<FullBitboards*>(this)->qrooks |= toMask;
					res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

					if (!res)
					{
						const_cast<FullBitboards*>(this)->qrooks ^= toMask;
						const_cast<FullBitboards*>(this)->qbishops |= toMask;
						res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();
					}
				}
			}
		}
		else
		{
			switch (promo)
			{
				case FGR_BISHOP:
					const_cast<FullBitboards*>(this)->qbishops |= toMask;
					break;
				case FGR_ROOK:
					const_cast<FullBitboards*>(this)->qrooks |= toMask;
					break;
				case FGR_QUEEN:
					const_cast<FullBitboards*>(this)->qbishops |= toMask;
					const_cast<FullBitboards*>(this)->qrooks |= toMask;
					break;
				case FGR_KNIGHT:
					const_cast<FullBitboards*>(this)->knights |= toMask;
					break;
			}

			res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();
		}

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbWhiteCastlingFlags, char tbBlackCastlingFlags>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteEnPassant(const int posFrom, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert((posTo & 7) != (posFrom & 7));
		assert(IsEmptyAt(posTo));
		assert(IsWhitePawnAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const auto captureMask = sq_to_bb((posTo & 7) + (posFrom >> 3) * 8);

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= captureMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;

		const bool res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= captureMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;

		return res;
	}

	template<char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhitePawnMove(const int posFrom, const int posTo, const int promo = FGR_EMPTY) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhitePawnAt(posFrom));

		if (posFrom >= _A7_)
			return IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo, promo);
		const bool bEnPassant = !SameFile(posFrom, posTo) & IsEmptyAt(posTo);
		if (bEnPassant)
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteEnPassant<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo);

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const auto captureMask = black & toMask;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->pawns |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_PAWN>(captureMask);

		bool res;
		if (posTo - posFrom == 16)
			res = IsImmediateMateAfterAnyBlackResponse<1, tbWhiteCastlingFlags, tbBlackCastlingFlags>(posTo);
		else
			res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	bool IsImmediateMateAfterAnyBlackResponseAfterWhiteMove(const TMove& move) CONST_RESTRICT
	{
		assert(white & (1ULL << move.nFrom));
		assert((~white) & (1ULL << move.nTo));

		const FIGURE fMoving = GetFigureAt(move.nFrom);
		switch (fMoving)
		{
		case FGR_KING:
			assert(Distance(move.nFrom, move.nTo) == 1); // separate method for castling
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove<tbBlackCastlingFlags>(move.nTo);
		case FGR_PAWN:
			return IsImmediateMateAfterAnyBlackResponseAfterWhitePawnMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(move.nFrom, move.nTo, move.IsPromotion());
		case FGR_BISHOP:
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteBishopMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(move.nFrom, move.nTo);
		case FGR_ROOK:
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteRookMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(move.nFrom, move.nTo);
		case FGR_QUEEN:
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteQueenMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(move.nFrom, move.nTo);
		case FGR_KNIGHT:
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteKnightMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(move.nFrom, move.nTo);
		}

		assert(false);
		return false;
	}

	template<char tbWhiteCastlingFlags, char tbBlackCastlingFlags>
	bool IsImmediateMateAfterAnyBlackResponseAfterWhiteMove(const int posFrom, const int posTo) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(white & (sq_to_bb(posFrom)));
		assert((~white) & (sq_to_bb(posTo)));

		const FIGURE fMoving = GetFigureAt(posFrom);
		switch (fMoving)
		{
		case FGR_KING:
			assert(Distance(posFrom, posTo) == 1); // separate method for castling
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove<tbBlackCastlingFlags>(posTo);
		case FGR_PAWN:
			return IsImmediateMateAfterAnyBlackResponseAfterWhitePawnMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo);
		case FGR_BISHOP:
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteBishopMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo);
		case FGR_ROOK:
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteRookMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo);
		case FGR_QUEEN:
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteQueenMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo);
		case FGR_KNIGHT:
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteKnightMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo);
		}

		assert(false);
		return false;
	}

	ALWAYS_INLINE bool IsWhiteCaptureEnPassant(const int posFrom, const int posCaptured) CONST_RESTRICT
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posCaptured));
		assert(posCaptured != posFrom);
		assert(IsWhiteAt(posFrom));
		assert(IsBlackAt(posCaptured));

		const auto res = AreSquaresAside(posFrom, posCaptured) & IsWhitePawnAt(posFrom);
		return res;
	}

	template<bool tbInclKnightsKingAndPawns = false>
	ALWAYS_INLINE Bitboard GetBlackRooksThatCanMakeDiscoveredCheck() CONST_RESTRICT
	{
		static_assert(tbInclKnightsKingAndPawns, ""); // the implemention slightly below (commented out) allowed for !tbInclKnightsKingAndPawns

		if constexpr (!tbBlackHaveBishopLikes)
			return 0ULL;
		else
		{
			Bitboard res = 0;
			const auto blackQBishops = black & qbishops;
			auto blackBishopLikes = get_raw_bishop_moves(posWhiteKing, white | blackQBishops) & blackQBishops;

			BEGIN_FOR_EACH_POS_IN_MASK(pos, blackBishopLikes)
			{
				const auto maskBetween = GetBetweenMask(posWhiteKing, pos);
				const auto blackPiecesBetween = maskBetween & black;

				assert(blackPiecesBetween != 0); // otherwise White King would be under check before Black move
				const bool exactlyOneBlackPieceBetween = HasSingleBit<1>(blackPiecesBetween); // exactly one black piece between?
				const auto maskToApply = BOOL_EXTEND64(exactlyOneBlackPieceBetween) & blackPiecesBetween;

				res |= maskToApply;
			}
			END_FOR_EACH_POS_IN_MASK(pos, blackBishopLikes);
			return res;
		}
	}

	template<bool tbInclKnightsKingAndPawns = false>
	ALWAYS_INLINE Bitboard GetBlackBishopsThatCanMakeDiscoveredCheck() CONST_RESTRICT
	{
		static_assert(tbInclKnightsKingAndPawns, ""); // the implemention slightly below (commented out) allowed for !tbInclKnightsKingAndPawns

		if constexpr (!tbBlackHaveRookLikes)
			return 0ULL;
		else
		{
			Bitboard res = 0;
			const auto blackQRooks = black & qrooks;
			auto blackRookLikes = get_raw_rook_moves(posWhiteKing, white | blackQRooks) & blackQRooks;

			BEGIN_FOR_EACH_POS_IN_MASK(pos, blackRookLikes)
			{
				const auto maskBetween = GetBetweenMask(posWhiteKing, pos);
				const auto blackPiecesBetween = maskBetween & black;

				assert(blackPiecesBetween != 0); // otherwise White King would be under check before Black move
				const bool exactlyOneBlackPieceBetween = HasSingleBit<1>(blackPiecesBetween); // exactly one black piece between?
				const auto maskToApply = BOOL_EXTEND64(exactlyOneBlackPieceBetween) & blackPiecesBetween;

				res |= maskToApply;
			}
			END_FOR_EACH_POS_IN_MASK(pos, blackRookLikes);
			return res;
		}
	}

	ALWAYS_INLINE Bitboard GetBlackPiecesThatCanMakeDiscoveredCheck() CONST_RESTRICT
	{
		if constexpr (!tbBlackHaveRookLikes && !tbBlackHaveBishopLikes)
			return 0ULL;
		else
			return GetBlackBishopsThatCanMakeDiscoveredCheck<1>() | GetBlackRooksThatCanMakeDiscoveredCheck<1>();
	}

	template<bool tbInclKnightsKingAndPawns = false>
	ALWAYS_INLINE Bitboard GetWhiteRooksThatCanMakeDiscoveredCheck() CONST_RESTRICT
	{
		static_assert(tbInclKnightsKingAndPawns, ""); // the implemention slightly below (commented out) allowed for !tbInclKnightsKingAndPawns

		#ifdef __USE_MOVEGENTWICEWHENLOOKINGFORPINNEDANDDISCOVEREDCHECKERS__ // tests show this slower		
		const auto occ = this->occ();
		const auto whiteRookCandidates = get_raw_bishop_moves(posBlackKing, occ) & white & (qrooks | (tbInclKnightsKingAndPawns ? kings | knights | pawns : 0ULL)); // or rooks() but qrooks is OK
		const auto whiteBishopLikes = get_raw_bishop_moves(posBlackKing, occ & ~whiteRookCandidates) & white & qbishops;

		Bitboard res = 0;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, whiteBishopLikes)
		{
			res |= GetBetweenMask(pos, posBlackKing) & whiteRookCandidates;
		}
		END_FOR_EACH_POS_IN_MASK(pos, whiteBishopLikes);
		return res;		
		#else

		Bitboard res = 0;
		const auto whiteQBishops = white & qbishops;
		auto whiteBishopLikes = get_raw_bishop_moves(posBlackKing, black | whiteQBishops) & whiteQBishops;

		BEGIN_FOR_EACH_POS_IN_MASK(pos, whiteBishopLikes)
		{
			const auto maskBetween = GetBetweenMask(posBlackKing, pos);
			const auto whitePiecesBetween = maskBetween & white;

			assert(whitePiecesBetween != 0); // otherwise Black King would be under check before White move
			const bool exactlyOneWhitePieceBetween = HasSingleBit<1>(whitePiecesBetween); // exactly one white piece between?
			const auto maskToApply = BOOL_EXTEND64(exactlyOneWhitePieceBetween) & whitePiecesBetween;

			res |= maskToApply;
		}
		END_FOR_EACH_POS_IN_MASK(pos, whiteBishopLikes);
		return res;
		#endif
	}

	template<bool tbInclKnightsKingAndPawns = false>
	ALWAYS_INLINE Bitboard GetWhiteBishopsThatCanMakeDiscoveredCheck() CONST_RESTRICT
	{
		static_assert(tbInclKnightsKingAndPawns, ""); //the implemention slightly below(commented out) allowed for !tbInclKnightsKingAndPawns

		#ifdef __USE_MOVEGENTWICEWHENLOOKINGFORPINNEDANDDISCOVEREDCHECKERS__ // tests show this slower	
		const auto occ = this->occ();
		const auto whiteBishopCandidates = get_raw_rook_moves(posBlackKing, occ) & white & (qbishops | (tbInclKnightsKingAndPawns ? kings | knights | pawns : 0ULL)); // or bishops() but qbishops is OK
		const auto whiteRookLikes = get_raw_rook_moves(posBlackKing, occ & ~whiteBishopCandidates) & white & qrooks;

		Bitboard res = 0;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, whiteRookLikes)
		{
			res |= GetBetweenMask(pos, posBlackKing) & whiteBishopCandidates;
		}
		END_FOR_EACH_POS_IN_MASK(pos, whiteRookLikes);
		return res;
		#else

		Bitboard res = 0;
		const auto whiteQRooks = white & qrooks;
		auto whiteRookLikes = get_raw_rook_moves(posBlackKing, black | whiteQRooks) & whiteQRooks;

		BEGIN_FOR_EACH_POS_IN_MASK(pos, whiteRookLikes)
		{
			const auto maskBetween = GetBetweenMask(posBlackKing, pos);
			const auto whitePiecesBetween = maskBetween & white;

			assert(whitePiecesBetween != 0); // otherwise Black King would be under check before White move
			const bool exactlyOneWhitePieceBetween = HasSingleBit<1>(whitePiecesBetween); // exactly one white piece between?
			const auto maskToApply = BOOL_EXTEND64(exactlyOneWhitePieceBetween) & whitePiecesBetween;

			res |= maskToApply;
		}
		END_FOR_EACH_POS_IN_MASK(pos, whiteRookLikes);
		return res;
		#endif				
	}

	ALWAYS_INLINE Bitboard GetWhitePiecesThatCanMakeDiscoveredCheck() CONST_RESTRICT
	{
		return GetWhiteBishopsThatCanMakeDiscoveredCheck<1>() | GetWhiteRooksThatCanMakeDiscoveredCheck<1>();
	}

	template<bool tbWhiteKingKnownToBeNotUnderCheck = false>
	Bitboard GetWhitePinnedPieces() CONST_RESTRICT
	{
		if constexpr (!tbBlackHaveRookLikes && !tbBlackHaveBishopLikes)
			return 0ULL;
		else
		{
			Bitboard pinned = 0ULL;

			#ifdef __USE_MOVEGENTWICEWHENLOOKINGFORPINNEDANDDISCOVEREDCHECKERS__ // tests show this slower	
			const auto occ = this->occ();
			const auto whiteCandidatesPinnedOnDiag = get_raw_bishop_moves(posWhiteKing, occ) & white;
			const auto whiteCandidatesPinnedOnLine = get_raw_rook_moves(posWhiteKing, occ) & white;		
			const auto blackPinnersOnDiag = get_raw_bishop_moves(posWhiteKing, occ & ~whiteCandidatesPinnedOnDiag) & qbishops;
			const auto blackPinnersOnLine = get_raw_rook_moves(posWhiteKing, occ & ~whiteCandidatesPinnedOnLine) & qrooks;		
			const auto whiteCandidates = whiteCandidatesPinnedOnDiag | whiteCandidatesPinnedOnLine;
			auto blackPinners = (blackPinnersOnDiag | blackPinnersOnLine) & black;

			BEGIN_FOR_EACH_POS_IN_MASK(posPinner, blackPinners)
			{
				pinned |= GetBetweenMask(posPinner, posWhiteKing) & whiteCandidates;
			}
			END_FOR_EACH_POS_IN_MASK(posPinner, blackPinners);
			return pinned;
			#else		

			const auto potential_pinners_rook = (tbBlackHaveRookLikes ? get_rook_moves(posWhiteKing, black, 0) : 0ULL) & black & qrooks; // here own pieces are 0 - as if we can "move through" them
			const auto potential_pinners_bishop = (tbBlackHaveBishopLikes ? get_bishop_moves(posWhiteKing, black, 0) : 0ULL) & black & qbishops;
			auto potential_pinners = potential_pinners_rook | potential_pinners_bishop;

			BEGIN_FOR_EACH_POS_IN_MASK(posPinner, potential_pinners)
			{
				const auto maskBetween = GetBetweenMask(posWhiteKing, posPinner);
				const auto whitePiecesBetween = maskBetween & white;

				assert(!tbWhiteKingKnownToBeNotUnderCheck || whitePiecesBetween != 0);
				const bool exactlyOneWhitePieceBetween = HasSingleBit<tbWhiteKingKnownToBeNotUnderCheck>(whitePiecesBetween); // exactly one white piece between?
				const auto maskToApply = BOOL_EXTEND64(exactlyOneWhitePieceBetween) & whitePiecesBetween;

				pinned |= maskToApply;
			}
			END_FOR_EACH_POS_IN_MASK(posPinner, potential_pinners);

			return pinned;	
			#endif
		}
	}

	template<bool tbBlackKingKnownToBeNotUnderCheck = false>
	Bitboard GetBlackPinnedPieces() CONST_RESTRICT
	{
		Bitboard pinned = 0ULL;

		const auto potential_pinners_rook = get_rook_moves(posBlackKing, white, 0) & white & qrooks; // here own pieces are 0 - as if we can "move through" them
		const auto potential_pinners_bishop = get_bishop_moves(posBlackKing, white, 0) & white & qbishops;
		auto potential_pinners = potential_pinners_rook | potential_pinners_bishop;

		BEGIN_FOR_EACH_POS_IN_MASK(posPinner, potential_pinners)
		{
			const auto maskBetween = GetBetweenMask(posBlackKing, posPinner);
			const auto blackPiecesBetween = maskBetween & black;

			assert(!tbBlackKingKnownToBeNotUnderCheck || blackPiecesBetween != 0);
			const bool exactlyOneBlackPieceBetween = HasSingleBit<tbBlackKingKnownToBeNotUnderCheck>(blackPiecesBetween);
			const auto maskToApply = BOOL_EXTEND64(exactlyOneBlackPieceBetween) & blackPiecesBetween;

			pinned |= maskToApply;
		}
		END_FOR_EACH_POS_IN_MASK(posPinner, potential_pinners);

		return pinned;
	}


	// See FindMoveThatMates above for description of params tbWhiteKingUnderCheck and tbEnPassantPossible
	// Flags tbWhiteCastlingFlags and tbBlackCastlingFlags have:
	// * set bit 1 if castling short is possible (wh.king is on e1 and did not move yet and wh.R is on h1 and did not move yet)
	// * set bit 2 if castling long is possible (wh.king is on e1 and did not move yet and wh.R is on a1 and did not move yet)
	template<char tbWhiteKingUnderCheck = -1, bool tbEnPassantPossible = false, char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3, bool tbFindAllSolutionsAndFillBuf = false, bool tbDispatchClass = true>
	int FindMoveThatMatesInTwoMoves(int posWhiteKingChecker = -1, const int bposToCaptureWithEnPassant = -1, TMove* pMoves = nullptr) CONST_RESTRICT
	{
		#ifdef __USE_OPTIMFORMISSINGBLACKLONGDISTANCEFIGURES__
		if constexpr (tbDispatchClass)
		{
			const bool bBlackRookLikes = (qrooks & black) != 0;
			const bool bBlackBishopLikes = (qbishops & black) != 0;
			#ifdef __USE_OPTIMFORMISSINGBLACKKNIGHTS__
			const bool bBlackKnights = (knights & black) != 0;
			#else
			constexpr bool bBlackKnights = true;
			#endif
			const auto dispatcher = bBlackRookLikes + bBlackRookLikes + bBlackBishopLikes + 4 * bBlackKnights;

			switch(dispatcher)
			{
				#ifdef __USE_OPTIMFORMISSINGBLACKKNIGHTS__
				case 0: return reinterpret_cast<const FullBitboards<MoveGenMethod, 0, 0, 0>*>(this)->template FindMoveThatMatesInTwoMoves<tbWhiteKingUnderCheck, tbEnPassantPossible, tbWhiteCastlingFlags, tbBlackCastlingFlags, tbFindAllSolutionsAndFillBuf, false>(posWhiteKingChecker, bposToCaptureWithEnPassant, pMoves);
				case 1: return reinterpret_cast<const FullBitboards<MoveGenMethod, 0, 1, 0>*>(this)->template FindMoveThatMatesInTwoMoves<tbWhiteKingUnderCheck, tbEnPassantPossible, tbWhiteCastlingFlags, tbBlackCastlingFlags, tbFindAllSolutionsAndFillBuf, false>(posWhiteKingChecker, bposToCaptureWithEnPassant, pMoves);
				case 2: return reinterpret_cast<const FullBitboards<MoveGenMethod, 1, 0, 0>*>(this)->template FindMoveThatMatesInTwoMoves<tbWhiteKingUnderCheck, tbEnPassantPossible, tbWhiteCastlingFlags, tbBlackCastlingFlags, tbFindAllSolutionsAndFillBuf, false>(posWhiteKingChecker, bposToCaptureWithEnPassant, pMoves);
				case 3: return reinterpret_cast<const FullBitboards<MoveGenMethod, 1, 1, 0>*>(this)->template FindMoveThatMatesInTwoMoves<tbWhiteKingUnderCheck, tbEnPassantPossible, tbWhiteCastlingFlags, tbBlackCastlingFlags, tbFindAllSolutionsAndFillBuf, false>(posWhiteKingChecker, bposToCaptureWithEnPassant, pMoves);
				#endif
				case 4: return reinterpret_cast<const FullBitboards<MoveGenMethod, 0, 0, 1>*>(this)->template FindMoveThatMatesInTwoMoves<tbWhiteKingUnderCheck, tbEnPassantPossible, tbWhiteCastlingFlags, tbBlackCastlingFlags, tbFindAllSolutionsAndFillBuf, false>(posWhiteKingChecker, bposToCaptureWithEnPassant, pMoves);
				case 5: return reinterpret_cast<const FullBitboards<MoveGenMethod, 0, 1, 1>*>(this)->template FindMoveThatMatesInTwoMoves<tbWhiteKingUnderCheck, tbEnPassantPossible, tbWhiteCastlingFlags, tbBlackCastlingFlags, tbFindAllSolutionsAndFillBuf, false>(posWhiteKingChecker, bposToCaptureWithEnPassant, pMoves);
				case 6: return reinterpret_cast<const FullBitboards<MoveGenMethod, 1, 0, 1>*>(this)->template FindMoveThatMatesInTwoMoves<tbWhiteKingUnderCheck, tbEnPassantPossible, tbWhiteCastlingFlags, tbBlackCastlingFlags, tbFindAllSolutionsAndFillBuf, false>(posWhiteKingChecker, bposToCaptureWithEnPassant, pMoves);
				case 7: return reinterpret_cast<const FullBitboards<MoveGenMethod, 1, 1, 1>*>(this)->template FindMoveThatMatesInTwoMoves<tbWhiteKingUnderCheck, tbEnPassantPossible, tbWhiteCastlingFlags, tbBlackCastlingFlags, tbFindAllSolutionsAndFillBuf, false>(posWhiteKingChecker, bposToCaptureWithEnPassant, pMoves);
			}

			assert(false);
			return 0;
		}
		#endif

		int count;
		if constexpr (tbFindAllSolutionsAndFillBuf)
			count = 0;

		if (tbWhiteKingUnderCheck > 0 || (tbWhiteKingUnderCheck < 0 && IsSquareAttackedByBlack(posWhiteKing)))
		{
			if (posWhiteKingChecker < 0)
				posWhiteKingChecker = GeWhiteKingCheckerPos();

			if (posWhiteKingChecker != DBL_CHECKED)
			{
				// 1) Capture checker:
				Bitboard mask;
				if (tbEnPassantPossible && posWhiteKingChecker != bposToCaptureWithEnPassant)
					mask = CanWhiteCapture<false, 1, 1, 0>(posWhiteKingChecker); // don't verify en passant if there is another checker (not the black pawn vulnerable to en passant) since en passant can never block discovered check
				else
					mask = CanWhiteCapture<tbEnPassantPossible, 1, 1, 0>(posWhiteKingChecker);

				BEGIN_FOR_EACH_POS_IN_MASK(posFrom, mask)
				{
					const auto posTo = (tbEnPassantPossible && IsWhiteCaptureEnPassant(posFrom, posWhiteKingChecker)) ? posWhiteKingChecker + 8 : posWhiteKingChecker;
					if (!tbFindAllSolutionsAndFillBuf || !IsWhitePromoMove(posFrom))
					{
						if (IsImmediateMateAfterAnyBlackResponseAfterWhiteMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo))
							if constexpr (!tbFindAllSolutionsAndFillBuf)
								return true;
							else
								pMoves[count++].set(posFrom, posTo);
					}
					else
					{
						if (IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo, FGR_QUEEN))
							pMoves[count++].set(posFrom, posTo, FGR_QUEEN);
						if (IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo, FGR_ROOK))
							pMoves[count++].set(posFrom, posTo, FGR_ROOK);
						if (IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo, FGR_BISHOP))
							pMoves[count++].set(posFrom, posTo, FGR_BISHOP);
						if (IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo, FGR_KNIGHT))
							pMoves[count++].set(posFrom, posTo, FGR_KNIGHT);
					}
				}
				END_FOR_EACH_POS_IN_MASK(posFrom, mask);

				// 2) Block check:
				if (!IsKnightDiff(posWhiteKingChecker, posWhiteKing) && !AreSquaresAdjacent(posWhiteKingChecker, posWhiteKing))
				{
					TMove aMoves[256];
					const auto num = CanWhiteMoveInBetween<1, 0>(posWhiteKingChecker, posWhiteKing, 0, aMoves);
					for (int i = 0; i < num; ++i)
					{
						const auto& move = aMoves[i];
						if (!tbFindAllSolutionsAndFillBuf || !IsWhitePromoMove(move.nFrom))
						{
							if (IsImmediateMateAfterAnyBlackResponseAfterWhiteMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(move))
								if constexpr (!tbFindAllSolutionsAndFillBuf)
									return true;
								else
									pMoves[count++].set(move.nFrom, move.nTo);
						}
						else
						{
							if (IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(move.nFrom, move.nTo, move.IsPromotion()))
								pMoves[count++] = move;
						}
					}
				}
			}

			// 3) Escape by white king:
			auto mask = King_Attacks[posWhiteKing] & ~white;
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, mask)
			{
				if (!IsSquareAttackedByBlackIfTakeOffWhiteKing(posTo))
					if (IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove<tbBlackCastlingFlags>(posTo))
						if constexpr (!tbFindAllSolutionsAndFillBuf)
							return true;
						else
							pMoves[count++].set(posWhiteKing, posTo);
			}
			END_FOR_EACH_POS_IN_MASK(posTo, mask);
		}
		else
		{
			constexpr bool tbCanBePinned = tbBlackHaveRookLikes || tbBlackHaveBishopLikes;
			constexpr bool tbWhiteKingKnownToBeNotUnderCheck = true;
			const auto whitePinnedPieces = GetWhitePinnedPieces<tbWhiteKingKnownToBeNotUnderCheck>();

			auto mask = white & queens();
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = get_bishop_moves(pos, occ(), white) | get_rook_moves(pos, occ(), white);
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
				{					
					if (!tbCanBePinned || !IsPosInBitmask(pos, whitePinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posWhiteKing))
						if (IsImmediateMateAfterAnyBlackResponseAfterWhiteQueenMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo))
							if constexpr (!tbFindAllSolutionsAndFillBuf)
								return true;
							else
								pMoves[count++].set(pos, posTo);
				}
				END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = white & rooks();
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = get_rook_moves(pos, occ(), white);
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
				{					
					if (!tbCanBePinned || !IsPosInBitmask(pos, whitePinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posWhiteKing))
					{
						// A rook while moving might have just lost its possibility of castling, so we may have to modify castling flags:
						bool res;
						if (!tbWhiteCastlingFlags || ((tbWhiteCastlingFlags & 1) && pos != _H1_) || ((tbWhiteCastlingFlags & 2) && pos != _A1_) || (tbWhiteCastlingFlags == 3 && ((pos != _A1_) & (pos != _H1_))))
							res = IsImmediateMateAfterAnyBlackResponseAfterWhiteRookMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo);
						else if (pos == _H1_)
							res = IsImmediateMateAfterAnyBlackResponseAfterWhiteRookMove<tbWhiteCastlingFlags & ~1, tbBlackCastlingFlags>(pos, posTo);
						else
							res = IsImmediateMateAfterAnyBlackResponseAfterWhiteRookMove<tbWhiteCastlingFlags & ~2, tbBlackCastlingFlags>(pos, posTo);

						if (res)
							if constexpr (!tbFindAllSolutionsAndFillBuf)
								return true;
							else
								pMoves[count++].set(pos, posTo);
					}

				}
				END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = white & bishops();
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = get_bishop_moves(pos, occ(), white);
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
				{					
					if (!tbCanBePinned || !IsPosInBitmask(pos, whitePinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posWhiteKing))
						if (IsImmediateMateAfterAnyBlackResponseAfterWhiteBishopMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo))
							if constexpr (!tbFindAllSolutionsAndFillBuf)
								return true;
							else
								pMoves[count++].set(pos, posTo);
				}
				END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = white & knights & ~whitePinnedPieces;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = Knight_Attacks[pos] & ~white;
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
				{
					if (IsImmediateMateAfterAnyBlackResponseAfterWhiteKnightMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo))
						if constexpr (!tbFindAllSolutionsAndFillBuf)
							return true;
						else
							pMoves[count++].set(pos, posTo);
				}
				END_FOR_EACH_POS_IN_MASK(posTo, movesMask);				
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);


			mask = white & pawns;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto maskMoves = White_Pawn_Attacks[pos] & black;
				if (IsEmptyAt(pos + 8))
				{
					maskMoves |= (1ULL << (pos + 8));
					if (pos <= _H2_ && IsEmptyAt(pos + 16))
						maskMoves |= (1ULL << (pos + 16));
				}
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskMoves)
				{
					if (!tbCanBePinned || !IsPosInBitmask(pos, whitePinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posWhiteKing))
						if (!tbFindAllSolutionsAndFillBuf || !IsWhitePromoMove(pos))
						{
							if (IsImmediateMateAfterAnyBlackResponseAfterWhitePawnMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo))
								if constexpr (!tbFindAllSolutionsAndFillBuf)
									return true;
								else
									pMoves[count++].set(pos, posTo);
						}
						else
						{
							if (IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo, FGR_QUEEN))
								pMoves[count++].set(pos, posTo, FGR_QUEEN);
							if (IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo, FGR_ROOK))
								pMoves[count++].set(pos, posTo, FGR_ROOK);
							if (IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo, FGR_BISHOP))
								pMoves[count++].set(pos, posTo, FGR_BISHOP);
							if (IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo, FGR_KNIGHT))
								pMoves[count++].set(pos, posTo, FGR_KNIGHT);
						}
				}
				END_FOR_EACH_POS_IN_MASK(posTo, maskMoves);

				if constexpr (tbEnPassantPossible)
					if (AreSquaresAside(pos, bposToCaptureWithEnPassant))
						if (!IsWhitePinnedIfTakeOffBlackPawn<1>(pos, bposToCaptureWithEnPassant + 8, bposToCaptureWithEnPassant))
							if (IsImmediateMateAfterAnyBlackResponseAfterWhitePawnMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, bposToCaptureWithEnPassant + 8))
								if constexpr (!tbFindAllSolutionsAndFillBuf)
									return true;
								else
									pMoves[count++].set(pos, bposToCaptureWithEnPassant + 8);
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			auto maskMoves = King_Attacks[posWhiteKing] & ~white & ~King_Attacks[posBlackKing] & ~BlackPawnAttacks() & ~BlackKnightAttacks();
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskMoves)
			{
				if (!IsSquareAttackedByBlack<-1>(posTo))
					if (IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove<tbBlackCastlingFlags>(posTo))
						if constexpr (!tbFindAllSolutionsAndFillBuf)
							return true;
						else
							pMoves[count++].set(posWhiteKing, posTo);
			}
			END_FOR_EACH_POS_IN_MASK(pos, maskMoves);

			// Castling short?
			if constexpr ((tbWhiteCastlingFlags & 1) != 0)
				if (posWhiteKing == _E1_ && IsWhiteRookAt(_H1_))
					if (IsEmptyAt(_F1_) && IsEmptyAt(_G1_))
						if (!IsSquareAttackedByBlack(_F1_) && !IsSquareAttackedByBlack(_G1_))
							if (IsImmediateMateAfterAnyBlackResponseAfterWhiteCastlingShort<tbBlackCastlingFlags>())
								if constexpr (!tbFindAllSolutionsAndFillBuf)
									return true;
								else
									pMoves[count++].set(_E1_, _G1_);

			// Castling long?
			if constexpr ((tbWhiteCastlingFlags & 2) != 0)
				if (posWhiteKing == _E1_ && IsWhiteRookAt(_A1_))
					if (IsEmptyAt(_B1_) && IsEmptyAt(_C1_) && IsEmptyAt(_D1_))
						if (!IsSquareAttackedByBlack(_C1_) && !IsSquareAttackedByBlack(_D1_))
							if (IsImmediateMateAfterAnyBlackResponseAfterWhiteCastlingLong<tbBlackCastlingFlags>())
								if constexpr (!tbFindAllSolutionsAndFillBuf)
									return true;
								else
									pMoves[count++].set(_E1_, _C1_);
		}

		if constexpr (tbFindAllSolutionsAndFillBuf)
			return count;
		else
			return false;
	}

	template<bool FindAllSolutionsAndFillBuf = false>
	ALWAYS_INLINE int SolveTwoMoverDispatcher(const int whiteKingChecker, const int enPassantSquare, const int castlingFlags, TMove* pMoves) CONST_RESTRICT
	{
		const bool bEnPassantPossible = enPassantSquare >= 0;

		const int dispatcher = (whiteKingChecker >= 0) * 32 + bEnPassantPossible * 16 + castlingFlags;
		switch (dispatcher)
		{
			case 0:
				return FindMoveThatMatesInTwoMoves<0, 0, 0, 0, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 1: // black castling short possible
				return FindMoveThatMatesInTwoMoves<0, 0, 0, 1, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 2: // black castling long possible
				return FindMoveThatMatesInTwoMoves<0, 0, 0, 2, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 3: // both black castling short and long possible
				return FindMoveThatMatesInTwoMoves<0, 0, 0, 3, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 4: // white castling short possible
				return FindMoveThatMatesInTwoMoves<0, 0, 1, 0, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 5: // white and black castling short possible
				return FindMoveThatMatesInTwoMoves<0, 0, 1, 1, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 6: // white castling short and black castling long possible
				return FindMoveThatMatesInTwoMoves<0, 0, 1, 2, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 7: // white castling short and both black castlings possible
				return FindMoveThatMatesInTwoMoves<0, 0, 1, 3, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 8: // white castling long possible
				return FindMoveThatMatesInTwoMoves<0, 0, 2, 0, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 9: // white castling long possible and black castling short 
				return FindMoveThatMatesInTwoMoves<0, 0, 2, 1, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 10: // white castling long possible and black castling long possible
				return FindMoveThatMatesInTwoMoves<0, 0, 2, 2, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 11: // white castling long possible and both black castlings possible
				return FindMoveThatMatesInTwoMoves<0, 0, 2, 3, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 12: // both white castlings possible
				return FindMoveThatMatesInTwoMoves<0, 0, 3, 0, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 13: // both white castlings possible and black castling short possible
				return FindMoveThatMatesInTwoMoves<0, 0, 3, 1, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 14: // both white castlings possible and black castling long possible
				return FindMoveThatMatesInTwoMoves<0, 0, 3, 2, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
			case 15: // both white castlings possible and both black castlings possible
				return FindMoveThatMatesInTwoMoves<0, 0, 3, 3, FindAllSolutionsAndFillBuf>(-1, -1, pMoves);
            // ---- En passant possible:
			case 16:
				return FindMoveThatMatesInTwoMoves<0, 1, 0, 0, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 17: // black castling short possible
				return FindMoveThatMatesInTwoMoves<0, 1, 0, 1, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 18: // black castling long possible
				return FindMoveThatMatesInTwoMoves<0, 1, 0, 2, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 19: // both black castling short and long possible
				return FindMoveThatMatesInTwoMoves<0, 1, 0, 3, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 20: // white castling short possible
				return FindMoveThatMatesInTwoMoves<0, 1, 1, 0, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 21: // white and black castling short possible
				return FindMoveThatMatesInTwoMoves<0, 1, 1, 1, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 22: // white castling short and black castling long possible
				return FindMoveThatMatesInTwoMoves<0, 1, 1, 2, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 23: // white castling short and both black castlings possible
				return FindMoveThatMatesInTwoMoves<0, 1, 1, 3, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 24: // white castling long possible
				return FindMoveThatMatesInTwoMoves<0, 1, 2, 0, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 25: // white castling long possible and black castling short 
				return FindMoveThatMatesInTwoMoves<0, 1, 2, 1, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 26: // white castling long possible and black castling long possible
				return FindMoveThatMatesInTwoMoves<0, 1, 2, 2, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 27: // white castling long possible and both black castlings possible
				return FindMoveThatMatesInTwoMoves<0, 1, 2, 3, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 28: // both white castlings possible
				return FindMoveThatMatesInTwoMoves<0, 1, 3, 0, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 29: // both white castlings possible and black castling short possible
				return FindMoveThatMatesInTwoMoves<0, 1, 3, 1, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 30: // both white castlings possible and black castling long possible
				return FindMoveThatMatesInTwoMoves<0, 1, 3, 2, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			case 31: // both white castlings possible and both black castlings possible
				return FindMoveThatMatesInTwoMoves<0, 1, 3, 3, FindAllSolutionsAndFillBuf>(-1, enPassantSquare, pMoves);
			// ----- white king under check:
			case 32:
				return FindMoveThatMatesInTwoMoves<1, 0, 0, 0, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 33: // black castling short possible
				return FindMoveThatMatesInTwoMoves<1, 0, 0, 1, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 34: // black castling long possible
				return FindMoveThatMatesInTwoMoves<1, 0, 0, 2, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 35: // both black castling short and long possible
				return FindMoveThatMatesInTwoMoves<1, 0, 0, 3, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 36: // white castling short possible
				return FindMoveThatMatesInTwoMoves<1, 0, 1, 0, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 37: // white and black castling short possible
				return FindMoveThatMatesInTwoMoves<1, 0, 1, 1, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 38: // white castling short and black castling long possible
				return FindMoveThatMatesInTwoMoves<1, 0, 1, 2, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 39: // white castling short and both black castlings possible
				return FindMoveThatMatesInTwoMoves<1, 0, 1, 3, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 40: // white castling long possible
				return FindMoveThatMatesInTwoMoves<1, 0, 2, 0, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 41: // white castling long possible and black castling short 
				return FindMoveThatMatesInTwoMoves<1, 0, 2, 1, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 42: // white castling long possible and black castling long possible
				return FindMoveThatMatesInTwoMoves<1, 0, 2, 2, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 43: // white castling long possible and both black castlings possible
				return FindMoveThatMatesInTwoMoves<1, 0, 2, 3, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 44: // both white castlings possible
				return FindMoveThatMatesInTwoMoves<1, 0, 3, 0, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 45: // both white castlings possible and black castling short possible
				return FindMoveThatMatesInTwoMoves<1, 0, 3, 1, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 46: // both white castlings possible and black castling long possible
				return FindMoveThatMatesInTwoMoves<1, 0, 3, 2, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			case 47: // both white castlings possible and both black castlings possible
				return FindMoveThatMatesInTwoMoves<1, 0, 3, 3, FindAllSolutionsAndFillBuf>(whiteKingChecker, -1, pMoves);
			// ----- white king under check AND en passant possible:
			case 48:
				return FindMoveThatMatesInTwoMoves<1, 1, 0, 0, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 49: // black castling short possible
				return FindMoveThatMatesInTwoMoves<1, 1, 0, 1, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 50: // black castling long possible
				return FindMoveThatMatesInTwoMoves<1, 1, 0, 2, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 51: // both black castling short and long possible
				return FindMoveThatMatesInTwoMoves<1, 1, 0, 3, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 52: // white castling short possible
				return FindMoveThatMatesInTwoMoves<1, 1, 1, 0, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 53: // white and black castling short possible
				return FindMoveThatMatesInTwoMoves<1, 1, 1, 1, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 54: // white castling short and black castling long possible
				return FindMoveThatMatesInTwoMoves<1, 1, 1, 2, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 55: // white castling short and both black castlings possible
				return FindMoveThatMatesInTwoMoves<1, 1, 1, 3, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 56: // white castling long possible
				return FindMoveThatMatesInTwoMoves<1, 1, 2, 0, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 57: // white castling long possible and black castling short 
				return FindMoveThatMatesInTwoMoves<1, 1, 2, 1, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 58: // white castling long possible and black castling long possible
				return FindMoveThatMatesInTwoMoves<1, 1, 2, 2, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 59: // white castling long possible and both black castlings possible
				return FindMoveThatMatesInTwoMoves<1, 1, 2, 3, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 60: // both white castlings possible
				return FindMoveThatMatesInTwoMoves<1, 1, 3, 0, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 61: // both white castlings possible and black castling short possible
				return FindMoveThatMatesInTwoMoves<1, 1, 3, 1, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 62: // both white castlings possible and black castling long possible
				return FindMoveThatMatesInTwoMoves<1, 1, 3, 2, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);
			case 63: // both white castlings possible and both black castlings possible
				return FindMoveThatMatesInTwoMoves<1, 1, 3, 3, FindAllSolutionsAndFillBuf>(whiteKingChecker, enPassantSquare, pMoves);

		}			
		assert(false);
		return false;
	}

	ALWAYS_INLINE bool IsImmediateCheckMateDispatcher(const int whiteKingChecker, const int enPassantSquare, const bool whiteCastlingShortPossible, const bool whiteCastlingLongPossible)
	{
		assert(IsValidPos(whiteKingChecker) || whiteKingChecker == -1 || whiteKingChecker == DBL_CHECKED);

		const bool bEnPassantPossible = enPassantSquare >= _A5_;
		const int dispatcher = (whiteKingChecker >= 0) * 8 + bEnPassantPossible * 4 + whiteCastlingLongPossible + whiteCastlingShortPossible * 2;
		switch (dispatcher)
		{
			case 0:
				return FindMoveThatMates<0, 0, 0, 0>();
			case 1: // castling long possible
				return FindMoveThatMates<0, 0, 0, 1>();
			case 2: // castling short possible
				return FindMoveThatMates<0, 0, 1, 0>();
			case 3: // both castling short and long possible
				return FindMoveThatMates<0, 0, 1, 1>();
			case 4: // en passant possible
				return FindMoveThatMates<0, 1, 0, 0>(-1, enPassantSquare);
			case 5: // en passant and castling long possible
				return FindMoveThatMates<0, 1, 0, 1>(-1, enPassantSquare);
			case 6: // en passant and castling short possible
				return FindMoveThatMates<0, 1, 1, 0>(-1, enPassantSquare);
			case 7: // en passant and both castling short and long possible
				return FindMoveThatMates<0, 1, 1, 1>(-1, enPassantSquare);
			case 8: // wh.king under check
				return FindMoveThatMates<1, 0, 0, 0>(whiteKingChecker);
			case 9: // wh.king under check and castling long possible
				return FindMoveThatMates<1, 0, 0, 1>(whiteKingChecker);
			case 10: // wh.king under check and castling short possible
				return FindMoveThatMates<1, 0, 1, 0>(whiteKingChecker);
			case 11: // wh.king under check and both castling short and long possible
				return FindMoveThatMates<1, 0, 1, 1>(whiteKingChecker);
			case 12: // wh.king under check and en passant possible
				return FindMoveThatMates<1, 1, 0, 0>(whiteKingChecker, enPassantSquare);
			case 13: // wh.king under check, en passant possible and castling long possible
				return FindMoveThatMates<1, 1, 0, 1>(whiteKingChecker, enPassantSquare);
			case 14: // wh.king under check, en passant possible and castling short possible
				return FindMoveThatMates<1, 1, 1, 0>(whiteKingChecker, enPassantSquare);
			case 15: // wh.king under check, en passant possible and both castling short and long possible
				return FindMoveThatMates<1, 1, 1, 1>(whiteKingChecker, enPassantSquare);
		}

		assert(false);
		return false;

	}

	// Returns move (0,0) on error
	TMove StringToMove(const char* szMove) CONST_RESTRICT
	{
		TMove move;

		if (strcmp(szMove, "0-0") == 0)
		{
			move.set(_E1_, _G1_);
			return move;
		}
		if (strcmp(szMove, "0-0-0") == 0)
		{
			move.set(_E1_, _C1_);
			return move;
		}

		move.set(0, 0);
		size_t idx = 0;

		if (isupper(szMove[0]))
			++idx;
		
		if (szMove[idx] == 0 || szMove[idx + 1] == 0 || szMove[idx + 2] == 0 || szMove[idx + 3] == 0 || szMove[idx + 4] == 0)
			return move;

		const int posFrom = szMove[idx] - 'a' + (szMove[idx + 1] - '1') * 8;
		const int posTo = szMove[idx + 3] - 'a' + (szMove[idx + 4] - '1') * 8;
		if (IsValidPos(posFrom) && IsValidPos(posTo) && posTo != posFrom)
			if (IsWhiteAt(posFrom) && !IsWhiteAt(posTo))
			{
				const bool isPromo = posTo >= _A8_ && IsWhitePawnAt(posFrom);
				if (isPromo)
				{
					if (szMove[idx + 5] == 0)
						return move;
					switch (szMove[idx + 5])
					{
						case 'Q': move.set(posFrom, posTo, FGR_QUEEN); break;
						case 'R': move.set(posFrom, posTo, FGR_ROOK); break;
						case 'B': move.set(posFrom, posTo, FGR_BISHOP); break;
						case 'N': move.set(posFrom, posTo, FGR_KNIGHT); break;
						default: return move;
					}
				}
				else
					move.set(posFrom, posTo);
			}

		return move;
	}

	// Move generation:

	template<char Dir = 0> //Dir 0 = any, 1/-1 == horizontal, 8/-8 == vertical
	ALWAYS_INLINE static Bitboard get_raw_rook_moves(const int square, const Bitboard occupancy)
	{
		static_assert(Dir == 0 || Dir == 1 || Dir == -1 || Dir == 8 || Dir == -8, "");
		assert(IsValidPos(square));

		#ifdef __INCLUDE_FANCY_MAGIC_BITBOARDS__
		if constexpr (MoveGenMethod == MoveGenMethodT::FancyMagics)
			if constexpr (Dir == 0)
				return get_raw_rook_moves_fmb(square, occupancy);
		#endif

		if constexpr (MoveGenMethod == MoveGenMethodT::DenseFancyMagics)
			if constexpr (Dir == 0)
				return get_raw_rook_moves_dfmb(square, occupancy);

		return get_raw_rook_moves_hq(square, occupancy);
	}

	//Dir 0 = any, 1/-1 == horizontal, 8/-8 == vertical
	template<char Dir = 0, bool tbCapturesOnly = false>
	ALWAYS_INLINE static Bitboard get_rook_moves(const int square, const Bitboard occupancy, const Bitboard pieces_of_same_color)
	{
		assert(IsValidPos(square));

		Bitboard raw_moves = get_raw_rook_moves<Dir>(square, occupancy);

		if constexpr (tbCapturesOnly)
			return raw_moves & (occupancy & ~pieces_of_same_color);
		else
			// Filter own pieces:
			return raw_moves & ~pieces_of_same_color;
	}

	// These are not moves yet, since blocking piece is not considered properly
	template<char Dir = 0>
	ALWAYS_INLINE static auto get_raw_bishop_moves(const int square, const Bitboard occupancy)
	{
		assert(IsValidPos(square));

		#ifdef __INCLUDE_FANCY_MAGIC_BITBOARDS__
		if constexpr (MoveGenMethod == MoveGenMethodT::FancyMagics)
			if constexpr (Dir == 0)
				return get_raw_bishop_moves_fmb(square, occupancy);
		#endif

		if constexpr (MoveGenMethod == MoveGenMethodT::DenseFancyMagics)
			if constexpr (Dir == 0)
				return get_raw_bishop_moves_dfmb(square, occupancy);

		return get_raw_bishop_moves_hq<Dir>(square, occupancy);
	}

	// Dir == 0 == any, 1==main diagonal, -1==anti diagonal
	template<bool tbCapturesOnly = false, char Dir = 0>
	ALWAYS_INLINE static Bitboard get_bishop_moves(const int square, const Bitboard occupancy, const Bitboard own_pieces) // own_pieces should be white_pieces for white bishop and vice versa
	{
		static_assert(Dir == 0 || Dir == 1 || Dir == -1, "");
		assert(IsValidPos(square));

		Bitboard raw_moves = get_raw_bishop_moves<Dir>(square, occupancy);
		if constexpr (tbCapturesOnly)
			raw_moves &= (occupancy & ~own_pieces);
		else
			raw_moves &= ~own_pieces;
		return raw_moves;
	}

public:
	// Returns empty vector on error:
	std::vector<TMove> StringToMoves(const std::string& moves) CONST_RESTRICT
	{
		std::vector<TMove> res;
		res.reserve(moves.size() / 4 + 1);
		
		char szMove[256];
		size_t idx = 0;
		for (size_t i = 0; i < moves.size(); ++i)
			if (moves[i] != ' ')
				szMove[idx++] = moves[i];
			else
			{
				szMove[idx] = 0;
				auto move = StringToMove(szMove);
				if (move.nFrom == 0 && move.nTo == 0)
				{
					res.clear();
					break;
				}
				res.push_back(move);
				idx = 0;
			}

		szMove[idx] = 0;
		auto move = StringToMove(szMove);
		if (move.nFrom == 0 && move.nTo == 0)		
			res.clear();
		else		
			res.push_back(move);

		return res;
	}

};

using FullBitboards_HQ = FullBitboards<MoveGenMethodT::HyperbolaQuintessence>;
using FullBitboards_FMB = FullBitboards<MoveGenMethodT::FancyMagics>;
using FullBitboards_DFMB = FullBitboards<MoveGenMethodT::DenseFancyMagics>;
static_assert(sizeof(FullBitboards_DFMB) <= 64); // let's not exceed this limit - it can degrade performance
