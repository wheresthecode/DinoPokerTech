#include "pch.h"
#include "HandRanking.h"

enum HandRanking
{
	HighCard = 0,
	OnePair,
	TwoPair,
	ThreeOfAKind,
	Straight,
	Flush,
	FullHouse,
	FourOfAKind,
	StraightFlush,
	HandRankCategoryCount
};

#define RankBits 16
#define HandRankMask ((1 << RankBits) - 1)
#define HandTypeBits 4
#define HandTypeMask ((1 << HandTypeBits) - 1) << RankBits
#define GetHandTypeRank(handRank) ((handRank & HandTypeMask) >> RankBits)
#define GetHandRankSubRank(handRank) ((handRank & HandTypeMask) >> RankBits)
#define CreateHandRank(type, rank) (type << RankBits) | (rank & HandRankMask);

std::string CardMaskToExplicitString(CardMask mask)
{
	return "";
}

inline int CheckStraight(uint16_t mask)
{
	int streak = 0;
	uint16_t _m = mask;
	uint16_t straightMask = 0b11111 << (Rank_10);
	for (int i = Rank_A; i >= Rank_6; i--)
	{
		if ((mask & straightMask) == straightMask)
			return i;
		straightMask = straightMask >> 1;
	}
	const uint16_t wheelMask = 1 << (kRankCardCount - 1) | 0b1111;
	if ((mask & wheelMask) == wheelMask)
		return Rank_5;
	return 0;
}

uint16_t Get5CardCountAndRank(uint16_t mask, int &outBitCount)
{
	uint16_t result = 0;
	outBitCount = 0;
	for (int i = kRankCardCount - 1; i >= 0; i--)
	{
		uint16_t m = (1 << i) & mask;
		if (m != 0)
		{
			if (outBitCount < 5)
				result = result | m;
			outBitCount++;
		}
	}
	return result;
}

HandRank Rank5CardHandFrom7Cards(CardMask cards)
{
	uint16_t *suits = (uint16_t *)&cards;

	uint16_t ranksCombined = suits[0] | suits[1] | suits[2] | suits[3];

	int straightRank = CheckStraight(ranksCombined);
	int flushRank = 0;
	int straightFlushRank = 0;
	for (int i = 0; i < kSuitCount; i++)
	{
		int cardsInMask;
		uint16_t subRank = Get5CardCountAndRank(suits[i], cardsInMask);
		if (cardsInMask >= 5)
		{
			flushRank = subRank;
			if (straightRank != 0)
			{
				straightFlushRank = CheckStraight(suits[i]);
			}
		}
	}

	if (straightFlushRank > 0)
		return CreateHandRank(HandRanking::StraightFlush, straightFlushRank);

	int duplicatedRanks[kSuitCount][kRankCardCount];
	int duplicatedRanksCount[kSuitCount] = {0, 0, 0, 0};

	int highestQuadRank = -1;
	int highestTripsRank = -1;

	for (int i = kRankCardCount - 1; i >= 0; i--)
	{
		CardMask thisMask = cards.v >> i;
		uint16_t *perSuit = (uint16_t *)&thisMask;
		int c = (0x1 & perSuit[0]) + (0x1 & perSuit[1]) + (0x1 & perSuit[2]) + (0x1 & perSuit[3]);
		if (c > 0)
			duplicatedRanks[kSuitCount - c][duplicatedRanksCount[kSuitCount - c]++] = i;
	}

	if (duplicatedRanksCount[0] > 0) // Quads
	{
		// Find kicker
		int quadRank = duplicatedRanks[0][0];
		int kicker = (duplicatedRanksCount[0] > 2) ? duplicatedRanks[0][1] : 0;
		kicker = std::max(kicker, (duplicatedRanksCount[1] > 0) ? duplicatedRanks[1][0] : 0);
		kicker = std::max(kicker, (duplicatedRanksCount[2] > 0) ? duplicatedRanks[2][0] : 0);
		kicker = std::max(kicker, (duplicatedRanksCount[3] > 0) ? duplicatedRanks[3][0] : 0);
		uint16_t subRank = (quadRank << 4) | kicker;
		return CreateHandRank(HandRanking::FourOfAKind, subRank);
	}
	else if (duplicatedRanksCount[1] >= 2 || (duplicatedRanksCount[1] == 1 && duplicatedRanksCount[2] > 0)) // full house
	{
		int topTrips = duplicatedRanks[1][0];
		int pairRank = (duplicatedRanksCount[1] > 1) ? duplicatedRanks[1][1] : 0;
		pairRank = std::max(pairRank, duplicatedRanksCount[2] > 0 ? duplicatedRanks[2][0] : 0);
		return CreateHandRank(HandRanking::FullHouse, (topTrips << 4) | (pairRank));
	}

	if (flushRank)
		return CreateHandRank(HandRanking::Flush, flushRank);

	if (straightRank)
		return CreateHandRank(HandRanking::Flush, straightRank);

	// Trips
	if (duplicatedRanksCount[1] > 0)
	{
		int tripsRank = duplicatedRanks[1][0];
		int kicker1 = duplicatedRanks[3][0];
		int kicker2 = duplicatedRanks[3][1];
		return CreateHandRank(HandRanking::ThreeOfAKind, (tripsRank << 8) | (kicker1 << 4) | (kicker2));
	}

	// Two Pair
	if (duplicatedRanksCount[2] > 1)
	{
		int p1 = duplicatedRanks[2][0];
		int p2 = duplicatedRanks[2][1];

		int kicker = (duplicatedRanksCount[2] > 2) ? duplicatedRanks[2][2] : 0;
		kicker = std::max(kicker, (duplicatedRanksCount[3] > 0) ? duplicatedRanks[3][0] : 0);
		return CreateHandRank(HandRanking::TwoPair, (p1 << 8) | (p2 << 4) | kicker);
	}

	// Single Pair
	if (duplicatedRanksCount[2] == 1)
	{
		int p = duplicatedRanks[2][0] << 12;
		int kickers = (duplicatedRanks[3][0] << 8) | (duplicatedRanks[3][1] << 4) | (duplicatedRanks[3][2]);
		return CreateHandRank(HandRanking::OnePair, p | kickers);
	}

	// High Card
	int r = (1 << duplicatedRanks[3][0]) | (1 << duplicatedRanks[3][1]) | (1 << duplicatedRanks[3][2]) | (1 << duplicatedRanks[3][3]) | (1 << duplicatedRanks[3][4]);
	return CreateHandRank(HandRanking::HighCard, r);
}