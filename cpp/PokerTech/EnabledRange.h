#pragma once
#include <assert.h>
#include "PokerTypes.h"
#include <iostream>
#include "PokerUtilities.h"
#include <cstring>

const int kTotalNonPairRankCombos = 13 * 6;
const int kTotalPairEntries = 13;
const int kPairCombosPerHand = 6;
const int kNonPairCombosPerHand = 16;
const int kTotalPairCombos = kTotalPairEntries * kPairCombosPerHand;
const int kTotalNonPairCombos = kTotalNonPairRankCombos * kNonPairCombosPerHand;
const int kTotalRangeEntries = kTotalPairCombos + kTotalNonPairCombos;

struct H2
{
	H2(Card c1, Card c2)
	{
		m_c1 = c1;
		m_c2 = c2;

		if (c1.Rank < c2.Rank)
		{
			std::swap(m_c1, m_c2);
		}
		else if (c1.Rank == c2.Rank && c1.Suit < c2.Suit)
		{
			std::swap(m_c1, m_c2);
		}
		else if (c1 == c2)
		{
			assert(false);
		}
	}

	Card C1() { return m_c1; }
	Card C2() { return m_c2; }

	std::string GetName();

	bool operator==(const H2 &other)
	{
		return m_c1 == other.m_c1 && m_c2 == other.m_c2;
	}

private:
	Card m_c1;
	Card m_c2;
};

// FullRangeFormat is a fixed sized block of memory that contains data
// for every combo. The data is grouped together by ranks and suits. For example,
// the data for all AK combos are stored consecutively with the suited combos first

const int FRF_RankBlockSuitedIndex = 0;
const int FRF_RankBlockUnustedIndex = 4;

inline void FRF_IndexToSuits(bool isPair, int idx, CardSuit &s1, CardSuit &s2)
{
	if (isPair)
	{
		if (idx < 3)
		{
			s1 = (CardSuit)0;
			s2 = (CardSuit)(idx + 1);
		}
		else if (idx == 3 || idx == 4)
		{
			s1 = (CardSuit)1;
			s2 = (CardSuit)(2 + (idx - 3));
		}
		else if (idx == 5)
		{
			s1 = (CardSuit)2;
			s2 = (CardSuit)3;
		}
	}
	else
	{
		s1 = (CardSuit)(idx / 4);
		s2 = (CardSuit)(idx % 4);
	}
}

inline int FRF_GetIndexWithinBlock(bool isPair, CardSuit s1, CardSuit s2)
{
	if (isPair)
	{
		if (s1 > s2)
			std::swap(s1, s2);
		if (s1 == 0)
			return s2 - 1;
		if (s1 == 1)
			return 3 + s2 - 2;
		return 5;
	}
	else
	{
		return s1 * 4 + s2;
	}
}

inline int GetBlockIndex(CardRanks r1, CardRanks r2, int *outCount)
{
	if (r1 > r2)
		std::swap(r1, r2);

	if (outCount != NULL)
		*outCount = r1 == r2 ? 6 : 16;

	if (r1 == r2)
	{
		return kTotalNonPairCombos + r1 * kPairCombosPerHand;
	}

	int entriesInPreviousRow = (kRankCardCount - r1);
	int entriesFirstRow = kRankCardCount - 1;
	int rowAboveCount = r1;

	int idx = ((entriesFirstRow + entriesInPreviousRow) * rowAboveCount) / 2;
	return (idx + (r2 - r1 - 1)) * kNonPairCombosPerHand;
}

inline int GetSpecificHandIndex(H2 hand)
{
	int bIdx = GetBlockIndex(hand.C1().Rank, hand.C2().Rank, NULL);
	int subIdx = FRF_GetIndexWithinBlock(hand.C1().Rank == hand.C2().Rank, hand.C1().Suit, hand.C2().Suit);
	return bIdx + subIdx;
}

enum RankComboCategory
{
	Combos_All,
	Combos_Suited,
	Combos_Unsuited
};

template <typename DATA>
class H2DataContainer
{
public:
	H2DataContainer()
	{
	}

	DATA &GetData(const H2 &h)
	{
		int idx = GetSpecificHandIndex(h);
		return m_Data[idx];
	}

	inline DATA *GetBlock(CardRanks r1, CardRanks r2, int &outCount)
	{
		bool isPair = r1 == r2;
		outCount = isPair ? 6 : 16;
		int blockOffset = GetBlockIndex(r1, r2, NULL);
		return &m_Data[blockOffset];
	}

	inline const DATA *GetBlock(CardRanks r1, CardRanks r2, int &outCount) const
	{
		bool isPair = r1 == r2;
		outCount = isPair ? 6 : 16;
		int blockOffset = GetBlockIndex(r1, r2, NULL);
		return &m_Data[blockOffset];
	}
	inline int Count() const { return kTotalRangeEntries; }
	DATA m_Data[kTotalRangeEntries];
};

class EnabledRange : public IRangeBuilder
{
public:
	H2DataContainer<Fixed16Ratio> m_Range;

	EnabledRange() {}

	// returns up to 16 ratios. 6 if ranks are equal
	int GetRatioBlock(CardRanks r1, CardRanks r2, float *outRatios) const
	{
		int count;
		const Fixed16Ratio *d = m_Range.GetBlock(r1, r2, count);
		for (int i = 0; i < count; i++)
			outRatios[i] = d[i].ToFloat();
		return count;
	}

	float GetRatio(H2 hand)
	{
		return m_Range.GetData(hand).ToFloat();
	}

	void SetCombo(H2 hand, float weight)
	{
		m_Range.GetData(hand).FromFloat(weight);
	}

	void Clear()
	{
		std::memset(m_Range.m_Data, 0, sizeof(m_Range.m_Data));
	}
	// Parse the range string markup and add it to the range
	bool ConvertFromRangeString(const std::string &range);

	std::string CalculateRangeSyntaxString();

	// Inherited via IRangeBuilder
	virtual void AddHand(CardRanks r1, CardSuit s1, CardRanks r2, CardSuit s2, float weight) override;

	void SetFromFloatArray(const float *floats, int count);
	void GetRangeAsFloatArray(float *floats, int count) const;
};
