#include "pch.h"
#include <tuple>
#include "../PokerTech/PokerUtilities.h"
#include "../PokerTech/HandRange.h"
#include "../PokerTech/StringUtility.h"
#include "../PokerTech/EnabledRange.h"
#include <algorithm>
#include "../PokerTech/RangeConverters/RangeBuilderSyntaxString.h"

using namespace std;

static HandRange RangeFromStringTest(std::string rangeString)
{
	HandRange r;
	EXPECT_TRUE(BuildRangeFromHandSyntaxString(rangeString, r));
	return r;
}

std::string RangeToExplicitHands(const HandRange &r)
{
	std::vector<CardMask> v(r.begin(), r.end());
	std::sort(v.begin(), v.end());
	std::vector<std::string> s;
	s.reserve(v.size());
	for (int i = 0; i < v.size(); i++)
	{
		s.push_back(CardMaskToString(v[i]));
	}
	return StringsJoin(", ", s);
}

void ExpectRangesEqual(const HandRange &expected, const HandRange &actual, const std::string &testName)
{
	bool match = expected.size() == actual.size();

	if (match)
	{
		for (auto it = expected.begin(); it != expected.end(); ++it)
			match = match && (actual.Contains(*it));
	}

	if (!match)
	{
		std::string eString = RangeToExplicitHands(expected);
		std::string aString = RangeToExplicitHands(actual);
		std::cout << "Test:" << testName << std::endl;
		std::cout << "Expected Range (" << expected.size() << "): " << eString << std::endl;
		std::cout << "Actual Range (" << actual.size() << "): " << aString << std::endl;
	}
	ASSERT_TRUE(match);
}

static const char *pairStrings[] = {"AA", "KK", "QQ", "JJ", "TT", "99", "88", "77", "66", "55", "44", "33", "22"};

TEST(ExpandRangeString, Pairs)
{
	HandRange totalRange;

	for (int i = 0; i < ARRAY_SIZE(pairStrings); i++)
	{
		HandRange nonPlusRange = RangeFromStringTest(std::string(pairStrings[i]));
		HandRange plusRange = RangeFromStringTest(std::string(pairStrings[i]) + "+");

		EXPECT_EQ(6, nonPlusRange.size());
		totalRange.AddRange(nonPlusRange);

		ExpectRangesEqual(totalRange, plusRange, std::string("Pair") + pairStrings[i]);
		ASSERT_EQ(6 * (i + 1), (int)plusRange.size());
	}
}

void MinFirst(int &i1, int &i2)
{
	int t = min(i1, i2);
	i2 = max(i1, i2);
	i1 = t;
}

static void ExpectPairRange(int pairStringIndex1, int pairStringIndex2)
{
	std::ostringstream rangeStream;
	rangeStream << pairStrings[pairStringIndex1] << "-" << pairStrings[pairStringIndex2];
	std::ostringstream rangeStreamReversed;
	rangeStreamReversed << pairStrings[pairStringIndex2] << "-" << pairStrings[pairStringIndex1];
	HandRange range = RangeFromStringTest(rangeStream.str());
	HandRange rangeReversed = RangeFromStringTest(rangeStream.str());
	ExpectRangesEqual(range, rangeReversed, rangeStream.str());

	MinFirst(pairStringIndex1, pairStringIndex2);
	HandRange expectedRange;
	for (int i = pairStringIndex1; i <= pairStringIndex2; i++)
	{
		HandRange thisPairRange = RangeFromStringTest(std::string(pairStrings[i]));
		expectedRange.AddRange(thisPairRange);
	}

	ASSERT_EQ(((pairStringIndex2 - pairStringIndex1 + 1) * 6), (int)range.size());
	ExpectRangesEqual(expectedRange, range, rangeStream.str());
}

TEST(ExpandRangeString, PairRange)
{
	for (int i = 0; i < ARRAY_SIZE(pairStrings); i++)
		for (int j = 0; j < ARRAY_SIZE(pairStrings); j++)
			ExpectPairRange(i, j);
}

struct RangeStringExpandText
{
	const char *RangeStr;
	HandRange Range;
};

TEST(ExpandRangeString, GeneralTests)
{
	RangeStringExpandText allCases[] = {
		{"AcTc+", HandRange({HAND(AcTc), HAND(AcJc), HAND(AcQc), HAND(AcKc)})},
		{"Kc9c+", HandRange({HAND(KcQc), HAND(KcJc), HAND(KcTc), HAND(Kc9c)})},
		{"AcQ+", HandRange({HAND(AcQc), HAND(AcQh), HAND(AcQs), HAND(AcQd), HAND(AcKc), HAND(AcKh), HAND(AcKs), HAND(AcKd)})},
		{"A.Kc", HandRange({HAND(AcKc), HAND(AsKc), HAND(AhKc), HAND(AdKc)})},
		{"Ac2-5c", HandRange({HAND(Ac2c), HAND(Ac3c), HAND(Ac4c), HAND(Ac5c)})},
		{"AQ+", HandRange(
					{HAND(AcQc), HAND(AcQh), HAND(AcQs), HAND(AcQd), HAND(AcKc), HAND(AcKh), HAND(AcKs), HAND(AcKd),
					 HAND(AdQc), HAND(AdQh), HAND(AdQs), HAND(AdQd), HAND(AdKc), HAND(AdKh), HAND(AdKs), HAND(AdKd),
					 HAND(AhQc), HAND(AhQh), HAND(AhQs), HAND(AhQd), HAND(AhKc), HAND(AhKh), HAND(AhKs), HAND(AhKd),
					 HAND(AsQc), HAND(AsQh), HAND(AsQs), HAND(AsQd), HAND(AsKc), HAND(AsKh), HAND(AsKs), HAND(AsKd)})},
		{"T8s+", HandRange({HAND(Tc8c), HAND(Tc9c), HAND(Ts8s), HAND(Ts9s), HAND(Th8h), HAND(Th9h), HAND(Td8d), HAND(Td9d)})},
		{"AcKc+", HandRange({HAND(AcKc)})},
		{"98o", HandRange(
					{
						HAND(9c8h),
						HAND(9c8s),
						HAND(9c8d),
						HAND(9h8c),
						HAND(9h8s),
						HAND(9h8d),
						HAND(9s8c),
						HAND(9s8h),
						HAND(9s8d),
						HAND(9d8s),
						HAND(9d8c),
						HAND(9d8h),
					})},
		{"AcAd", HandRange({HAND(AcAd)})},
		{"AA", HandRange({HAND(AcAd), HAND(AcAs), HAND(AcAh), HAND(AdAs), HAND(AdAh), HAND(AsAh)})},
		{"AcJ", HandRange({HAND(AcJc), HAND(AcJd), HAND(AcJh), HAND(AcJs)})},
		{"AJs", HandRange({HAND(AcJc), HAND(AdJd), HAND(AhJh), HAND(AsJs)})},
		{"AcKc,AsKs", HandRange({HAND(AcKc), HAND(AsKs)})},
		{"AcKc, AsKs", HandRange({HAND(AcKc), HAND(AsKs)})},
		{"Ac*c", HandRange({HAND(AcKc), HAND(AcQc), HAND(AcJc), HAND(AcTc), HAND(Ac9c), HAND(Ac8c), HAND(Ac7c), HAND(Ac6c), HAND(Ac5c), HAND(Ac4c), HAND(Ac3c), HAND(Ac2c)})},
		{"AshKsho", HandRange({HAND(AsKh), HAND(AhKs)})}};

	for (int i = 0; i < ARRAY_SIZE(allCases); i++)
	{
		RangeStringExpandText &e = allCases[i];
		HandRange actual = RangeFromStringTest(e.RangeStr);
		ExpectRangesEqual(e.Range, actual, e.RangeStr);
	}
}

TEST(ExpandRangeString, FailureCases)
{
	const char *allCases[] = {
		"Ac+Tc+",
		"Ac10c",
		"2c2s++",
		"2c-2s++",
		"2cAs2d",
		"AsAs",
		"AcAs,AsAs",
		"%20",
		"20-40-2%",
		"-20%",
		"a%",
	};
	HandRange r;
	for (int i = 0; i < ARRAY_SIZE(allCases); i++)
		EXPECT_FALSE(BuildRangeFromHandSyntaxString(allCases[i], r)) << allCases[i];
}

struct PercentTest
{
	const char *input;
	const char *expected;
	const char *notExpected;
	int exactExpectedHands;
};

TEST(ExpandRangeString, PercentageTests)
{
	PercentTest cases[] = {
		{"0%", "", "AA, 72,22", 0},
		{"10%", "AA,KK,QQ,AKs", "72,22", -1},
		{"100%", "AA,KK,QQ,AKs,72", "", kTotalRangeEntries},
		{"10-20%", "KQo", "AA,72,22", -1},
		{"20-10%", "10-20%", "AA,72,22", -1},
	};

	for (int i = 0; i < ARRAY_SIZE(cases); i++)
	{
		HandRange r;
		HandRange notExpected;
		HandRange expected;
		const PercentTest &x = cases[i];

		BuildRangeFromHandSyntaxString(x.input, r);
		BuildRangeFromHandSyntaxString(x.notExpected, notExpected);
		BuildRangeFromHandSyntaxString(x.expected, expected);
		
		for (auto it = expected.begin(); it != expected.end(); it++)
		{
			ASSERT_TRUE(r.Contains(*it)) << "Test " << i << ": " << x.input << " Was not in expected list: " << CardMaskToString(*it) << " List contains: " << RangeToExplicitHands(r) << "  Expected:" << RangeToExplicitHands(expected);
		}
		for (auto it = notExpected.begin(); it != notExpected.end(); it++)
		{
			ASSERT_FALSE(r.Contains(*it)) << "Test " << i << ": " << x.input;
		}
		if (x.exactExpectedHands != -1)
			ASSERT_EQ(x.exactExpectedHands, r.size()) << "Test " << i << ": " << x.input;
	}
}