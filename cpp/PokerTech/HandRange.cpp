#include "pch.h"
#include "HandRange.h"

HandRange::HandRange(std::initializer_list<CardMask> ranges)
{
	insert(ranges);
}

void HandRange::AddRange(const HandRange &range)
{
	this->insert(range.begin(), range.end());
}

std::vector<CardMask> HandRange::ToMaskArray() const
{
	std::vector<CardMask> a(begin(), end());
	return a;
}