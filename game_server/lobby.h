#ifndef LOBBY_H
#define LOBBY_H

#include <vector>
#include "player.h"  // Include Player structure
#include "deck.h"    // Include Deck structure
#include "json.h"  // Include SimpleJSON class

struct Lobby {
    Player* player1;
    Player* player2;
    std::vector<Card> discard_pile;  // Cards in the discard pile
    Deck deck;  // The deck of cards
    SimpleJSON game_state;  // Use SimpleJSON for game state
    bool paused;

    bool is_full() const { return player1 != nullptr && player2 != nullptr; }
    void initialize_game();
};

#endif // LOBBY_H
