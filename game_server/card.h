#ifndef CARD_H
#define CARD_H

#include <string>

class Card {
public:
    std::string suit;
    std::string value;

    // Constructor to initialize suit and value
    Card(std::string s, std::string v);

    // Method to return a string representation of the card
    std::string to_string() const;
};

#endif // CARD_H