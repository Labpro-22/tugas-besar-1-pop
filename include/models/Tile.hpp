#pragma once
#include "../core/Player.hpp"
#include <map>
#include <string>
#include <vector>

using namespace std;

class Tile {
  protected:
    int id;
    std::string kode;
    std::string name;

  public:
    Tile(int id, string kode, string name);
    virtual EffectType onLanded(Player &player) = 0;
    virtual ~Tile() = default;
    int getIndex() const;
    string getKode() const;
    string getName() const;
    virtual int getOwnerId() const = 0;
    virtual int calcRent(int diceRoll = 0) const = 0;
    virtual int calcValue() const = 0;
    virtual void demolish() {}

    virtual bool isProperty()   const { return false; }
    virtual bool isStreet()     const { return false; }
    virtual bool isRailroad()   const { return false; }
    virtual bool isUtility()    const { return false; }
    virtual bool isCardTile()   const { return false; }
    virtual bool isTaxTile()    const { return false; }
    virtual bool isFestivalTile() const { return false; }
    virtual bool isJailTile()    const { return false; }
    virtual bool isGoToJailTile() const { return false; }
    virtual bool isGoTile()      const { return false; }

    virtual std::string getTileCategory() const { return "SPECIAL"; }

    virtual std::string getColorGroup() const { return ""; }
    virtual int getRentLevel() const { return 0; }

    virtual bool hasBuildings()           const { return false; }
    virtual int  getHouseCount()          const { return 0; }
    virtual bool hasHotel()               const { return false; }
    virtual int  calcBuildingResaleValue() const { return 0; }
    virtual int  getHouseCost()           const { return 0; }
    virtual int  getHotelCost()           const { return 0; }
};
