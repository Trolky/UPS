#include "card.h"

// Constructor for Card class
Card::Card(std::string s, std::string v) : suit(s), value(v) {}

// to_string method to represent the card as a string
std::string Card::to_string() const {
    return value + suit;
}
