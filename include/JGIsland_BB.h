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
	uint64_t white;
	uint64_t black;

	uint64_t kings;
	uint64_t pawns;
	uint64_t bishops;
	uint64_t rooks;
	uint64_t queens; // an interesting alternative is to avoid this field and code queens as both bishop and rook; in such case we'd need methods similar to occ(), e.g.: uint64_t rooks() {return rooks & ~bishops;} and uint64_t bishops() {return bishops & ~rooks;} 
	uint64_t knights;

	bool operator == (const FullBitboards&) const = default;

	[[nodiscard]] ALWAYS_INLINE uint64_t occ() const noexcept {
		return white | black;
	}
	void clear()
	{
		white = black = kings = pawns = bishops = rooks = queens = knights = 0ULL;
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
		uint64_t mask;
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
					bishops |= mask;
					break;
				case 'b':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					black |= mask;
					bishops |= mask;
					break;
				case 'R':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					white |= mask;
					rooks |= mask;
					break;
				case 'r':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					black |= mask;
					rooks |= mask;
					break;
				case 'Q':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					white |= mask;
					queens |= mask;
					break;
				case 'q':
					pos = file + line * 8;
					mask = sq_to_bb(pos);
					black |= mask;
					queens |= mask;
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

		while (((szFEN[i] == ' ') | (szFEN[i] == '\n')) & (szFEN[i] != 0))
			++i;
		
		if (szFEN[i] == 0)
		{
			// short FEN (no castling/en passant info) ? If so, then assume castlings available and no en passant
			res.first = true;
			res.second.whiteCastlingLongPossible = (((1ULL << _E1_) & white & kings) != 0) & (((1ULL << _A1_) & white & rooks) != 0);
			res.second.whiteCastlingShortPossible = (((1ULL << _E1_) & white & kings) != 0) & (((1ULL << _H1_) & white & rooks) != 0);
			res.second.blackCastlingLongPossible = (((1ULL << _E8_) & black & kings) != 0) & (((1ULL << _A8_) & black & rooks) != 0);
			res.second.blackCastlingShortPossible = (((1ULL << _E8_) & black & kings) != 0) & (((1ULL << _H8_) & black & rooks) != 0);
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
	ALWAYS_INLINE FIGURE GetFigureAt(const int pos) const
	{
		assert(IsValidPos(pos));
		const auto mask = sq_to_bb(pos);
		assert(mask & occ());

		if constexpr (tbSkipKing)
			return ((pawns & mask) != 0) * FGR_PAWN + ((bishops & mask) != 0) * FGR_BISHOP + ((rooks & mask) != 0) * FGR_ROOK + MUL<FGR_QUEEN>((queens & mask) != 0) + ((knights & mask) != 0) * FGR_KNIGHT;
		else
			return ((kings & mask) != 0) * FGR_KING + ((pawns & mask) != 0) * FGR_PAWN + ((bishops & mask) != 0) * FGR_BISHOP + ((rooks & mask) != 0) * FGR_ROOK + MUL<FGR_QUEEN>((queens & mask) != 0) + ((knights & mask) != 0) * FGR_KNIGHT;
	}
	ALWAYS_INLINE FIGURE GetLongDistanceFigureAt(const int pos) const
	{
		assert(IsValidPos(pos));
		const auto mask = sq_to_bb(pos);
		assert(mask & occ());

		return ((bishops & mask) != 0) * FGR_BISHOP + ((rooks & mask) != 0) * FGR_ROOK + MUL<FGR_QUEEN>((queens & mask) != 0);
	}
	ALWAYS_INLINE bool AllBetweenEmpty(const int pos1, const int pos2) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(pos1 != pos2);

		return (GetBetweenMask(pos1, pos2) & (white | black)) == 0;
	}
	ALWAYS_INLINE bool AllBetweenEmptyIfTakeOffWhitePawn(const int pos1, const int pos2, const int posWhitePawnToTakeOff) const
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
	// -------------------- This group of methods can be extended by switching on __USE_BETWEENLOOKUP (aditional 16kB while originally only 6kB buffers in total) -------------------------
	template<bool tbIncludeEnds = false>
	ALWAYS_INLINE static constexpr std::uint64_t GetBetweenMask(const char sq1, const char sq2)
	{
		assert(IsValidPos(sq1));
		assert(IsValidPos(sq2));
		assert(sq1 != sq2);
		assert(SameDiagonalOrLine(sq1, sq2));

		#ifdef __USE_BETWEENLOOKUP__
		const auto res = betweenLookup.GetBetweenMask(sq1, sq2);
		assert(res != ~0ULL);
		if constexpr (tbIncludeEnds)
			return res | (sq_to_bb(sq1)) | (sq_to_bb(sq2));
		else
			return res;		

		#else

		const Bitboard rook_match = Rook_Attacks[sq1] & Rook_Attacks[sq2];
		const Bitboard bishop_match = Bishop_Attacks[sq1] & Bishop_Attacks[sq2];

		const bool is_rook_line = std::popcount(rook_match) >= 6;

		const Bitboard full_line = is_rook_line ? rook_match : bishop_match;

		const int min_sq = std::min(sq1, sq2);
		const int max_sq = std::max(sq1, sq2);

		const Bitboard up_mask = ~((sq_to_bb(min_sq + 1)) - 1);
		const Bitboard down_mask = (sq_to_bb(max_sq)) - 1;

		const auto res = full_line & up_mask & down_mask;
		if constexpr (tbIncludeEnds)
			return res | (sq_to_bb(sq1)) | (sq_to_bb(sq2)); 
		else
			return res;			
		#endif
	}

	template<bool tbSquaresS1S2KnownToBeOnSameDiagonalOrLine = false, bool tbExcludingEnds = true, bool tbOptim = true>
	ALWAYS_INLINE static constexpr bool IsSquareBetween(const char square, const char s1, const char s2)
	{
		assert(IsValidPos(square));
		assert(IsValidPos(s1));
		assert(IsValidPos(s2));
		assert(s1 != s2); // let's assume that square can be the same as s1 or s2
		#ifndef __USE_BETWEENLOOKUP__ 
		if constexpr (!tbSquaresS1S2KnownToBeOnSameDiagonalOrLine)
			return GetBetweenMask<!tbExcludingEnds>(s1, s2) & (sq_to_bb(square));		
		#endif	
		assert(!tbSquaresS1S2KnownToBeOnSameDiagonalOrLine || SameDiagonalOrLine(s1, s2));

		#ifdef __USE_BETWEENLOOKUP__ 
		if constexpr(tbSquaresS1S2KnownToBeOnSameDiagonalOrLine)
			return GetBetweenMask<!tbExcludingEnds>(s1, s2) & (sq_to_bb(square));
		else
		{
			const auto mask = GetBetweenMask<!tbExcludingEnds>(s1, s2);
			const bool match = (mask & (sq_to_bb(square))) != 0;
			const bool res = (mask != ~0ULL) ? match : false;
			return res;
		}
		#else
		return GetBetweenMask<!tbExcludingEnds>(s1, s2) & (sq_to_bb(square));
		#endif

	}

	#ifdef __USE_BETWEENLOOKUP__
	// First two optional helper methods:
	ALWAYS_INLINE bool AllBetweenEmptyByLookup(const int pos1, const int pos2) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(pos1 != pos2);

		const auto mask = betweenLookup.GetBetweenMask(pos1, pos2);

		#ifndef NDEBUG
		const bool sameDiagOrLine = SameDiagonalOrLine(pos1, pos2);
		assert((sameDiagOrLine && mask == GetBetweenMask(pos1, pos2)) || (!sameDiagOrLine && mask == -1)); // cross-check
		#endif

		return (mask & occ()) == 0;
	}
	ALWAYS_INLINE bool AllBetweenEmptyByLookupIfTakeOffWhitePawn(const int pos1, const int pos2, const int posWhitePawnToTakeOff) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(pos1 != pos2);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		const auto pawnMask = (sq_to_bb(posWhitePawnToTakeOff));
		const_cast<FullBitboards*>(this)->white ^= pawnMask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= pawnMask;
		#endif

		const auto res = AllBetweenEmptyByLookup(pos1, pos2);

		const_cast<FullBitboards*>(this)->white ^= pawnMask;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->pawns ^= pawnMask;
		#endif

		return res;
	}
	#endif
	ALWAYS_INLINE bool SameLineAndAllBetweenEmpty(const int pos1, const int pos2) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(pos1 != pos2);
		
		#ifndef __USE_BETWEENLOOKUP__
		return SameLine(pos1, pos2) && AllBetweenEmpty(pos1, pos2);
		#else
		return AllBetweenEmptyByLookup(pos1, pos2) & (SameLine(pos1, pos2));
		#endif
	}
	ALWAYS_INLINE bool SameDiagAndAllBetweenEmpty(const int pos1, const int pos2) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(pos1 != pos2);

		#ifndef __USE_BETWEENLOOKUP__
		return SameDiag(pos1, pos2) && AllBetweenEmpty(pos1, pos2);
		#else
		return AllBetweenEmptyByLookup(pos1, pos2) & (SameDiag(pos1, pos2));
		#endif
	}
	ALWAYS_INLINE bool SameDiagonalOrLineAndAllBetweenEmpty(const int pos1, const int pos2) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(pos1 != pos2);

		#ifndef __USE_BETWEENLOOKUP__
		return SameDiagonalOrLine(pos1, pos2) && AllBetweenEmpty(pos1, pos2);
		#else
		return AllBetweenEmptyByLookup(pos1, pos2);
		#endif
	}
	ALWAYS_INLINE bool SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(const int pos1, const int pos2, const int posWhitePawnToTakeOff) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(pos1 != pos2);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		#ifndef __USE_BETWEENLOOKUP__
		return SameDiagonalOrLine(pos1, pos2) && AllBetweenEmptyIfTakeOffWhitePawn(pos1, pos2, posWhitePawnToTakeOff);
		#else
		return AllBetweenEmptyByLookupIfTakeOffWhitePawn(pos1, pos2, posWhitePawnToTakeOff);
		#endif
	}
	ALWAYS_INLINE bool SameDiagAndAllBetweenEmptyIfTakeOffWhitePawn(const int pos1, const int pos2, const int posWhitePawnToTakeOff) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(pos1 != pos2);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		#ifndef __USE_BETWEENLOOKUP__
		return SameDiag(pos1, pos2) && AllBetweenEmptyIfTakeOffWhitePawn(pos1, pos2, posWhitePawnToTakeOff);
		#else
		return AllBetweenEmptyByLookupIfTakeOffWhitePawn(pos1, pos2, posWhitePawnToTakeOff) & (SameDiag(pos1, pos2));
		#endif
	}
	ALWAYS_INLINE bool SameLineAndAllBetweenEmptyIfTakeOffWhitePawn(const int pos1, const int pos2, const int posWhitePawnToTakeOff) const
	{
		assert(IsValidPos(pos1));
		assert(IsValidPos(pos2));
		assert(IsValidPos(posWhitePawnToTakeOff));
		assert(pos1 != pos2);
		assert((sq_to_bb(posWhitePawnToTakeOff)) & white & pawns);

		#ifndef __USE_BETWEENLOOKUP__
		return SameLine(pos1, pos2) && AllBetweenEmptyIfTakeOffWhitePawn(pos1, pos2, posWhitePawnToTakeOff);
		#else
		return AllBetweenEmptyByLookupIfTakeOffWhitePawn(pos1, pos2, posWhitePawnToTakeOff) & (SameLine(pos1, pos2));
		#endif
	}	
	// ---------------------------------------------------------------------------------------------

	template<bool tbGetPos = false, bool tbBlack = true>
	ALWAYS_INLINE int LongDistanceFigureInDir(const int pos, const int posBase) const
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posBase));
		assert(SameDiagonalOrLine(pos, posBase));

		const auto rayMask = GetRay(pos, posBase);
		const auto bishopLike = queens | bishops;
		const auto rookLike = queens | rooks;
		const auto matchingPieceBitboard = SameDiag(pos, posBase) ? bishopLike : rookLike;
		const auto mask = rayMask & (tbBlack ? black : white) & matchingPieceBitboard;
		if (mask)
		{
			const bool bRayUpward = pos > posBase;
			const int posPiece = bRayUpward ? std::countr_zero(mask) : (63 - std::countl_zero(mask));
			const bool bAllBetweenEmpty = AllBetweenEmpty(pos, posPiece);
			if constexpr (tbGetPos)
				return bAllBetweenEmpty ? posPiece : -1;
			else
				return bAllBetweenEmpty;
		}
		if constexpr (tbGetPos)
			return -1;
		else
			return false;
	}

	template<bool tbGetPos = false, bool tbBlack = true>
	ALWAYS_INLINE int LongDistanceFigureInDir(const int pos, const int dx, const int dy) const
	{
		assert(IsValidPos(pos));
		assert(abs(dx) <= 1 && abs(dy) <= 1 && (dx | dy));

		const auto rayMask = GetRayInDir(pos, dx, dy);
		const auto matchingPieceBitboard = (dx & dy) ? (queens | bishops) : (queens | rooks);
		const auto mask = rayMask & (tbBlack ? black : white) & matchingPieceBitboard;
		if (mask)
		{
			const bool bRayUpward = (dy > 0) | ((dy == 0) & (dx > 0));
			const int posPiece = bRayUpward ? std::countr_zero(mask) : (63 - std::countl_zero(mask));
			const bool bAllBetweenEmpty = AllBetweenEmpty(pos, posPiece);
			if constexpr (tbGetPos)
				return bAllBetweenEmpty ? posPiece : -1;
			else
				return bAllBetweenEmpty;
		}
		
		if constexpr (tbGetPos)
			return -1;
		else
			return false;
	}
	template<bool tbGetPos = false>
	ALWAYS_INLINE int WhiteLongDistanceFigureInDir(const int pos, const int posBase) const
	{
		return LongDistanceFigureInDir<tbGetPos, 0>(pos, posBase);
	}
	template<bool tbGetPos = false>
	ALWAYS_INLINE int WhiteLongDistanceFigureInDir(const int pos, const int dx, const int dy) const
	{
		return LongDistanceFigureInDir<tbGetPos, 0>(pos, dx, dy);
	}
	template<bool tbGetPos = false>
	ALWAYS_INLINE int BlackLongDistanceFigureInDir(const int pos, const int posBase) const
	{
		return LongDistanceFigureInDir<tbGetPos, 1>(pos, posBase);
	}
	template<bool tbGetPos = false>
	ALWAYS_INLINE int BlackLongDistanceFigureInDir(const int pos, const int dx, const int dy) const
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

	template<bool tbInclKing = true>
	ALWAYS_INLINE bool IsSquareAttackedByWhiteIfTakeOffBlackKing(const int sq) const
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
	template<bool tbInclKing = true>
	ALWAYS_INLINE bool IsSquareAttackedByBlackIfTakeOffWhiteKing(const int sq) const
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

	// Improved implementations using methods of Hyperbola Quintessence
	template<bool tbInclKing = true, bool tbFindAll = true>
	ALWAYS_INLINE uint64_t IsSquareAttackedByWhite_hq(const int target_sq) const
	{
		assert(IsValidPos(target_sq));

		Bitboard occ = white | black;
		Bitboard white_pawns = white & pawns;
		Bitboard white_knights = white & knights;
		
		Bitboard direct_attackers;
		if constexpr(tbInclKing)
			direct_attackers = (Knight_Attacks[target_sq] & white_knights) | (King_Attacks[target_sq] & white & kings) | (Black_Pawn_Attacks[target_sq] & white_pawns);
		else
			direct_attackers = (Knight_Attacks[target_sq] & white_knights) | (Black_Pawn_Attacks[target_sq] & white_pawns);

		if constexpr (!tbFindAll)
			if (direct_attackers)
				return direct_attackers;

		Bitboard direct_b_moves = get_raw_bishop_moves_hq(target_sq, occ);
		Bitboard direct_r_moves = get_raw_rook_moves_hq(target_sq, occ);

		Bitboard direct_b_pieces = direct_b_moves & (bishops | queens) & white;
		Bitboard direct_r_pieces = direct_r_moves & (rooks | queens) & white;

		direct_attackers |= direct_b_pieces | direct_r_pieces;
		
		return direct_attackers;
	}

	template<bool tbInclKing = true, bool tbFindAll = true>
	ALWAYS_INLINE uint64_t IsSquareAttackedByBlack_hq(const int target_sq) const
	{
		assert(IsValidPos(target_sq));

		Bitboard occ = white | black;
		Bitboard black_pawns = black & pawns;
		Bitboard black_knights = black & knights;

		Bitboard direct_attackers; 
		if constexpr (tbInclKing)
			direct_attackers = (Knight_Attacks[target_sq] & black_knights) | (King_Attacks[target_sq] & black & kings) | (White_Pawn_Attacks[target_sq] & black_pawns);
		else
			direct_attackers = (Knight_Attacks[target_sq] & black_knights) | (White_Pawn_Attacks[target_sq] & black_pawns);

		if constexpr (!tbFindAll)
			if (direct_attackers)
				return direct_attackers;

		Bitboard direct_b_moves = get_raw_bishop_moves_hq(target_sq, occ);
		Bitboard direct_r_moves = get_raw_rook_moves_hq(target_sq, occ);

		Bitboard direct_b_pieces = direct_b_moves & (bishops | queens) & black;
		Bitboard direct_r_pieces = direct_r_moves & (rooks | queens) & black;

		direct_attackers |= direct_b_pieces | direct_r_pieces;
		return direct_attackers;
	}

	// bitmask of attackers is returned, even if tbOneIsEnough = false provided that tbOneIsEnough > 1 (see tbReturnBitmaskEvenIfOneIsEnough)
	template<bool tbInclKing = true, bool tbInclPinned = true, char tbOneIsEnough = true>
	ALWAYS_INLINE uint64_t IsSquareAttackedByWhite(const int sq) const
	{
		constexpr bool tbReturnBitmaskEvenIfOneIsEnough = tbOneIsEnough > 1;
		assert(IsValidPos(sq));

		if constexpr (tbInclPinned) // TODO: use IsSquareAttackedByWhite_hq also when pinned attackers must be excluded
		{
			constexpr bool tbFindAll = !tbOneIsEnough;
			const auto mask = IsSquareAttackedByWhite_hq<tbInclKing, tbFindAll>(sq);
			if constexpr (tbOneIsEnough == 1)
				return mask != 0;
			else
				return mask;
		}
		
		uint64_t mask;
		if constexpr (tbInclKing)
			mask = ((Black_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights) | (King_Attacks[sq] & kings)) & white;
		else
			mask = ((Black_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights)) & white;

		uint64_t res = 0;

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

		auto maskLongDist = ((Rook_Attacks[sq] & rooks) | (Bishop_Attacks[sq] & bishops) | (Queen_Attacks[sq] & queens)) & white;

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
	template<bool tbInclKing = true, bool tbInclPinned = true, char tbOneIsEnough = true>
	ALWAYS_INLINE uint64_t IsSquareAttackedByBlack(const int sq) const
	{
		constexpr bool tbReturnBitmaskEvenIfOneIsEnough = tbOneIsEnough > 1;
		assert(IsValidPos(sq));

		if constexpr (tbInclPinned) // TODO: use IsSquareAttackedByBlack_hq also when pinned attackers must be excluded
		{
			constexpr bool tbFindAll = !tbOneIsEnough;
			const auto mask = IsSquareAttackedByBlack_hq<tbInclKing, tbFindAll>(sq);			
			if constexpr (tbOneIsEnough == 1)
				return mask != 0;
			else
				return mask;						
		}
		
		uint64_t mask;
		if constexpr (tbInclKing)
			mask = ((White_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights) | (King_Attacks_Ext[sq] & kings)) & black;
		else
			mask = ((White_Pawn_Attacks[sq] & pawns) | (Knight_Attacks[sq] & knights)) & black;

		uint64_t res = 0;

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

		auto maskLongDist = ((Rook_Attacks[sq] & rooks) | (Bishop_Attacks[sq] & bishops) | (Queen_Attacks[sq] & queens)) & black;
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

	ALWAYS_INLINE bool IsEmptyAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((white | black) & (sq_to_bb(sq))) == 0;
	}
	ALWAYS_INLINE bool IsBlackAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & black) != 0;
	}
	ALWAYS_INLINE bool IsWhiteAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white) != 0;
	}
	ALWAYS_INLINE bool IsWhitePawnAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & pawns) != 0;
	}
	ALWAYS_INLINE bool IsBlackPawnAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & black & pawns) != 0;
	}
	ALWAYS_INLINE bool IsKingAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & kings) != 0;
	}
	ALWAYS_INLINE bool IsWhiteKingAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & kings) != 0;
	}
	ALWAYS_INLINE bool IsWhiteRookAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & rooks) != 0;
	}
	ALWAYS_INLINE bool IsWhiteBishopAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & bishops) != 0;
	}
	ALWAYS_INLINE bool IsWhiteKnightAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & knights) != 0;
	}
	ALWAYS_INLINE bool IsBlackRookAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & black & rooks) != 0;
	}
	ALWAYS_INLINE bool IsWhiteQueenAt(const int sq) const
	{
		assert(IsValidPos(sq));
		return ((sq_to_bb(sq)) & white & queens) != 0;
	}
	ALWAYS_INLINE bool IsBlackAbsolutelyPinned(const int pos) const
	{
		assert(IsValidPos(pos));
		assert(black & kings);
		assert((sq_to_bb(pos)) & black);

		const int posBlackKing = GetBlackKingPos();
		if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, pos))
			if (WhiteLongDistanceFigureInDir(pos, posBlackKing))
				return true;

		return false;
	}
	ALWAYS_INLINE bool IsWhiteAbsolutelyPinned(const int pos) const
	{
		assert(IsValidPos(pos));
		assert(white & kings);
		assert((sq_to_bb(pos)) & white);

		const int posWhiteKing = GetWhiteKingPos();
		if (SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, pos))
			if (BlackLongDistanceFigureInDir(pos, posWhiteKing))
				return true;

		return false;
	}

	ALWAYS_INLINE int GetBlackKingPos() const
	{
		return std::countr_zero(black & kings);
	}
	ALWAYS_INLINE int GetWhiteKingPos() const
	{
		return std::countr_zero(white & kings);
	}
	// En passant is not verified here (use IsBlackPawnPinned in such case)
	ALWAYS_INLINE bool IsBlackPinned(const int pos, const int posTo) const
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posTo));
		assert(pos != posTo);
		assert(black & kings);
		assert((sq_to_bb(pos)) & black);

		int posLongDistanceAttacker;
		const auto posBlackKing = GetBlackKingPos();

		#ifdef __VERIFY_PINNING_PREREQUISITE__
		const auto whiteLongDistancePieces = white & (queens | rooks | bishops);
		if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, pos) & ((whiteLongDistancePieces & Queen_Attacks[posBlackKing]) != 0) & ((whiteLongDistancePieces & Queen_Attacks[pos]) != 0)) // cheap verification of prerequisites
		#else
		if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, pos))
		#endif
			if ((posLongDistanceAttacker = WhiteLongDistanceFigureInDir<1>(pos, posBlackKing)) >= 0)
				if (!IsSquareBetween<1, 0>(posTo, posLongDistanceAttacker, posBlackKing)) // incl.ends (e.g. capture of the pinning piece is legal)
					return true;

		return false;
	}
	// En passant is not verified here (use IsWhitePawnPinned in such case or better IsWhitePinnedIfTakeOffBlackPawn)
	template<bool tbSkipAssertionForEnPassant = false>
	ALWAYS_INLINE bool IsWhitePinned(const int pos, const int posTo) const
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posTo));
		assert(pos != posTo);
		assert(white & kings);
		assert((sq_to_bb(pos)) & white);
		assert(tbSkipAssertionForEnPassant || ((sq_to_bb(pos)) & white & pawns) == 0 || (pos & 7) == (posTo & 7) || IsBlackAt(posTo)); // do not call this method for en passant! (see IsWhitePawnPinned)

		int posLongDistanceAttacker;
		const int posWhiteKing = GetWhiteKingPos();

		#ifdef __VERIFY_PINNING_PREREQUISITE__
		const auto blackLongDistancePieces = black & (queens | rooks | bishops);
		if (SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, pos) & ((blackLongDistancePieces & Queen_Attacks[posWhiteKing]) != 0) & ((blackLongDistancePieces & Queen_Attacks[pos]) != 0)) // cheap verification of prerequisites
		#else
		if (SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, pos))
		#endif
			if ((posLongDistanceAttacker = BlackLongDistanceFigureInDir<1>(pos, posWhiteKing)) >= 0)
				if (!IsSquareBetween<1, 0>(posTo, posLongDistanceAttacker, posWhiteKing)) // incl.ends (e.g. capture of the pinning piece is legal)
					return true;

		return false;
	}
	// Mainly for en passant:
	bool IsWhitePawnPinned(const int pos, const int posTo) const
	{
		assert(IsValidPos(pos));
		assert(IsValidPos(posTo));
		assert(pos != posTo);
		assert((sq_to_bb(pos)) & white & pawns);

		const bool bEnPassant = (pos & 7) != (posTo & 7) && IsEmptyAt(posTo);
		if (bEnPassant)		
			return IsWhitePinnedIfTakeOffBlackPawn(pos, posTo, posTo - 8);
		else
			return IsWhitePinned(pos, posTo);
	}

	ALWAYS_INLINE bool IsBlackPinnedIfTakeOffWhitePawn(const int pos, const int posTo, const int posWhitePawnToTakeOff) const
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
	ALWAYS_INLINE bool IsWhitePinnedIfTakeOffBlackPawn(const int pos, const int posTo, const int posBlackPawnToTakeOff) const
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

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackLongDistFigure(const int from, const int to) const
	{
		const auto f = GetLongDistanceFigureAt(from);
		switch (f)
		{
		case FGR_BISHOP:
			return IsImmediateMateAfterMoveByBlackBishop<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(from, to);
		case FGR_ROOK:
			return IsImmediateMateAfterMoveByBlackRook<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(from, to);
		case FGR_QUEEN:
			return IsImmediateMateAfterMoveByBlackQueen<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(from, to);
		}
		
		assert(false);
		return false;
	}

	ALWAYS_INLINE bool IsDirectCheckByBlackQueen(const int toPos) const
	{
		assert(IsValidPos(toPos));

		const auto posWhiteKing = GetWhiteKingPos();
		const auto res = SameDiagonalOrLineAndAllBetweenEmpty(toPos, posWhiteKing);
		return res;
	}
	ALWAYS_INLINE int IsCheckByBlackRook(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);

		const auto posWhiteKing = GetWhiteKingPos();
		const bool bDirectCheck = SameLineAndAllBetweenEmpty(toPos, posWhiteKing);
		const auto posDiscoveredChecker = (SameDiagAndAllBetweenEmpty(fromPos, posWhiteKing)) ? BlackLongDistanceFigureInDir<1>(fromPos, posWhiteKing) : -1;
		if (posDiscoveredChecker >= 0)
			return bDirectCheck ? DBL_CHECKED : posDiscoveredChecker;
		else
			return bDirectCheck ? toPos : -1;
	}
	ALWAYS_INLINE int IsCheckByBlackBishop(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);

		const auto posWhiteKing = GetWhiteKingPos();
		const bool bDirectCheck = SameDiagAndAllBetweenEmpty(toPos, posWhiteKing);
		const auto posDiscoveredChecker = (SameLineAndAllBetweenEmpty(fromPos, posWhiteKing)) ? BlackLongDistanceFigureInDir<1>(fromPos, posWhiteKing) : -1;
		if (posDiscoveredChecker >= 0)
			return bDirectCheck ? DBL_CHECKED : posDiscoveredChecker;
		else
			return bDirectCheck ? toPos : -1;
	}
	ALWAYS_INLINE int IsCheckByBlackKnight(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);

		const auto posWhiteKing = GetWhiteKingPos();
		const bool bDirectCheck = IsKnightDiff(posWhiteKing, toPos);
		const auto posDiscoveredChecker = (SameDiagonalOrLineAndAllBetweenEmpty(fromPos, posWhiteKing)) ? BlackLongDistanceFigureInDir<1>(fromPos, posWhiteKing) : -1;
		if (posDiscoveredChecker >= 0)
			return bDirectCheck ? DBL_CHECKED : posDiscoveredChecker;
		else
			return bDirectCheck ? toPos : -1;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackQueen(const int fromPos, const int toPos) const
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
			const bool bCapture = (white & toMask) != 0;
			const auto captureMask = bCapture ? toMask : 0;
	
			const auto bbSaved = *this; // save
	
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_EMPTY>(captureMask);
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->queens ^= moveMask;
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
			const_cast<FullBitboards*>(this)->queens ^= moveMask;

			// Verify if white king checked and dispatch to proper template version:
			const bool bDirectCheck = IsDirectCheckByBlackQueen(toPos);
			if (bDirectCheck)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(toPos);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->queens ^= moveMask;			
		}
		
		return res;
	}
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackRook(const int fromPos, const int toPos) const
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
			const bool bCapture = (white & toMask) != 0;
			const auto captureMask = bCapture ? toMask : 0;
	
			const auto bbSaved = *this; // save
	
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_EMPTY>(captureMask);
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->rooks ^= moveMask;
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
			const_cast<FullBitboards*>(this)->rooks ^= moveMask;

			// Find checker(s) and dispatch to proper template version:
			const auto posWhiteKingChecker = IsCheckByBlackRook(fromPos, toPos);
			if (posWhiteKingChecker >= 0)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->rooks ^= moveMask;			
		}
		
		return res;
	}
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackBishop(const int fromPos, const int toPos) const
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
			const bool bCapture = (white & toMask) != 0;
			const auto captureMask = bCapture ? toMask : 0;
	
			const auto bbSaved = *this; // save
	
			const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_EMPTY>(captureMask);
			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->bishops ^= moveMask;
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
			const_cast<FullBitboards*>(this)->bishops ^= moveMask;

			// First find checker(s) and dispatch to proper template version:
			const auto posWhiteKingChecker = IsCheckByBlackBishop(fromPos, toPos);			
			if (posWhiteKingChecker >= 0)
				res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
			else
				res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

			const_cast<FullBitboards*>(this)->black ^= moveMask;
			const_cast<FullBitboards*>(this)->bishops ^= moveMask;
		}
		
		return res;
	}
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbKnownThatItIsNotACapture = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackKnight(const int fromPos, const int toPos) const
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
			const bool bCapture = (white & toMask) != 0;
			const auto captureMask = bCapture ? toMask : 0;
	
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
	ALWAYS_INLINE bool IsImmediateMateAfterCaptureByBlackPawn(const int fromPos, const int toPos) const
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
	ALWAYS_INLINE bool IsImmediateMateAfterCaptureWithPromo(const int fromPos, const int toPos) const
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
		const_cast<FullBitboards*>(this)->queens ^= toMask;
		// TODO: maybe find checker(s) and dispatch to proper template version?
		auto res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
		if (res)
		{
			// 2) Try promo to knight (no need to check bishop and rook for immediate checkmate)
			const_cast<FullBitboards*>(this)->queens ^= toMask;
			const_cast<FullBitboards*>(this)->knights ^= toMask;
			// TODO: maybe find checker(s) and dispatch to proper template version?
			res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
		}

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterPromoMoveForwardByBlackPawn(const int fromPos, const int toPos) const
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
		const_cast<FullBitboards*>(this)->queens |= toMask;
		// TODO: maybe find checker(s) and dispatch to proper template version?
		auto res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(); // TODO: maybe find checker(s) and dispatch to proper template version?
		const_cast<FullBitboards*>(this)->queens ^= toMask;
		if (res)
		{
			// 2) Try promo to knight (no need to check bishop and rook for immediate checkmate)

			const_cast<FullBitboards*>(this)->knights ^= toMask;
			// TODO: maybe find checker(s) and dispatch to proper template version?
			res = FindMoveThatMates<-1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();
			const_cast<FullBitboards*>(this)->knights ^= toMask;
		}

		const_cast<FullBitboards*>(this)->black ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= moveMask;

		return res;
	}

	ALWAYS_INLINE int GetWhiteKingCheckerAfterBlackPawnMoveForward(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert((fromPos & 7) == (toPos & 7));
		assert((black & pawns & (sq_to_bb(fromPos))) == 0); // after the move
		assert((black & pawns & (sq_to_bb(toPos))) != 0); // after the move

		const int posWhiteKing = GetWhiteKingPos();
		const bool bDirectCheck = (white & kings & Black_Pawn_Attacks[toPos]) != 0;
		const auto posDiscoveredChecker = (SameDiagonalOrLineAndAllBetweenEmpty(fromPos, posWhiteKing)) ? BlackLongDistanceFigureInDir<1>(fromPos, posWhiteKing) : -1; // verification is AFTER making move on bitboard, so no need to bother with a move along the line			
		const int posWhiteKingChecker = bDirectCheck ? toPos : posDiscoveredChecker; // no chance for double check in case of a pawn move forward
		return posWhiteKingChecker;
	}

	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible, bool tbVerifyIfPromo = false>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveForwardByBlackPawn(const int fromPos, const int toPos) const
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
	ALWAYS_INLINE bool IsImmediateMateAfterLongMoveByBlackPawn(const int fromPos, const int toPos) const
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
	ALWAYS_INLINE bool IsImmediateMateAfterBlackEnPassant(const int fromPos, const int toPos) const
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
	ALWAYS_INLINE bool IsImmediateMateAfterBlackCastlingShort() const
	{
		constexpr auto fromMask = (1ULL << _E8_);
		constexpr auto toMask = (1ULL << _G8_);
		constexpr auto kingMoveMask = fromMask | toMask;

		constexpr auto fromMaskRook = (1ULL << _H8_);
		constexpr auto toMaskRook = (1ULL << _F8_);
		constexpr auto rookMoveMask = fromMaskRook | toMaskRook;

		const_cast<FullBitboards*>(this)->black ^= (kingMoveMask | rookMoveMask);
		const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
		const_cast<FullBitboards*>(this)->rooks ^= rookMoveMask;

		// Verify if wh.king checked and dispatch template version:
		const auto posWhiteKing = GetWhiteKingPos();
		const bool bCheck = SameLineAndAllBetweenEmpty(posWhiteKing, _F8_);
		bool res;
		if (bCheck)
			res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(_F8_);
		else
			res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

		const_cast<FullBitboards*>(this)->black ^= (kingMoveMask | rookMoveMask);
		const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
		const_cast<FullBitboards*>(this)->rooks ^= rookMoveMask;

		return res;
	}
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterBlackCastlingLong() const
	{
		constexpr auto fromMask = (1ULL << _E8_);
		constexpr auto toMask = (1ULL << _C8_);
		constexpr auto kingMoveMask = fromMask | toMask;

		constexpr auto fromMaskRook = (1ULL << _A8_);
		constexpr auto toMaskRook = (1ULL << _D8_);
		constexpr auto rookMoveMask = fromMaskRook | toMaskRook;

		const_cast<FullBitboards*>(this)->black ^= (kingMoveMask | rookMoveMask);
		const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
		const_cast<FullBitboards*>(this)->rooks ^= rookMoveMask;

		// Verify if wh.king checked and dispatch template version:
		const auto posWhiteKing = GetWhiteKingPos();
		const bool bCheck = SameLineAndAllBetweenEmpty(posWhiteKing, _D8_);
		bool res;
		if (bCheck)
			res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(_D8_);
		else
			res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

		const_cast<FullBitboards*>(this)->black ^= (kingMoveMask | rookMoveMask);
		const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
		const_cast<FullBitboards*>(this)->rooks ^= rookMoveMask;

		return res;
	}

	// Alias: FindMoveThatMatesAfterMoveByBlackKing
	template<bool tbWhiteShortCastlingPossible, bool tbWhiteLongCastlingPossible>
	ALWAYS_INLINE bool IsImmediateMateAfterMoveByBlackKing(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(!IsBlackAt(toPos));

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (white & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->black ^= moveMask;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->white ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KING>(captureMask);

		// First find potential discovered checker and dispatch to proper template version:
		const int posWhiteKing = GetWhiteKingPos();
		const int posWhiteKingChecker = (SameDiagonalOrLineAndAllBetweenEmpty(fromPos, posWhiteKing)) ? BlackLongDistanceFigureInDir<1>(fromPos, posWhiteKing) : -1; // verification is AFTER making move on bitboard, so no need to bother with a move along the line
		bool res;
		if (posWhiteKingChecker >= 0) // no need to verify !IsSquareBetween(to, posDiscoveredChecker, posWhiteKing) since AllBetweenEmpty and BlackLongDistanceFigureInDir were called AFTER moving bl.king
			res = FindMoveThatMates<1, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posWhiteKingChecker);
		else
			res = FindMoveThatMates<0, 0, tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	// It's assummed that bl.king is not checked or 'sq' is the only checker (no double check)
	// If tbFindAll == true, then bitmask of pieces that can capture sq is returned
	// NOTE!!! If tbOnlyIfPreventsImmediateMate is on, together white castling flags must be passed in tbOnlyIfPreventsImmediateMateAndFlags
	template<bool tbEnPassantPossible = false, bool tbInclKing = true, bool tbFindAll = false, char tbOnlyIfPreventsImmediateMateAndFlags = false>
	uint64_t CanBlackCapture(const int sq) const
	{
		constexpr bool tbOneIsEnough = !tbFindAll;
		constexpr bool tbOnlyIfPreventsImmediateMate = (tbOnlyIfPreventsImmediateMateAndFlags & 1) != 0;
		constexpr bool tbWhiteShortCastlingPossible = (tbOnlyIfPreventsImmediateMateAndFlags & 2) != 0;
		constexpr bool tbWhiteLongCastlingPossible = (tbOnlyIfPreventsImmediateMateAndFlags & 4) != 0;

		assert(IsValidPos(sq));
		assert(white & (sq_to_bb(sq)));
		assert(IsBlackKingChecked() < 0 || IsBlackKingChecked() == sq);

		uint64_t res;
		if constexpr (!tbOneIsEnough)
			res = 0;

		auto mask = black & ((queens & Queen_Attacks[sq]) | (rooks & Rook_Attacks[sq]) | (bishops & Bishop_Attacks[sq]));
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (AllBetweenEmpty(pos, sq))
				if (!IsBlackPinned(pos, sq))
					if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackLongDistFigure<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, sq))
						if constexpr (tbOneIsEnough)
							return true;
						else
							res |= (sq_to_bb(pos));
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

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
			mask = black & kings & King_Attacks[sq];
			if (mask)
				if (!const_cast<FullBitboards*>(this)->IsSquareAttackedByWhiteIfTakeOffBlackKing(sq))
					if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackKing<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(GetBlackKingPos(), sq))
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
					[[maybe_unused]]const int posBlackKing = GetBlackKingPos();
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
	ALWAYS_INLINE uint64_t CanWhiteCaptureWithCheck(const int sq) const
	{
		return CanWhiteCapture<tbEnPassantPossible, tbInclKing, tbFindAll, true>(sq);
	}
	template<bool tbEnPassantPossible = false, bool tbInclKing = true, bool tbFindAll = false>
	ALWAYS_INLINE uint64_t CanWhiteCaptureWithCheckMate(const int sq) const
	{
		return CanWhiteCapture<tbEnPassantPossible, tbInclKing, tbFindAll, 2>(sq);
	}

	template<bool tbCheckMateOnly>
	ALWAYS_INLINE bool WillWhiteQueenMoveBeCheck(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteQueenAt(posFrom));

		const auto posBlackKing = GetBlackKingPos();
		if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, posTo))
			if (!tbCheckMateOnly)
				return true;
			else
				return IsCheckMateAfterQueenCheck(posFrom, posTo);

		return false;
	}

	template<bool tbCheckMateOnly>
	ALWAYS_INLINE bool WillWhiteRookMoveBeCheck(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteRookAt(posFrom));

		const auto posBlackKing = GetBlackKingPos();
		const auto posDiscoveredAttacker = (SameDiagAndAllBetweenEmpty(posBlackKing, posFrom)) ? WhiteLongDistanceFigureInDir<1>(posFrom, posBlackKing) : -1;

		if (posDiscoveredAttacker >= 0)
			if (!tbCheckMateOnly)
				return true;
			else
				return IsCheckMateAfterRookDiscoveredCheck(posFrom, posTo, posDiscoveredAttacker);

		const bool bDirectCheck = SameLineAndAllBetweenEmpty(posBlackKing, posTo);
		if (bDirectCheck)
			if (!tbCheckMateOnly)
				return true;
			else
				return IsCheckMateAfterRookDirectCheck(posFrom, posTo);

		return false;
	}

	template<bool tbCheckMateOnly>
	ALWAYS_INLINE bool WillWhiteBishopMoveBeCheck(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteBishopAt(posFrom));

		const auto posBlackKing = GetBlackKingPos();
		const auto posDiscoveredAttacker = (SameLineAndAllBetweenEmpty(posBlackKing, posFrom)) ? WhiteLongDistanceFigureInDir<1>(posFrom, posBlackKing) : -1;

		if (posDiscoveredAttacker >= 0)
			if (!tbCheckMateOnly)
				return true;
			else
				return IsCheckMateAfterBishopDiscoveredCheck(posFrom, posTo, posDiscoveredAttacker);

		const bool bDirectCheck = SameDiagAndAllBetweenEmpty(posBlackKing, posTo);
		if (bDirectCheck)
			if (!tbCheckMateOnly)
				return true;
			else
				return IsCheckMateAfterBishopDirectCheck(posFrom, posTo);

		return false;
	}

	template<bool tbCheckMateOnly = false>
	ALWAYS_INLINE bool WillLongDistanceWhiteFigureMoveBeCheck(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteAt(posFrom));
		
		const FIGURE f = GetLongDistanceFigureAt(posFrom);

		switch (f)
		{
			case FGR_BISHOP:
				return WillWhiteBishopMoveBeCheck<tbCheckMateOnly>(posFrom, posTo);
			
			case FGR_ROOK:			
				return WillWhiteRookMoveBeCheck<tbCheckMateOnly>(posFrom, posTo);			

			case FGR_QUEEN:			
				return WillWhiteQueenMoveBeCheck<tbCheckMateOnly>(posFrom, posTo);
			
		}

		assert(false);
		return false;
	}


	template<bool tbCheckMateOnly = false>
	ALWAYS_INLINE bool WillWhiteKnightMoveBeCheck(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteAt(posFrom));

		const auto posBlackKing = GetBlackKingPos();
		const bool bDirectCheck = IsKnightDiff(posBlackKing, posTo);
		if (bDirectCheck || (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, posFrom) && WhiteLongDistanceFigureInDir(posFrom, posBlackKing)))
		{
			if constexpr (tbCheckMateOnly)
			{
				if (IsCheckMateAfterKnightCheck(posFrom, posTo))
					return true;
			}
			else
				return true;
		}

		return false;
	}

	// En passant not handled by this method
	template<bool tbCheckMateOnly = false, FIGURE fPromo = 0> // when fPromo == 0 (FGR_EMPTY), and the move is a promo, both promotions to queen and knight are verified
	ALWAYS_INLINE bool WillMoveByWhitePawnBeCheck(const int posFrom, const int posTo) const
	{
		static_assert(fPromo == 0 || fPromo == FGR_QUEEN || fPromo == FGR_ROOK || fPromo == FGR_BISHOP || fPromo == FGR_KNIGHT, "");
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteAt(posFrom));
		assert((posFrom & 7) == (posTo & 7) || IsBlackAt(posTo)); // en passant not handled by this method

		const auto blackKingPos = GetBlackKingPos();
		if (posTo >= _A8_)
		{
			if constexpr (fPromo != 0) // analyze specific promo only or any one?
			{
				bool bDirectCheck;
				if constexpr (fPromo == FGR_KNIGHT)
					bDirectCheck = IsKnightDiff(blackKingPos, posTo);
				else if constexpr (fPromo == FGR_QUEEN)
					bDirectCheck = SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(blackKingPos, posTo, posFrom);
				else if constexpr (fPromo == FGR_ROOK)
					bDirectCheck = SameLineAndAllBetweenEmptyIfTakeOffWhitePawn(blackKingPos, posTo, posFrom);
				else if constexpr (fPromo == FGR_ROOK)
					bDirectCheck = SameDiagAndAllBetweenEmptyIfTakeOffWhitePawn(blackKingPos, posTo, posFrom);

				if (bDirectCheck)
				{
					if constexpr (!tbCheckMateOnly)
						return true;
					const bool bDiscoveredCheck = SameDiagonalOrLineAndAllBetweenEmpty(blackKingPos, posFrom) && WhiteLongDistanceFigureInDir(posFrom, blackKingPos);
					if constexpr (fPromo == FGR_KNIGHT)
						return IsCheckMateAfterPromoToKnightDirectCheck(posFrom, posTo, bDiscoveredCheck);
					else if constexpr (fPromo == FGR_QUEEN)
						return IsCheckMateAfterPromoToQueenDirectCheck(posFrom, posTo, bDiscoveredCheck);
					else if constexpr (fPromo == FGR_ROOK)
						return IsCheckMateAfterPromoToRookDirectCheck(posFrom, posTo, bDiscoveredCheck);
					else if constexpr (fPromo == FGR_BISHOP)
						return IsCheckMateAfterPromoToBishopDirectCheck(posFrom, posTo, bDiscoveredCheck);
				}
				else
				{
					const auto posDiscoveredChecker = (SameDiagonalOrLineAndAllBetweenEmpty(blackKingPos, posFrom)) ? WhiteLongDistanceFigureInDir<1>(posFrom, blackKingPos) : -1;
					if (posDiscoveredChecker >= 0)
					{
						if constexpr (!tbCheckMateOnly)
							return true;
						if constexpr (fPromo == FGR_KNIGHT)
							return IsCheckMateAfterPromoToKnightDiscoveredCheck(posFrom, posTo, posDiscoveredChecker);
						else if constexpr (fPromo == FGR_QUEEN)
							return IsCheckMateAfterPromoToQueenDiscoveredCheck(posFrom, posTo, posDiscoveredChecker);
						else if constexpr (fPromo == FGR_ROOK)
							return IsCheckMateAfterPromoToRookDiscoveredCheck(posFrom, posTo, posDiscoveredChecker);
						else if constexpr (fPromo == FGR_BISHOP)
							return IsCheckMateAfterPromoToBishopDiscoveredCheck(posFrom, posTo, posDiscoveredChecker);
					}
				}
			}
			else
			{
				// Any promo should be considered, so we can limit to knight and queen:
				const bool bPromoToKnightDirectCheck = IsKnightDiff(blackKingPos, posTo);
				const bool bPromoToQueenDirectCheck = SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(blackKingPos, posTo, posFrom);

				if constexpr (!tbCheckMateOnly)
					if (bPromoToKnightDirectCheck | bPromoToQueenDirectCheck)
						return true;

				const auto posDiscoveredChecker = (SameDiagonalOrLineAndAllBetweenEmpty(blackKingPos, posFrom)) ? WhiteLongDistanceFigureInDir<1>(posFrom, blackKingPos) : -1;
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
							if (IsCheckMateAfterPromoToKnightDirectCheck(posFrom, posTo, bDiscoveredCheck))
								return true;
						}
						else
							if (bPromoToQueenDirectCheck)
								if (IsCheckMateAfterPromoToQueenDirectCheck(posFrom, posTo, bDiscoveredCheck))
									return true;
					}

					if (posDiscoveredChecker >= 0)
					{
						if (IsCheckMateAfterPromoToQueenDiscoveredCheck(posFrom, posTo, posDiscoveredChecker))
							return true;
						else
							return IsCheckMateAfterPromoToKnightDiscoveredCheck(posFrom, posTo, posDiscoveredChecker);
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
					if (blackKingPos == posFrom + 16 && IsEmptyAt(posFrom + 8) && WhiteLongDistanceFigureInDir(posFrom, 0, -1))
						return IsCheckMateAfterPawnDirectCheck(posFrom, posTo, true); // double check
					else
						if (posTo == posFrom + 16)
							return IsCheckMateAfterPawnDirectCheck<1>(posFrom, posTo);
						else
							return IsCheckMateAfterPawnDirectCheck(posFrom, posTo);

			if (SameDiagonalOrLineAndAllBetweenEmpty(posFrom, blackKingPos))
			{
				const auto posDiscoveredChecker = WhiteLongDistanceFigureInDir<1>(posFrom, blackKingPos);
				if (posDiscoveredChecker >= 0)
				{
					const bool bMoveForward = (posTo & 7) == (posFrom & 7);
					if (!bMoveForward || (posDiscoveredChecker & 7) != (posFrom & 7)) // not a move forward while long distance attacker on the same file?
						if constexpr (!tbCheckMateOnly)
							return true;
						else
							return IsCheckMateAfterPawnDiscoveredCheck(posFrom, posTo, posDiscoveredChecker);
				}
			}
		}

		return false;
	}

	ALWAYS_INLINE bool CanWhiteKingMoveBeCheck(const auto posWhiteKing) const
	{
		const auto posBlackKing = GetBlackKingPos();

		const auto res = SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, posBlackKing) && WhiteLongDistanceFigureInDir(posWhiteKing, posBlackKing);

		return res;
	}

	template<bool tbCheckMateOnly = false>
	ALWAYS_INLINE bool WillWhiteKingMoveBeCheck(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posFrom != posTo);
		assert(IsWhiteAt(posFrom));
		assert(Distance(posFrom, posTo) == 1);

		const auto posBlackKing = GetBlackKingPos();
		if (SameDiagonalOrLineAndAllBetweenEmpty(posFrom, posBlackKing))
		{
			const auto posDiscoveredChecker = WhiteLongDistanceFigureInDir<1>(posFrom, posBlackKing);
			if (posDiscoveredChecker >= 0)
				if (!IsSquareBetween<1>(posTo, posDiscoveredChecker, posBlackKing)) // not a move along the diagonal/line ?
					if constexpr (!tbCheckMateOnly)
						return true;
					else
						return IsCheckMateAfterKingDiscoveredCheck(posFrom, posTo, posDiscoveredChecker);
		}

		return false;
	}

	
	template<bool tbCheckMateOnly = false>
	bool WillWhiteEnPassantBeCheck(const int posFrom, const int posTo) const
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
				const auto posBlackKing = GetBlackKingPos();
				const bool bDiscoveredCheck = posBlackKing == posFrom + 16 && IsEmptyAt(posFrom + 8) && WhiteLongDistanceFigureInDir(posFrom, 0, -1);
				return IsCheckMateAfterEnPassantDirectCheck(posFrom, posTo, bDiscoveredCheck);
			}

		const auto posBlackKing = GetBlackKingPos();
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
	template<bool tbEnPassantPossible = false, bool tbInclKing = true, bool tbFindAll = false, char tbOnlyCheckingMoves = false>
	uint64_t CanWhiteCapture(const int sq) const
	{
		constexpr bool tbOneIsEnough = !tbFindAll;
		constexpr bool tbOnlyMatingMoves = tbOnlyCheckingMoves > 1;

		assert(IsValidPos(sq));
		assert(black & (sq_to_bb(sq)));
		assert(IsWhiteKingChecked() < 0 || IsWhiteKingChecked() == sq);

		uint64_t res;
		if constexpr (!tbOneIsEnough)
			res = 0;

		auto mask = white & ((queens & Queen_Attacks[sq]) | (rooks & Rook_Attacks[sq]) | (bishops & Bishop_Attacks[sq]));
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (AllBetweenEmpty(pos, sq))
				if (!IsWhitePinned(pos, sq))
					if (!tbOnlyCheckingMoves || WillLongDistanceWhiteFigureMoveBeCheck<tbOnlyMatingMoves>(pos, sq))
						if constexpr (tbOneIsEnough)
							return true;
						else
							res |= (sq_to_bb(pos));
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		mask = white & knights & Knight_Attacks[sq];
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (!IsWhiteAbsolutelyPinned(pos))
				if (!tbOnlyCheckingMoves || WillWhiteKnightMoveBeCheck<tbOnlyMatingMoves>(pos, sq))
					if constexpr (tbOneIsEnough)
						return true;
					else
						res |= (sq_to_bb(pos));
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		mask = white & pawns & Black_Pawn_Attacks[sq];
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
			mask = white & kings & King_Attacks[sq];
			if (mask)
				if (!const_cast<FullBitboards*>(this)->IsSquareAttackedByBlackIfTakeOffWhiteKing(sq))
					if (!tbOnlyCheckingMoves || WillWhiteKingMoveBeCheck<tbOnlyMatingMoves>(GetWhiteKingPos(), sq))
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
					[[maybe_unused]]const int posWhiteKing = GetWhiteKingPos();
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

	// !!! Method does not take into account en passant nor castling (en passant can never prevent a discovered check by a long distance black attacker)
	// Method assumes that either bl.king is not checked, or is checked so that moving in between can prevent it
	// NOTE!!! If tbOnlyIfPreventsImmediateMate is on, together white castling flags must be passed in tbOnlyIfPreventsImmediateMateAndFlags
	template<bool tbFindAllAndFillBuf = false, char tbOnlyIfPreventsImmediateMateAndFlags = false> // if tbFindAllAndFillBuf == false, then aMoves will not be filled in
	int CanBlackMoveInBetween(const int sq1, const int sq2, TMove* aMoves = nullptr) const
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

		auto tmpMaskBetween = maskBetween;
		uint64_t bishopBitboard{}, rookBitboard{}, queenBitboard{}, knightBitboard{};
		BEGIN_FOR_EACH_POS_IN_MASK(pos, tmpMaskBetween)
		{
			knightBitboard |= Knight_Attacks[pos];
			bishopBitboard |= Bishop_Attacks[pos];
			rookBitboard |= Rook_Attacks[pos];
			queenBitboard |= Queen_Attacks[pos];
		}
		END_FOR_EACH_POS_IN_MASK(pos, tmpMaskBetween);

		// Queens:
		auto mask = black & queens & queenBitboard;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			auto matchMask = Queen_Attacks[pos] & maskBetween;
			BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
			{
				if (AllBetweenEmpty(pos, posBetween))
					if (!tbVerifyPinning || !IsBlackPinned(pos, posBetween))
						if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackQueen<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, posBetween))
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, posBetween);
						}
			}
			END_FOR_EACH_POS_IN_MASK(posBetween, matchMask);
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		// Rooks
		mask = black & rooks & rookBitboard;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			auto matchMask = Rook_Attacks[pos] & maskBetween;
			BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
			{
				if (AllBetweenEmpty(pos, posBetween))
					if (!tbVerifyPinning || !IsBlackPinned(pos, posBetween))
						if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackRook<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, posBetween))
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, posBetween);
						}
			}
			END_FOR_EACH_POS_IN_MASK(posBetween, matchMask);
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		// Bishops:
		mask = black & bishops & bishopBitboard;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			auto matchMask = Bishop_Attacks[pos] & maskBetween;
			BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
			{
				if (AllBetweenEmpty(pos, posBetween))
					if (!tbVerifyPinning || !IsBlackPinned(pos, posBetween))
						if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackBishop<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, posBetween))
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, posBetween);
						}
			}
			END_FOR_EACH_POS_IN_MASK(posBetween, matchMask);
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		// Knights:
		mask = black & knights & knightBitboard;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			auto matchMask = Knight_Attacks[pos] & maskBetween;
			if (matchMask)
				if (!tbVerifyPinning || !IsBlackAbsolutelyPinned(pos))
					if constexpr (tbOneIsEnough && !tbOnlyIfPreventsImmediateMate)
						return 1;
					else
					{
						// matchMask already known to be non-zero:
						BEGIN_DOWHILE_POS_IN_MASK(posBetween, matchMask)
						{
							if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackKnight<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(pos, posBetween))
								if constexpr (tbOneIsEnough)
									return 1;
								else
									aMoves[count++].set(pos, posBetween);
						}
						END_DOWHILE_POS_IN_MASK(posBetween, matchMask);
					}
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		// King:
		if constexpr (tbInclKing)
		{
			const int posBlackKing = GetBlackKingPos();
			auto matchMask = King_Attacks[posBlackKing] & maskBetween;
			BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
			{
				if (!tbVerifyPinning || !const_cast<FullBitboards*>(this)->IsSquareAttackedByWhiteIfTakeOffBlackKing(posBetween))
					if (!tbOnlyIfPreventsImmediateMate || !IsImmediateMateAfterMoveByBlackKing<tbWhiteShortCastlingPossible, tbWhiteLongCastlingPossible>(posBlackKing, posBetween))
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
		mask = (black & pawns) >> 8;
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
	ALWAYS_INLINE int CanWhiteMoveInBetweenWithCheck(const int sq1, const int sq2, TMove* aMoves = nullptr) const
	{
		return CanWhiteMoveInBetween<tbFindAllAndFillBuf, 1>(sq1, sq2, aMoves);
	}

	template<bool tbFindAllAndFillBuf = false>
	ALWAYS_INLINE int CanWhiteMoveInBetweenWithCheckMate(const int sq1, const int sq2, TMove* aMoves = nullptr) const
	{
		return CanWhiteMoveInBetween<tbFindAllAndFillBuf, 2>(sq1, sq2, aMoves);
	}

	// !!! Method does not take into account en passant nor castling (en passant can never prevent a discovered check by a long distance attacker)
	// Method assumes that either wh.king is not checked, or is checked so that moving in between can prevent it
	template<bool tbFindAllAndFillBuf = false, char tbOnlyCheckingMoves = false> //  !!! if tbFindAllAndFillBuf == false, aMoves will NOE be filled in
	int CanWhiteMoveInBetween(const int sq1, const int sq2, TMove* aMoves = nullptr) const
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

		auto tmpMaskBetween = maskBetween;
		uint64_t bishopBitboard{}, rookBitboard{}, queenBitboard{}, knightBitboard{};
		BEGIN_FOR_EACH_POS_IN_MASK(pos, tmpMaskBetween)
		{
			knightBitboard |= Knight_Attacks[pos];
			bishopBitboard |= Bishop_Attacks[pos];
			rookBitboard |= Rook_Attacks[pos];
			queenBitboard |= Queen_Attacks[pos];
		}
		END_FOR_EACH_POS_IN_MASK(pos, tmpMaskBetween);

		// Queens:
		auto mask = white & queens & queenBitboard;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			auto matchMask = Queen_Attacks[pos] & maskBetween;
			BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
			{
				if (AllBetweenEmpty(pos, posBetween))
					if (!tbVerifyPinning || !IsWhitePinned(pos, posBetween))
						if (!tbOnlyCheckingMoves || WillWhiteQueenMoveBeCheck<tbOnlyMatingMoves>(pos, posBetween))
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, posBetween);
						}
			}
			END_FOR_EACH_POS_IN_MASK(posBetween, matchMask);
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		// Rooks
		mask = white & rooks & rookBitboard;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			auto matchMask = Rook_Attacks[pos] & maskBetween;
			BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
			{
				if (AllBetweenEmpty(pos, posBetween))
					if (!tbVerifyPinning || !IsWhitePinned(pos, posBetween))
						if (!tbOnlyCheckingMoves || WillWhiteRookMoveBeCheck<tbOnlyMatingMoves>(pos, posBetween)) 
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, posBetween);
						}
			}
			END_FOR_EACH_POS_IN_MASK(posBetween, matchMask);
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		// Bishops:
		mask = white & bishops & bishopBitboard;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			auto matchMask = Bishop_Attacks[pos] & maskBetween;
			BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
			{
				if (AllBetweenEmpty(pos, posBetween))
					if (!tbVerifyPinning || !IsWhitePinned(pos, posBetween))
						if (!tbOnlyCheckingMoves || WillWhiteBishopMoveBeCheck<tbOnlyMatingMoves>(pos, posBetween)) 
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, posBetween);
						}
			}
			END_FOR_EACH_POS_IN_MASK(posBetween, matchMask);
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		// Knights:
		mask = white & knights & knightBitboard;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			auto matchMask = Knight_Attacks[pos] & maskBetween;
			if (matchMask)
				if (!tbVerifyPinning || !IsWhiteAbsolutelyPinned(pos))
					if constexpr (tbOneIsEnough && !tbOnlyCheckingMoves)
						return 1;
					else
					{
						// matchMask already known to be non-zero:
						BEGIN_DOWHILE_POS_IN_MASK(posBetween, matchMask)
						{
							if (!tbOnlyCheckingMoves || WillWhiteKnightMoveBeCheck<tbOnlyMatingMoves>(pos, posBetween))
							{
								if (tbOneIsEnough)
									return 1;
								else
									aMoves[count++].set(pos, posBetween);
							}
						}
						END_DOWHILE_POS_IN_MASK(posBetween, matchMask);
					}
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		// King:
		if constexpr (tbInclKing)
		{
			const int posWhiteKing = GetWhiteKingPos();
			if (!tbOnlyCheckingMoves || CanWhiteKingMoveBeCheck(posWhiteKing))
			{
				auto matchMask = King_Attacks[posWhiteKing] & maskBetween;
				BEGIN_FOR_EACH_POS_IN_MASK(posBetween, matchMask)
				{
					if (!tbVerifyPinning || !const_cast<FullBitboards*>(this)->IsSquareAttackedByBlackIfTakeOffWhiteKing(posBetween))
						if (!tbOnlyCheckingMoves || WillWhiteKingMoveBeCheck<tbOnlyMatingMoves>(posWhiteKing, posBetween))
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
		mask = (white & pawns) << 8;
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
						if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, FGR_QUEEN>(pos, pos + 8))
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, pos + 8, FGR_QUEEN);
						}
						if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, FGR_KNIGHT>(pos, pos + 8))
						{
							if constexpr (tbOneIsEnough)
								return 1;
							else
								aMoves[count++].set(pos, pos + 8, FGR_KNIGHT);
						}
						if constexpr (!tbOneIsEnough) // if tbOneIsEnough, then it is enough to analyze promo to queen and promo to knight only
						{
							if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, FGR_BISHOP>(pos, pos + 8))
								aMoves[count++].set(pos, pos + 8, FGR_BISHOP);
							if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves, FGR_ROOK>(pos, pos + 8))
								aMoves[count++].set(pos, pos + 8, FGR_ROOK);
						}
					}
				}
				else
				{
					if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves>(pos, pos + 8))
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
					if (!tbOnlyCheckingMoves || WillMoveByWhitePawnBeCheck<tbOnlyMatingMoves>(pos, pos + 16))
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


	ALWAYS_INLINE bool FindOneValidMove4BlackKingWhenChecked(const int posChecker) const
	{
		const auto blackKing = black & kings;
		const_cast<FullBitboards*>(this)->black ^= blackKing;
		#ifdef __JGI_BB_PEDANTIC__
		const_cast<FullBitboards*>(this)->kings ^= blackKing;
		#endif

		auto mask = King_Attacks[std::countr_zero(blackKing)] & ~black;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (!IsSquareAttackedByWhite(pos))
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
	// Method assumes that either black king is not checked, or moving on this square will block check
	template<bool tbInclKing = false>
	ALWAYS_INLINE bool CanBlackMoveOn(const int sq) const
	{
		static_assert(!tbInclKing, "TODO");
		assert(!IsBlackAt(sq));

		auto mask = Knight_Attacks[sq] & black & knights;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (!IsBlackAbsolutelyPinned(pos))
				return true;
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		mask = ((Queen_Attacks[sq] & queens) | (Rook_Attacks[sq] & rooks) | (Bishop_Attacks[sq] & bishops)) & black;
		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (AllBetweenEmpty(pos, sq))
				if (!IsBlackPinned(pos, sq))
					return true;
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		if (IsWhiteAt(sq))
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
	template<bool tbEnPassantPossible = false>
	bool FindOneValidMove4BlackWhenChecked(const int posChecker) const
	{
		assert(IsBlackKingChecked() >= 0);

		if (FindOneValidMove4BlackKingWhenChecked(posChecker))
			return true;

		if (posChecker != DBL_CHECKED)
		{
			if (CanBlackCapture<tbEnPassantPossible, 0>(posChecker)) // excl king (bl. king's neighborhood already verified in a call to FindOneValidMove4BlackKingWhenChecked)
				return true;
			const int posBlackKing = GetBlackKingPos();
			const auto dist = Distance(posBlackKing, posChecker);
			switch (dist)
			{
			case 1:
				break;
			case 2:
				if (!IsKnightDiff(posBlackKing, posChecker))
					if (CanBlackMoveOn((posBlackKing + posChecker) / 2))
						return true;
				break;
			default:
				if (CanBlackMoveInBetween(posBlackKing, posChecker))
					return true;
				break;
			}
		}

		return false;
	}
	ALWAYS_INLINE bool IsCheckMateAfterQueenCheck(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & queens & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->queens |= toMask; // TODO: with __JGI_BB_PEDANTIC__ maybe take off from fromMask? (anyway it does not matter before FindOneValidMove4BlackWhenChecked since no white or black at this pos)
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_QUEEN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterRookDirectCheck(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & rooks & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->rooks |= toMask; // TODO: with __JGI_BB_PEDANTIC__ maybe take off from fromMask? (anyway it does not matter before FindOneValidMove4BlackWhenChecked since no white or black at this pos)
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterBishopDirectCheck(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & bishops & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->bishops |= toMask; // TODO: with __JGI_BB_PEDANTIC__ maybe take off from fromMask? (anyway it does not matter before FindOneValidMove4BlackWhenChecked since no white or black at this pos)
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	template<FIGURE fToSkip, FIGURE fAnotherTypeToSkip = 0>
	ALWAYS_INLINE void ClearOnPieceBitboardsExcept(const uint64_t maskBitsToClear)
	{
		// kings are not affected by this method
		if constexpr (fToSkip != FGR_PAWN && fAnotherTypeToSkip != FGR_PAWN)
			pawns &= ~maskBitsToClear;
		if constexpr (fToSkip != FGR_BISHOP && fAnotherTypeToSkip != FGR_BISHOP)
			bishops &= ~maskBitsToClear;
		if constexpr (fToSkip != FGR_ROOK && fAnotherTypeToSkip != FGR_ROOK)
			rooks &= ~maskBitsToClear;
		if constexpr (fToSkip != FGR_QUEEN && fAnotherTypeToSkip != FGR_QUEEN)
			queens &= ~maskBitsToClear;
		if constexpr (fToSkip != FGR_KNIGHT && fAnotherTypeToSkip != FGR_KNIGHT)
			knights &= ~maskBitsToClear;
		return;
	}
	ALWAYS_INLINE bool IsCheckMateAfterKnightDirectCheck(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(toPos != fromPos);
		assert(white & knights & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);

		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->knights |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KNIGHT>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	template<bool tbBlackEnPassantPossible = false>
	ALWAYS_INLINE bool IsCheckMateAfterPawnDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->pawns |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked<tbBlackEnPassantPossible>(bDoubleCheck ? DBL_CHECKED : toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterEnPassantDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) const
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

		const auto res = !FindOneValidMove4BlackWhenChecked(bDoubleCheck ? DBL_CHECKED : toPos);

		// Restore
		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= (moveMask | captureMask);
		const_cast<FullBitboards*>(this)->black ^= captureMask;

		return res;
	}
	// posWhiteLongDistAttackerInEnPassant can be DBL_CHECKED, e.g. 8/6N1/3k1P2/1K1Pp3/7p/2p3B1/3R4/8 d5:e6++
	ALWAYS_INLINE bool IsCheckMateAfterEnPassantDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttackerInEnPassant) const
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

		const auto res = !FindOneValidMove4BlackWhenChecked(posWhiteLongDistAttackerInEnPassant);

		// Restore:
		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= (moveMask | captureMask);
		const_cast<FullBitboards*>(this)->black ^= captureMask;

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterPromoToKnightDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->knights |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KNIGHT, FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(bDoubleCheck ? DBL_CHECKED : toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterPromoToQueenDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->queens |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_QUEEN, FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(bDoubleCheck ? DBL_CHECKED : toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterPromoToRookDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->rooks |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK, FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(bDoubleCheck ? DBL_CHECKED : toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterPromoToBishopDirectCheck(const int fromPos, const int toPos, bool bDoubleCheck = false) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->bishops |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP, FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(bDoubleCheck ? DBL_CHECKED : toPos);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	// There are also IsCheckMateAfterRookDirectCheck and IsCheckMateAfterRookDiscoveredCheck. This version is a dispacher that should be used when it is unknown.
	ALWAYS_INLINE bool IsCheckMateAfterRookCheck(const int fromPos, const int toPos)
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(fromPos != toPos);

		const auto posBlackKing = GetBlackKingPos();
		if (int posWhiteLongDistFigure; SameDiagAndAllBetweenEmpty(fromPos, posBlackKing) && (posWhiteLongDistFigure = WhiteLongDistanceFigureInDir<1>(fromPos, posBlackKing)) >= 0)
			return IsCheckMateAfterRookDiscoveredCheck(fromPos, toPos, posWhiteLongDistFigure);
		else
		{
			assert(SameLine(toPos, posBlackKing) && AllBetweenEmpty(toPos, posBlackKing));
			return IsCheckMateAfterRookDirectCheck(fromPos, toPos);
		}
	}
	// There are also IsCheckMateAfterBishopDirectCheck and IsCheckMateAfterBishopDiscoveredCheck. This version is a dispacher that should be used when it is unknown.
	ALWAYS_INLINE bool IsCheckMateAfterBishopCheck(const int fromPos, const int toPos)
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(fromPos != toPos);

		const auto posBlackKing = GetBlackKingPos();
		if (int posWhiteLongDistFigure; SameLineAndAllBetweenEmpty(fromPos, posBlackKing) && (posWhiteLongDistFigure = WhiteLongDistanceFigureInDir<1>(fromPos, posBlackKing)) >= 0)
			return IsCheckMateAfterBishopDiscoveredCheck(fromPos, toPos, posWhiteLongDistFigure);
		else
		{
			assert(SameDiagAndAllBetweenEmpty(toPos, posBlackKing));
			return IsCheckMateAfterBishopDirectCheck(fromPos, toPos);
		}
	}
	// There are also IsCheckMateAfterKnightDirectCheck and IsCheckMateAfterKnightDiscoveredCheck. This version is a dispacher that should be used when it is unknown.
	ALWAYS_INLINE bool IsCheckMateAfterKnightCheck(const int fromPos, const int toPos) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(fromPos != toPos);

		const auto posBlackKing = GetBlackKingPos();
		if (int posWhiteLongDistFigure; SameDiagonalOrLineAndAllBetweenEmpty(fromPos, posBlackKing) && (posWhiteLongDistFigure = WhiteLongDistanceFigureInDir<1>(fromPos, posBlackKing)) >= 0)
			return IsCheckMateAfterKnightDiscoveredCheck(fromPos, toPos, posWhiteLongDistFigure);
		else
		{
			assert(IsKnightDiff(toPos, posBlackKing));
			return IsCheckMateAfterKnightDirectCheck(fromPos, toPos);
		}
	}

	ALWAYS_INLINE bool IsCheckMateAfterRookDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & rooks & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const int posBlackKing = GetBlackKingPos();
		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->rooks ^= fromMask;
		const_cast<FullBitboards*>(this)->rooks |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK>(captureMask);

		assert(AllBetweenEmpty(posWhiteLongDistAttacker, GetBlackKingPos()));
		const bool doubleCheck = SameLineAndAllBetweenEmpty(posBlackKing, toPos);
		const auto res = !FindOneValidMove4BlackWhenChecked(doubleCheck ? DBL_CHECKED : posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterBishopDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & bishops & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const int posBlackKing = GetBlackKingPos();
		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->bishops ^= fromMask;
		const_cast<FullBitboards*>(this)->bishops |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP>(captureMask);

		assert(AllBetweenEmpty(posWhiteLongDistAttacker, GetBlackKingPos()));
		const bool doubleCheck = SameDiagAndAllBetweenEmpty(posBlackKing, toPos);
		const auto res = !FindOneValidMove4BlackWhenChecked(doubleCheck ? DBL_CHECKED : posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterKnightDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) const
	{
		assert(IsValidPos(fromPos));
		assert(IsValidPos(toPos));
		assert(IsValidPos(posWhiteLongDistAttacker));
		assert(toPos != fromPos);
		assert(posWhiteLongDistAttacker != fromPos && posWhiteLongDistAttacker != toPos);
		assert(white & knights & (sq_to_bb(fromPos)));
		assert((white & (sq_to_bb(toPos))) == 0);
		assert((white & (1ULL << posWhiteLongDistAttacker)) != 0);

		const int posBlackKing = GetBlackKingPos();
		const auto fromMask = (sq_to_bb(fromPos));
		const auto toMask = (sq_to_bb(toPos));
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->knights ^= fromMask;
		const_cast<FullBitboards*>(this)->knights |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KNIGHT>(captureMask);

		assert(AllBetweenEmpty(posWhiteLongDistAttacker, GetBlackKingPos()));
		const bool doubleCheck = IsKnightDiff(posBlackKing, toPos);
		const auto res = !FindOneValidMove4BlackWhenChecked(doubleCheck ? DBL_CHECKED : posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterKingDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) const
	{
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KING>(captureMask);

		assert(AllBetweenEmpty(posWhiteLongDistAttacker, GetBlackKingPos()));
		const auto res = !FindOneValidMove4BlackWhenChecked(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterPromoToQueenDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->queens |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_QUEEN, FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterPromoToRookDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->rooks |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK, FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterPromoToBishopDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->bishops |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP, FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterPromoToKnightDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->knights |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KNIGHT, FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}
	ALWAYS_INLINE bool IsCheckMateAfterPawnDiscoveredCheck(const int fromPos, const int toPos, const int posWhiteLongDistAttacker) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;
		const_cast<FullBitboards*>(this)->pawns |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_PAWN>(captureMask);

		const auto res = !FindOneValidMove4BlackWhenChecked(posWhiteLongDistAttacker);

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	bool IsPinnedFlagOK(const char sq, const bool pinned) const
	{
		assert(IsValidPos(sq));
		assert(!IsEmptyAt(sq));

		if (IsWhiteAt(sq))
		{
			const auto posKing = GetWhiteKingPos();
			const auto pinningPieceFound = SameDiagonalOrLineAndAllBetweenEmpty(posKing, sq) && BlackLongDistanceFigureInDir(sq, posKing);
			return pinningPieceFound == pinned;
		}
		else
		{
			const auto posKing = GetBlackKingPos();
			const auto pinningPieceFound = SameDiagonalOrLineAndAllBetweenEmpty(posKing, sq) && WhiteLongDistanceFigureInDir(sq, posKing);
			return pinningPieceFound == pinned;
		}
	}
	#endif

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	ALWAYS_INLINE bool CanWhiteQueenCheckMate(const int qpos, const bool pinned) const
	#else
	ALWAYS_INLINE bool CanWhiteQueenCheckMate(const int qpos) const
	#endif
	{
		assert(!IsSquareAttackedByBlack(GetWhiteKingPos()));
		assert(IsValidPos(qpos));
		assert(white & queens & (1ULL << qpos));
		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(qpos, pinned));
		#endif

		const int posBlackKing = GetBlackKingPos();
		auto mask = Queen_Attacks[posBlackKing] & Queen_Attacks[qpos] & (~white);

		BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
		{
			if (AllBetweenEmpty(qpos, pos) & AllBetweenEmpty(pos, posBlackKing))			
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!pinned || IsSquareAlongTheLineOrDiag(pos, qpos, GetWhiteKingPos()))
				#else
				if (!IsWhitePinned(qpos, pos))
				#endif
					if (IsCheckMateAfterQueenCheck(qpos, pos))
						return true;			
		}
		END_FOR_EACH_POS_IN_MASK(pos, mask);

		return false;
	}

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	bool CanWhiteRookCheckMate(const int rpos, const bool pinned) const
	#else
	bool CanWhiteRookCheckMate(const int rpos) const
	#endif
	{
		assert(!IsSquareAttackedByBlack(GetWhiteKingPos()));
		assert(IsValidPos(rpos));
		assert(white & rooks & (1ULL << rpos));
		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(rpos, pinned));
		#endif

		const int posBlackKing = GetBlackKingPos();

		if (int posWhiteLongDistAttacker; SameDiagAndAllBetweenEmpty(posBlackKing, rpos) && (posWhiteLongDistAttacker = WhiteLongDistanceFigureInDir<1>(rpos, posBlackKing)) >= 0)
		{
			// Discovered check (and direct check maybe)
			auto trgtBitboard = get_rook_moves_hq(rpos, occ(), white);

			const auto posWhiteKing = GetWhiteKingPos();
			#if !defined(__PREEMPTIVE_WHITEPINNEDPIECES__)
			const bool pinned = SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, rpos) && BlackLongDistanceFigureInDir(rpos, posWhiteKing);
			#endif

			if (pinned)
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
		}
		else
		{
			// Direct check:
			auto mask = Rook_Attacks[posBlackKing] & Rook_Attacks[rpos] & (~white);

			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				if (AllBetweenEmpty(rpos, pos) & AllBetweenEmpty(pos, posBlackKing))
					#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
					if (!pinned || IsSquareAlongTheLineOrDiag(pos, rpos, GetWhiteKingPos()))
					#else
					if (!IsWhitePinned(rpos, pos))
					#endif
						if (IsCheckMateAfterRookDirectCheck(rpos, pos))
							return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		return false;
	}

	#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
	bool CanWhiteBishopCheckMate(const int bpos, const bool pinned) const
	#else
	bool CanWhiteBishopCheckMate(const int bpos) const
	#endif
	{
		assert(!IsSquareAttackedByBlack(GetWhiteKingPos()));
		assert(IsValidPos(bpos));
		assert(white & bishops & (1ULL << bpos));
		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(bpos, pinned));
		#endif

		const int posBlackKing = GetBlackKingPos();

		if (int posWhiteLongDistAttacker; SameLineAndAllBetweenEmpty(posBlackKing, bpos) && (posWhiteLongDistAttacker = WhiteLongDistanceFigureInDir<1>(bpos, posBlackKing)) >= 0)
		{
			// Discovered check (and direct check maybe)
			auto trgtBitboard = get_bishop_moves_hq(bpos, occ(), white);

			const auto posWhiteKing = GetWhiteKingPos();
			#if !defined(__PREEMPTIVE_WHITEPINNEDPIECES__)
			const bool pinned = SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, bpos) && BlackLongDistanceFigureInDir(bpos, posWhiteKing);
			#endif

			if (pinned)
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
		}
		else
		{
			// Direct check:
			auto mask = Bishop_Attacks[posBlackKing] & Bishop_Attacks[bpos] & (~white);

			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				if (AllBetweenEmpty(bpos, pos) & AllBetweenEmpty(pos, posBlackKing))
					#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
					if (!pinned || IsSquareAlongTheLineOrDiag(pos, bpos, GetWhiteKingPos()))
					#else
					if (!IsWhitePinned(bpos, pos))
					#endif
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterBishopDirectCheck(bpos, pos))
							return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);
		}

		return false;
	}
	bool CanWhiteKnightCheckMate(const int kpos) const
	{
		assert(!IsSquareAttackedByBlack(GetWhiteKingPos()));
		assert(IsValidPos(kpos));
		assert(white & knights & (1ULL << kpos));
		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(kpos, false));
		#endif
		
		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		constexpr bool pinned = false; // already verified before the call; compiler will optimize out the below 'if' condition
		#else
		const auto posWhiteKing = GetWhiteKingPos();
		const bool pinned = SameDiagonalOrLineAndAllBetweenEmpty(posWhiteKing, kpos) && BlackLongDistanceFigureInDir(kpos, posWhiteKing);
		#endif
		if (!pinned)
		{
			const int posBlackKing = GetBlackKingPos();

			if (int posWhiteLongDistAttacker; SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, kpos) && (posWhiteLongDistAttacker = WhiteLongDistanceFigureInDir<1>(kpos, posBlackKing)) >= 0)
			{
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
					if (IsCheckMateAfterKnightDirectCheck(kpos, pos))
						return true;
				}
				END_FOR_EACH_POS_IN_MASK(pos, mask);
			}
		}

		return false;
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
	bool CanWhiteKingCheckMate(const int kpos) const
	{
		assert(IsValidPos(kpos));
		assert(white & kings & (1ULL << kpos));

		const int posBlackKing = GetBlackKingPos();

		if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, kpos))
		{
			const int posWhiteLongDistAttacker = WhiteLongDistanceFigureInDir<1>(kpos, posBlackKing);
			if (posWhiteLongDistAttacker >= 0)
			{
				auto mask = King_Attacks[kpos] & (~white) & ~GetBetweenMask(posWhiteLongDistAttacker, posBlackKing);

				BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
				{
					if (!const_cast<FullBitboards*>(this)->IsSquareAttackedByBlackIfTakeOffWhiteKing(pos))
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterKingDiscoveredCheck(kpos, pos, posWhiteLongDistAttacker))
							return true;
				}
				END_FOR_EACH_POS_IN_MASK(pos, mask);
			}
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
								const_cast<FullBitboards*>(this)->rooks ^= rookMoveMask;

								const auto res = !FindOneValidMove4BlackWhenChecked(_F1_);

								const_cast<FullBitboards*>(this)->white ^= bothMasks;
								const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
								const_cast<FullBitboards*>(this)->rooks ^= rookMoveMask;

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
								const_cast<FullBitboards*>(this)->rooks ^= rookMoveMask;

								const auto res = !FindOneValidMove4BlackWhenChecked(_D1_);

								const_cast<FullBitboards*>(this)->white ^= bothMasks;
								const_cast<FullBitboards*>(this)->kings ^= kingMoveMask;
								const_cast<FullBitboards*>(this)->rooks ^= rookMoveMask;

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
	bool CanWhitePawnCheckMate(const int ppos, const int bposToCaptureWithEnPassant, const bool pinned) const
	#else
	bool CanWhitePawnCheckMate(const int ppos, const int bposToCaptureWithEnPassant) const
	#endif
	{
		assert(!IsSquareAttackedByBlack(GetWhiteKingPos())); // CanWhiteCheckMateWhenChecked can only be called in case wh.king is under check
		assert(IsValidPos(ppos));
		assert(white & pawns & (1ULL << ppos));
		assert(!tbEnPassantPossible || (bposToCaptureWithEnPassant >= _A5_ && bposToCaptureWithEnPassant <= _H5_));				
		#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
		assert(IsPinnedFlagOK(ppos, pinned));
		#endif

		const int posBlackKing = GetBlackKingPos();
		int posWhiteLongDistAttacker = -1;
		if (SameDiagonalOrLineAndAllBetweenEmpty(posBlackKing, ppos))
			posWhiteLongDistAttacker = WhiteLongDistanceFigureInDir<1>(ppos, posBlackKing);

		// Capture with direct check?
		auto maskToCapture = White_Pawn_Attacks[ppos] & black;
		BEGIN_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture)
		{
			if (White_Pawn_Attacks[posToCapture] & black & kings)
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!pinned || IsSquareAlongTheLineOrDiag(posToCapture, ppos, GetWhiteKingPos()))
				#else
				if (!IsWhitePinned(ppos, posToCapture))
				#endif
				{
					const bool bDoubleCheck = posWhiteLongDistAttacker >= 0 && (ppos & 7) != (posToCapture & 7);
					if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPawnDirectCheck(ppos, posToCapture, bDoubleCheck))
						return true;
				}
		}
		END_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture);

		// Move forward with direct check?
		const bool bMoveForwardPossible = IsEmptyAt(ppos + 8);
		const bool bKingCheckedAfterMoveForward = ((White_Pawn_Attacks[ppos + 8] & black & kings) != 0) & bMoveForwardPossible;
		if (bKingCheckedAfterMoveForward)
			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			if (!pinned || IsSquareAlongTheLineOrDiag(ppos + 8, ppos, GetWhiteKingPos()))
			#else
			if (!IsWhitePinned(ppos, ppos + 8))
			#endif
				if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPawnDirectCheck(ppos, ppos + 8))
					return true;

		// Double move forward with direct check?
		const bool bKingCheckedAfterDoubleMoveForward = ((ppos >> 3) == _2_) & ((posBlackKing >> 3) == _5_) & bMoveForwardPossible & (abs((ppos & 7) - (posBlackKing & 7)) == 1);
		if (bKingCheckedAfterDoubleMoveForward)
			if (IsEmptyAt(ppos + 16))
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!pinned || IsSquareAlongTheLineOrDiag(ppos + 16, ppos, GetWhiteKingPos()))
				#else
				if (!IsWhitePinned(ppos, ppos + 16))
				#endif
					if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPawnDirectCheck<1>(ppos, ppos + 16))
						return true;

		// promo:
		if (ppos >= _A7_)
		{
			// promo forward:
			const bool bPromoFowardPossible = IsEmptyAt(ppos + 8);
			if (bPromoFowardPossible)
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!pinned) // promo forward cannot be along the pinning line or diagonal || IsSquareAlongTheLineOrDiag(ppos + 8, ppos, GetWhiteKingPos()))
				#else
				if (!IsWhitePinned(ppos, ppos + 8))
				#endif
				{
					if (IsKnightDiff(ppos + 8, posBlackKing))
					{
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPromoToKnightDirectCheck(ppos, ppos + 8, posWhiteLongDistAttacker >= 0))
							return true;
					}
					else
						if (SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(ppos + 8, posBlackKing, ppos))
						{
							if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPromoToQueenDirectCheck(ppos, ppos + 8, posWhiteLongDistAttacker >= 0))
								return true;
						}

					if (posWhiteLongDistAttacker >= 0)
					{
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPromoToQueenDiscoveredCheck(ppos, ppos + 8, posWhiteLongDistAttacker))
							return true;
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPromoToKnightDiscoveredCheck(ppos, ppos + 8, posWhiteLongDistAttacker))
							return true;
					}
				}

			// promo capture:
			auto maskToCapture = White_Pawn_Attacks[ppos] & black;
			BEGIN_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture)
			{
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!pinned || IsSquareAlongTheLineOrDiag(posToCapture, ppos, GetWhiteKingPos()))
				#else
				if (!IsWhitePinned(ppos, posToCapture))
				#endif
				{
					if (IsKnightDiff(posToCapture, posBlackKing))
					{
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPromoToKnightDirectCheck(ppos, posToCapture, posWhiteLongDistAttacker >= 0))
							return true;
					}
					else
						if (SameDiagonalOrLineAndAllBetweenEmptyIfTakeOffWhitePawn(posToCapture, posBlackKing, ppos))
						{
							if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPromoToQueenDirectCheck(ppos, posToCapture, posWhiteLongDistAttacker >= 0))
								return true;
						}

					if (posWhiteLongDistAttacker >= 0)
					{
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPromoToQueenDiscoveredCheck(ppos, posToCapture, posWhiteLongDistAttacker))
							return true;
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPromoToKnightDiscoveredCheck(ppos, posToCapture, posWhiteLongDistAttacker))
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
					if (!pinned || IsSquareAlongTheLineOrDiag(posToCapture, ppos, GetWhiteKingPos()))
					#else
					if (!IsWhitePinned(ppos, posToCapture))
					#endif
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPawnDiscoveredCheck(ppos, posToCapture, posWhiteLongDistAttacker))
							return true;
				}
				END_FOR_EACH_POS_IN_MASK(posToCapture, maskToCapture);

				// Discovered check with a move forward:
				if (((posBlackKing & 7) != (ppos & 7)) & IsEmptyAt(ppos + 8))
					#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
					if (!pinned || IsSquareAlongTheLineOrDiag(ppos + 8, ppos, GetWhiteKingPos()))
					#else
					if (!IsWhitePinned(ppos, ppos + 8))
					#endif
					{
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPawnDiscoveredCheck(ppos, ppos + 8, posWhiteLongDistAttacker))
							return true;

						// Discovered check with a double move forward:
						if (((ppos >> 3) == _2_) & IsEmptyAt(ppos + 16))
							if (const_cast<FullBitboards*>(this)->IsCheckMateAfterPawnDiscoveredCheck(ppos, ppos + 16, posWhiteLongDistAttacker)) // no need to pass info about possible en passant, since it never prevents discovered check
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
						if (const_cast<FullBitboards*>(this)->IsCheckMateAfterEnPassantDirectCheck(ppos, bposToCaptureWithEnPassant + 8, bDoubleCheck))
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
							if (const_cast<FullBitboards*>(this)->IsCheckMateAfterEnPassantDiscoveredCheck(ppos, bposToCaptureWithEnPassant + 8, posWhiteLongDistAttackerInEnPassant))
								return true;
				}
			}
		}

		return false;
	}

	// posChecker can be DBL_CHECKED
	template<bool tbEnPassantPossible>
	bool CanWhiteCheckMateWhenChecked(const int posChecker) const
	{
		assert(posChecker >= 0 && posChecker <= DBL_CHECKED);

		const auto posWhiteKing = GetWhiteKingPos();
		if (CanWhiteKingCheckMate<0, 0>(posWhiteKing))
			return true;

		if (posChecker != DBL_CHECKED)
		{
			auto mask = CanWhiteCaptureWithCheckMate<tbEnPassantPossible, false, true>(posChecker);
			if (mask)
				return true;

			if (Distance(posChecker, posWhiteKing) > 1)
				if (!IsKnightDiff(posChecker, posWhiteKing))
					return CanWhiteMoveInBetweenWithCheckMate(posChecker, posWhiteKing);
		}

		return false;
	}

	// returns DBL_CHECKED (64) on double check; -1 when not checked or checking piece pos.
	ALWAYS_INLINE int IsBlackKingChecked(const int posBlackKing) const
	{
		assert(IsValidPos(posBlackKing));
		assert(black & kings & (1ULL << posBlackKing));

		const auto res = IsSquareAttackedByWhite<EXCL_KING, INCL_PINNED, FIND_ALL>(posBlackKing);
		if (res)
			return (std::popcount(res) > 1) ? DBL_CHECKED : std::countr_zero(res);
		else
			return -1;
	}

	// returns DBL_CHECKED (64) on double check; -1 when not checked or checking piece pos.
	ALWAYS_INLINE int IsBlackKingChecked() const
	{
		const int posBlackKing = GetBlackKingPos();

		return IsBlackKingChecked(posBlackKing);
	}

	// returns DBL_CHECKED (64) on double check; -1 when not checked or checking piece pos.
	ALWAYS_INLINE int IsWhiteKingChecked() const
	{
		const int posWhiteKing = GetWhiteKingPos();

		const auto res = IsSquareAttackedByBlack<EXCL_KING, INCL_PINNED, FIND_ALL>(posWhiteKing);
		if (res)
			return (std::popcount(res) > 1) ? DBL_CHECKED : std::countr_zero(res);
		else
			return -1;
	}

	// Alias; returns DBL_CHECKED (64) on double check; -1 when not checked or checking piece pos.
	ALWAYS_INLINE int GeWhiteKingCheckerPos() const
	{
		return IsWhiteKingChecked();
	}

	// tbWhiteKingUnderCheck can be:
	// 0 - not under check
	// 1 - under check; in such case param. posWhiteKingChecker can be filled in (pos or DBL_CHECKED), or left with -1 for the method to find out
	// -1 - unknown, check yourself
	template<char tbWhiteKingUnderCheck = -1, bool tbEnPassantPossible = false, bool tbCastlingShortPossible = true, bool tbCastlingLongPossible = true>
	bool FindMoveThatMates(int posWhiteKingChecker = -1, const int bposToCaptureWithEnPassant = -1) const
	{
		assert(std::popcount(black & kings) == 1);
		assert(std::popcount(white & kings) == 1);
		assert(!IsSquareAttackedByWhite(GetBlackKingPos()));

		const int posWhiteKing = GetWhiteKingPos();

		if (tbWhiteKingUnderCheck > 0 || (tbWhiteKingUnderCheck < 0 && IsSquareAttackedByBlack(posWhiteKing)))
		{
			if (posWhiteKingChecker < 0)
				posWhiteKingChecker = GeWhiteKingCheckerPos();

			if (tbEnPassantPossible && posWhiteKingChecker != bposToCaptureWithEnPassant)
				return CanWhiteCheckMateWhenChecked<false>(posWhiteKingChecker); // en passant not possible after discovered check with double move by a pawn
			else
				return CanWhiteCheckMateWhenChecked<tbEnPassantPossible>(posWhiteKingChecker);
		}
		else
		{
			#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
			const auto whitePinnedPieces = GetWhitePinnedPieces();
			#endif
			auto mask = queens & white;
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

			mask = rooks & white;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask);
			{				
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (CanWhiteRookCheckMate(pos, IsPosInBitmask(pos, whitePinnedPieces)))
				#else
				if (CanWhiteRookCheckMate(pos))
				#endif
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = bishops & white;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask);
			{
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (CanWhiteBishopCheckMate(pos, IsPosInBitmask(pos, whitePinnedPieces)))
				#else
				if (CanWhiteBishopCheckMate(pos))
				#endif
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = knights & white;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask);
			{
				#ifdef __PREEMPTIVE_WHITEPINNEDPIECES__
				if (!IsPosInBitmask(pos, whitePinnedPieces))
				#endif
				if (CanWhiteKnightCheckMate(pos))
					return true;
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = pawns & white;
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

			if (CanWhiteKingCheckMate<tbCastlingShortPossible, tbCastlingLongPossible>(posWhiteKing))
				return true;

			return false;
		}
	}

	// Here starts the part of code strictly for FindMoveThatMatesInTwoMoves

	template<bool tbEnPassantPossible = false, char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	bool IsImmediateMateAfterAnyBlackResponse(const int bpposForEnPassant = -1) const
	{
		constexpr bool tbWhiteCastlingShortPossible = tbWhiteCastlingFlags & 1;
		constexpr bool tbWhiteCastlingLongPossible = (tbWhiteCastlingFlags & 2) != 0;

		const auto posBlackKing = GetBlackKingPos();
		const auto posBlackKingChecker = IsBlackKingChecked(posBlackKing);
		
		if (posBlackKingChecker >= 0)
		{			
			auto mask = King_Attacks[posBlackKing] & ~black;

			#ifdef __USE_OPTIM_FOR_NON_CAPTURE_BY_KING__
			// First let's analyze capture moves (they have higher probability of being a refutation and additionally it allows to call IsImmediateMateAfterMoveByBlackKing with param. tbKnownThatItIsNotACapture == true in the next loop
			auto maskCaptureMoves = mask & white; 
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskCaptureMoves)
			{
				if (!IsSquareAttackedByWhiteIfTakeOffBlackKing(posTo))
					if (!IsImmediateMateAfterMoveByBlackKing<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(posBlackKing, posTo))
						return false;
			}
			END_FOR_EACH_POS_IN_MASK(posTo, maskCaptureMoves);

			// Now non-capture moves by black king:
			auto maskForNonCapture = mask & ~white;
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskForNonCapture)
			{
				if (!IsSquareAttackedByWhiteIfTakeOffBlackKing(posTo))
					if (!IsImmediateMateAfterMoveByBlackKing<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible, true>(posBlackKing, posTo))
						return false;
			}
			END_FOR_EACH_POS_IN_MASK(posTo, maskForNonCapture);		

			#else
			
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, mask)
			{
				if (!IsSquareAttackedByWhiteIfTakeOffBlackKing(posTo))
					if (!IsImmediateMateAfterMoveByBlackKing<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(posBlackKing, posTo))
						return false;
			}
			END_FOR_EACH_POS_IN_MASK(posTo, mask);

			#endif

			if (posBlackKingChecker != DBL_CHECKED)
			{
				constexpr bool tbOnlyIfPreventsImmediateMateAndFlags = 1;
				constexpr bool tbOneIsEnough = 1;

				uint64_t captureMask;
				constexpr auto tbFlags = tbOnlyIfPreventsImmediateMateAndFlags + 2 * tbWhiteCastlingFlags;
				if (!tbEnPassantPossible || posBlackKingChecker == bpposForEnPassant)
					captureMask = CanBlackCapture<tbEnPassantPossible, 1, tbOneIsEnough, tbFlags>(posBlackKingChecker);
				else
					captureMask = CanBlackCapture<0, 1, tbOneIsEnough, tbFlags>(posBlackKingChecker);

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
			#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
			const auto blackPinnedPieces = GetBlackPinnedPieces();
			#endif

			bool legalMovesFound = false;
			auto mask = queens & black;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = get_bishop_moves_hq(pos, occ(), black) | get_rook_moves_hq(pos, occ(), black);
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
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = rooks & black;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = get_rook_moves_hq(pos, occ(), black);
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
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = bishops & black;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = get_bishop_moves_hq(pos, occ(), black);
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
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			#ifdef __PREEMPTIVE_BLACKPINNEDPIECES__
			mask = knights & black & ~blackPinnedPieces;
			#else
			mask = knights & black;
			#endif
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				#if !defined(__PREEMPTIVE_BLACKPINNEDPIECES__)
				if (!IsBlackAbsolutelyPinned(pos))
				#endif
				{
					auto movesMask = Knight_Attacks[pos] & ~black;
					BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
					{
						if (!IsImmediateMateAfterMoveByBlackKnight<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(pos, posTo))
							return false;
						legalMovesFound = true;
					}
					END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
				}
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = kings & black;
			assert(mask);
			const auto posBlackKing = std::countr_zero(mask);
			mask = King_Attacks[posBlackKing] & ~black;
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, mask)
			{
				if (!IsSquareAttackedByWhite(posTo))
				{
					if (!IsImmediateMateAfterMoveByBlackKing<tbWhiteCastlingShortPossible, tbWhiteCastlingLongPossible>(posBlackKing, posTo))
						return false;
					legalMovesFound = true;
				}
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			if constexpr (tbBlackCastlingFlags != 0)
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
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteQueenMove(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteQueenAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->queens ^= fromMask;
		const_cast<FullBitboards*>(this)->queens |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_QUEEN>(captureMask);

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteRookMove(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteRookAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->rooks ^= fromMask;
		const_cast<FullBitboards*>(this)->rooks |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_ROOK>(captureMask);

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteBishopMove(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteBishopAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->bishops ^= fromMask;
		const_cast<FullBitboards*>(this)->bishops |= toMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_BISHOP>(captureMask);

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteKnightMove(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteKnightAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

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
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove(const int posFrom, const int posTo) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhiteKingAt(posFrom));

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_KING>(captureMask);

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, 0, tbBlackCastlingFlags>();

		*(const_cast<FullBitboards*>(this)) = bbSaved; // restore

		return res;
	}

	template<char tbBlackCastlingFlags = 3>
	bool IsImmediateMateAfterAnyBlackResponseAfterWhiteCastlingShort() const
	{
		constexpr auto fromMask = 1ULL << _E1_;
		constexpr auto toMask = 1ULL << _G1_;
		constexpr auto moveMask = fromMask | toMask;

		constexpr auto fromMaskRook = 1ULL << _H1_;
		constexpr auto toMaskRook = 1ULL << _F1_;
		constexpr auto moveMaskRook = fromMaskRook | toMaskRook;

		constexpr auto moveCastling = moveMask | moveMaskRook;

		const_cast<FullBitboards*>(this)->white ^= moveCastling;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->rooks ^= moveMaskRook;

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, 0, tbBlackCastlingFlags>();

		// Restore:
		const_cast<FullBitboards*>(this)->white ^= moveCastling;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->rooks ^= moveMaskRook;

		return res;
	}

	template<char tbBlackCastlingFlags = 3>
	bool IsImmediateMateAfterAnyBlackResponseAfterWhiteCastlingLong() const
	{
		constexpr auto fromMask = 1ULL << _E1_;
		constexpr auto toMask = 1ULL << _C1_;
		constexpr auto moveMask = fromMask | toMask;

		constexpr auto fromMaskRook = 1ULL << _A1_;
		constexpr auto toMaskRook = 1ULL << _D1_;
		constexpr auto moveMaskRook = fromMaskRook | toMaskRook;

		constexpr auto moveCastling = moveMask | moveMaskRook;

		const_cast<FullBitboards*>(this)->white ^= moveCastling;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->rooks ^= moveMaskRook;

		const auto res = IsImmediateMateAfterAnyBlackResponse<false, 0, tbBlackCastlingFlags>();

		// Restore:
		const_cast<FullBitboards*>(this)->white ^= moveCastling;
		const_cast<FullBitboards*>(this)->kings ^= moveMask;
		const_cast<FullBitboards*>(this)->rooks ^= moveMaskRook;

		return res;
	}

	ALWAYS_INLINE bool IsWhitePromoMove(const int posFrom) const
	{
		assert(IsValidPos(posFrom));

		return IsWhitePawnAt(posFrom) & (posFrom >= _A7_);
	}

	template<char tbWhiteCastlingFlags, char tbBlackCastlingFlags>
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove(const int posFrom, const int posTo, const int promo = FGR_EMPTY) const
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
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

		const auto bbSaved = *this; // save

		const_cast<FullBitboards*>(this)->white ^= moveMask;
		const_cast<FullBitboards*>(this)->pawns ^= fromMask;

		const_cast<FullBitboards*>(this)->black ^= captureMask;
		const_cast<FullBitboards*>(this)->ClearOnPieceBitboardsExcept<FGR_PAWN>(captureMask);

		bool res;

		if (promo == FGR_EMPTY) // verify all 4 promos?
		{
			const_cast<FullBitboards*>(this)->queens |= toMask;

			res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();
			if (!res)
			{
				const_cast<FullBitboards*>(this)->queens ^= toMask;
				const_cast<FullBitboards*>(this)->knights |= toMask;

				res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();
				if (!res)
				{
					const_cast<FullBitboards*>(this)->knights ^= toMask;
					const_cast<FullBitboards*>(this)->rooks |= toMask;
					res = IsImmediateMateAfterAnyBlackResponse<0, tbWhiteCastlingFlags, tbBlackCastlingFlags>();

					if (!res)
					{
						const_cast<FullBitboards*>(this)->rooks ^= toMask;
						const_cast<FullBitboards*>(this)->bishops |= toMask;
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
				const_cast<FullBitboards*>(this)->bishops |= toMask;
				break;
			case FGR_ROOK:
				const_cast<FullBitboards*>(this)->rooks |= toMask;
				break;
			case FGR_QUEEN:
				const_cast<FullBitboards*>(this)->queens |= toMask;
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
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhiteEnPassant(const int posFrom, const int posTo) const
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
	ALWAYS_INLINE bool IsImmediateMateAfterAnyBlackResponseAfterWhitePawnMove(const int posFrom, const int posTo, const int promo = FGR_EMPTY) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posTo));
		assert(posTo != posFrom);
		assert(!IsWhiteAt(posTo));
		assert(IsWhitePawnAt(posFrom));

		if (posFrom >= _A7_)
			return IsImmediateMateAfterAnyBlackResponseAfterWhitePromoMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo, promo);
		const bool bEnPassant = (posFrom & 7) != (posTo & 7) && IsEmptyAt(posTo);
		if (bEnPassant)
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteEnPassant<tbWhiteCastlingFlags, tbBlackCastlingFlags>(posFrom, posTo);

		const auto fromMask = sq_to_bb(posFrom);
		const auto toMask = sq_to_bb(posTo);
		const auto moveMask = fromMask | toMask;
		const bool bCapture = (black & toMask) != 0;
		const auto captureMask = bCapture ? toMask : 0;

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
	bool IsImmediateMateAfterAnyBlackResponseAfterWhiteMove(const TMove& move) const
	{
		assert(white & (1ULL << move.nFrom));
		assert((~white) & (1ULL << move.nTo));

		const FIGURE fMoving = GetFigureAt(move.nFrom);
		switch (fMoving)
		{
		case FGR_KING:
			assert(Distance(move.nFrom, move.nTo) == 1); // separate method for castling
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove<tbBlackCastlingFlags>(move.nFrom, move.nTo);
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
	bool IsImmediateMateAfterAnyBlackResponseAfterWhiteMove(const int posFrom, const int posTo) const
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
			return IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove<tbBlackCastlingFlags>(posFrom, posTo);
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

	ALWAYS_INLINE bool IsWhiteCaptureEnPassant(const int posFrom, const int posCaptured) const
	{
		assert(IsValidPos(posFrom));
		assert(IsValidPos(posCaptured));
		assert(posCaptured != posFrom);
		assert(IsWhiteAt(posFrom));
		assert(IsBlackAt(posCaptured));

		const auto res = AreSquaresAside(posFrom, posCaptured) & IsWhitePawnAt(posFrom);
		return res;
	}
	
	uint64_t GetWhitePinnedPieces() const
	{
		Bitboard pinned = 0ULL;

		const auto posWhiteKing = GetWhiteKingPos();
		const auto potential_pinners_rook = get_rook_moves_hq(posWhiteKing, black, 0) & black & (rooks|queens); // here own pieces are 0 - as if we can "move through" them
		const auto potential_pinners_bishop = get_bishop_moves_hq(posWhiteKing, black, 0) & black & (bishops|queens);
		auto potential_pinners = potential_pinners_rook | potential_pinners_bishop;

		BEGIN_FOR_EACH_POS_IN_MASK(posPinner, potential_pinners)
		{
			const auto maskBetween = GetBetweenMask(posWhiteKing, posPinner);
			const auto whitePiecesBetween = maskBetween & white;

			const bool exactlyOneWhitePieceBetween = (std::popcount(whitePiecesBetween) == 1); // exactly one white piece between?
			const auto maskToApply = exactlyOneWhitePieceBetween ? whitePiecesBetween : 0ULL; // let's try to be branchless (cmov)

			pinned |= maskToApply;
		}
		END_FOR_EACH_POS_IN_MASK(posPinner, potential_pinners);

		return pinned;
	}

	uint64_t GetBlackPinnedPieces() const
	{
		Bitboard pinned = 0ULL;

		const auto posBlackKing = GetBlackKingPos();
		const auto potential_pinners_rook = get_rook_moves_hq(posBlackKing, white, 0) & white & (rooks | queens); // here own pieces are 0 - as if we can "move through" them
		const auto potential_pinners_bishop = get_bishop_moves_hq(posBlackKing, white, 0) & white & (bishops | queens);
		auto potential_pinners = potential_pinners_rook | potential_pinners_bishop;

		BEGIN_FOR_EACH_POS_IN_MASK(posPinner, potential_pinners)
		{
			const auto maskBetween = GetBetweenMask(posBlackKing, posPinner);
			const auto blackPiecesBetween = maskBetween & black;

			const bool exactlyOneBlackPieceBetween = (std::popcount(blackPiecesBetween) == 1); 
			const auto maskToApply = exactlyOneBlackPieceBetween ? blackPiecesBetween : 0ULL; // let's try to be branchless (cmov)

			pinned |= maskToApply;
		}
		END_FOR_EACH_POS_IN_MASK(posPinner, potential_pinners);

		return pinned;
	}


	// See FindMoveThatMates above for description of params tbWhiteKingUnderCheck and tbEnPassantPossible
	// Flags tbWhiteCastlingFlags and tbBlackCastlingFlags have:
	// * set bit 1 if castling short is possible (wh.king is on e1 and did not move yet and wh.R is on h1 and did not move yet)
	// * set bit 2 if castling long is possible (wh.king is on e1 and did not move yet and wh.R is on a1 and did not move yet)
	template<char tbWhiteKingUnderCheck = -1, bool tbEnPassantPossible = false, char tbWhiteCastlingFlags = 3, char tbBlackCastlingFlags = 3, bool tbFindAllSolutionsAndFillBuf = false>
	int FindMoveThatMatesInTwoMoves(int posWhiteKingChecker = -1, const int bposToCaptureWithEnPassant = -1, TMove* pMoves = nullptr) const
	{
		const int posWhiteKing = GetWhiteKingPos();
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
				uint64_t mask;
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
					const auto num = CanWhiteMoveInBetween<1, 0>(posWhiteKingChecker, posWhiteKing, aMoves);
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
					if (IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove<tbBlackCastlingFlags>(posWhiteKing, posTo))
						if constexpr (!tbFindAllSolutionsAndFillBuf)
							return true;
						else
							pMoves[count++].set(posWhiteKing, posTo);
			}
			END_FOR_EACH_POS_IN_MASK(posTo, mask);
		}
		else
		{
			const auto whitePinnedPieces = GetWhitePinnedPieces();

			auto mask = white & queens;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = get_bishop_moves_hq(pos, occ(), white);
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
				{					
					if (!IsPosInBitmask(pos, whitePinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posWhiteKing))
						if (IsImmediateMateAfterAnyBlackResponseAfterWhiteQueenMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo))
							if constexpr (!tbFindAllSolutionsAndFillBuf)
								return true;
							else
								pMoves[count++].set(pos, posTo);
				}
				END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
				movesMask = get_rook_moves_hq(pos, occ(), white);
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
				{					
					if (!IsPosInBitmask(pos, whitePinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posWhiteKing))
						if (IsImmediateMateAfterAnyBlackResponseAfterWhiteQueenMove<tbWhiteCastlingFlags, tbBlackCastlingFlags>(pos, posTo))
							if constexpr (!tbFindAllSolutionsAndFillBuf)
								return true;
							else
								pMoves[count++].set(pos, posTo);
				}
				END_FOR_EACH_POS_IN_MASK(posTo, movesMask);
			}
			END_FOR_EACH_POS_IN_MASK(pos, mask);

			mask = white & rooks;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = get_rook_moves_hq(pos, occ(), white);
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
				{					
					if (!IsPosInBitmask(pos, whitePinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posWhiteKing))
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

			mask = white & bishops;
			BEGIN_FOR_EACH_POS_IN_MASK(pos, mask)
			{
				auto movesMask = get_bishop_moves_hq(pos, occ(), white);
				BEGIN_FOR_EACH_POS_IN_MASK(posTo, movesMask)
				{					
					if (!IsPosInBitmask(pos, whitePinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posWhiteKing))
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
					if (!IsPosInBitmask(pos, whitePinnedPieces) || IsSquareAlongTheLineOrDiag(posTo, pos, posWhiteKing))
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

			auto maskMoves = King_Attacks[posWhiteKing] & ~white;
			BEGIN_FOR_EACH_POS_IN_MASK(posTo, maskMoves)
			{
				if (!IsSquareAttackedByBlack(posTo))
					if (IsImmediateMateAfterAnyBlackResponseAfterWhiteKingMove<tbBlackCastlingFlags>(posWhiteKing, posTo))
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
	ALWAYS_INLINE int SolveTwoMoverDispatcher(const int whiteKingChecker, const int enPassantSquare, const int castlingFlags, TMove* pMoves) const
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
	TMove StringToMove(const char* szMove) const
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

public:
	// Returns empty vector on error:
	std::vector<TMove> StringToMoves(const std::string& moves) const
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


