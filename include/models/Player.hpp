#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string>
#include <vector>
#include "Card.hpp"

class Player {
private:
    std::string username;
    int money;
    int position;
    int status; // 0: ACTIVE, 1: JAILED, 2: BANKRUPT
    std::vector<int> ownedPropertyIds;
    std::vector<SkillCard*> handCards;
public:
    Player();
    
    void move(); 
    void goToJail(); 
    void declareBankruptcy();
    
    // Operator Overloading
    Player& operator+=(int amount);
    Player& operator-=(int amount);
    bool operator<(const Player& other) const; 
};

#endif