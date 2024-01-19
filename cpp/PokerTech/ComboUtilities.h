#pragma once

#include "PokerTypes.h"
#include <set>
#include <cstring>
#include "FixedArray.h"

const int kMaxComboIteratorLists = 16;

void CreateRunoutCombos(CardMask blockedCards, int cardsToRunOut, std::set<CardMask> &combos);
std::vector<CardMask> CardMaskToIndividualCards(CardMask m);

uint64_t ComboCountNChooseR(int n, int r);

template <int MAX_R>
class NChooseRIterator
{
public:
	struct IterationState
	{
		IterationState()
		{
			std::memset(&it[0], 0, sizeof(it));
		}
		int it[MAX_R];
	};

	IterationState m_s;
	int m_N;
	int m_R;

	NChooseRIterator() : m_N(0), m_R(0), m_s() {}
	NChooseRIterator(int n, int r)
	{
		Initialize(n, r);
	}

	NChooseRIterator(int n, int r, const IterationState &state) : m_N(n), m_R(r), m_s(state)
	{
	}

	void Initialize(int n, int r, const IterationState *state = NULL)
	{
		m_N = n;
		m_R = r;
		if (state == NULL)
		{
			if (m_R == 0)
			{
				m_s.it[0] = 0;
				return;
			}
			for (int i = 0; i < r; i++)
				m_s.it[i] = i;
		}
		else
		{
			m_s = *state;
		}
	}

	inline int GetLastIndex(int stackIndex)
	{
		return m_N - ((int)m_R - stackIndex);
	}

	void InitializeStack(int idx, int startIndex)
	{
		for (int i = idx; i < m_R; i++)
			m_s.it[i] = startIndex++;
	}

	bool IsDone()
	{
		if (m_R == 0)
			return m_s.it[0] != 0;
		return m_s.it[0] > GetLastIndex(0);
	}

	void Next()
	{
		if (!IsDone())
		{
			if (m_R == 0)
				m_s.it[0] = 1;

			for (int i = (int)m_R - 1; i >= 0; i--)
			{
				if (++m_s.it[i] <= GetLastIndex(i))
				{
					InitializeStack(i + 1, m_s.it[i] + 1);
					break;
				}
			}
		}
	}

	const IterationState &GetState() const { return m_s; }
};

class RunoutIterator
{
	std::vector<CardMask> m_Cards;
	NChooseRIterator<5> m_It;

	CardMask v;

	void UpdateV()
	{
		if (!IsDone())
		{
			v = 0;
			for (int i = 0; i < m_It.m_R; i++)
			{
				v = v | m_Cards[m_It.m_s.it[i]];
			}
		}
	}

public:
	inline CardMask GetMask() { return v; }

	typedef NChooseRIterator<5>::IterationState StateType;

	const StateType &GetState() { return m_It.GetState(); }

	RunoutIterator(CardMask blocked, int r, const StateType *state = NULL)
	{
		m_Cards = CardMaskToIndividualCards(~blocked);
		m_It.Initialize((int)m_Cards.size(), r, state);
		UpdateV();
	}

	bool IsDone()
	{
		return m_It.IsDone();
	}

	void Next()
	{
		m_It.Next();
		UpdateV();
	}
};

class ComboIterator
{
public:
	ComboIterator() : m_IsDone(false)
	{
	}
	void Initialize(int *sizes, int sizesCount)
	{
		m_Sizes.Initialize(sizes, sizesCount);
		std::memset(m_CurIndex, 0, sizeof(m_CurIndex));
	}

	bool IsDone() { return m_IsDone; }

	void Next()
	{
		if (!IsDone())
		{
			for (int i = (int)m_Sizes.size() - 1; i >= 0; i--)
			{
				if (++m_CurIndex[i] < m_Sizes[i])
					break;

				m_CurIndex[i] = 0;
				if (i == 0)
					m_IsDone = true;
			}
		}
	}

	int m_CurIndex[kMaxComboIteratorLists];

private:
	FixedArray<int, kMaxComboIteratorLists> m_Sizes;
	bool m_IsDone;
};

class MultiPlayerRangeComboIterator
{
	ComboIterator m_Iterator;
	const std::vector<std::vector<CardMask>> &m_PlayerRanges;
	CardMask m_Board;
	CardMask m_DeadCards;

	bool IsValidIteration()
	{
		if (m_Iterator.IsDone())
			return false;

		CardMask cards = m_Board;
		for (int i = 0; i < m_PlayerRanges.size(); i++)
			cards = cards | m_PlayerRanges[i][m_Iterator.m_CurIndex[i]];

		if (cards.HasOverlappingCards(m_DeadCards))
			return false;

		// Count bits
		int expectedBits = 5 + (int)m_PlayerRanges.size() * 2;
		return cards.Count() == expectedBits;
	}

public:
	MultiPlayerRangeComboIterator(CardMask board, CardMask deadCards, const std::vector<std::vector<CardMask>> &playerRanges) : m_PlayerRanges(playerRanges)
	{
		m_DeadCards = deadCards;
		m_Board = board;
		FixedArray<int, kMaxComboIteratorLists> sizes;
		for (int i = 0; i < playerRanges.size(); i++)
			sizes.push_back((int)playerRanges[i].size());
		m_Iterator.Initialize(&sizes[0], sizes.size());
		if (!IsValidIteration())
			Next();
	}

	void Next()
	{
		while (!m_Iterator.IsDone())
		{
			m_Iterator.Next();

			if (IsValidIteration())
				break;
		}
	}

	bool IsDone() { return m_Iterator.IsDone(); }

	const int *GetState() { return &m_Iterator.m_CurIndex[0]; }
};