#pragma once
#include "../core/Player.hpp"
#include <string>
using namespace std;
#include <bits/stdc++.h>

class Tile {
  protected:
    int id;
    std::string kode;
    std::string name;

  public:
    Tile(int id, string kode, string name);
    virtual EffectType onLanded(Player &player) = 0;
    virtual ~Tile();
    int getIndex() const;
    string getKode() const;
    string getName() const;
    virtual int getOwnerId() const = 0;
    virtual bool isStreet() const { return false; }
    virtual string getColorGroup() const { return ""; }
    virtual int calcRent(int diceRoll = 0) const = 0;
    virtual int calcValue() const = 0;
    virtual int getRentLevel() const { return 0; }
    virtual void demolish();
};
