#ifndef AUCTION_HPP
#define AUCTION_HPP

#include <vector>
#include "Player.hpp"
#include "PropertyTile.hpp"

class Auction {
private:
    PropertyTile* targetProperty;
    std::vector<Player*> activeBidders;
    int currentHighestBid;
    Player* currentHighestBidder;
    int currentBidderIndex;
    
public:
    Auction();
    
    void initialize();
    void placeBid();
    void passTurn();
    void resolve();
};

#endif