#include "../../include/models/ActionTile.hpp"
#include <iostream>
using namespace std;

ActionTile::ActionTile(int index, const std::string &code,
                       const std::string &name)
    : Tile(index, code, name) {}

EffectType ActionTile::onLanded(Player &player) {
    return triggerEffect(player);
}

int ActionTile::calcRent(int diceRoll) const {
    (void)diceRoll; // ActionTile tidak memiliki harga sewa
    return 0;
}

int ActionTile::getOwnerId() const {
    // ActionTile tidak dimiliki siapapun
    return -1;
}

// CardTile

CardTile::CardTile(int index, const std::string &code, const std::string &name,
                   const std::string &type)
    : ActionTile(index, code, name), cardType(type) {}

EffectType CardTile::triggerEffect(Player &player) {
    (void)player;
    if (cardType == "1") {
        return EffectType::DRAW_CHANCE;
    } else if (cardType == "2") {
        return EffectType::DRAW_COMMUNITY;
    }
    return EffectType::NONE;
}

std::string CardTile::getCardType() const { return this->cardType; }

void CardTile::applyChanceCard(Player &player) {
    (void)player; // TODO : Implementasi apabila player mengambil kartu kesempatan
}

void CardTile::applyCommunityCard(Player &player) {
    (void)player; // TODO : Implementasi apabila player mengambil kartu komunitas
}

// TaxTile

TaxTile::TaxTile(int index, const std::string &code, const std::string &name,
                 const std::string &type, int flatAmount, double percentageRate)
    : ActionTile(index, code, name), taxType(type), flatAmount(flatAmount),
      percentageRate(percentageRate) {}

EffectType TaxTile::triggerEffect(Player &player) {
    (void)player;
    if (taxType == "PPH") {
        return EffectType::PAY_TAX_CHOICE;
    } else if (taxType == "PBM") {
        return EffectType::PAY_TAX_FLAT;
    }
    return EffectType::NONE;
}

void TaxTile::handlePPH(Player &player) {
    (void)player;
    int pilihan = 0;
    if (pilihan == 1) {
        // membayar sejumlah flat
    } else if (pilihan == 2) {
        // membayar persentase total dari kekayaan
    }
}

void TaxTile::handlePBM(Player &player) {
    if (player.getMoney() >= flatAmount) {
        // player -= flatAmount;
    } else {
        // player.declareBankruptcy();
    }
}

int TaxTile::computeNetWorth(const Player &player) const {
    int total = player.getMoney();
    for (auto *v : player.getOwnedProperties()) {
        total += v->getPrice();
    }
    return total;
}

std::string TaxTile::getTaxType() const { return this->taxType; }

int TaxTile::getFlatAmount() const { return this->flatAmount; }

double TaxTile::getPercentageRate() const { return this->percentageRate; }

// FestivalTile

FestivalTile::FestivalTile(int index, const std::string &code,
                           const std::string &name)
    : ActionTile(index, code, name) {}

EffectType FestivalTile::triggerEffect(Player &player) {
    (void)player;
    return EffectType::FESTIVAL_TRIGGER;
}

void FestivalTile::applyFestivalToProperty(Player &player) {
    if (player.getOwnedProperties().empty()) {
        return;
    }
    std::string pilihan;
    PropertyTile *choose = nullptr;
    // masukin pilihan
    for (auto *v : player.getOwnedProperties()) {
        if (v->getKode() == pilihan) {
            choose = v;
            break;
        }
    }

    if (!choose)
        return;
    // TODO : Efek festival pada StreetTile
}

// SpecialTile

SpecialTile::SpecialTile(int index, const std::string &code,
                         const std::string &name)
    : ActionTile(index, code, name) {}

// GoTile

GoTile::GoTile(int index, const std::string &code, const std::string &name,
               int salary)
    : SpecialTile(index, code, name), salary(salary) {}

EffectType GoTile::handleArrival(Player &player) {
    (void)player;
    // Pengurangan/Penambahan saldo akan dieksekusi di GameEngine
    // setelah menerima EffectType::AWARD_SALARY
    return EffectType::AWARD_SALARY;
}

void GoTile::awardSalary(Player &player) const {
    (void)player; // GameEngine nambahin salary
}

int GoTile::getSalary() const { return salary; }

// JailTile

JailTile::JailTile(int index, const std::string &code, const std::string &name,
                   int fine)
    : SpecialTile(index, code, name), fine(fine) {}

int JailTile::getFine() const { return fine; }

EffectType JailTile::handleArrival(Player &player) {
    (void)player;
    // Jika fungsi ini dipanggil, artinya pemain mendarat ke sini via lemparan
    // dadu biasa (berjalan). Logika ketika pemain sedang "dipenjara" (JAILED)
    // dan masuk turn-nya TIDAK dipanggil dari onLanded(), melainkan dicek oleh
    // GameEngine di awal turn sebelum dadu dilempar.
    return EffectType::JUST_VISITING;
}

void JailTile::imprisonPlayer(Player &player) {
    player.goToJail();
    // Pindahkan posisi pemain ke petak penjara (index 11 sesuai papan)
    int steps = abs(11 - player.getPosition());
    player.move(steps);

    // Harusnya gameEngine juga
}

void JailTile::releasePlayer(Player &player) {
    (void)player; // TODO : ubah status dari Jailed ke Active (oleh GameEngine)
}

EffectType JailTile::processTurnInJail(Player &player) {
    int turnsInJail = player.getJailTurnsLeft();

    // Giliran ke-3 wajib bayar denda, tidak ada pilihan lain
    if (turnsInJail >= 3) {
        return handlePayFine(player);
    }

    // TODO: opsi 3 - gunakan kartu Bebas dari Penjara (jika punya)
    // cout << "Pilihan (1/2): ";
    int pilihan = 0;
    cin >> pilihan;

    if (pilihan == 1) {
        return handlePayFine(player);
    } else if (pilihan == 2) {
        return handleRollForDouble(player);
    } else {
        player.decrementJailTurn();
        return EffectType::JAIL_TURN; // Tetap di jail, tunggu turn berikutnya
    }
}

EffectType JailTile::handlePayFine(Player &player) {
    if (player.getMoney() >= fine) {
        // player -= fine;  TODO: GameEngine
        releasePlayer(player);
        // TODO: GameLogic melanjutkan lemparan dadu normal untuk giliran ini
        return EffectType::JAIL_PAID_FINE; // Pemain berhasil bayar dan bebas
    } else {
        // TODO: GameLogic::handleBankruptcy(player, -1)
        return EffectType::JAIL_FORCED_BANKRUPTCY; // Pemain tidak punya uang
    }
}

EffectType JailTile::handleRollForDouble(Player &player) {
    // TODO: GameLogic melempar dadu dan mengecek apakah double
    // Jika double: releasePlayer() lalu gerakkan pemain
    // Jika tidak double: player tidak bergerak, increment jailTurns
    // Simulasi sementara, entar digantikan GameLogic
    int d1 = rand() % 6 + 1;
    int d2 = rand() % 6 + 1;

    if (d1 == d2) {
        releasePlayer(player);
        // TODO: GameLogic
        return EffectType::JAIL_ROLLED_DOUBLE; // Pemain roll double dan bebas
    } else {
        player.decrementJailTurn();
        return EffectType::JAIL_ROLLED_NO_DOUBLE; // Tetap di jail
    }
}

// FreeParkingTile

FreeParkingTile::FreeParkingTile(int index, const std::string &code,
                                 const std::string &name)
    : SpecialTile(index, code, name) {}

EffectType FreeParkingTile::handleArrival(Player &player) {
    (void)player;
    return EffectType::NONE;
}

// GoToJailTile

GoToJailTile::GoToJailTile(int index, const std::string &code,
                           const std::string &name)
    : SpecialTile(index, code, name), jailRef(nullptr) {}

void GoToJailTile::setJailTile(JailTile *jail) { jailRef = jail; }

EffectType GoToJailTile::handleArrival(Player &player) {
    (void)player;
    return EffectType::SEND_TO_JAIL;
}
