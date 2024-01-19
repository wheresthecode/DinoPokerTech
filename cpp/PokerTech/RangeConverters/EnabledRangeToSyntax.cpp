#include "PokerTech/pch.h"
#include "PokerTech/EnabledRange.h"
#include "PokerTech/PokerUtilities.h"
#include "EnabledRangeToSyntax.h"
#include <sstream>

struct EnabledRangeToSyntaxMaps
{
public:
	EnabledRangeToSyntaxMaps()
	{
		kAll = (1 << 16) - 1;

		kAllSuited = 0;
		for (int i = 0; i < kSuitCount; i++)
			kAllSuited |= 1 << (FRF_GetIndexWithinBlock(false, (CardSuit)i, (CardSuit)i));

		kAllUnsuited = kAll & (~kAllSuited);

		kPairAll = (1 << 6) - 1;
		for (int i = 0; i < kSuitCount; i++)
		{
			kRowsSuited[i] = 0;
			kRowsUnsuited[i] = 0;
			kColsSuited[i] = 0;
			kColsUnsuited[i] = 0;
			kPairSuits[i] = 0;
			for (int j = 0; j < kSuitCount; j++)
			{
				kRowsSuited[i] |= 1 << FRF_GetIndexWithinBlock(false, (CardSuit)i, (CardSuit)j);
				kColsSuited[i] |= 1 << FRF_GetIndexWithinBlock(false, (CardSuit)j, (CardSuit)i);
				if (i != j)
				{
					kRowsUnsuited[i] |= 1 << FRF_GetIndexWithinBlock(false, (CardSuit)i, (CardSuit)j);
					kColsUnsuited[i] |= 1 << FRF_GetIndexWithinBlock(false, (CardSuit)j, (CardSuit)i);
					kPairSuits[i] |= 1 << FRF_GetIndexWithinBlock(true, (CardSuit)i, (CardSuit)j);
				}
			}
		}
	}
	uint32_t kAll;
	uint32_t kAllSuited;
	uint32_t kAllUnsuited;
	uint32_t kRowsSuited[kSuitCount];
	uint32_t kRowsUnsuited[kSuitCount];
	uint32_t kColsSuited[kSuitCount];
	uint32_t kColsUnsuited[kSuitCount];

	uint32_t kPairAll;
	uint32_t kPairSuits[kSuitCount];
};

EnabledRangeToSyntaxMaps gC;

uint32_t CreateBitfieldForHand(const EnabledRange &range, CardRanks r1, CardRanks r2)
{
	uint32_t result = 0;
	float ratios[16];
	int count = range.GetRatioBlock(r1, r2, ratios);
	for (int i = 0; i < count; i++)
	{
		if (ratios[i] > 0.5f)
			result |= (1 << i);
	}
	return result;
}

enum SuitStart
{
	SuitStart_None,
	SuitStart_All,
	SuitStart_Suited,
	SuitStart_Unsuited
};

enum SquareEntry
{
	SE_StartSuit,
	SE_RowSuited,
	SE_RowUnsuited,
	SE_ColSuited,
	SE_ColUnsuited,
	SE_Row1Singles,
	SE_Row2Singles,
	SE_Row3Singles,
	SE_Row4Singles,
	SE_COUNT
};

struct Square
{
	Square() : d() {}
	uint32_t d[SE_COUNT];
};

struct ParseState
{
	ParseState()
		: firstParse(true),
		  lastRankOfEntries()
	{
	}
	bool firstParse;
	Square lastSquare;
	CardRanks lastRankOfEntries[SE_COUNT];
};

bool ApplyMask(uint32_t b, uint32_t m, uint32_t &inOutRemaining)
{
	if ((b & m) != m)
		return false;

	if ((inOutRemaining & m) == 0) // if we don't need any of them
		return false;

	inOutRemaining = inOutRemaining & ~m;
	return true;
}

void EvalRanks(uint32_t b, Square &outSquare)
{
	uint32_t remaining = b;
	outSquare = Square();
	if (remaining == 0)
		return;
	if (ApplyMask(b, gC.kAll, remaining))
	{
		outSquare.d[SE_StartSuit] = SuitStart_All;
		return;
	}
	else if (ApplyMask(b, gC.kAllSuited, remaining))
		outSquare.d[SE_StartSuit] = SuitStart_Suited;
	else if (ApplyMask(b, gC.kAllUnsuited, remaining))
		outSquare.d[SE_StartSuit] = SuitStart_Unsuited;

	for (int i = 0; i < kSuitCount; i++)
		if (ApplyMask(b, gC.kRowsSuited[i], remaining))
			outSquare.d[SE_RowSuited] |= 1 << i;

	for (int i = 0; i < kSuitCount; i++)
		if (ApplyMask(b, gC.kColsSuited[i], remaining))
			outSquare.d[SE_ColSuited] |= 1 << i;

	for (int i = 0; i < kSuitCount; i++)
		if (ApplyMask(b, gC.kRowsUnsuited[i], remaining))
			outSquare.d[SE_RowUnsuited] |= 1 << i;

	for (int i = 0; i < kSuitCount; i++)
		if (ApplyMask(b, gC.kColsUnsuited[i], remaining))
			outSquare.d[SE_ColUnsuited] |= 1 << i;

	for (int i = 0; i < kSuitCount; i++)
	{
		outSquare.d[SE_Row1Singles + i] = remaining & gC.kRowsSuited[i];
		remaining = remaining & ~gC.kRowsSuited[i];
	}
	assert(remaining == 0);
}

struct OutputEntry
{
	SquareEntry e;
	uint32_t v;
	CardRanks r1;
	CardRanks r2s;
	CardRanks r2e;
};

void ParseSquare(bool finalSquare, const EnabledRange &range, ParseState &state, CardRanks c1, CardRanks c2, std::vector<OutputEntry> &outEntries)
{
	Square square;
	uint32_t b = CreateBitfieldForHand(range, c1, c2);
	EvalRanks(b, square);

	if (state.firstParse)
	{
		state.lastSquare = square;
	}
	else
	{
		for (int i = 0; i < SE_COUNT; i++)
		{
			uint32_t lastValue = state.lastSquare.d[i];
			if (lastValue != 0 && (lastValue != square.d[i])) // Need to output this entry
			{
				OutputEntry e;
				e.e = (SquareEntry)i;
				e.v = lastValue;
				e.r1 = c1;
				e.r2s = (lastValue == 0) ? (CardRanks)(c2 + 1) : state.lastRankOfEntries[i];
				e.r2e = (CardRanks)(c2 + 1);
				outEntries.push_back(e);
			}
		}
	}

	for (int i = 0; i < SE_COUNT; i++)
	{
		if (state.lastSquare.d[i] != square.d[i] || state.firstParse)
			state.lastRankOfEntries[i] = c2;
	}
	state.firstParse = false;
	state.lastSquare = square;

	// Output anything remaining
	if (finalSquare)
	{
		for (int i = 0; i < SE_COUNT; i++)
		{
			uint32_t lastValue = state.lastSquare.d[i];
			if (lastValue)
			{
				OutputEntry e;
				e.e = (SquareEntry)i;
				e.v = lastValue;
				e.r1 = c1;
				e.r2s = state.lastRankOfEntries[i];
				e.r2e = (CardRanks)(c2);
				outEntries.push_back(e);
			}
		}
	}
}

std::string GetSuitCharsForMask(uint32_t m)
{
	std::string result;
	for (int i = 0; i < 4; i++)
	{
		if (m & (1 << i))
			result += CardSuitToChar((CardSuit)i);
	}
	return result;
}

void ParseEntry(const OutputEntry &entry, std::stringstream &stream)
{
	std::string r1SuitChars;
	std::string r2SuitChars;
	switch (entry.e)
	{
	case SE_StartSuit:
	{
		if (entry.v == SuitStart_Suited)
			r2SuitChars = "s";
		else if ((entry.v == SuitStart_Unsuited))
			r2SuitChars = "o";
	}
	break;
	case SE_RowSuited:
	{
		r2SuitChars = ".";
		r1SuitChars = GetSuitCharsForMask(entry.v);
	}
	break;
	case SE_RowUnsuited:
	{
		r2SuitChars = ".o";
		r1SuitChars = GetSuitCharsForMask(entry.v);
	}
	break;
	case SE_ColSuited:
	{
		r1SuitChars = ".";
		r2SuitChars = GetSuitCharsForMask(entry.v);
	}
	break;
	case SE_ColUnsuited:
	{
		r1SuitChars = ".";
		r2SuitChars = GetSuitCharsForMask(entry.v) + "o";
	}
	break;
	case SE_Row1Singles:
	case SE_Row2Singles:
	case SE_Row3Singles:
	case SE_Row4Singles:
	{
		uint32_t s2Chars = 0;
		for (int i = 0; i < 16; i++)
		{
			if ((1 << i) & entry.v)
			{
				CardSuit s1, s2;
				FRF_IndexToSuits(false, i, s1, s2);
				s2Chars |= 1 << s2;
			}
		}
		r1SuitChars = CardSuitToChar((CardSuit)(entry.e - SE_Row1Singles));
		r2SuitChars = GetSuitCharsForMask(s2Chars);
	}
	break;
	default:
		break;
	}

	stream << CardRankToChar(entry.r1) << r1SuitChars;

	CardRanks r2s = std::min(entry.r2s, entry.r2e);
	CardRanks r2e = std::max(entry.r2s, entry.r2e);
	if (r2s == r2e)
	{
		stream << CardRankToChar(r2e) << r2SuitChars;
	}
	else
	{
		bool usePlus(r2e == entry.r1 - 1);
		if (usePlus)
			stream << CardRankToChar(r2s) << r2SuitChars << "+";
		else
			stream << CardRankToChar(r2e) << "-" << CardRankToChar(r2s) << r2SuitChars;
	}
}

struct OutputPairEntry
{
	CardRanks highRank;
	CardRanks lowRank;
	uint32_t cardMask;
};

void ParsePairEntry(const OutputPairEntry &entry, std::stringstream &stream)
{
	std::vector<std::string> outputs;
	uint32_t remaining = entry.cardMask;
	std::string appendix;
	char r1 = CardRankToChar(entry.lowRank);

	if (entry.highRank != entry.lowRank)
	{
		if (entry.highRank == kRankCardCount - 1)
		{
			appendix = "+";
		}
		else
		{
			char r2 = CardRankToChar(entry.highRank);
			appendix += (std::string("-") + r2) + r2;
		}
	}

	if (ApplyMask(entry.cardMask, gC.kPairAll, remaining))
	{
		outputs.push_back(std::string("") + r1 + r1 + appendix);
	}

	uint32_t rowSuitMask = 0;
	for (int i = 0; i < 4 && remaining; i++)
	{
		if (ApplyMask(entry.cardMask, gC.kPairSuits[i], remaining))
		{
			rowSuitMask |= 1 << i;
		}
	}
	if (rowSuitMask != 0)
	{
		outputs.push_back(std::string("") + r1 + GetSuitCharsForMask(rowSuitMask) + r1 + "." + appendix);
	}

	for (int i = 0; i < 6 && remaining; i++)
	{
		if (ApplyMask(entry.cardMask, 1 << i, remaining))
		{
			CardSuit s1, s2;
			FRF_IndexToSuits(true, i, s1, s2);
			outputs.push_back(std::string("") + r1 + CardSuitToChar(s1) + r1 + CardSuitToChar(s2) + appendix);
		}
	}

	for (int i = 0; i < outputs.size(); i++)
	{
		if (i != 0)
			stream << ",";
		stream << outputs[i];
	}
}

std::string EnabledRangeToString(const EnabledRange &range)
{
	std::stringstream stream;
	std::vector<OutputEntry> entries;
	for (int i = kRankCardCount - 1; i > 0; i--)
	{
		ParseState parseState;
		for (int j = i - 1; j >= 0; j--)
		{
			ParseSquare(j == 0, range, parseState, (CardRanks)i, (CardRanks)j, entries);
		}
	}

	std::vector<OutputPairEntry> pairEntries;
	uint32_t lastBitField = 0;
	pairEntries.reserve(kRankCardCount * 6);
	for (int i = kRankCardCount - 1; i >= 0; i--)
	{
		uint32_t b = CreateBitfieldForHand(range, (CardRanks)i, (CardRanks)i);
		if (b != 0)
		{
			if (lastBitField == b)
				pairEntries.back().lowRank = (CardRanks)i;
			else
				pairEntries.push_back({(CardRanks)i, (CardRanks)i, b});
		}
		lastBitField = b;
	}

	for (int i = 0; i < pairEntries.size(); i++)
	{
		if (i != 0)
			stream << ",";
		ParsePairEntry(pairEntries[i], stream);
	}

	for (int i = 0; i < entries.size(); i++)
	{
		if (i != 0 || !pairEntries.empty())
			stream << ",";
		ParseEntry(entries[i], stream);
	}

	return stream.str();
}