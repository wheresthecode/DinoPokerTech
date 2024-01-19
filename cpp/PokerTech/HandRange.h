#pragma once

#include <unordered_set>
#include "PokerTypes.h"
#include "RangeConverters/IRangeBuilder.h"

class HandRange : public std::unordered_set<CardMask, CardMask>, public IRangeBuilder
{
public:
	HandRange() {}
	// HandRange(const std::string& rangeString);
	HandRange(std::initializer_list<CardMask> masks);
	HandRange(const HandRange &) = default;
	HandRange &operator=(const HandRange &) = default;

	void AddRange(const HandRange &range);

	bool Contains(CardMask m) const { return find(m) != end(); }

	std::vector<CardMask> ToMaskArray() const;

	void AddHand(CardRanks r1, CardSuit s1, CardRanks r2, CardSuit s2, float weight) override { this->insert(CardMask(s1, r1) | CardMask(s2, r2)); }
};