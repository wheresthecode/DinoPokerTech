#include "PokerTech/pch.h"
#include <assert.h>
#include <sstream>
#include "PokerTech/PokerTypes.h"
#include "PokerTech/ComboUtilities.h"
#include "PreflopPrecomputeTableAPI.h"
#include "PreflopPrecomputeTable.h"

static std::vector<uint8_t> gSuitVariations;
static std::vector<uint16_t> gSuitVariationToIndex;

float FP16ToFloat(uint16_t v)
{
	return CLAMP((float)v / (float)0xffff, 0.0f, 1.0f);
}

static void LazyInitializeLookupTables()
{
	if (gSuitVariations.size() > 0)
		return;

	gSuitVariationToIndex.resize(32, -1); // 2 * 2 * 1

	int idx = 0;
	for (int a = 0; a < 2; a++)
	{
		int bMax = a + 1;
		for (int b = 0; b < bMax + 1; b++)
		{
			int cMax = std::max(a, b) + 1;
			for (int c = 0; c < cMax + 1; c++)
			{
				uint8_t result = a;
				result = ((result << 2) | b);
				result = ((result << 2) | c);
				gSuitVariationToIndex[result] = (uint16_t)gSuitVariations.size();
				gSuitVariations.push_back(result);
			}
		}
	}

	assert(kSuitVariationCount == gSuitVariations.size());
}

void SuitVariationMaskToSuitIndices(uint8_t m, FixedArray<CardSuit, 4> &outSuits)
{
	outSuits.resize_uninitialized(4);
	outSuits[3] = (CardSuit)(m & 0b11);
	outSuits[2] = (CardSuit)((m >> 2) & 0b11);
	outSuits[1] = (CardSuit)((m >> 4) & 0b1);
	outSuits[0] = (CardSuit)0;
}

void SuitVariationIndexToSuits(int idx, FixedArray<CardSuit, 4> &outSuits)
{
	uint8_t suitVariation = gSuitVariations[idx];
	SuitVariationMaskToSuitIndices(suitVariation, outSuits);
}

uint8_t GetSuitVariationMask(const CardSuit *suits)
{
	int suitToIndex[kSuitCount] = {-1, -1, -1, -1};
	int nextSuitIndex = 0;
	for (int i = 0; i < 4; i++)
	{
		if (suitToIndex[suits[i]] == -1)
			suitToIndex[suits[i]] = nextSuitIndex++;
	}

	// The first suit only has two options so it only needs one bit
	uint8_t result = suitToIndex[suits[1]];
	result = ((result << 2) | suitToIndex[suits[2]]);
	result = ((result << 2) | suitToIndex[suits[3]]);
	return result;
}

int GetSuitVariationIndex(const CardSuit *suits)
{
	uint16_t mask = GetSuitVariationMask(suits);
	assert(mask < gSuitVariationToIndex.size());
	return gSuitVariationToIndex[mask];
}

void CreateHandMatchupLUTs(int rankCount, RankMatchupToIndexMap *outRankMatchupToIndex, std::vector<PrecomputeHandMatchup> *outRankMatchupList)
{
	LazyInitializeLookupTables();
	// We have 13 choose 2 and then + all the pairs
	// 13 c 2 = 13! / (2! * (13-2)!) = 13 * 12 / 2 = 78
	// so 78 non-pairs + 13 pairs = 91 unique rank hands
	// now all combos of those unique rank hands:
	// 91 c 2 = 91 * 90 / 2 = 4095 + 91 = 4186
	const int kUniqueHands = (int)ComboCountNChooseR(rankCount, 2) + rankCount;
	const int kExpectedMatchups = (int)ComboCountNChooseR(kUniqueHands, 2) + kUniqueHands;

	if (outRankMatchupToIndex != NULL)
	{
		outRankMatchupToIndex->clear();
		outRankMatchupToIndex->reserve(kExpectedMatchups);
	}

	std::vector<PrecomputeHand> handRankCombos;
	for (int i = 0; i < rankCount; i++)
		for (int j = i; j < rankCount; j++)
		{
			handRankCombos.push_back({(CardRanks)i, (CardRanks)j});
		}

	const int handRankComboCount = (int)handRankCombos.size();
	int idx = 0;
	for (int i = 0; i < handRankComboCount; i++)
	{
		for (int j = i; j < handRankComboCount; j++)
		{
			PrecomputeHandMatchup mu = {handRankCombos[i], handRankCombos[j]};
			mu.ReorderForTable();
			if (outRankMatchupToIndex)
				(*outRankMatchupToIndex)[mu] = idx;
			if (outRankMatchupList)
				outRankMatchupList->push_back(mu);
			idx++;
		}
	}

	if (outRankMatchupToIndex != NULL)
	{
		assert(outRankMatchupToIndex->size() == idx);
	}
	assert(kExpectedMatchups == idx);
}

PreflopPrecomputeTable::PreflopPrecomputeTable(const std::string &filePath)
{
	LazyInitializeLookupTables();

	if (!ReadEntireFile(filePath.c_str(), m_RawData))
		throw std::runtime_error("Failed To Load Precompute File ");

	m_Header = (const PreflopPrecomputeTableHeader *)&m_RawData[0];

	if (m_Header->FileFormatCode != kPreflopPrecomputeFormatCode)
		throw std::runtime_error("Invalid Precompute File Format");

	if (m_Header->Version != kPreflopPrecomputeVersion)
		throw std::runtime_error("Invalid Preocmpute File Version");

	CreateHandMatchupLUTs(m_Header->RankCount, &m_RankMatchupToIndex, NULL);
}

PrecomputeResult PreflopPrecomputeTable::GetValue(const CardSet &p1, const CardSet &p2) const
{
	Card cards[]{p1[0], p1[1], p2[0], p2[1]};
	if (cards[0].Rank < cards[1].Rank)
		std::swap(cards[0], cards[1]);
	if (cards[2].Rank < cards[3].Rank)
		std::swap(cards[2], cards[3]);

	bool shouldSwapForLookup = (cards[0].Rank < cards[2].Rank) || ((cards[0].Rank == cards[2].Rank) && cards[1].Rank < cards[3].Rank);

	if (shouldSwapForLookup)
	{
		std::swap(cards[0], cards[2]);
		std::swap(cards[1], cards[3]);
	}

	CardSuit suits[]{cards[0].Suit, cards[1].Suit, cards[2].Suit, cards[3].Suit};

	uint8_t suitVariationIndex = GetSuitVariationIndex(suits);

	assert(m_Header->PrecisionBitCount == 16);
	PrecomputeHandMatchup mu;
	mu.h1.r1 = cards[0].Rank;
	mu.h1.r2 = cards[1].Rank;
	mu.h2.r1 = cards[2].Rank;
	mu.h2.r2 = cards[3].Rank;
	int matchupSize = ALIGN_SIZE(kValuesPerMatchup * (m_Header->PrecisionBitCount / 8) * kSuitVariationCount, 4);
	int matchupIndex = m_RankMatchupToIndex.find(mu)->second;
	int matchupOffset = matchupSize * matchupIndex;

	uint16_t *d = (uint16_t *)&m_RawData[m_Header->DataOffset + matchupSize * matchupIndex];
	d = d + kValuesPerMatchup * suitVariationIndex;
	PrecomputeResult r;
	float p1Value = FP16ToFloat(d[shouldSwapForLookup ? 1 : 0]);
	float p2Value = FP16ToFloat(d[shouldSwapForLookup ? 0 : 1]);

	/*
	I.E.
	10% to win or tie, and 25% p1 wins, and 65% p2 wins
	in the precompute table it will have 35% and 75%
	we then extract out the tie by subtracting the sum from 100:
	35+75 - 100 = 10
	*/
	float tiePercent = ((p1Value + p2Value) - 1.0f);
	r.p1WinEquity = p1Value - tiePercent;
	r.p2WinEquity = p2Value - tiePercent;
	r.tie = tiePercent;
	return r;
}

PreflopPrecomputeTable *PreflopPrecomputeTable_Create(const std::string &filePath)
{
	return new PreflopPrecomputeTable(filePath);
}

PrecomputeResult PreflopPrecomputeTable_GetValue(const PreflopPrecomputeTable *table, const CardSet &p1, const CardSet &p2)
{
	return table->GetValue(p1, p2);
}
void PreflopPrecomputeTable_Destroy(PreflopPrecomputeTable *table)
{
	delete table;
}