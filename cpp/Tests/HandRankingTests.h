#pragma once

#include "../PokerTech/PokerTypes.h"

struct HandRankTestEntry
{
	const char *hand;
	const char *desc;
	bool sameAsPrevious;
	CardMask handMask;
};

const HandRankTestEntry *GetHandRankTestEntries(int &outCount);

void GenerateRandomHands(int handCount, int cardCount, int seed, std::vector<CardMask> &outHands);