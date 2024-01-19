#pragma once
#include "IRangeBuilder.h"

// Expands the rangeString and adds to the HandRange provided
// Comma separated list of ranges. Can specify variations with modifiers
// I.E.
// KK+ = KK, AA
// AQ+ = AQ, AK
// KT+ = KT, KJ, KQ+
// KTs+ = KTs, KJs, KQs All suit combos
// AJo = All unsuited combos
// 32+ = All non-paired hands
// AcKs = Ace of clubs King of spades
// AsK = Aces of spades with all combos of kings
// KsAo = King of spades with any ace not a spade
bool BuildRangeFromHandSyntaxString(std::string rangeString, IRangeBuilder &outRange);
