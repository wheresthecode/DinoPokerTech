#pragma once

#include <unordered_map>
#include <functional>
#include "PokerTech/PokerUtilities.h"
#include "PokerTech/FixedArray.h"
#include "PreflopPrecomputeTableAPI.h"

const uint32_t kPreflopPrecomputeFormatCode = ((uint32_t)'P') << 24 | ((uint32_t)'R') << 16 | ((uint32_t)'E') << 8 | ((uint32_t)'T');
const uint32_t kPreflopPrecomputeVersion = 1;

const int kSuitVariationCount = 15;

struct PreflopPrecomputeTableHeader
{
	uint32_t FileFormatCode;
	uint32_t Version;
	uint32_t DataSize;
	uint32_t DataOffset;
	uint32_t PrecisionBitCount;
	uint32_t RankCount;
	uint32_t MatchupCount;
};

const int kValuesPerMatchup = 2;

struct PrecomputeHand
{
	CardRanks r1;
	CardRanks r2;
	bool operator==(const PrecomputeHand &other) const
	{
		return (r1 == other.r1) && (r2 == other.r2);
	}

	void ReorderForTable()
	{
		if (r1 < r2)
			std::swap(r1, r2);
	}
};

struct PrecomputeHandMatchup
{
	bool operator==(const PrecomputeHandMatchup &other) const
	{
		return (h1 == other.h1) && (h2 == other.h2);
	}
	PrecomputeHand h1;
	PrecomputeHand h2;

	bool ReorderForTable()
	{
		h1.ReorderForTable();
		h2.ReorderForTable();

		bool swap = (h1.r1 < h2.r1) || (h1.r1 == h2.r1 && h1.r2 < h2.r2);
		if (swap)
			std::swap(h1, h2);
		return swap;
	}
};

namespace std
{
	template <>
	struct hash<PrecomputeHandMatchup>
	{
		std::size_t operator()(const PrecomputeHandMatchup &k) const
		{
			size_t r;
			r = k.h1.r1;
			r = r * kRankCardCount + k.h1.r2;
			r = r * kRankCardCount + k.h2.r1;
			r = r * kRankCardCount + k.h2.r2;
			return r;
		}
	};
}
typedef std::unordered_map<PrecomputeHandMatchup, int> RankMatchupToIndexMap;

class PreflopPrecomputeTable
{
public:
	PreflopPrecomputeTable(const std::string &filePath);

	PrecomputeResult GetValue(const CardSet &p1, const CardSet &p2) const;

private:
	std::vector<uint8_t> m_RawData;
	const PreflopPrecomputeTableHeader *m_Header;

	RankMatchupToIndexMap m_RankMatchupToIndex;
};

// Use by building code
void SuitVariationIndexToSuits(int idx, FixedArray<CardSuit, 4> &outSuits);
void CreateHandMatchupLUTs(int rankCount, RankMatchupToIndexMap *outRankMatchupToIndex, std::vector<PrecomputeHandMatchup> *outRankMatchupList);
