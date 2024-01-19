#include "pch.h"
#include <tuple>
#include "../PokerTech/PokerUtilities.h"
#include <algorithm>


TEST(IsValidBoardString, SuccessCases)
{
	const char* allCases[] = {
	"",
	"   ",
	"AcAsKd",
	"AcAsKd2s",
	"AcAsKd2s3d",
	"Ac As Kd "
	};

	for (int i = 0; i < ARRAY_SIZE(allCases); i++)
		EXPECT_TRUE(IsValidBoardString(allCases[i]));
}

TEST(IsValidBoardString, FailureCases)
{
	const char* allCases[] = {
	"A",
	"Ac2d3sQ",
	"Q",
	"~",
	"Ac2d",
	"Ac2d3sAc",
	"Ac",
	"Ac2d3c4d5s6h"
	};

	for (int i = 0; i < ARRAY_SIZE(allCases); i++)
		EXPECT_FALSE(IsValidBoardString(allCases[i]));
}

TEST(Fixed16Ratio, Fixed16Tests)
{
	float floats[] = { 0.0f, 0.5f, 1.0f };
	Fixed16Ratio fixedValues[ARRAY_SIZE(floats)];
	
	for (int i = 0; i < ARRAY_SIZE(fixedValues); i++)
	{
		fixedValues[i] = floats[i];
		float convertedBack = fixedValues[i];
		EXPECT_NEAR(floats[i], convertedBack, 0.0001f);
	}

	Fixed16Ratio big = 1.1f;
	Fixed16Ratio sm = -1.0f;
	float bigF = big;
	float smF = sm;
	EXPECT_FLOAT_EQ(1.0f, bigF);
	EXPECT_FLOAT_EQ(0.0f, smF);
}