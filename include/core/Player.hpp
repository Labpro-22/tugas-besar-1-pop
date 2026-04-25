#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "../models/Card.hpp"
#include <string>
#include <vector>

using namespace std;

class PropertyTile;
class SkillCard;

class Player {
  private:
    int playerId;
    string username;
    int money;
    int position;
    PlayerStatus status;
    vector<PropertyTile *> ownedProperties;
    vector<SkillCard *> handCards;

    int activeDiscount;
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

    int getId() const;
    void setId(int id);

    int getWealth() const;
    string getUsername() const;
    int getMoney() const;
    int getPosition() const;
    PlayerStatus getStatus() const;
    const vector<SkillCard *> &getHandCards() const;
    const vector<PropertyTile *> &getOwnedProperties() const;
    int getJailTurnsLeft() const;
    int getDoubleStreak() const;
    bool getHasUsedCardThisTurn() const;
    bool isShieldActive() const;
    int getActiveDiscountPercent() const;

    void setHasUsedCardThisTurn(bool used);
    void setActiveDiscountPercent(int percent);
    void setMoney(int amount);
    void setPosition(int pos);
    void setStatus(PlayerStatus s);
    void setJailTurnsLeft(int turns);

    void addCard(SkillCard *card);
    void removeCard(int index);
    void addProperty(PropertyTile *property);
    void removeProperty(PropertyTile *property);

    void setShieldActive(bool active);
    void incrementDoubleStreak();
    void decrementJailTurn();

    // Operator Overloading
    Player &operator+=(int amount);
    Player &operator-=(int amount);
    bool operator<(const Player &other) const;
};

#endif
