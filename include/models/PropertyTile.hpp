#ifndef PROPERTY_TILE_HPP
#define PROPERTY_TILE_HPP

#include "Tile.hpp"
#include <string>

class PropertyTile : public Tile {
protected:
    int price;
    int ownerId; // -1 jika milik Bank
    int status; // 0: BANK, 1: OWNED, 2: MORTGAGED
public:
    PropertyTile();
    void onLanded() override;
    
    void buy(); 
    void payRent(); 
    void mortgage(); 
    void unmortgage();
};

class StreetTile : public PropertyTile {
private:
    std::string colorGroup;
    int houseCount;
    bool hasHotel;
public:
    StreetTile();
    void buildHouse(); 
    void buildHotel(); 
    void triggerFestival();
};

class RailroadTile : public PropertyTile {
public:
    RailroadTile();
    void calculateRent(); 
};

class UtilityTile : public PropertyTile {
public:
    UtilityTile();
    void calculateRent(); 
};

#endif