#include "../../include/core/Player.hpp"
#include "../../include/core/Exceptions.hpp"
#include "../../include/models/PropertyTile.hpp"
#include "../../include/models/Card.hpp"
#include <algorithm>

Player::Player(string usn, int startingMoney) 
    : username(usn), money(startingMoney), position(0), status(PlayerStatus::ACTIVE), jailTurnsLeft(0), doubleStreak(0), hasUsedCardThisTurn(false), shieldActive(false) {}

void Player::move(int steps) {
    position = (position + steps) % 40;
}

void Player::goToJail() {
    status = PlayerStatus::JAILED;
    jailTurnsLeft = 3;
    position = 10;
}

void Player::declareBankruptcy() {
    status = PlayerStatus::BANKRUPT;
    money = 0;
}

void Player::resetTurnFlags() {
    hasUsedCardThisTurn = false;
    doubleStreak = 0;
    shieldActive = false;
}

void Player::setShieldActive(bool active) {
    shieldActive = active;
}

void Player::incrementDoubleStreak() {
    doubleStreak++;
}

void Player::decrementJailTurn() {
    if (jailTurnsLeft > 0) {
        jailTurnsLeft--;
    }
}

void Player::addCard(SkillCard* card) {
    if (handCards.size() >= 3) {
        throw MaxCardLimitException();
    }
    handCards.push_back(card);
}

void Player::removeCard(int idx) {
    if (idx >= 0 && idx < handCards.size()) {
        handCards.erase(handCards.begin() + idx);
    }
}

void Player::addProperty(PropertyTile* property) {
    ownedProperties.push_back(property);
}

void Player::removeProperty(PropertyTile* property) {
    auto it = find(ownedProperties.begin(), ownedProperties.end(), property);
    if (it != ownedProperties.end()) {
        ownedProperties.erase(it);
    }
}

string Player::getUsername() const { return username; }
int Player::getMoney() const { return money; }
int Player::getPosition() const { return position; }
PlayerStatus Player::getStatus( ) const { return status; }
const vector<SkillCard*>& Player::getHandCards() const { return handCards; }
const vector<PropertyTile*>& Player::getOwnedProperties() const { return ownedProperties; }
int Player::getJailTurnsLeft() const { return jailTurnsLeft; }
int Player::getDoubleStreak() const { return doubleStreak; }
bool Player::getHasUsedCardThisTurn() const { return hasUsedCardThisTurn; }
bool Player::isShieldActive() const { return shieldActive; }
int Player::getActiveDiscountPercent() const { return activeDiscount; }

void Player::setHasUsedCardThisTurn(bool used) { hasUsedCardThisTurn = used; }
void Player::setActiveDiscountPercent(int percent) { activeDiscount = percent; }

int Player::getWealth() const {
    int totalWealth = money;
    
    for (PropertyTile* prop : ownedProperties) {
        totalWealth += prop->getPrice();
        totalWealth += prop->calcValue();
    }

    return totalWealth;
}

Player& Player::operator-=(int amount) {
    if (amount <= 0) return *this;
    
    if (money < amount) {
        throw NotEnoughMoneyException(money, amount, "transaksi (" + username + ")");
    }
    money -= amount;
    return *this;
}

bool Player::operator<(const Player& other) const {
    return this->getWealth() < other.getWealth();
}