#ifndef PROPERTY_TILE_HPP
#define PROPERTY_TILE_HPP

#include "../models/Tile.hpp"
#include <string>

class PropertyTile : public Tile {

protected:
    int mortgageValue;
    int price;
    int ownerId; // -1 jika milik Bank
    int status; // 0: BANK, 1: OWNED, 2: MORTGAGED

public:
    PropertyTile(int index, const std::string& code, const std::string& name,
                 int buyPrice, int mortgageValue);
    virtual ~PropertyTile();
    void onLanded(Player& player) override;

    virtual int calcRent(int diceRoll = 0) const = 0;
    void buy(); 
    void payRent(); 
    virtual void mortgage(); 
    virtual void unmortgage();  
    virtual void sellTobank(Player& owner);
};


class StreetTile : public PropertyTile {

public:
    StreetTile(int index, const std::string& code, const std::string& name,
               const std::string& colorGroup,
               int buyPrice, int mortgageValue,
               const std::vector<int>& rentTable,
               int houseCost, int hotelCost);
 
    int calcRent(int diceRoll = 0) const override;
    void onLanded(Player& player) override;
 
    // jual properti ke bank
    void sellTobank(Player& owner) override;
 
    // fungsi dan prosedur untuk mengatur terkait bangun membangun.

    void buildHouse();
    void buildHotel();
    void demolish();
    bool hasBuildings();
    int getHouseCost();
    int getHotelCost();
 
    // Color group
    string getColorGroup() const;
 
    /** Hitung nilai jual bangunan saat ini (50% dari total harga beli bangunan) */
    int calcBuildingResaleValue() const;
    void setMonopolized(bool val);
 
private:
    std::string       colorGroup;
    std::vector<int>  rentTable;  // Indeks sesuai RentLevel
    int houseCost;
    int rentLevel;
    int hotelCost;
    int festivalEffectMultiplier  = 1;
    bool isMonopolized = false;
    bool isFestivalEffectActive = false;
    bool hasBuilding = false;

};


class RailroadTile : public PropertyTile {
public:
    RailroadTile(int index, const std::string& code, const std::string& name,
                 int mortgageValue,
                 const map<int, int>& rentByCount);

    int calcRent(int diceRoll = 0) const override;
    void onLanded(Player& player) override;
 
 
private:
    map<int, int> rentByCount;  // jumlah railroad -> sewa
    
};


class UtilityTile : public PropertyTile {
    
public:
    UtilityTile(int index, const std::string& code, const std::string& name,
                int mortgageValue,
                const std::map<int, int>& multiplierByCount);

    // Perhitungan biaya sewa Utility ditentukan oleh total angka dadu yang didapatkan pemain ketika mendarat, 
    //dikalikan dengan faktor pengali yang bergantung pada jumlah petak Utility yang dimiliki oleh pemiliknya            
    int calcRent(int diceRoll = 0) const override;
    void onLanded(Player& player) override;
 
private:
    std::map<int, int> multiplierByCount;  // jumlah utility -> faktor pengali
};


#endif
