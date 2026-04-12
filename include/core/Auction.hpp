#ifndef AUCTION_HPP
#define AUCTION_HPP

#include "../models/PropertyTile.hpp"
#include "Player.hpp"
#include <vector>

class Auction {
  private:
    PropertyTile *targetProperty;
    std::vector<Player *> activeBidders;
    int currentHighestBid;
    Player *currentHighestBidder;
    int currentBidderIndex;

  public:
    Auction(PropertyTile *property,
            const std::vector<Player *> &initialBidders);

    // Run utama
    void run();

    // Method memasang bid untuk player pada gilirannya 
    bool placeBid(Player *bidder, int amount);

    // Method pass untuk player pada gilirannya
    void passTurn(Player *bidder);

    // Menentukan pemenang bid dan merubah kepemilikan properti
    void delegateProperty();
};

#endif
