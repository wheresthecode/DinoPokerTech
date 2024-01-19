#include "pch.h"
#include "ComboUtilities.h"

void CreateR(CardMask curMask, std::vector<CardMask> &n, int startIndex, int r, std::set<CardMask> &combos)
{
	if (r == 0)
	{
		combos.insert(curMask);
		return;
	}
	for (int i = startIndex; i < n.size() + 1 - r; i++)
	{
		CardMask board = curMask | n[i];
		CreateR(board, n, i + 1, r - 1, combos);
	}
}

std::vector<CardMask> CardMaskToIndividualCards(CardMask m)
{
	std::vector<CardMask> result;
	result.reserve(52);
	m = m & kValidCardMask;
	for (int i = 0; i < 64; i++)
	{
		uint64_t bitMask = (uint64_t)1 << i;
		if ((m & bitMask).v != 0)
		{
			result.push_back(bitMask);
		}
	}
	return result;
}

void CreateRunoutCombos(CardMask blockedCards, int cardsToRunOut, std::set<CardMask> &combos)
{
	combos.clear();
	std::vector<CardMask> n = CardMaskToIndividualCards(~blockedCards);
	CreateR(0, n, 0, cardsToRunOut, combos);
}

uint64_t factorial(uint64_t v)
{
	uint64_t result = 1;
	while (v > 0)
	{
		result *= v;
		v--;
	}
	return result;
}

uint64_t factorialCap(uint64_t v, int cap)
{
	uint64_t result = 1;
	while (v > 0 && cap > 0)
	{
		result *= v;
		v--;
		cap--;
	}
	return result;
}

uint64_t ComboCountNChooseR(int n, int r)
{
	// (n!) / (r! * (n-r)!)
	return factorialCap(n, r) / factorial(r);
}