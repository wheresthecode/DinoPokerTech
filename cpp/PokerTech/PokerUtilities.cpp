#include "pch.h"
#include "StringUtility.h"

#include "PokerUtilities.h"

static char kBitTableToCardChar[] = {
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'T',
	'J',
	'Q',
	'K',
	'A'};
CASSERT_ARRAY_SIZE(kBitTableToCardChar, (int)kRankCardCount);

static char kSuitChars[] = {
	'c',
	's',
	'd',
	'h'};
CASSERT_ARRAY_SIZE(kSuitChars, (int)kSuitCount);

char CardSuitToChar(CardSuit suit) { return kSuitChars[suit]; }
char CardRankToChar(CardRanks rank) { return kBitTableToCardChar[rank]; }

CardSuit SuitCharToIndex(char suitChar)
{
	if (suitChar == 'c')
		return CardSuit::Suit_Club;
	if (suitChar == 's')
		return CardSuit::Suit_Spade;
	if (suitChar == 'd')
		return CardSuit::Suit_Diamond;
	if (suitChar == 'h')
		return CardSuit::Suit_Heart;
	return CardSuit::kCardSuitInvalid;
}

CardSet::CardSet(CardMask mask)
{
	for (int r = kRankCardCount - 1; r >= 0; r--)
		for (int s = 0; s < kSuitCount; s++)
		{
			if (mask.HasCard((CardSuit)s, (CardRanks)r))
			{
				Card e = {(CardSuit)s, (CardRanks)r};
				push_back(e);
			}
		}
}

CardMask CardSet::ToMask() const
{
	CardMask r;
	for (int i = 0; i < size(); i++)
	{
		r = r | CardMask((*this)[i]);
	}
	return r;
}

std::string CardIndexToName(int cardIndex)
{
	int suitIndex = cardIndex / kRankCardCount;
	int rankIndex = cardIndex % kRankCardCount;
	std::string result = (std::string("") + kBitTableToCardChar[rankIndex]) + kSuitChars[suitIndex];
	return result;
}

std::string CardToName(const Card &card)
{
	std::string result = (std::string("") + kBitTableToCardChar[card.Rank]) + kSuitChars[card.Suit];
	return result;
}

std::string CardSetToString(const CardSet &set)
{
	std::string result;
	for (auto it = set.begin(); it != set.end(); it++)
	{
		result.push_back(kBitTableToCardChar[it->Rank]);
		result.push_back(kSuitChars[it->Suit]);
	}
	return result;
}

std::string CardMaskToString(CardMask mask)
{
	CardSet s(mask);
	return CardSetToString(s);
}

CardRanks RankCharToIndex(char rankChar)
{
	char upperChar = toupper(rankChar);
	for (int i = 0; i < ARRAY_SIZE(kBitTableToCardChar); i++)
	{
		if (kBitTableToCardChar[i] == upperChar)
			return (CardRanks)i;
	}
	return CardRanks::kRankInvalid;
}

bool CardCharDescToCardMask(char rankChar, char suitChar, CardMask &outMask)
{
	CardSuit suitIndex = SuitCharToIndex(suitChar);
	CardRanks rankIndex = RankCharToIndex(rankChar);

	if (suitIndex == CardSuit::kCardSuitInvalid || rankIndex == CardRanks::kRankInvalid)
		return false;

	outMask = CardMask(suitIndex, rankIndex);
	return true;
}

bool ExplicitStringToCardMask(const std::string &cards, CardMask &outMask)
{
	std::string s = StripWhitespace(cards);
	CardMask ret = 0;
	int len = (int)s.length();
	if (len != 0)
	{
		const char *itr = s.c_str();
		while (len > 1)
		{
			char rank = itr[0];
			char suit = itr[1];

			CardMask oneCard;
			if (!CardCharDescToCardMask(rank, suit, oneCard))
				return false;
			if (ret.HasOverlappingCards(oneCard))
				return false;
			ret = ret | oneCard;

			len -= 2;
			itr += 2;
		}
	}
	if (len != 0) // odd number of letters
		return false;
	outMask = ret;
	return true;
}

CardMask ExplicitStringToCardMask(const std::string &cards)
{
	CardMask m;
	if (!ExplicitStringToCardMask(cards, m))
		throw std::runtime_error("Invalid card string!");
	return m;
}

std::vector<CardMask> CardMask::Explode(CardMask m)
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

bool ExpandRangeEntry(std::string rangeString, HandRange &outRange)
{
	// KK+ = KK, AA
	// AQ+ = AQ, AK
	// KT+ = KT, KJ, KQ
	// KTs+ = KTs, KJs, KQs All suit combos
	// AJo = All unsuited combos
	// AcKs = Ace of clubs King of spades
	// AsK = Aces of spades with all combos of kings
	// KsAo = King of spades with any ace not a spade
	// Invalid input returns failure and error message

	CardRanks r[2] = {(CardRanks)-1, (CardRanks)-1};
	int s[2] = {-1, -1};
	bool plusModifier = false;
	bool suitedModifier = false;
	bool unsuitedModfiier = false;

	if (rangeString.back() == '+')
	{
		plusModifier = true;
		rangeString.pop_back();
	}
	if (rangeString.back() == 'o')
	{
		unsuitedModfiier = true;
		rangeString.pop_back();
	}
	else if (rangeString.back() == 's')
	{
		suitedModifier = true;
		rangeString.pop_back();
	}

	if (rangeString.size() < 2)
		return false;

	const char *str = rangeString.c_str();
	for (int i = 0; i < 2; i++)
	{
		r[i] = RankCharToIndex(*(str++));
		if (r[i] == kRankInvalid)
			return false;

		if (*str == 0)
			continue;

		// could be modifiying r1
		s[i] = SuitCharToIndex(*str);
		if (s[i] != -1)
			str++;
	}

	if (s[0] != -1 && suitedModifier)
	{
		suitedModifier = false;
		s[1] = SuitCharToIndex('s');
	}
	if (s[0] != -1 && unsuitedModfiier)
	{
		return false;
	}

	if (*str != 0)
		return false;

	if (r[0] == r[1] && s[0] == s[1] && s[0] != -1)
		return false;

	// Two cards
	if (r[0] != r[1])
	{
		for (int ri = r[1]; ri < kRankCardCount && ri != r[0]; ri++)
		{
			for (int i = 0; i < kSuitCount; i++)
				for (int j = 0; j < kSuitCount; j++)
				{
					bool shouldAdd = !suitedModifier && !unsuitedModfiier;
					shouldAdd |= suitedModifier && i == j;
					shouldAdd |= unsuitedModfiier && i != j;
					if (s[0] != -1)
						shouldAdd &= s[0] == i;
					if (s[1] != -1)
						shouldAdd &= s[1] == j;
					shouldAdd &= (r[0] != ri) || (i != j);

					if (shouldAdd)
						outRange.insert(CardMask((CardSuit)i, r[0]) | CardMask((CardSuit)j, (CardRanks)ri));
				}

			if (!plusModifier)
				break;
		}
	}
	else // Pairs
	{
		for (int ri = r[0]; ri < kRankCardCount; ri++)
		{
			for (int i = 0; i < kSuitCount; i++)
				for (int j = 0; j < kSuitCount; j++)
				{
					if (s[0] != -1 && s[0] != i)
						continue;
					if (s[1] != -1 && s[1] != j)
						continue;
					if (i != j)
						outRange.insert(CardMask((CardSuit)i, (CardRanks)ri) | CardMask((CardSuit)j, (CardRanks)ri));
				}
			if (!plusModifier)
				break;
		}
	}
	return true;
}

bool ExpandRangeString(std::string rangeString, HandRange &outRange)
{
	rangeString = StripWhitespace(rangeString);
	std::vector<std::string> entries = StringSplit(rangeString, ',');

	for (int i = 0; i < entries.size(); i++)
	{
		if (!ExpandRangeEntry(entries[i], outRange))
			return false;
	}
	return true;
}

bool IsValidRangeString(const std::string &rangeString)
{
	HandRange range;
	return ExpandRangeString(rangeString, range) && !range.empty();
}

int GetRangeStringHandCount(const std::string &rangeString)
{
	HandRange range;
	return ExpandRangeString(rangeString, range) ? range.size() : -1;
}

bool IsValidBoardString(const std::string &rangeString)
{
	CardMask m;
	if (!ExplicitStringToCardMask(rangeString, m))
		return false;
	int c = m.Count();
	return c == 0 || c == 3 || c == 4 || c == 5;
}