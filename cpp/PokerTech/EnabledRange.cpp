#include "pch.h"
#include "EnabledRange.h"
#include "PokerUtilities.h"
#include "PokerTech/RangeConverters/EnabledRangeToSyntax.h"
#include "PokerTech/RangeConverters/RangeBuilderSyntaxString.h"

void EnabledRange::AddHand(CardRanks r1, CardSuit s1, CardRanks r2, CardSuit s2, float weight)
{
	SetCombo(H2({s1, r1}, {s2, r2}), weight);
}

bool EnabledRange::ConvertFromRangeString(const std::string &range)
{
	Clear();
	return BuildRangeFromHandSyntaxString(range, *this);
}

std::string EnabledRange::CalculateRangeSyntaxString()
{
	return EnabledRangeToString(*this);
}

void EnabledRange::SetFromFloatArray(const float *floats, int count)
{
	assert(count == kTotalRangeEntries);
	for (int i = 0; i < kTotalRangeEntries; i++)
		m_Range.m_Data[i].FromFloat(floats[i]);
}
void EnabledRange::GetRangeAsFloatArray(float *floats, int count) const
{
	assert(count == kTotalRangeEntries);
	for (int i = 0; i < kTotalRangeEntries; i++)
		floats[i] = m_Range.m_Data[i].ToFloat();
}