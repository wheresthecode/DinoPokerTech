#pragma once

// The hand rank is a sortable 64-bit integer that represents the strength of a poker hand.
// The higher the value, the stronger the hand.
typedef uint64_t HandRank;
HandRank Rank5CardHandFrom7Cards(CardMask m1);
