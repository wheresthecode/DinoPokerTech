#pragma once
#include "PokerTech/PokerTypes.h"

struct PrecomputeResult
{
    float p1WinEquity;
    float p2WinEquity;
    float tie;
};

class PreflopPrecomputeTable;

PreflopPrecomputeTable *PreflopPrecomputeTable_Create(const std::string &filePath);
PrecomputeResult PreflopPrecomputeTable_GetValue(const PreflopPrecomputeTable *table, const CardSet &p1, const CardSet &p2);
void PreflopPrecomputeTable_Destroy(PreflopPrecomputeTable *table);
