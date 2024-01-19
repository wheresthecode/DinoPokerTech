#include "pch.h"

#include "../PokerTech/Precompute/PreflopPrecomputeTableAPI.h"
#include "../PokerTech/Precompute/PreflopPrecomputeTableBuilder.h"
#include "../PokerTech/PokerUtilities.h"
#include "../PokerTech/EquityCalculator/EquityCalculator.h"

TEST(PreflopPrecomputeTests, CanCreateAndLoad)
{
	// Create a mini table
	int rankCount = 2;
	int bitsPerValue = 16;
	float acceptableRoundingError = 2.0f / bitsPerValue;

	std::function<void(int, int)> callback = [](int complete, int total)
	{
		// std::cout << complete << "/" << total << std::endl;
	};

	EXPECT_TRUE(PrecomputeCalculateProcess("tempFile.bin", "cache", bitsPerValue, rankCount, callback));

	PreflopPrecomputeTable *table = PreflopPrecomputeTable_Create("tempFile.bin");

	for (int p1R1 = 0; p1R1 < rankCount; p1R1++)
		for (int p1R2 = p1R1; p1R2 < rankCount; p1R2++)
			for (int p2R1 = 0; p2R1 < rankCount; p2R1++)
				for (int p2R2 = p2R1; p2R2 < rankCount; p2R2++)
				{
					CardSet set1, set2;
					set1.push_back({CardSuit::Suit_Club, (CardRanks)p1R1});
					set1.push_back({CardSuit::Suit_Diamond, (CardRanks)p1R2});
					set2.push_back({CardSuit::Suit_Spade, (CardRanks)p2R1});
					set2.push_back({CardSuit::Suit_Heart, (CardRanks)p2R2});
					PrecomputeResult preComputeResult = PreflopPrecomputeTable_GetValue(table, set1, set2);

					EquityCalculationInput input;
					EquityCalculationOutput output;
					input.expandedPlayerRanges.resize(2);
					input.expandedPlayerRanges[0].push_back(set1.ToMask());
					input.expandedPlayerRanges[1].push_back(set2.ToMask());
					CalculateEquity(input, output);

					EXPECT_NEAR(output.playerResults[0].eq.win, preComputeResult.p1WinEquity, acceptableRoundingError);
					EXPECT_NEAR(output.playerResults[1].eq.win, preComputeResult.p2WinEquity, acceptableRoundingError);
					EXPECT_NEAR(output.playerResults[0].eq.tie, preComputeResult.tie, acceptableRoundingError);
					EXPECT_NEAR(output.playerResults[1].eq.tie, preComputeResult.tie, acceptableRoundingError);
				}

	PreflopPrecomputeTable_Destroy(table);
}