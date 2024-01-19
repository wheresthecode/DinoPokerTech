#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include "Utilities.h"

enum CardsAll
{
	_2C = 0,
	_3C,
	_4C,
	_5C,
	_6C,
	_7C,
	_8C,
	_9C,
	_TC,
	_JC,
	_QC,
	_KC,
	_AC,
	_2S = 16,
	_3S,
	_4S,
	_5S,
	_6S,
	_7S,
	_8S,
	_9S,
	_TS,
	_JS,
	_QS,
	_KS,
	_AS,
	_2D = 32,
	_3D,
	_4D,
	_5D,
	_6D,
	_7D,
	_8D,
	_9D,
	_TD,
	_JD,
	_QD,
	_KD,
	_AD,
	_2H = 48,
	_3H,
	_4H,
	_5H,
	_6H,
	_7H,
	_8H,
	_9H,
	_TH,
	_JH,
	_QH,
	_KH,
	_AH,
};

enum CardRanks
{
	Rank_2 = 0,
	Rank_3,
	Rank_4,
	Rank_5,
	Rank_6,
	Rank_7,
	Rank_8,
	Rank_9,
	Rank_10,
	Rank_J,
	Rank_Q,
	Rank_K,
	Rank_A,
	kRankCardCount,
	kRankInvalid = -1
};

enum CardSuit
{
	Suit_Club = 0,
	Suit_Spade,
	Suit_Diamond,
	Suit_Heart,
	kSuitCount,
	kCardSuitInvalid = -1
};

const int kMaxCards = (int)kRankCardCount * (int)kSuitCount;

class HandRange;

typedef uint64_t HandRank;

struct Card
{
	CardSuit Suit;
	CardRanks Rank;

	bool operator==(const Card &other)
	{
		return Suit == other.Suit && Rank == other.Rank;
	}
};

struct CardMask
{
	CardMask() : v(0) {}
	CardMask(uint64_t _v) : v(_v) {}
	CardMask(CardSuit suit, CardRanks rank) { v = (uint64_t)((uint64_t)1 << (rank + suit * 16)); }
	CardMask(Card c) { v = (uint64_t)((uint64_t)1 << (c.Rank + c.Suit * 16)); }

	uint64_t v;

	inline CardMask operator~() const { return ~v; }
	inline CardMask operator|(const CardMask &m) const { return m.v | v; }
	inline CardMask operator&(const CardMask &m) const { return m.v & v; }
	inline bool operator<(const CardMask &m) const { return v < m.v; }
	bool operator==(const CardMask &other) const
	{
		return other.v == v;
	};

	bool HasOverlappingCards(const CardMask &otherMask) { return (otherMask.v & v) != 0; }
	bool HasCards() const { return v != 0; }
	inline int Count() const { return (int)bit_popcount64(v); }

	std::vector<CardMask> Explode(CardMask m);

	size_t operator()(const CardMask &pointToHash) const noexcept
	{
		return v;
	};

	bool HasCard(CardSuit suit, CardRanks rank) { return (v & ((uint64_t)1 << (rank + suit * 16))) != 0; }
};

class CardSet : public std::vector<Card>
{
public:
	CardSet()
	{
	}

	CardSet(CardMask mask);

	CardMask ToMask() const;

	void FromMask(CardMask mask)
	{
	}
};

const uint64_t kValidRankMask = 0b0001111111111111;
const uint64_t kValidCardMask = kValidRankMask | (kValidRankMask << 16) | (kValidRankMask << 32) | (kValidRankMask << 48);
