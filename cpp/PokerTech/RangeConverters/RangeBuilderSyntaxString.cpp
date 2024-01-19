#include "PokerTech/pch.h"
#include "PokerTech/StringUtility.h"
#include "PokerTech/PokerUtilities.h"
#include "PokerTech/FixedArray.h"
#include <algorithm>
#include "RangeBuilderPercentage.h"

template <class T>
void MinFirst(T &i1, T &i2)
{
	T t = std::min(i1, i2);
	i2 = std::max(i1, i2);
	i1 = t;
}

enum SuitSelect
{
	SuitSelect_All,
	SuitSelect_Unsuited,
	SuitSelect_Suited
};

typedef FixedArray<CardSuit, kSuitCount> SuitRange;
typedef FixedArray<CardRanks, kRankCardCount> RankRange;

static void SetAllSuits(SuitRange &suitRange)
{
	suitRange.clear();
	for (int i = 0; i < kSuitCount; i++)
		suitRange.push_back((CardSuit)i);
}

bool ParseSuit(char const *&str, SuitRange &suitRange)
{
	while (true)
	{
		if (*str == '.')
		{
			str++;
			SetAllSuits(suitRange);
			break;
		}
		CardSuit s = SuitCharToIndex(*str);

		if (s == kCardSuitInvalid)
			break;
		suitRange.push_back(s);
		str++;
	}
	if (suitRange.size() != 0)
		return true;
	SetAllSuits(suitRange);
	return false;
}

bool ParseRankChar(char const *&str, CardRanks &rank)
{
	rank = RankCharToIndex(*str);
	if (rank != kRankInvalid)
	{
		str++;
		return true;
	}
	return false;
}

static bool TryParsePair(char const *&str, CardRanks &r, SuitRange &s1, SuitRange &s2)
{
	CardRanks r2;
	if (!ParseRankChar(str, r))
		return false;

	bool hasSuit1Specified = ParseSuit(str, s1);

	if (!ParseRankChar(str, r2) || r != r2)
		return false;

	bool hasSuit2Specified = ParseSuit(str, s2);

	return true;
}
enum PairParseResult
{
	PairParseResult_InvalidTextError,
	PairParseResult_NotPair,
	PairParseResult_Success,
};

bool ParseChar(char const *&str, char c)
{
	if (str[0] == c)
	{
		str++;
		return true;
	}
	return false;
}

static bool TryParsePairRange(char const *str, IRangeBuilder &rangeBuilder)
{
	CardRanks r1, r2;
	SuitRange s1, s2;
	if (!TryParsePair(str, r1, s1, s2))
		return false;
	r2 = r1;

	if (ParseChar(str, '+'))
		r2 = (CardRanks)(kRankCardCount - 1);
	else if (ParseChar(str, '-'))
	{
		SuitRange s2_1, s2_2;
		if (!TryParsePair(str, r2, s2_1, s2_2))
			return false;
	}

	if (*str != 0)
		return false;

	int count = 0;
	for (int i = std::min(r1, r2), c = std::max(r1, r2); i <= c; i++)
	{
		for (int si = 0; si < s1.size(); si++)
			for (int sj = 0; sj < s2.size(); sj++)
			{
				if (s1[si] != s2[sj])
				{
					rangeBuilder.AddHand((CardRanks)i, (CardSuit)s1[si], (CardRanks)i, (CardSuit)s2[sj], 1.0f);
					count++;
				}
			}
	}
	return count != 0;
}

static bool ParseRank(const char *&str, RankRange &outRange)
{
	CardRanks r1, r2;
	if (*str == '*')
	{
		str++;
		r1 = (CardRanks)0;
		r2 = (CardRanks)(kRankCardCount - 1);
	}
	else
	{
		if (!ParseRankChar(str, r1))
			return false;

		r2 = r1;

		if (ParseChar(str, '-'))
		{
			if (!ParseRankChar(str, r2))
				return false;
		}
	}

	MinFirst(r1, r2);
	for (int i = r1; i <= r2; i++)
	{
		outRange.push_back((CardRanks)i);
	}
	return true;
}

bool ParseModifiers(char const *str, SuitSelect &outSuitSelect, bool &isPlus)
{
	outSuitSelect = SuitSelect_All;
	isPlus = false;

	while (*str != 0)
	{
		switch (*str)
		{
		case 'o':
		case 's':
		{
			if (outSuitSelect != SuitSelect_All)
				return false;
			outSuitSelect = *str == 's' ? SuitSelect_Suited : SuitSelect_Unsuited;
		}
		break;
		case '+':
		{
			if (isPlus)
				return false;
			isPlus = true;
		}
		break;
		default:
		{
			return false; // Unknown character
		}
		}
		str++;
	}

	return true;
}

static bool TryParseUnpairedRange(char const *str, IRangeBuilder &rangeBuilder)
{
	RankRange r1r, r2r;
	SuitRange s1r, s2r;
	SuitSelect suitSelect;
	bool isPlus;

	// Not a pair syntax
	if (!ParseRank(str, r1r))
		return false;

	bool hadSuit1Specified = ParseSuit(str, s1r);

	if (!ParseRank(str, r2r))
		return false;

	if (hadSuit1Specified)
		ParseSuit(str, s2r);
	else
		SetAllSuits(s2r);

	if (!ParseModifiers(str, suitSelect, isPlus))
		return false;

	if (isPlus)
	{
		if (r1r.size() > 1 || r2r.size() > 1)
			return false;
		RankRange *sm = (r1r[0] < r2r[0]) ? &r1r : &r2r;
		RankRange *lg = (r1r[0] < r2r[0]) ? &r2r : &r1r;
		for (int i = (*sm)[0] + 1; i < (*lg)[0]; i++)
		{
			sm->push_back((CardRanks)i);
		}
	}

	bool offsuitOnly;
	int count = 0;
	for (int si = 0; si < s1r.size(); si++)
	{
		CardSuit s1 = s1r[si];
		for (int sj = 0; sj < s2r.size(); sj++)
		{
			CardSuit s2 = s2r[sj];
			if (suitSelect == SuitSelect_Suited && s1 != s2)
				continue;
			if (suitSelect == SuitSelect_Unsuited && s1 == s2)
				continue;
			for (int ri = 0; ri < r1r.size(); ri++)
			{
				CardRanks r1 = r1r[ri];
				for (int rj = 0; rj < r2r.size(); rj++)
				{
					CardRanks r2 = r2r[rj];
					if (r1 != r2)
					{
						rangeBuilder.AddHand(r1, s1, r2, s2, 1.0f);
						count++;
					}
				}
			}
		}
	}
	return count != 0;
}

static bool GetFloat(const std::string &str, float &outFloat)
{
	outFloat = 0.0f;
	if (str[0] == '0' || str == "0.0")
		return true;

	outFloat = atof(str.c_str()) / 100.0f;
	return outFloat > 0.0f && outFloat < 1.00001f;
}

static bool TryParsePercentRange(const std::string &rangeString, IRangeBuilder &rangeBuilder)
{
	size_t len = rangeString.length();
	if (len < 2)
		return false;
	if (rangeString[len - 1] != '%')
		return false;
	if (rangeString[0] == '-')
		return false;

	std::string m = rangeString.substr(0, len - 1);
	std::vector<std::string> ra = StringSplit(m, "-");
	if (ra.size() != 1 && ra.size() != 2)
		return false;

	float p1 = 0.0f, p2 = 0.0f;
	if (ra.size() == 1)
	{
		if (!GetFloat(ra[0], p2))
			return false;
	}
	else if (ra.size() == 2)
	{
		if (!GetFloat(ra[0], p1))
			return false;
		if (!GetFloat(ra[1], p2))
			return false;
	}

	BuildRangeFromRangePercentage(p1, p2, rangeBuilder);
	return true;
}

static bool ExpandRangeEntry2(const std::string &rangeString, IRangeBuilder &rangeBuilder)
{
	if (TryParsePercentRange(rangeString, rangeBuilder))
		return true;

	if (TryParsePairRange(rangeString.c_str(), rangeBuilder))
		return true;

	return TryParseUnpairedRange(rangeString.c_str(), rangeBuilder);
}

bool BuildRangeFromHandSyntaxString(std::string rangeString, IRangeBuilder &rangeBuilder)
{
	rangeString = StripWhitespace(rangeString);
	std::vector<std::string> entries = StringSplit(rangeString, ',');

	for (int i = 0; i < entries.size(); i++)
	{
		if (!ExpandRangeEntry2(entries[i], rangeBuilder))
			return false;
	}
	return true;
}
