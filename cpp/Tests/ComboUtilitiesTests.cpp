#include "pch.h"
#include "../PokerTech/ComboUtilities.h"

TEST(ComboUtilities, Combos)
{
	std::set<CardMask> combos;
	CreateRunoutCombos(0, 1, combos);
	EXPECT_EQ(ComboCountNChooseR(52, 1), combos.size());

	CreateRunoutCombos(0, 2, combos);
	EXPECT_EQ(ComboCountNChooseR(52, 2), combos.size());

	for (RunoutIterator i(0, 2); !i.IsDone(); i.Next())
	{
		EXPECT_EQ(1, combos.count(i.GetMask()));
		combos.erase(i.GetMask());
	}
	EXPECT_EQ(0, combos.size());
}

TEST(ComboUtilities, WhenRIsZero_HasOneRunout)
{
	std::set<CardMask> combos;
	CreateRunoutCombos(0, 0, combos);
	EXPECT_EQ(ComboCountNChooseR(52, 0), combos.size());
	EXPECT_EQ(1, combos.size());

	for (RunoutIterator i(0, 0); !i.IsDone(); i.Next())
	{
		EXPECT_EQ(1, combos.count(i.GetMask()));
		combos.erase(i.GetMask());
	}
	EXPECT_EQ(0, combos.size());
}
