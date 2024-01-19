// PokerTech.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "PokerTech.h"
#include "Precompute/PreflopPrecomputeTableAPI.h"
#include "PokerTypes.h"
#include <sstream>
#include <assert.h>

PreflopPrecomputeTable *gGlobalPreflopTable;

void InitializePokerTech()
{
	gGlobalPreflopTable = PreflopPrecomputeTable_Create("preflop_precompute_16_13.bin");
}

void ShutdownPokerTech()
{
	PreflopPrecomputeTable_Destroy(gGlobalPreflopTable);
	gGlobalPreflopTable = NULL;
}

const PreflopPrecomputeTable *GetGlobalPrecomputeTable()
{
	return gGlobalPreflopTable;
}