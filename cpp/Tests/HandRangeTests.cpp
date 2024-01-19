#include "pch.h"
#include "../PokerTech/EnabledRange.h"
#include "../PokerTech/PokerUtilities.h"
#include "../PokerTech/RangeConverters/RangeBuilderSyntaxString.h"

TEST(HandRangeTests, BlockIndexCalculator)
{

	int curIndex = 0;
	for (int c1 = 0; c1 < kRankCardCount; c1++)
	{
		for (int c2 = c1 + 1; c2 < kRankCardCount; c2++)
		{
			ASSERT_EQ(curIndex, GetBlockIndex((CardRanks)c1, (CardRanks)c2, NULL));
			curIndex += kNonPairCombosPerHand;
		}
	}
	for (int c1 = 0; c1 < kRankCardCount; c1++)
	{
		ASSERT_EQ(curIndex, GetBlockIndex((CardRanks)c1, (CardRanks)c1, NULL));
		curIndex += kPairCombosPerHand;
	}
}

TEST(HandRangeTests, TestAllIndexCalculations)
{

	std::set<int> visited;
	for (int s1 = 0; s1 < kSuitCount; s1++)
	{
		for (int s2 = 0; s2 < kSuitCount; s2++)
		{
			int idx = FRF_GetIndexWithinBlock(false, (CardSuit)s1, (CardSuit)s2);
			EXPECT_EQ(0, visited.count(idx));
			visited.insert(idx);
			CardSuit rs1, rs2;
			FRF_IndexToSuits(false, idx, rs1, rs2);
			EXPECT_EQ(s1, rs1);
			EXPECT_EQ(s2, rs2);
		}
	}

	visited.clear();
	for (int s1 = 0; s1 < kSuitCount - 1; s1++)
	{
		for (int s2 = s1 + 1; s2 < kSuitCount; s2++)
		{
			int idx = FRF_GetIndexWithinBlock(true, (CardSuit)s1, (CardSuit)s2);
			EXPECT_EQ(0, visited.count(idx));
			visited.insert(idx);
			CardSuit rs1, rs2;
			FRF_IndexToSuits(true, idx, rs1, rs2);
			EXPECT_EQ(s1, rs1);
			EXPECT_EQ(s2, rs2);
		}
	}
}

TEST(HandRangeToSyntax, EnabledRangeToStringPairs)
{
	const char *syntaxTests[] =
		{
			"KcKd+",
			"AcAs",
			"77-99,33-55",
			"22-44",
			"22+",
			"AA",
			"KK+",
			"33-55",
			"AcA.",
			"AsA.",
			"AdA.",
			"AhA."};

	for (int i = 0; i < ARRAY_SIZE(syntaxTests); i++)
	{
		EnabledRange range;
		BuildRangeFromHandSyntaxString(syntaxTests[i], range);
		std::string result = range.CalculateRangeSyntaxString();
		ASSERT_EQ(syntaxTests[i], result);
	}
}

TEST(HandRangeToSyntax, EnabledRangeToStringNonPairs)
{
	// Create a EnabledRange. Add some hands. Convert to syntax string
	const char *syntaxTests[] =
		{
			"32",
			"AK",
			"AKs",
			"AKo",
			"AQ+",
			"AcK.",
			"AdK.",
			"AhK.",
			"AsK.",
			"A.Kc",
			"A.Kd",
			"A.Kh",
			"A.Ks",
			"AcshK.",
			"A.Kcsh",
			"AQ-J",
			"AK,AQ-Js",
			"KJo+",
			"KQ",
			"A2+",
			"AQ+,AT-2",
			"AcKch,AsKs",
			"7h2h"};

	for (int i = 0; i < ARRAY_SIZE(syntaxTests); i++)
	{
		EnabledRange range;
		BuildRangeFromHandSyntaxString(syntaxTests[i], range);
		std::string result = range.CalculateRangeSyntaxString();
		ASSERT_EQ(syntaxTests[i], result);
	}
}

void GenerateRandomRange(EnabledRange &range, float saturation)
{
	int maxRangeEntries = range.m_Range.Count();
	std::vector<int> a(maxRangeEntries);
	for (int i = 0; i < range.m_Range.Count(); i++)
		a[i] = i;

	int toAdd = std::min(maxRangeEntries, (int)((float)maxRangeEntries * saturation));

	for (int i = 0; i < toAdd; i++)
	{
		int j = rand() % a.size();
		range.m_Range.m_Data[a[j]].FromFloat(1.0f);
		a[j] = a[a.size() - 1];
		a.pop_back();
	}
}

void AssertRangesEqual(const EnabledRange &expected, const EnabledRange &actual, std::string &rangeString)
{
	bool pass = true;
	for (int i = 0; i < kRankCardCount; i++)
		for (int j = i; i < kRankCardCount; i++)
		{
			float ratios1[16];
			float ratios2[16];
			int r1Count = expected.GetRatioBlock((CardRanks)i, (CardRanks)j, ratios1);
			int r2Count = actual.GetRatioBlock((CardRanks)i, (CardRanks)j, ratios2);

			for (int k = 0; k < r1Count; k++)
			{
				bool equal = abs(ratios1[k] - ratios2[k]) < 0.001f;
				if (!equal)
				{
					if (pass)
					{
						std::cout << "Ranges were not equal. Range string " << rangeString << std::endl;
					}
					CardSuit s1, s2;
					FRF_IndexToSuits(i == j, k, s1, s2);
					std::cout << "Rank block didn't match:" << CardRankToChar((CardRanks)i) << CardRankToChar((CardRanks)j) << std::endl;
					std::cout << "Suit '" << CardSuitToChar(s1) << CardSuitToChar(s2) << "' was '" << ratios2[k] << "' expected '" << ratios1[k] << "'" << std::endl;
					pass = false;
				}
			}
		}

	EXPECT_TRUE(pass);
}

TEST(HandRangeToSyntax, StressConversions)
{
	for (int i = 0; i < 10; i++)
	{
		srand(i);
		EnabledRange r1;
		EnabledRange reversedR1;
		GenerateRandomRange(r1, (float)(i + 1) / 10.0f);

		std::string r1String = r1.CalculateRangeSyntaxString();
		BuildRangeFromHandSyntaxString(r1String, reversedR1);

		AssertRangesEqual(r1, reversedR1, r1String);
	}
}