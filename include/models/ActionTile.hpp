#ifndef ACTION_TILE_HPP
#define ACTION_TILE_HPP

#include "Tile.hpp"

 
class ActionTile : public Tile {
public:
    ActionTile(int index, const std::string& code, const std::string& name);
 
    // onLanded mendelegasikan ke triggerEffect — subclass tidak perlu override keduanya
    void onLanded(Player& player);
 
    // Pure virtual — setiap ActionTile punya efek berbeda
    virtual void triggerEffect(Player& player) = 0;
};


class CardTile : public ActionTile {
public:
    CardTile(int index, const std::string& code, const std::string& name,
             string type);
 
    void triggerEffect(Player& player) override;
 
    string getCardType();
 
private:
    string cardType; // 1 : Kartu Kesempatan dan 2 : Dana Umum
 
    void applyChanceCard(Player& player);
    void applyCommunityCard(Player& player);

    // Asumsi : CardDeck hanya 1 Objek saja, daripada mempunyai hubungan Is-A dengan CardTile
};


class TaxTile : public ActionTile {
public:
    TaxTile(int index, const std::string& code, const std::string& name,
            string type,
            int flatAmount,
            double percentageRate = 0.0);
 
    void triggerEffect(Player& player) override;
 
    string getTaxType() const ;
    int getFlatAmount() const;
    double getPercentageRate();

    // Tangani alur PPH
    void handlePPH(Player& player);
 
    // Tangani Alur PBM
    void handlePBM(Player& player);

    // hitung total kekayaan
    int computeNetWorth(const Player& player) const;
 
private:
    string taxType;
    int     flatAmount;
    double  percentageRate;
};


class FestivalTile : public ActionTile {
public:
    FestivalTile(int index, const std::string& code, const std::string& name);
 
    void triggerEffect(Player& player) override;
    void applyFestivalToProperty(Player& player);
};

 

class SpecialTile : public ActionTile {
public:
    SpecialTile(int index, const std::string& code, const std::string& name);
 
    // onLanded mendelegasikan ke handleArrival — konsisten dengan ActionTile
    void onLanded(Player& player) ;
 
    // Pure virtual — setiap petak spesial punya logika kedatangan unik
    virtual void handleArrival(Player& player) = 0; // fungsi khusus SpecialTile Class mirip dengan onLanded

    void triggerEffect(Player& player) override {
        handleArrival(player);
    }
 
};

class GoTile : public SpecialTile{
    public:
        GoTile(int index, const std::string& code, const std::string& name,
            int salary);
    
        void handleArrival(Player& player) override;

        // Dipanggil khusus ketika melewati petak, bukan berhenti /
        void awardPassSalary(Player& player) const;
        int getSalary() const;
 
    private:
        int salary;
        void awardSalary(Player& player) const;

};

class JailTile : public SpecialTile {
public:
    JailTile(int index, const std::string& code, const std::string& name,
             int fine);
 
    void handleArrival(Player& player);

    // Kirim pemain ke penjara (dipanggil GoToJailTile dan kartu "Masuk Penjara
    void imprisonPlayer(Player& player);
    /// Keluarkan pemain dari penjara (setelah bayar/double/kartu bebas)
    void releasePlayer(Player& player);
    int getFine();
    void processTurnInJail(Player& player);
    void handlePayFine(Player& player);
    void handleRollForDouble(Player& player);
private:
    int fine;

};

class FreeParkingTile : public SpecialTile {
public:
    FreeParkingTile(int index, const std::string& code, const std::string& name);
    void handleArrival(Player& player) override;
};
 
class GoToJailTile : public SpecialTile {
public:
    GoToJailTile(int index, const std::string& code, const std::string& name);
    void handleArrival(Player& player) override;
 
    // Dipanggil Board setelah semua tile diinisialisasi
    void setJailTile(JailTile* jail);
 
private:
    JailTile* jailRef = nullptr; // supaya bisa memasukkan pemain ke penjara
};

#endif