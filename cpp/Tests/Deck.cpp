#include "pch.h"
#include "Deck.h"

Deck::Deck()
{
	m_Cards.reserve((int)kSuitCount * (int)kRankCardCount);
	for (int s = 0; s < kSuitCount; s++)
		for (int r = 0; r < kRankCardCount; r++)
		{
			m_Cards.push_back({(CardSuit)s, (CardRanks)r});
		}
}

void Deck::ReturnCard(const Card &card)
{
	m_Cards.push_back(card);
}

Card Deck::DealRandomCard()
{
	if (m_Cards.empty())
		throw std::runtime_error("The deck is empty!");
	int idx = rand() % m_Cards.size();
	Card outCard = m_Cards[idx];
	m_Cards[idx] = m_Cards.back();
	m_Cards.pop_back();
	return outCard;
}

CardMask Deck::DealRandomCardMask(int cardCount)
{
	CardMask m;
	for (int i = 0; i < cardCount; i++)
		m = m & DealRandomCard();
	return m;
}

void Deck::ReturnCardMask(const CardMask &mask)
{
	ReturnCardSet(CardSet(mask));
}

void Deck::DealCardSet(CardSet &outSet, int count)
{
	outSet.clear();
	for (int i = 0; i < count; i++)
		outSet.push_back(DealRandomCard());
}

void Deck::ReturnCardSet(const CardSet &outSet)
{
	for (int i = 0; i < outSet.size(); i++)
		ReturnCard(outSet[i]);
}