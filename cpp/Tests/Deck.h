#pragma once

#include "PokerTech/PokerTypes.h"

class Deck
{
public:
	Deck();

	void ReturnCard(const Card &card);
	Card DealRandomCard();

	CardMask DealRandomCardMask(int cardCount);
	void ReturnCardMask(const CardMask &mask);

	void DealCardSet(CardSet &outSet, int count);
	void ReturnCardSet(const CardSet &outSet);

private:
	std::vector<Card> m_Cards;
};
