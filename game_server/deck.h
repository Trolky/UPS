#ifndef DECK_H
#define DECK_H

#include "card.h"
#include <vector>
#include <algorithm>
#include <stdexcept>

class Deck {
public:
    std::vector<Card> cards;

    // Constructor to initialize a deck of cards
    Deck();

    // Method to draw the top card from the deck
    Card draw_card();

    // Method to get the current number of cards in the deck
    int size() const;
};

#endif // DECK_H
