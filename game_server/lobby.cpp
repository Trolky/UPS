#include "lobby.h"

void Lobby::initialize_game() {
    // Create a shuffled deck
    deck = Deck(); // Ensure you have a Deck class that handles the card initialization and shuffling.

    // Deal 4 cards to each player (adjust this number if needed)
    if (player1) {
        for (int i = 0; i < 4; ++i) {
            player1->hand.push_back(deck.draw_card()); // Ensure draw_card is implemented
        }
    }
    if (player2) {
        for (int i = 0; i < 4; ++i) {
            player2->hand.push_back(deck.draw_card()); // Ensure draw_card is implemented
        }
    }

    // Initialize discard pile (start with one card)
    discard_pile.push_back(deck.draw_card());

    // Initialize game state
    game_state.assign_array("players", {player1->name, player2->name});
    game_state.assign_string("current_player", player1->name);
    game_state.assign_int("deck_size", deck.size());
    game_state.assign_int("discard_pile", discard_pile.size());
    game_state.assign_card("top_card", discard_pile.back());

    std::map<std::string, std::vector<Card>> hands;
    hands[player1->name] = player1->hand;
    hands[player2->name] = player2->hand;

    SimpleJSON::NestedObject current_player_hand;
    current_player_hand[player1->name] = SimpleJSON::convert_hand_to_nested(player1->hand);
    current_player_hand[player2->name] = SimpleJSON::convert_hand_to_nested(player2->hand);

    game_state.assign_nested_object("current_player_hand", current_player_hand);
}