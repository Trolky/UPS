#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>
#include <chrono>
#include "card.h"
#include "deck.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSE_SOCKET closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #define CLOSE_SOCKET close
#endif


struct Player {
    std::string name;
    sockaddr_in address;         // Player's network address
    std::chrono::steady_clock::time_point last_seen;
    std::vector<Card> hand;      // Cards in hand
    bool disconnected;
    std::chrono::steady_clock::time_point disconnect_time; // New field to track when disconnect happened
};

#endif // PLAYER_H
