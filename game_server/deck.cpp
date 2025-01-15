#include "deck.h"
#include <algorithm>
#include <random>
#include <stdexcept>

// Constructor for Deck class, initializes 52 cards and shuffles them
Deck::Deck() {
    std::vector<std::string> suits = {"♥", "♦", "♣", "♠"};
    std::vector<std::string> values = { "7", "8", "9", "10", "J", "Q", "K", "A"};

    // Create a deck of cards
    for (const auto& suit : suits) {
        for (const auto& value : values) {
            cards.push_back(Card(suit, value));
        }
    }

    // Create a random number generator
    std::random_device rd;
    std::mt19937 g(rd());

    // Shuffle the cards using std::shuffle
    std::shuffle(cards.begin(), cards.end(), g);
}

// Method to draw the top card from the deck
Card Deck::draw_card() {
    if (cards.empty()) {
        throw std::runtime_error("Deck is empty");
    }
    Card top_card = cards.back();
    cards.pop_back();
    return top_card;
}

// Method to return the current size of the deck
int Deck::size() const {
    return cards.size();
}
