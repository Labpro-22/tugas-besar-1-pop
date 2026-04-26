#ifndef PROPERTY_TILE_HPP
#define PROPERTY_TILE_HPP

#include "../core/Enums.hpp"
#include "../models/Tile.hpp"
#include <string>

class PropertyTile : public Tile {
  protected:
    int price;
    int mortgageValue;
    int ownerId; // -1 jika milik Bank
    int status;  // 0: BANK, 1: OWNED, 2: MORTGAGED
    Player *ownerPtr = nullptr; // pointer ke owner (untuk SaveLoadManager)

  public:
    PropertyTile(int index, const std::string &code, const std::string &name,
                 int buyPrice, int mortgageValue);
    virtual ~PropertyTile() = default;
    EffectType onLanded(Player &player) override;

    virtual int calcRent(int diceRoll = 0) const = 0;
    virtual int calcValue() const = 0;
    virtual void buy() {}
    virtual void payRent() {}
    virtual void mortgage();
    virtual void unmortgage();
    int getmortgageValue() const;
    int getPrice() const;
    int getOwnerId() const override;
    int getStatus() const;
    void setOwnerId(int id) { ownerId = id; }
    void setStatus(int s) { status = s; }
    virtual void sellTobank(Player &owner);

    // --- API untuk SaveLoadManager (PropertyStatus enum wrapper) ---
    PropertyStatus getPropertyStatus() const;
    void setPropertyStatus(PropertyStatus ps);
    const Player *getOwner() const { return ownerPtr; }
    void setOwner(Player *p) { ownerPtr = p; if (p) ownerId = p->getId(); else ownerId = -1; }

    bool isProperty() const override { return true; }
    std::string getTileCategory() const override { return "PROPERTY"; }

    // Virtual stubs untuk festival info (override di StreetTile)
    virtual int getFestivalMultiplier() const { return 1; }
    virtual int getFestivalDuration()   const { return 0; }
};

class StreetTile : public PropertyTile {

  public:
    StreetTile(int index, const std::string &code, const std::string &name,
               const std::string &colorGroup, int buyPrice, int mortgageValue,
               const std::vector<int> &rentTable, int houseCost, int hotelCost);

    int calcRent(int diceRoll = 0) const override;
    int calcValue() const override;
    EffectType onLanded(Player &player) override;

    // jual properti ke bank
    void sellTobank(Player &owner) override;

    // fungsi dan prosedur untuk mengatur terkait bangun membangun.

    void buildHouse();
    void buildHotel();
    void demolish() override;
    void removeOneBuilding(); // Kurangi 1 level bangunan (untuk DemolitionCard)
    
    int getHouseCost() const override;
    int getHotelCost() const override;
    int getRentLevel() const override;

    // --- Type-query overrides ---
    bool isStreet() const override { return true; }
    std::string getTileCategory() const override { return "STREET"; }
    std::string getColorGroup() const override;

    // --- Building-info overrides (menggantikan dynamic_cast ke StreetTile) ---
    bool hasBuildings()            const override;
    int  getHouseCount()           const override;
    bool hasHotel()                const override;
    int  calcBuildingResaleValue() const override;

    void setMonopolized(bool val);

    // efek festival
    void setFestivalEffect(int multiplier);
    void clearFestivalEffect();
    int  getFestivalMultiplier() const override;  // untuk SaveLoadManager
    int  getFestivalDuration()   const override;  // untuk SaveLoadManager
    void setFestivalMultiplier(int m);   // untuk SaveLoadManager
    void setFestivalDuration(int d);     // untuk SaveLoadManager

    // setter untuk building state (untuk SaveLoadManager)
    void setHotel(bool val);
    void setHouseCount(int count);

  private:
    std::string colorGroup;
    std::vector<int> rentTable; // Indeks sesuai RentLevel
    int houseCost;
    int hotelCost;
    int rentLevel;
    int festivalEffectMultiplier = 1;
    bool isMonopolized = false;
    bool isFestivalEffectActive = false;
    bool hasBuilding = false;
};

class RailroadTile : public PropertyTile {
  public:
    RailroadTile(int index, const std::string &code, const std::string &name,
                 int mortgageValue, const map<int, int> &rentByCount);

    int calcRent(int diceRoll = 0) const override;
    int calcValue() const override;
    EffectType onLanded(Player &player) override;
    int getRailroadOwnedCount() const;
    void setrailroadOwnedCount(int count);

    // --- Type-query overrides ---
    bool isRailroad() const override { return true; }
    std::string getTileCategory() const override { return "RAILROAD"; }

  private:
    map<int, int> rentByCount; // jumlah railroad -> sewa
    int railroadOwnedCount = 1;
};

class UtilityTile : public PropertyTile {

  public:
    UtilityTile(int index, const std::string &code, const std::string &name,
                int mortgageValue, const std::map<int, int> &multiplierByCount);

    // Perhitungan biaya sewa Utility ditentukan oleh total angka dadu yang
    // didapatkan pemain ketika mendarat,
    // dikalikan dengan faktor pengali yang bergantung pada jumlah petak Utility
    // yang dimiliki oleh pemiliknya
    int calcRent(int diceRoll = 0) const override;
    int calcValue() const override;
    EffectType onLanded(Player &player) override;
    void setUtilityOwnedCount(int count);
    void setLastDiceRoll(int dice);
    int getUtilityOwnedCount() const;
    int getLastDiceRoll() const;

    // --- Type-query overrides ---
    bool isUtility() const override { return true; }
    std::string getTileCategory() const override { return "UTILITY"; }

  private:
    std::map<int, int> multiplierByCount; // jumlah utility -> faktor pengali
    int utilityOwnedCount = 1;
    int lastDiceRoll = 1;
};

#endif
