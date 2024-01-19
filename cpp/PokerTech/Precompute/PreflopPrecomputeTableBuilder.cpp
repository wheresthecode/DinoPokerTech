#include "PokerTech/pch.h"

#if !EMSCRIPTEN

#include "PreflopPrecomputeTableBuilder.h"
#include "PreflopPrecomputeTable.h"
#include "PokerTech/PokerTypes.h"
#include "PokerTech/ComboUtilities.h"
#include "PokerTech/EquityCalculator/EquityCalculator.h"
#include <sstream>

// Preload Table Subfile Creator
// Because precomputing will take so long, we need to do it in pieces
// We have 13 choose 2 and then + all the pairs
// 13 c 2 = 13! / (2! * (13-2)!) = 13 * 12 / 2 = 78
// so 78 non-pairs + 13 pairs = 91 unique rank hands
// now all combos of those unique rank hands:
// 91 c 2 = 91 * 90 / 2 = 4095 + 13 = 4108
// now if we have 24 suit combinations 2*3*4
// we will need 24 * 4108 = 98592 precomputes
// at .2 seconds each. It will run for ~5.4 hours

// start by precomputing every combo into an array. will be 4108
// now walk through each one and check to see if a cached file exists for it.
// if not, process the result for that file. write it to a tmp file and move it to the cache folder
// repeat.
// once all the files are created (4108 files), create the final precompute data
// precompute table size = 4108 * 24 * 2 * 2 = 394k

uint16_t FloatToFP16(float f)
{
	return (uint16_t)std::min((int)((float)0xffff * f), 0xffff);
}

struct RankMatchupPrecomputeResult
{
	PrecomputeResult Results[kSuitVariationCount];
};

PrecomputeResult PrecomputeSingleRankMatchup(const PrecomputeHandMatchup &rankMatchup, int suitVariationIndex)
{
	FixedArray<CardSuit, 4> suits;
	SuitVariationIndexToSuits(suitVariationIndex, suits);

	EquityCalculationInput input;
	input.expandedPlayerRanges.resize(2);
	CardMask h1 = CardMask(suits[0], rankMatchup.h1.r1) | CardMask(suits[1], rankMatchup.h1.r2);
	CardMask h2 = CardMask(suits[2], rankMatchup.h2.r1) | CardMask(suits[3], rankMatchup.h2.r2);
	input.expandedPlayerRanges[0].push_back(h1);
	input.expandedPlayerRanges[1].push_back(h2);

	PrecomputeResult r{0, 0};
	if ((h1 | h2).Count() == 4)
	{
		EquityCalculationOutput output;
		CalculateEquityThreaded(input, output);

		r.p1WinEquity = output.playerResults[0].eq.win;
		r.p2WinEquity = output.playerResults[1].eq.win;
		r.tie = output.playerResults[0].eq.tie;
	}
	return r;
}

RankMatchupPrecomputeResult PrecomputeRankMatchup(const PrecomputeHandMatchup &rankMatchup)
{
	RankMatchupPrecomputeResult r;
	for (int i = 0; i < kSuitVariationCount; i++)
	{
		r.Results[i] = PrecomputeSingleRankMatchup(rankMatchup, i);
	}
	return r;
}

std::string GetCachedFilenameForMatch(const PrecomputeHandMatchup &matchup)
{
	std::ostringstream out;
	out << "data_" << (int)matchup.h1.r1 << "_" << (int)matchup.h1.r2 << "v" << (int)matchup.h2.r1 << "_" << (int)matchup.h2.r2 << ".bin";
	return out.str();
}

std::string GetCachedPathForMatch(const std::string &cacheDirectory, const PrecomputeHandMatchup &matchup)
{
	return cacheDirectory + std::string("\\") + GetCachedFilenameForMatch(matchup);
}

RankMatchupPrecomputeResult ComputeHandMatch(std::string cachingDirectory, const PrecomputeHandMatchup &matchup)
{
	bool supportCaching = !cachingDirectory.empty();
	RankMatchupPrecomputeResult r;
	std::string cachePath;
	if (supportCaching)
	{
		cachePath = GetCachedPathForMatch(cachingDirectory, matchup);
		std::vector<uint8_t> cachedData;
		if (ReadEntireFile(cachePath.c_str(), cachedData))
		{
			assert(cachedData.size() == sizeof(RankMatchupPrecomputeResult));
			return *((RankMatchupPrecomputeResult *)&cachedData[0]);
		}
	}

	r = PrecomputeRankMatchup(matchup);

	if (supportCaching)
	{
		bool cacheWriteSuccess = WriteDataToFile(cachePath.c_str(), &r, sizeof(r));
		assert(cacheWriteSuccess);
	}

	return r;
}

bool CreatePrecomputeFile(const std::string &filename, int bitsPerValue, int rankCount, const std::vector<RankMatchupPrecomputeResult> &results)
{
	const int kValuesPerMatchup = 2;
	const int kBitsPerMatchup = ALIGN_SIZE(kSuitVariationCount * kValuesPerMatchup * bitsPerValue, 8);
	const int kBytesPerMatchup = kBitsPerMatchup / 8;
	const int kLUTSize = (int)results.size() * kBytesPerMatchup;
	const int kHeaderSize = ALIGN_SIZE(sizeof(PreflopPrecomputeTableHeader), 4);

	std::vector<uint8_t> fileOut;
	fileOut.resize(kHeaderSize + kLUTSize);
	memset(&fileOut[0], 0, fileOut.size());

	PreflopPrecomputeTableHeader &header = *(PreflopPrecomputeTableHeader *)&fileOut[0];
	header.FileFormatCode = kPreflopPrecomputeFormatCode;
	header.Version = kPreflopPrecomputeVersion;
	header.DataSize = kLUTSize;
	header.DataOffset = kHeaderSize;
	header.PrecisionBitCount = bitsPerValue;
	header.RankCount = rankCount;
	header.MatchupCount = results.size();

	assert(bitsPerValue == 16);
	for (int i = 0; i < results.size(); i++)
	{
		uint16_t *offset = (uint16_t *)&fileOut[kHeaderSize + i * kBytesPerMatchup];
		int fpIndex = 0;
		for (int j = 0; j < kSuitVariationCount; j++)
		{
			const PrecomputeResult &r = results[i].Results[j];
			offset[fpIndex++] = FloatToFP16(r.p1WinEquity + r.tie);
			offset[fpIndex++] = FloatToFP16(r.p2WinEquity + r.tie);
		}
	}

	return WriteDataToFile(filename.c_str(), &fileOut[0], fileOut.size());
}

// Changing the rankCount could be used in a short deck version of the game. It is also useful for
// testing that the precompute system works properly on a smaller set of data
bool PrecomputeCalculateProcess(const std::string &outputPrecomputeFile, const std::string &cacheDirectory, int bitsPerValue, int rankCount, PrecomputeProgressCallback callback)
{
	if (!cacheDirectory.empty())
		CreateDirectory(cacheDirectory.c_str());

	RankMatchupToIndexMap rankMatchupToIndex;
	std::vector<PrecomputeHandMatchup> handMatchups;
	CreateHandMatchupLUTs(rankCount, &rankMatchupToIndex, &handMatchups);

	std::vector<RankMatchupPrecomputeResult> preComputeResults;
	preComputeResults.resize(handMatchups.size());
	for (int i = 0; i < handMatchups.size(); i++)
	{
		if (callback)
			callback(i, (int)handMatchups.size());
		preComputeResults[i] = ComputeHandMatch(cacheDirectory, handMatchups[i]);
	}

	std::vector<uint8_t> outData;
	return CreatePrecomputeFile(outputPrecomputeFile, bitsPerValue, rankCount, preComputeResults);
}

#endif