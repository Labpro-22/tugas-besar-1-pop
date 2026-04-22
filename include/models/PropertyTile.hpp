#ifndef PROPERTY_TILE_HPP
#define PROPERTY_TILE_HPP

#include "../models/Tile.hpp"
#include <string>
enum class EffectType {
    NONE,               // Tidak ada efek (FreeParkingTile)
 
    // --- Properti ---
    OFFER_BUY,          // Tawarkan pembelian street ke pemain aktif
    AUTO_ACQUIRE,       // Kepemilikan otomatis tanpa beli (Railroad / Utility)
    PAY_RENT,           // Pemain harus bayar sewa ke owner
    ALREADY_OWNED_SELF, // Pemain mendarat di propertinya sendiri, tidak ada aksi
 
    // --- Pajak ---
    PAY_TAX_FLAT,       // Bayar pajak flat langsung (PBM)
    PAY_TAX_CHOICE,     // Pemain pilih flat atau persentase (PPH)
 
    // --- Kartu ---
    DRAW_CHANCE,        // Ambil kartu Kesempatan
    DRAW_COMMUNITY,     // Ambil kartu Dana Umum
 
    // --- Festival ---
    FESTIVAL_TRIGGER,   // Pemain pilih properti untuk efek festival
 
    // --- Spesial ---
    AWARD_SALARY,       // Berikan gaji Go ke pemain
    SEND_TO_JAIL,       // Pindahkan pemain ke penjara
    JAIL_TURN,          // Pemain sedang di penjara, proses giliran penjara
    JUST_VISITING,      // Pemain mampir di penjara, tidak ada aksi
    START_AUCTION,      // Mulai lelang properti ini
};


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
    EffectType onLanded(Player& player) override;

    virtual int calcRent(int diceRoll = 0) const = 0;
    virtual int calcValue() const = 0;
    void buy(); 
    void payRent(); 
    virtual void mortgage(); 
    virtual void unmortgage();  
    int getmortgageValue() const{ return mortgageValue;}
    int getPrice() const { return price; }
    int getOwnerId() const { return ownerId; }
    int getStatus() const { return status; }
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
    int calcValue() const override;
    EffectType onLanded(Player& player) override;
 
    // jual properti ke bank
    void sellTobank(Player& owner) override;
 
    // fungsi dan prosedur untuk mengatur terkait bangun membangun.

    void buildHouse();
    void buildHotel();
    void demolish();
    bool hasBuildings();
    int getHouseCost() const;
    int getHotelCost() const;
    int getRentLevel() const;
 
    // Color group
    string getColorGroup() const;
 
    /** Hitung nilai jual bangunan saat ini (50% dari total harga beli bangunan) */
    int calcBuildingResaleValue() const;
    void setMonopolized(bool val);

    // efek festival
    void setFestivalEffect(int multiplier);
    void clearFestivalEffect();
 
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
    int calcValue() const override;
    EffectType onLanded(Player& player) override;
    int getRailroadOwnedCount() const;
    void setrailroadOwnedCount(int count);
 
private:
    map<int, int> rentByCount;  // jumlah railroad -> sewa
    int railroadOwnedCount = 1;
    
};


class UtilityTile : public PropertyTile {
    
public:
    UtilityTile(int index, const std::string& code, const std::string& name,
                int mortgageValue,
                const std::map<int, int>& multiplierByCount);

    // Perhitungan biaya sewa Utility ditentukan oleh total angka dadu yang didapatkan pemain ketika mendarat, 
    //dikalikan dengan faktor pengali yang bergantung pada jumlah petak Utility yang dimiliki oleh pemiliknya            
    int calcRent(int diceRoll = 0) const override;
    int calcValue() const override;
    EffectType onLanded(Player& player) override;
    void setUtilityOwnedCount(int count);
    void setLastDiceRoll(int dice);
    int getUtilityOwnedCount() const;
    int getLastDiceRoll()const;

 
private:
    std::map<int, int> multiplierByCount;  // jumlah utility -> faktor pengali
    int utilityOwnedCount = 1;
    int lastDiceRoll = 1;
};


#endif
