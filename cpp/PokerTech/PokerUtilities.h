#pragma once

#include "PokerTypes.h"
#include "HandRange.h"
#include <set>

// I.E. "Ac As Qc Qd"
bool ExplicitStringToCardMask(const std::string &cards, CardMask &outMask);
CardMask ExplicitStringToCardMask(const std::string &cards);

std::string CardIndexToName(int cardIndex);
std::string CardToName(const Card &card);

CardRanks RankCharToIndex(char rankChar);
CardSuit SuitCharToIndex(char suitChar);
char CardSuitToChar(CardSuit suit);
char CardRankToChar(CardRanks rank);

// Is a 5 card or less string of explicit cards: I.E. Ac 3s 2d
bool IsValidBoardString(const std::string &rangeString);

#define HAND(x) (ExplicitStringToCardMask(#x))

std::string CardMaskToString(CardMask set);

struct Fixed16Ratio
{
	Fixed16Ratio() { r = 0; }
	Fixed16Ratio(float f) { FromFloat(f); }
	uint16_t r;
	operator float() const { return ToFloat(); }

	inline float ToFloat() const { return (float)r / ((float)(1 << 16) - 1); }
	inline void FromFloat(float f)
	{
		if (f < 0.0001f)
		{
			r = 0;
			return;
		}
		else if (f > 0.999f)
		{
			r = 0xffff;
			return;
		}
		r = 0xffff;
		float v = f * (float)r;
		r = (uint16_t)v;
	}
};