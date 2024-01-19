#pragma once
#include "PokerTech/PokerTypes.h"

// Ranges can be stored in multiple structure types. If the structure type implements this interface,
// various operations can be performed on it.
class IRangeBuilder
{
public:
    virtual void AddHand(CardRanks r1, CardSuit s1, CardRanks r2, CardSuit s2, float weight) = 0;
};