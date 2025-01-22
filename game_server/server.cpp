#include "server.h"

game_server::game_server(const std::string& ip, int port) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
    if (server_socket == INVALID_SOCKET) {
#else
    if (server_socket < 0) {
#endif
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP string to network address
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid IP address");
    }

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }

    running = true;
    std::cout << "Server started on " << ip << ":" << port << std::endl;
    }

Lobby* game_server::find_or_create_lobby(Player* player) {
        std::cout << "Finding lobby for player: " << player->name << std::endl;

        // First, try to find a non-full lobby
        for (auto lobby : lobbies) {
            if (!lobby->is_full()) {
                if (lobby->player1 == nullptr) {
                    lobby->player1 = player;
                    std::cout << "Player " << player->name << " joined lobby as player 1" << std::endl;
                    // Mark the first player, they will start the game
                    lobby-> game_state.assign_string("current_player", player->name);
                } else {
                    lobby->player2 = player;
                    std::cout << "Player " << player->name << " joined lobby as player 2" << std::endl;
                    // Initialize game state for new lobby
                    lobby->game_state.assign_array("players", {lobby->player1->name, lobby->player2->name});
                    lobby->game_state.assign_string("current_player", lobby->player1->name);
                    lobby->game_state.assign_string("top_card", "");
                    lobby->game_state.assign_int("deck_size", 32);

                }
                // Initialize the game once both players are in the lobby
                if (lobby->is_full()) {
                    lobby->initialize_game();
                }
                return lobby;
            }
        }

        // If no available lobby found, create a new one
        std::cout << "Creating new lobby for player: " << player->name << std::endl;
        Lobby* new_lobby = new Lobby();
        new_lobby->player1 = player;
        new_lobby->player2 = nullptr;
        lobbies.push_back(new_lobby);
        return new_lobby;
}

Lobby* game_server::find_player_lobby(const std::string& player_name) {
    for (auto lobby : lobbies) {
        if ((lobby->player1 && lobby->player1->name == player_name) ||
            (lobby->player2 && lobby->player2->name == player_name)) {
            return lobby;
        }
    }
    return nullptr;
}

bool game_server::is_reconnection_valid(const std::string& player_name, const sockaddr_in& new_addr) {
    auto it = players.find(player_name);
    return (it != players.end() && it->second->disconnected);
}

void game_server::notify_reconnection(Player* player, Lobby* lobby) {
    if (!lobby || !player) return;
    
    Player* other_player = (lobby->player1 == player) ? lobby->player2 : lobby->player1;
    if (other_player && !other_player->disconnected) {
        SimpleJSON notify_msg;
        notify_msg.assign_string("type", "player_reconnected");
        notify_msg.assign_string("player", player->name);
        notify_msg.assign_string("message", "Player " + player->name + " has reconnected.");
        send_to_client(notify_msg, other_player->address);
    }
}

void game_server::notify_disconnection(Player* player, Lobby* lobby) {
    if (!lobby || !player) return;
    
    Player* other_player = (lobby->player1 == player) ? lobby->player2 : lobby->player1;
    if (other_player && !other_player->disconnected) {
        SimpleJSON disconnect_msg;
        disconnect_msg.assign_string("type", "player_disconnected");
        disconnect_msg.assign_string("player", player->name);
        disconnect_msg.assign_string("message", "Player " + player->name + " has disconnected.");
        send_to_client(disconnect_msg, other_player->address);
    }
}

void game_server::check_disconnections() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Check every 5 seconds
        std::lock_guard<std::mutex> lock(mtx);

        auto now = std::chrono::steady_clock::now();

        for (auto it = lobbies.begin(); it != lobbies.end(); ) {
            Lobby* lobby = *it;
            bool should_delete = false;
            Player* disconnected_player = nullptr;
            Player* other_player = nullptr;

            // Check player1 disconnection
            if (lobby->player1) {
                auto time_since_seen = std::chrono::duration_cast<std::chrono::seconds>(
                    now - lobby->player1->last_seen).count();

                // If player is waiting alone and disconnects, remove after short threshold
                if (time_since_seen > SHORT_DISCONNECT_THRESHOLD && !lobby->player1->disconnected) {
                    std::cout << "Player " + lobby->player1->name + " has disconnected." << std::endl;
                    lobby->player1->disconnected = true;
                    notify_disconnection(lobby->player1, lobby);

                    // If player was alone in lobby, mark for immediate deletion
                    if (!lobby->player2) {
                        disconnected_player = lobby->player1;
                        should_delete = true;
                    }
                }
            }

            // Check player2 disconnection
            if (lobby->player2) {
                auto time_since_seen = std::chrono::duration_cast<std::chrono::seconds>(
                    now - lobby->player2->last_seen).count();
                if (time_since_seen > SHORT_DISCONNECT_THRESHOLD && !lobby->player2->disconnected) {
                    std::cout << "Player " + lobby->player2->name + " has disconnected." << std::endl;
                    lobby->player2->disconnected = true;
                    notify_disconnection(lobby->player2, lobby);
                }
            }

            // For full lobbies, handle long disconnections
            if (!should_delete && lobby->is_full()) {
                if (lobby->player1 && lobby->player1->disconnected) {
                    auto time_since_seen = std::chrono::duration_cast<std::chrono::seconds>(
                        now - lobby->player1->last_seen).count();
                    if (time_since_seen > LONG_DISCONNECT_THRESHOLD) {
                        disconnected_player = lobby->player1;
                        other_player = lobby->player2;
                        should_delete = true;
                    }
                }

                if (lobby->player2 && lobby->player2->disconnected) {
                    auto time_since_seen = std::chrono::duration_cast<std::chrono::seconds>(
                        now - lobby->player2->last_seen).count();
                    if (time_since_seen > LONG_DISCONNECT_THRESHOLD) {
                        disconnected_player = lobby->player2;
                        other_player = lobby->player1;
                        should_delete = true;
                    }
                }
            }

            if (should_delete) {
                if (other_player && !other_player->disconnected) {
                    SimpleJSON game_over_msg;
                    game_over_msg.assign_string("type", "game_over");
                    game_over_msg.assign_string("winner", other_player->name);
                    game_over_msg.assign_string("message", "Game Over! Opponent was disconnected for too long");
                    send_to_client(game_over_msg, other_player->address);
                }

                if (disconnected_player) {
                    players.erase(disconnected_player->name);
                    delete disconnected_player;
                }
                if (other_player) {
                    players.erase(other_player->name);
                    delete other_player;
                }
                std::cout << "Deleting lobby." << std::endl;
                delete lobby;
                it = lobbies.erase(it);
                continue;
            }
            ++it;
        }
    }
}

void game_server::handle_reconnection(const std::string& player_name, const sockaddr_in& new_addr) {
    auto it = players.find(player_name);
    if (it == players.end() || !it->second->disconnected) {
        SimpleJSON error_msg;
        error_msg.assign_string("type", "error");
        error_msg.assign_string("message", "Invalid reconnection attempt.");
        send_to_client(error_msg, new_addr);
        return;
    }

    auto player = it->second;
    auto lobby = find_player_lobby(player_name);

    if (!lobby) {
        return;
    }

    // Update player's connection info
    player->address = new_addr;
    player->disconnected = false;
    player->last_seen = std::chrono::steady_clock::now();

    // Unpause the lobby if both players are now connected
    if (lobby->player1 && lobby->player2 &&
        !lobby->player1->disconnected && !lobby->player2->disconnected) {
        lobby->paused = false;
    }

    // Send the current game state to the reconnected player
    SimpleJSON state_msg;
    state_msg.assign_string("type", "game_state_update");
    state_msg.assign_array("players", {lobby->player1->name, lobby->player2->name});
    state_msg.assign_string("current_player", remove_quotes(lobby->game_state["current_player"]));
    state_msg.assign_int("deck_size", lobby->deck.size());
    state_msg.assign_int("discard_pile", lobby->discard_pile.size());
    state_msg.assign_card("top_card", lobby->discard_pile.back());

    std::map<std::string, std::vector<Card>> hands;
    hands[lobby->player1->name] = lobby->player1->hand;
    hands[lobby->player2->name] = lobby->player2->hand;
    state_msg.assign_multiple_hands(hands);

    send_to_client(state_msg, new_addr);

    notify_reconnection(player, lobby);

    std::cout << "Player " << player_name << " reconnected successfully.\n";
}

void game_server::handle_message(const SimpleJSON& msg, const sockaddr_in& client_addr) {
    std::lock_guard<std::mutex> lock(mtx);
    if (msg["type"] == "connect") {
        std::string player_name = msg["name"];
        std::cout << "\nReceived connection request from: " << player_name << std::endl;

        // Check if this is a reconnection attempt
        auto it = players.find(player_name);
        if (it != players.end() && it->second->disconnected) {
            handle_reconnection(player_name, client_addr);
            return;
        }
        if(it != players.end() && !it->second->disconnected) {
            SimpleJSON error_msg;
            error_msg.assign_string("type","name_taken");
            error_msg.assign_string("message","Player name already in use. Choose a different name.");
            send_to_client(error_msg, client_addr);
            return;
        }

        player_name = remove_quotes(player_name);
        // Create new player
        Player* new_player = new Player{player_name, client_addr, std::chrono::steady_clock::now()};
        players[player_name] = new_player;

        // Find or create lobby
        Lobby* lobby = find_or_create_lobby(new_player);

        SimpleJSON ack;
        ack.assign_string("type","connect_ack");
        ack.assign_string("player_id",player_name);
        ack.assign_bool("waiting_for_player",!lobby->is_full());

        if (lobby->is_full()) {
            std::cout << "Lobby is full. Starting game with players: "
                      << lobby->player1->name << " and "
                      << lobby->player2->name << std::endl;
            // Add basic game state values
            ack.assign_array("players", {lobby->player1->name, lobby->player2->name});
            ack.assign_string("current_player", lobby->player1->name);
            ack.assign_int("deck_size", lobby->deck.size());
            ack.assign_int("discard_pile", lobby->discard_pile.size());
            ack.assign_card("top_card", lobby->discard_pile.back());

            // Create and assign the player hands
            std::map<std::string, std::vector<Card>> hands;
            hands[lobby->player1->name] = lobby->player1->hand;
            hands[lobby->player2->name] = lobby->player2->hand;

            ack.assign_multiple_hands(hands);
        } else {
            std::cout << "Waiting for another player to join lobby" << std::endl;
        }

        send_to_client(ack, client_addr);
        if (lobby->is_full()) {
            broadcast_game_state(lobby);
        }
    } else if (msg["type"] == "heartbeat") {
        std::string player_name = msg["name"];
        auto it = players.find(player_name);
        if (it != players.end()) {
            Player* player = it->second;
            player->last_seen = std::chrono::steady_clock::now();
            
            // If player was disconnected, handle reconnection
            if (player->disconnected) {
                handle_reconnection(player_name, client_addr);
            } else {
                // Update player's address in case it changed
                player->address = client_addr;
            }
        }
    } else if (msg["type"] == "play_card" || msg["type"] == "draw_card") {
        std::string player_name = msg["player_name"];
        auto lobby = find_player_lobby(player_name);

        if (lobby && lobby->is_full()) {
            if (remove_quotes(lobby->game_state["current_player"]) != player_name) {
                SimpleJSON error_msg;
                error_msg.assign_string("type","error");
                error_msg.assign_string("message","Not your turn yet.");
                send_to_client(error_msg, client_addr);
                return;
            }

            std::cout << "Player " << player_name << " "
                      << (msg["type"] == "play_card" ? "played a card" : "drew a card")
                      << std::endl;

            if (msg["type"] == "play_card") {
                std::string played_card = msg["card"];

                Player* current_player = (remove_quotes(lobby->game_state["current_player"]) == lobby->player1->name) ? lobby->player1 : lobby->player2;
                // Check if the player has the card in their hand
                auto it = std::find_if(current_player->hand.begin(), current_player->hand.end(),
                 [&played_card](const Card& card) {
                        return card.to_string() == played_card;  // Compare string representation
                });

                if (it == current_player->hand.end()) {
                    // Card not found in player's hand, send error message
                    SimpleJSON error_msg;
                    error_msg.assign_string("type","error");
                    error_msg.assign_string("message","You do not have this card in your hand.");
                    send_to_client(error_msg, client_addr);
                    return;
                }

                // Get the top card from discard pile to check if the move is valid
                Card top_card = lobby->discard_pile.back();

                // Check if the move is valid (same suit or same value)
                if (it->suit != top_card.suit && it->value != top_card.value) {
                    SimpleJSON error_msg;
                    error_msg.assign_string("type","error");
                    error_msg.assign_string("message","Invalid move. Card must match suit or value of the top card.");
                    send_to_client(error_msg, client_addr);
                    return;
                }

                // Handle special cards
                bool skip_next_turn = false;
                std::string message = "Player "+player_name+" played a card.";
                if (it->value == "7") {
                    // When 7 is played, next player draws 2 cards
                    Player* next_player = (current_player == lobby->player1) ? lobby->player2 : lobby->player1;
                    for (int i = 0; i < 2; i++) {
                        if (lobby->deck.size() > 0) {
                            Card drawn_card = lobby->deck.draw_card();
                            next_player->hand.push_back(drawn_card);
                        }
                    }
                    skip_next_turn = true;
                    message = "Player "+player_name+" played a seven, you take two cards and skip a turn.";
                } else if (it->value == "A") {
                    message = "Player "+player_name+" played an ace you skip a turn.";
                    skip_next_turn = true;
                }

                // Add the played card to the discard pile
                lobby->discard_pile.push_back(*it);

                // Update the game state
                lobby->game_state.assign_card("top_card", lobby->discard_pile.back());
                // Remove the card from player's hand
                current_player->hand.erase(it);

                std::map<std::string, std::vector<Card>> hands;
                hands[lobby->player1->name] = lobby->player1->hand;
                hands[lobby->player2->name] = lobby->player2->hand;

                SimpleJSON::NestedObject current_player_hand;
                current_player_hand[lobby->player1->name] = SimpleJSON::convert_hand_to_nested(lobby->player1->hand);
                current_player_hand[lobby->player2->name] = SimpleJSON::convert_hand_to_nested(lobby->player2->hand);

                lobby->game_state.assign_nested_object("current_player_hand", current_player_hand);

                lobby->game_state.assign_int("discard_pile",  lobby->discard_pile.size());
                lobby->game_state.assign_int("deck_size",  lobby->deck.size());


                // Check if player has won (0 cards)

                if (current_player->hand.empty()) {
                    SimpleJSON win_msg;
                    win_msg.assign_string("type", "game_over");
                    win_msg.assign_string("winner", remove_quotes(lobby->game_state["current_player"]));
                    win_msg.assign_string("message", "Game Over! winner: "+remove_quotes(lobby->game_state["current_player"]));
                    send_to_client(win_msg, lobby->player1->address);
                    send_to_client(win_msg, lobby->player2->address);

                    // Clean up the players from the players map
                    players.erase(lobby->player1->name);
                    players.erase(lobby->player2->name);

                    // Remove the lobby from the lobbies vector
                    auto it = std::find(lobbies.begin(), lobbies.end(), lobby);
                    if (it != lobbies.end()) {
                        lobbies.erase(it);
                    }

                    // Delete the player objects
                    delete lobby->player1;
                    delete lobby->player2;

                    // Delete the lobby
                    delete lobby;

                    return;
                }

                // Change current player
                if (!skip_next_turn) {
                    lobby->game_state.assign_string("current_player",(remove_quotes(lobby->game_state["current_player"]) ==
                        lobby->player1->name) ? lobby->player2->name : lobby->player1->name);
                }

                Player* other_player = (current_player == lobby->player1) ? lobby->player2 : lobby->player1;
                SimpleJSON play_card_msg;
                play_card_msg.assign_string("type", "player_played_card");
                play_card_msg.assign_string("player_name", current_player->name);
                play_card_msg.assign_string("message", message);
                send_to_client(play_card_msg, other_player->address);



            } else if (msg["type"] == "draw_card") {
                try {
                Player* current_player = (remove_quotes(lobby->game_state["current_player"]) == lobby->player1->name) ? lobby->player1 : lobby->player2;

                if (lobby->deck.size() == 0) {
                    // Deck is empty, refill it from the discard pile (excluding the topmost card)
                    if (lobby->discard_pile.size() > 1) {  // Ensure there's more than 1 card in the discard pile
                        // Exclude the topmost card from the discard pile
                        std::vector<Card> cards_to_refill(lobby->discard_pile.begin(), lobby->discard_pile.end() - 1);

                        // Shuffle the refill cards and put them in the deck
                        std::random_device rd;
                        std::mt19937 g(rd());
                        std::shuffle(cards_to_refill.begin(), cards_to_refill.end(), g);

                        // Add the shuffled cards back to the deck
                        lobby->deck.cards.insert(lobby->deck.cards.end(), cards_to_refill.begin(), cards_to_refill.end());

                        // Clear the discard pile, excluding the top card
                        lobby->discard_pile.erase(lobby->discard_pile.begin(), lobby->discard_pile.end() - 1);
                    } else {
                        // If there are not enough cards to refill the deck, send an error message
                        SimpleJSON error_msg;
                        error_msg.assign_string("type","error");
                        error_msg.assign_string("message","Not enough cards in discard pile to refill deck.");
                        send_to_client(error_msg, client_addr);
                        return;
                    }
                }

                // Now that the deck is refilled (or still has cards), draw a card
                Card drawn_card = lobby->deck.draw_card();  // Use Deck's draw_card method
                current_player->hand.push_back(drawn_card); // Add the card to the player's han
                std::map<std::string, std::vector<Card>> hands;
                hands[lobby->player1->name] = lobby->player1->hand;
                hands[lobby->player2->name] = lobby->player2->hand;

                SimpleJSON::NestedObject current_player_hand;
                current_player_hand[lobby->player1->name] = SimpleJSON::convert_hand_to_nested(lobby->player1->hand);
                current_player_hand[lobby->player2->name] = SimpleJSON::convert_hand_to_nested(lobby->player2->hand);

                lobby->game_state.assign_nested_object("current_player_hand", current_player_hand);

                lobby->game_state.assign_int("discard_pile",  lobby->discard_pile.size());
                lobby->game_state.assign_int("deck_size",  lobby->deck.size());
                lobby->game_state.assign_string("current_player",(remove_quotes(lobby->game_state["current_player"]) == lobby->player1->name) ? lobby->player2->name : lobby->player1->name);

                    // Notify the other player
                    Player* other_player = (current_player == lobby->player1) ? lobby->player2 : lobby->player1;
                    SimpleJSON draw_card_msg;
                    draw_card_msg.assign_string("type", "player_drawn_card");
                    draw_card_msg.assign_string("player_name", current_player->name);
                    draw_card_msg.assign_string("message", "Player "+player_name+" draw a card.");
                    send_to_client(draw_card_msg, other_player->address);
            }
            catch (const std::runtime_error& e) {
                // If the deck is empty and discard pile couldn't refill it, send an error message
                SimpleJSON error_msg;
                error_msg.assign_string("type","error");
                error_msg.assign_string("message",e.what());
                send_to_client(error_msg, client_addr);
                return;
            }
        }

            // Broadcast the updated game state
            broadcast_game_state(lobby);
        }
    }
}

void game_server::broadcast_game_state(Lobby* lobby) {
    if (!lobby || !lobby->is_full()) return;

    SimpleJSON state_update;
    state_update.assign_string("type", "game_state_update");

    // Add basic game state values
    state_update.assign_array("players", {lobby->player1->name, lobby->player2->name});
    state_update.assign_string("current_player",remove_quotes(lobby->game_state["current_player"]));

    state_update.assign_int("deck_size", lobby->deck.size());
    state_update.assign_int("discard_pile", lobby->discard_pile.size());
    state_update.assign_card("top_card", lobby->discard_pile.back());

    // Create and assign the player hands
    std::map<std::string, std::vector<Card>> hands;
    hands[lobby->player1->name] = lobby->player1->hand;
    hands[lobby->player2->name] = lobby->player2->hand;

    state_update.assign_multiple_hands(hands);  // Assuming assign_multiple_hands can handle this


    // Send the updated game state to both players
    send_to_client(state_update, lobby->player1->address);
    send_to_client(state_update, lobby->player2->address);
}


    void game_server::send_to_client(SimpleJSON& msg, const sockaddr_in& client_addr) {
        // Serialize the SimpleJSON object to a string
        std::string data = msg.serialize();

        // Send the serialized string over the socket
        sendto(server_socket, data.c_str(), data.length(), 0,
               (struct sockaddr*)&client_addr, sizeof(client_addr));
    }

void game_server::start() {
    std::cout << "UDP Server started. Waiting for players..." << std::endl;

    // Start disconnection checker thread
    std::thread checker_thread(&game_server::check_disconnections, this);
    checker_thread.detach();

    char buffer[4096];
    sockaddr_in client_addr{};
    int client_len = sizeof(client_addr);

    while (running) {
        memset(buffer, 0, sizeof(buffer));
        #ifdef _WIN32
            int bytesRead = recvfrom(server_socket, buffer, sizeof(buffer), 0,
                                   (struct sockaddr*)&client_addr, &client_len);
        #else
            ssize_t bytesRead = recvfrom(server_socket, buffer, sizeof(buffer), 0,
                                       (struct sockaddr*)&client_addr, (socklen_t*)&client_len);
        #endif

        if (bytesRead > 0) {
            try {
                SimpleJSON msg;
                msg.deserialize_object(std::string(buffer, bytesRead)); // Deserialize buffer to SimpleJSON
                handle_message(msg, client_addr);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing message: " << e.what() << std::endl;
                SimpleJSON error;
                error.assign_string("type", "unknown");
                send_to_client(error, client_addr);

            }
        }
    }
}

void game_server::stop() {
    running = false;
}

std::string game_server::remove_quotes(const std::string& str) {
    if (!str.empty() && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.size() - 2);
    }
    return str;
}

game_server::~game_server() {
    for (auto lobby : lobbies) {
        delete lobby->player1;
        delete lobby->player2;
        delete lobby;
    }
    lobbies.clear();
    players.clear();

    CLOSE_SOCKET(server_socket);
    #ifdef _WIN32
        WSACleanup();
    #endif
}
