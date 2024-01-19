#include "PokerTech/pch.h"
#include "PokerTech/Utilities.h"
#include "PokerTech/PokerUtilities.h"
#include <algorithm>
#include <cmath>

static const char *kRankedHands[] =
	{
		"AA",
		"KK",
		"QQ",
		"AKs",
		"JJ",
		"AQs",
		"KQs",
		"AJs",
		"KJs",
		"TT",
		"AKo",
		"ATs",
		"KTs",
		"QJs",
		"QTs",
		"99",
		"JTs",
		"A9s",
		"AQo",
		"KQo",
		"88",
		"K9s",
		"A8s",
		"T9s",
		"Q9s",
		"AJo",
		"J9s",
		"77",
		"A5s",
		"A7s",
		"A4s",
		"KJo",
		"A3s",
		"A6s",
		"QJo",
		"66",
		"K8s",
		"A2s",
		"T8s",
		"98s",
		"J8s",
		"ATo",
		"K7s",
		"Q8s",
		"KTo",
		"55",
		"JTo",
		"QTo",
		"87s",
		"44",
		"22",
		"K6s",
		"97s",
		"76s",
		"K5s",
		"T7s",
		"K3s",
		"K4s",
		"K2s",
		"Q7s",
		"65s",
		"86s",
		"J7s",
		"54s",
		"Q6s",
		"33",
		"75s",
		"96s",
		"Q5s",
		"Q4s",
		"64s",
		"T9o",
		"Q3s",
		"T6s",
		"Q2s",
		"A9o",
		"85s",
		"53s",
		"J6s",
		"J9o",
		"K9o",
		"J5s",
		"Q9o",
		"43s",
		"J4s",
		"74s",
		"95s",
		"J3s",
		"63s",
		"J2s",
		"A8o",
		"T5s",
		"52s",
		"84s",
		"T4s",
		"T3s",
		"42s",
		"T2s",
		"98o",
		"T8o",
		"A5o",
		"A7o",
		"73s",
		"A4o",
		"32s",
		"94s",
		"J8o",
		"93s",
		"T7o",
		"A3o",
		"62s",
		"K8o",
		"92s",
		"A6o",
		"Q8o",
		"87o",
		"83s",
		"A2o",
		"82s",
		"72s",
		"97o",
		"K7o",
		"76o",
		"65o",
		"K6o",
		"54o",
		"86o",
		"K5o",
		"75o",
		"J7o",
		"K4o",
		"Q7o",
		"K2o",
		"K3o",
		"96o",
		"64o",
		"Q6o",
		"53o",
		"85o",
		"T6o",
		"43o",
		"Q5o",
		"Q3o",
		"Q4o",
		"Q2o",
		"74o",
		"J6o",
		"63o",
		"J5o",
		"95o",
		"52o",
		"J4o",
		"42o",
		"J3o",
		"J2o",
		"84o",
		"T5o",
		"32o",
		"T4o",
		"73o",
		"T3o",
		"T2o",
		"62o",
		"94o",
		"92o",
		"93o",
		"83o",
		"72o",
		"82o",
};
CASSERT_ARRAY_SIZE(kRankedHands, (int)13 * 13);

void BuildRangeFromRangePercentage(float startPercentage, float endPercentage, IRangeBuilder &rangeBuilder)
{
	if (startPercentage > endPercentage)
		std::swap(startPercentage, endPercentage);

	if (std::abs(startPercentage - endPercentage) < 0.00001f)
		return;

	float kMaxItems = ARRAY_SIZE(kRankedHands);
	int firstIndex = std::clamp<int>((int)(startPercentage * kMaxItems), 0, (int)(kMaxItems - 1));
	int lastIndex = std::clamp<int>((int)(endPercentage * kMaxItems), 0, (int)(kMaxItems - 1));

	for (int i = firstIndex; i <= lastIndex; i++)
	{
		const char *str = kRankedHands[i];

		CardRanks r1 = RankCharToIndex(str[0]);
		CardRanks r2 = RankCharToIndex(str[1]);

		if (r1 == r2)
		{
			for (int j = 0; j < kSuitCount - 1; j++)
				for (int k = j + 1; k < kSuitCount; k++)
				{
					rangeBuilder.AddHand(r1, (CardSuit)j, r2, (CardSuit)k, 1.0f);
				}
		}
		else if (str[2] == 's')
		{
			for (int j = 0; j < kSuitCount; j++)
				rangeBuilder.AddHand(r1, (CardSuit)j, r2, (CardSuit)j, 1.0f);
		}
		else
		{
			for (int j = 0; j < kSuitCount; j++)
				for (int k = 0; k < kSuitCount; k++)
				{
					if (j != k)
						rangeBuilder.AddHand(r1, (CardSuit)j, r2, (CardSuit)k, 1.0f);
				}
		}
	}
}
