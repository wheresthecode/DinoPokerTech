#include "pch.h"
#include "HandRankingTests.h"
#include "../PokerTech/HandRanking.h"
#include "../PokerTech/PokerUtilities.h"
#include "Deck.h"

CardMask ExplicitStringToCardMaskUnsafe(const std::string &cards)
{
	CardMask m;
	ExplicitStringToCardMask(cards, m);
	return m;
}

#define N(x, y)                                  \
	{                                            \
		x, y, false, ExplicitStringToCardMask(x) \
	}
#define NS(x, y)                                \
	{                                           \
		x, y, true, ExplicitStringToCardMask(x) \
	}

const HandRankTestEntry ranks[] =
	{
		// Straight Flush
		N("Ac Kc Qc Qd 2s Tc Jc", "Royal Flush"),
		N("9c Kc Qc Qd 2s Tc Jc", "Straight Flush King High"),
		N("Ac 2c 3c Qd 2s 4c 5c", "Straight Flush Wheel"),

		// Quads
		N("Ac Ad As Ah 2s Kc Jc", "4 Aces w/ King kicker"),
		NS("Ac Ad As Ah 2s Kc Kd", "4 Aces w/ Pair of Kings"),
		NS("Ac Ad As Ah Ks Kc Kd", "4 Aces w/ Three Kings"),
		N("Ac Ad As Ah 2s Qc Jc", "4 Aces w/ Queen kicker"),
		N("Ac Ad As Ah 2s 2c 3c", "4 Aces w/ Three kicker"),
		N("Ac Ad As Ah 2s 2c 2d", "4 Aces w/ Two kicker"),
		N("Tc Td Ts Th As Ac Ad", "4 Tens w/ Ace Kicker"),
		N("2c 2d 2s 2h 3s Ac Ad", "4 Twos w/ Ace Kicker"),

		// Full House
		N("Ac Ad As Kh Ks Kc 2c", "Aces full of Kings"),
		NS("Ac Ad As Kh Ks Qc 2c", "No Kickers For Full House"),
		N("Ac Ad As 2h 2s 3c 4c", "Aces full of Twos"),
		N("Ac Ad Ks Kh Kc 3c 4c", "Kings full of Aces"),
		N("2c 2d Ks Kh Kc 3c 4c", "Kings full of Twos"),
		N("Ac Ad 2s 2h 2c 3c 4c", "Twos full of Aces"),
		NS("Ac Ad 2s 2h 2c Kc Kd", "Two Pairs with Trips don't play"),

		// Flush
		N("Ac Kc Qc Tc 9c 6c 2c", "Ace High Flush #1"),
		NS("Ac Kc Qc Tc 9c Js 2c", "Ace High Flush with straight"),
		NS("Ac Kc Qc Tc 9c 7c 2c", "6th flush card doesn't play"),
		N("Ac Kc Qc Tc 8c 6c 2c", "Ace High Flush #2"),
		N("7c Kc Qc Tc 8c 6d 2d", "King High Flush"),

		// Straights
		N("Ac Kd Qs Tc Jd 6c 2c", "Ace high straight"),
		N("9c Kd Qs Tc Jd 6c 2c", "King high straight"),
		NS("9c Kd Qs Tc Jd 8c 2c", "King high straight kickers don't play"),
		N("9c 8d 7s 6c 5c 8s 2d", "Nine high straight"),
		N("Ac As 2s 3c 4c 5s 6d", "6 high with AA"),
		NS("Kc Ks 2s 3c 4c 5s 6d", "6 high with AA"),
		N("Ac 2c 3s 4c 5c 8s 2d", "Wheel"),
		NS("Ac 2c 3s 4c 5c 5s 5d", "Wheel with trips"),
		NS("Ac 2c 3s 4c 5c 5s 4d", "Wheel with two pair"),
		NS("Ac 2c 3s 4c 5c Js 2d", "Wheel kicker doesn't play"),

		// Three of a kind
		N("Ac Ad As Kc Qd 6c 2c", "Three Aces KQ kicker"),
		NS("Ac Ad As Kc Qd Tc 2c", "Three Aces KQ kicker Third Kicker Doesn't play"),
		N("Ac Ad As Kc Jd 6c 2c", "Three Aces KJ kicker"),
		N("Kc Kd Ks Ac Qd 6c 2c", "Three Kings AQ kicker"),
		N("Kc Kd Ks Tc 9d 6c 2c", "Three Kings T9 kicker"),
		N("2c 2d 2s Tc 9d Kc Ac", "Three Twos AK kicker"),
		N("2c 2d 2s 3c 4d 6c Ac", "Three Twos A6 kicker"),

		// Two pairs
		N("Ac Ad Ks Kc Qd 6c 3c", "Aces and Ks"),
		NS("Ac Ad Ks Kc Qd 6c 6s", "Three Pair"),
		N("Ac Ad 2s 2c Qd 6c 3c", "Aces and 2s w/ Q"),
		N("Ac Ad 2s 2c Td 6c 3c", "Aces and 2s w/ T"),
		N("Kc Kd 9s 9c Ad 6c 3c", "Kings and 9s w/ A"),
		N("Kc Kd 2s 2c Ad 6c 3c", "Kings and 2s w/ A"),

		// Pair
		N("Ac Ad Ks Jc Qd 6c 3c", "Aces KJQ"),
		NS("Ac Ad Ks Jc Qd 7c 9c", "Aces KJQ kicker doesn't play"), // last two cards shouldn't play
		N("Ac Ad Ks Jc Td 6c 3c", "Aces KJT"),
		N("Kc Kd 4s Jc Td 6c 3c", "Kings"),
		N("2c 2d As Jc Td 6c 3c", "2s with A"),
		N("2c 2d 4s Jc Td 6c 3c", "2s with J"),

		// High Card
		N("Ac Kc Qs Jc 9d 6c 3s", "Ace high"),
		NS("Ac Kc Qs Jc 9d 8c 3s", "Ace high kicker doesn't play"),
		N("5c Kc Qs Jc 9d 6c 3s", "King high"),
		N("2c 3c 4s 5c 7d 8c 9s", "9 high")

};

const HandRankTestEntry *GetHandRankTestEntries(int &outCount)
{
	outCount = ARRAY_SIZE(ranks);
	return ranks;
}

void GenerateRandomHands(int handCount, int cardCount, int seed, std::vector<CardMask> &outHands)
{
	outHands.reserve(handCount);
	outHands.clear();

	Deck deck;

	CardSet s;

	std::vector<CardMask> allCards;
	s.resize(cardCount);
	srand(seed);
	bool result = true;
	for (int i = 0; i < handCount; i++)
	{
		deck.DealCardSet(s, cardCount);
		outHands.push_back(s.ToMask());
		deck.ReturnCardSet(s);
	}
	assert(result);
}

// Keeping this here so the compiler doesn't remove this enum for debugging purposes
CardsAll gTest;

TEST(HandRankTest, RankTest)
{

	int rankCount;
	const HandRankTestEntry *ranks = GetHandRankTestEntries(rankCount);
	HandRank lastRank = (uint64_t)1 << 31;
	for (int i = 0; i < rankCount; i++)
	{
		CardMask handMask = ExplicitStringToCardMask(ranks[i].hand);
		HandRank rank = Rank5CardHandFrom7Cards(handMask);
		if (ranks[i].sameAsPrevious)
		{
			EXPECT_EQ(lastRank, rank) << ranks[i].desc;
		}
		else
		{
			EXPECT_GT(lastRank, rank) << ranks[i].desc;
		}
		lastRank = rank;
	}
}