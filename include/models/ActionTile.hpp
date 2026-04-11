#ifndef ACTION_TILE_HPP
#define ACTION_TILE_HPP

#include "Tile.hpp"

class ActionTile : public Tile {
public:
    ActionTile();
    virtual void onLanded() override = 0; 
};

class SpecialTile : public ActionTile {
private:
    int specialType;
public:
    SpecialTile();
    void onLanded() override;
    
    void triggerEffect(); 
};

class TaxTile : public ActionTile {
private:
    int taxType; // 0: PPH , 1: PBM
public:
    TaxTile();
    void onLanded() override;
    void chargeTax(); 
};

class FestivalTile : public ActionTile {
public:
    FestivalTile();
    void onLanded() override;
    void triggerFestival(); 
};

#endif