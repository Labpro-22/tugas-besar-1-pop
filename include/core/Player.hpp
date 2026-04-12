#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <string>
#include <vector>
#include "Card.hpp"

using namespace std;

class PropertyTile;
class SkillCard;

enum class PlayerStatus {
    ACTIVE,
    JAILED,
    BANKRUPT
};

class Player {
private:
    string username;
    int money;
    int position;
    vector<int> ownedProperties;
    vector<SkillCard*> handCards;

    int jailTurnsLeft;
    int doubleStreak;
    bool hasUsedCardThisTurn;
    bool shieldActive;
public:
    Player(string username, int startingMoney);
    
    void move(int steps);
    void goToJail();
    void declareBankruptcy();
    void resetTurnFlags();

    int getWealth() const; 
    string getUsername() const;
    int getMoney() const;
    int getPosition() const;
    PlayerStatus getStatus() const;
    const vector<SkillCard*>& getHandCards() const;
    const vector<PropertyTile*>& getOwnedProperties() const;

    void addCard(SkillCard* card);
    void removeCard(int index);
    void addProperty(PropertyTile* property);
    void removeProperty(PropertyTile* property);

    void setShieldActive(bool active);
    void incrementDoubleStreak();
    void decrementJailTurn();
    
    // Operator Overloading
    Player& operator+=(int amount);
    Player& operator-=(int amount);
    bool operator<(const Player& other) const; 
};

#endif