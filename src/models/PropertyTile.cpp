#include "../../include/models/PropertyTile.hpp"

PropertyTile::PropertyTile(int index, const std::string &code,
                           const std::string &name, int buyPrice,
                           int mortgageValue)
    : Tile(index, code, name), price(buyPrice), mortgageValue(mortgageValue),
      ownerId(-1), status(0) {}

EffectType PropertyTile::onLanded(Player &player) {
    if (status == 0) {
        return EffectType::OFFER_BUY;
    } else if (status == 1) {
        if (ownerId == player.getId()) {
            return EffectType::ALREADY_OWNED_SELF;
        } else {
            return EffectType::PAY_RENT;
        }
    }
    return EffectType::NONE;
}

void PropertyTile::mortgage() {
    if (status != 1) {
        return;
    }
    status = 2;
}

void PropertyTile::unmortgage() {
    if (status != 2) {
        return;
    }
    status = 1;
}

void PropertyTile::sellTobank(Player &owner) {
    (void)owner;
    ownerId = -1;
    status = 0;
}

int PropertyTile::getmortgageValue() const { return mortgageValue; }
int PropertyTile::getPrice() const { return price; }
int PropertyTile::getOwnerId() const { return ownerId; }
int PropertyTile::getStatus() const { return status; }

PropertyStatus PropertyTile::getPropertyStatus() const {
    if (status == 1) return PropertyStatus::OWNED;
    if (status == 2) return PropertyStatus::MORTGAGED;
    return PropertyStatus::BANK;
}

void PropertyTile::setPropertyStatus(PropertyStatus ps) {
    if (ps == PropertyStatus::OWNED)     status = 1;
    else if (ps == PropertyStatus::MORTGAGED) status = 2;
    else                                 status = 0;
}

// Street Tile
StreetTile::StreetTile(int index, const std::string &code,
                       const std::string &name, const std::string &colorGroup,
                       int buyPrice, int mortgageValue,
                       const std::vector<int> &rentTable, int houseCost,
                       int hotelCost)
    : PropertyTile(index, code, name, buyPrice, mortgageValue),
      colorGroup(colorGroup), rentTable(rentTable), houseCost(houseCost),
      hotelCost(hotelCost), rentLevel(0) {}

int StreetTile::calcRent(int diceRoll) const {
    (void)diceRoll;
    int cur = 0;
    if (rentLevel == 0) {
        if (isMonopolized) {
            cur = 2 * rentTable[0]; // Monopoli tanpa bangunan = sewa dasar x2
        } else {
            cur = rentTable[0]; // Sewa dasar biasa
        }
    } else {
        cur = rentTable[rentLevel];
    }

    if (isFestivalEffectActive) {
        cur *= festivalEffectMultiplier;
    }
    return cur;
}

EffectType StreetTile::onLanded(Player &player) {
    if (this->status == 0) {
        return EffectType::OFFER_BUY;
    } else if (this->status == 1) {
        if (this->ownerId == player.getId()) {
            return EffectType::ALREADY_OWNED_SELF;
        } else {
            return EffectType::PAY_RENT;
        }
    }
    // Status 2 = Digadaikan
    return EffectType::NONE;
}

void StreetTile::sellTobank(Player &owner) {
    (void)owner;
}

void StreetTile::buildHouse() {
    if (!isMonopolized) {
        return;
    }
    if (rentLevel >= 4) {
        return;
    }

    rentLevel++;
    hasBuilding = true;
}

void StreetTile::buildHotel() {
    if (!isMonopolized)
        return;
    if (rentLevel != 4)
        return;

    rentLevel = 5;
}

void StreetTile::demolish() {
    if (!hasBuilding)
        return;
    rentLevel = 0;
    hasBuilding = false;
}

void StreetTile::removeOneBuilding() {
    if (rentLevel <= 0) return;
    rentLevel--;
    if (rentLevel == 0) hasBuilding = false;
}

bool StreetTile::hasBuildings() const { return this->hasBuilding; }

int StreetTile::getHouseCount() const {
    return (rentLevel < 5) ? rentLevel : 0;
}

bool StreetTile::hasHotel() const { return rentLevel == 5; }

int StreetTile::getHouseCost() const { return this->houseCost; }

int StreetTile::getHotelCost() const { return this->hotelCost; }

int StreetTile::getRentLevel() const { return this->rentLevel; }

string StreetTile::getColorGroup() const { return this->colorGroup; }

void StreetTile::setMonopolized(bool val) { this->isMonopolized = val; }

int StreetTile::calcBuildingResaleValue() const {
    return getPrice() + 0.5 * calcValue();
}

int StreetTile::calcValue() const {
    if (!hasBuilding)
        return 0;
    if (rentLevel == 5)
        return (4 * houseCost) + hotelCost;
    else
        return rentLevel * houseCost;
}

void StreetTile::setFestivalEffect(int multiplier) {
    if (isFestivalEffectActive) {
        this->festivalEffectMultiplier *= multiplier;
    } else {
        this->isFestivalEffectActive = true;
        this->festivalEffectMultiplier = multiplier;
    }
}

void StreetTile::clearFestivalEffect() {
    this->isFestivalEffectActive = false;
    this->festivalEffectMultiplier = 1;
}

int StreetTile::getFestivalMultiplier() const {
    return festivalEffectMultiplier;
}

int StreetTile::getFestivalDuration() const {
    return 0;
}

void StreetTile::setFestivalMultiplier(int m) {
    if (m > 1) {
        isFestivalEffectActive = true;
        festivalEffectMultiplier = m;
    }
}

void StreetTile::setFestivalDuration(int d) {
    (void)d;
}

void StreetTile::setHotel(bool val) {
    if (val) {
        rentLevel = 5;
        hasBuilding = true;
    } else {
        if (rentLevel == 5) rentLevel = 0;
        hasBuilding = (rentLevel > 0);
    }
}

void StreetTile::setHouseCount(int count) {
    rentLevel = count;
    hasBuilding = (count > 0);
}

// RailroadTile

RailroadTile::RailroadTile(int index, const std::string &code,
                           const std::string &name, int mortgageValue,
                           const map<int, int> &rentByCount)
    : PropertyTile(index, code, name, 0, mortgageValue),
      rentByCount(rentByCount) {}

int RailroadTile::calcRent(int diceRoll) const {
    (void)diceRoll;
    auto it = rentByCount.find(railroadOwnedCount);
    if (it != rentByCount.end()) {
        return it->second;
    }

    return 0;
}

int RailroadTile::calcValue() const { return getmortgageValue(); }

EffectType RailroadTile::onLanded(Player &player) {
    if (this->status == 0) {
        return EffectType::AUTO_ACQUIRE;
    } else if (this->status == 1) {
        if (this->ownerId == player.getId()) {
            return EffectType::ALREADY_OWNED_SELF;
        } else {
            return EffectType::PAY_RENT;
        }
    }
    return EffectType::NONE;
}

int RailroadTile::getRailroadOwnedCount() const {
    return this->railroadOwnedCount;
}

void RailroadTile::setrailroadOwnedCount(int count) {
    railroadOwnedCount = count;
}

// UtilityTile

UtilityTile::UtilityTile(int index, const std::string &code,
                         const std::string &name, int mortgageValue,
                         const std::map<int, int> &multiplierByCount)
    : PropertyTile(index, code, name, 0, mortgageValue),
      multiplierByCount(multiplierByCount) {}

int UtilityTile::calcRent(int diceroll) const {
    auto it = multiplierByCount.find(utilityOwnedCount);
    if (it != multiplierByCount.end()) {
        return diceroll * it->second;
    }
    return 0;
}

int UtilityTile::calcValue() const { return getmortgageValue(); }

EffectType UtilityTile::onLanded(Player &player) {
    if (this->status == 0) {
        return EffectType::AUTO_ACQUIRE;
    } else if (this->status == 1) {
        if (this->ownerId == player.getId()) {
            return EffectType::ALREADY_OWNED_SELF;
        } else {
            return EffectType::PAY_RENT;
        }
    }
    return EffectType::NONE;
}

void UtilityTile::setUtilityOwnedCount(int count) {
    this->utilityOwnedCount = count;
}

void UtilityTile::setLastDiceRoll(int dice) { this->lastDiceRoll = dice; }

int UtilityTile::getUtilityOwnedCount() const {
    return this->utilityOwnedCount;
}

int UtilityTile::getLastDiceRoll() const { return this->lastDiceRoll; }