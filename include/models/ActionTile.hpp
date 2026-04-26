#ifndef ACTION_TILE_HPP
#define ACTION_TILE_HPP

#include "../../include/models/PropertyTile.hpp"

class ActionTile : public Tile {
  public:
    ActionTile(int index, const std::string &code, const std::string &name);
    virtual ~ActionTile() = default;

    // onLanded mendelegasikan ke triggerEffect — subclass tidak perlu override
    // keduanya
    EffectType onLanded(Player &player) override;

    // Pure virtual — setiap ActionTile punya efek berbeda
    virtual EffectType triggerEffect(Player &player) = 0;

    int calcRent(int diceRoll = 0) const override;
    int calcValue() const override { return 0; }
    int getOwnerId() const override;
};

class CardTile : public ActionTile {
  public:
    CardTile(int index, const std::string &code, const std::string &name,
             const std::string &type);

    EffectType triggerEffect(Player &player) override;

    std::string getCardType() const;

    // --- Type-query override ---
    bool isCardTile() const override { return true; }
    std::string getTileCategory() const override { return "CARD"; }

  private:
    std::string cardType; // 1 : Kartu Kesempatan dan 2 : Dana Umum

    void applyChanceCard(Player &player);
    void applyCommunityCard(Player &player);

    // Asumsi : CardDeck hanya 1 Objek saja, daripada mempunyai hubungan Is-A
    // dengan CardTile
};

class TaxTile : public ActionTile {
  public:
    TaxTile(int index, const std::string &code, const std::string &name,
            const std::string &type, int flatAmount,
            double percentageRate = 0.0);

    EffectType triggerEffect(Player &player) override;

    std::string getTaxType() const;
    int getFlatAmount() const;
    double getPercentageRate() const;

    // Tangani alur PPH
    void handlePPH(Player &player);

    // Tangani Alur PBM
    void handlePBM(Player &player);

    // hitung total kekayaan
    int computeNetWorth(const Player &player) const;

    // --- Type-query override ---
    bool isTaxTile() const override { return true; }
    std::string getTileCategory() const override { return "TAX"; }

  private:
    std::string taxType;
    int flatAmount;
    double percentageRate;
};

class FestivalTile : public ActionTile {
  public:
    FestivalTile(int index, const std::string &code, const std::string &name);

    EffectType triggerEffect(Player &player) override;
    void applyFestivalToProperty(Player &player);

    // --- Type-query override ---
    bool isFestivalTile() const override { return true; }
    std::string getTileCategory() const override { return "FESTIVAL"; }
};

class SpecialTile : public ActionTile {
  public:
    SpecialTile(int index, const std::string &code, const std::string &name);

    // Pure virtual — setiap petak spesial punya logika kedatangan unik
    virtual EffectType
    handleArrival(Player &player) = 0; // fungsi khusus SpecialTile Class mirip
                                       // dengan onLanded

    EffectType triggerEffect(Player &player) override {
        return handleArrival(player);
    }
};

class GoTile : public SpecialTile {
  public:
    GoTile(int index, const std::string &code, const std::string &name,
           int salary);

    EffectType handleArrival(Player &player) override;
    void awardSalary(Player &player) const;
    int getSalary() const;

    bool isGoTile() const override { return true; }

  private:
    int salary;
};

class JailTile : public SpecialTile {
  public:
    JailTile(int index, const std::string &code, const std::string &name,
             int fine);

    EffectType handleArrival(Player &player) override;

    // Kirim pemain ke penjara (dipanggil GoToJailTile dan kartu "Masuk Penjara
    void imprisonPlayer(Player &player);
    /// Keluarkan pemain dari penjara (setelah bayar/double/kartu bebas)
    void releasePlayer(Player &player);
    int getFine() const;

    // Proses turn di penjara dan return EffectType outcome
    EffectType processTurnInJail(Player &player);
    EffectType handlePayFine(Player &player);
    EffectType handleRollForDouble(Player &player);

    bool isJailTile() const override { return true; }

  private:
    int fine;
};

class FreeParkingTile : public SpecialTile {
  public:
    FreeParkingTile(int index, const std::string &code,
                    const std::string &name);
    EffectType handleArrival(Player &player) override;
};

class GoToJailTile : public SpecialTile {
  public:
    GoToJailTile(int index, const std::string &code, const std::string &name);
    EffectType handleArrival(Player &player) override;

    // Dipanggil Board setelah semua tile diinisialisasi
    void setJailTile(JailTile *jail);

    bool isGoToJailTile() const override { return true; }

  private:
    JailTile *jailRef = nullptr; // supaya bisa memasukkan pemain ke penjara
};

#endif
