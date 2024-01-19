#pragma once
#include "PokerTypes.h"

class PreflopPrecomputeTable;

void InitializePokerTech();
void ShutdownPokerTech();

const PreflopPrecomputeTable *GetGlobalPrecomputeTable();