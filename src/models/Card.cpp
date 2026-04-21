/*
 * Card.cpp
 * Implementasi semua class kartu sesuai spesifikasi Nimonspoli.
 *
 * SKILL CARD (kartu tangan, maks 3 di tangan, 1x pakai per giliran, hanya sebelum dadu):
 *  - MoveCard        : Maju sekian petak. Jumlah langkah acak saat kartu DIDAPAT.
 *  - DiscountCard    : Diskon persentase acak saat kartu DIDAPAT, berlaku 1 giliran.
 *  - ShieldCard      : Kebal semua tagihan/sanksi selama 1 giliran.
 *  - TeleportCard    : Pindah ke petak manapun.
 *  - LassoCard       : Tarik pemain lawan yang berada di DEPAN ke posisi pemain saat ini.
 *  - DemolitionCard  : Hancurkan 1 bangunan properti milik lawan.
 *
 * ACTION CARD (diambil saat mendarat di petak kartu, langsung dieksekusi, lalu reshuffle):
 *  - ChanceCard      : NEAREST_STATION / MOVE_BACK_3 / GO_TO_JAIL
 *  - CommunityChestCard : BIRTHDAY (dapat M100 dari tiap pemain) /
 *                         DOCTOR   (bayar M700 ke Bank) /
 *                         ELECTION (bayar M200 ke tiap pemain)
 */

#include "Card.hpp"
#include "Player.hpp"
#include "GameEngine.hpp"
#include "Exceptions.hpp"
#include <iostream>
#include <random>

//random integer
static int randomInt(int lo, int hi) {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}


SkillCard::SkillCard(const std::string& name,
                     const std::string& description,
                     SkillCardType type)
    : cardName(name), cardDescription(description), skillType(type) {}

SkillCard::~SkillCard() {}

void SkillCard::use(Player& player, GameEngine& engine) {

}

std::string SkillCard::getCardName()const {
    return cardName; 
}
std::string SkillCard::getCardDescription()const {
    return cardDescription;
}
SkillCardType SkillCard::getSkillType()const {
    return skillType; 
}



// MoveCard , majunya dirandom

MoveCard::MoveCard()
    : SkillCard("MoveCard",
                "Maju sejumlah petak. Langkah ditentukan acak saat kartu didapat.",
                SkillCardType::MOVE),
      steps(randomInt(1, 6)) {}

//load dari saved
MoveCard::MoveCard(int savedSteps)
    : SkillCard("MoveCard",
                "Maju sejumlah petak. Langkah ditentukan acak saat kartu didapat.",
                SkillCardType::MOVE),
      steps(savedSteps) {}

int MoveCard::getSteps() const {
    return steps; 
}


//BELOM ADA GETTERNYEEE PLAYER
void MoveCard::use(Player& player, GameEngine& engine) {
    // std::cout << "[MoveCard] " << player.getUsername()
    //           << " menggunakan MoveCard — maju " << steps << " petak.\n";
    // GameEngine menangani pergerakan, pengecekan melewati GO, dan efek mendarat
    // engine.movePlayerBy(player, steps); perlu liat  implementasi engine lagi
}

std::string MoveCard::getValueString() const {
    return std::to_string(steps);
}



static int randomDiscountPercent() {
    return randomInt(1, 5) * 10; 
}

DiscountCard::DiscountCard()
    : SkillCard("DiscountCard",
                "Diskon persentase acak pada sewa/pajak selama 1 giliran.",
                SkillCardType::DISCOUNT),
      discountPercent(randomDiscountPercent()),
      remainingTurns(1) {}

//load dari save file
DiscountCard::DiscountCard(int savedPercent, int savedTurns)
    : SkillCard("DiscountCard",
                "Diskon persentase acak pada sewa/pajak selama 1 giliran.",
                SkillCardType::DISCOUNT),
      discountPercent(savedPercent),
      remainingTurns(savedTurns) {}

int  DiscountCard::getDiscountPercent() const {
    return discountPercent;
}
int  DiscountCard::getRemainingTurns() const {
    return remainingTurns; 
}
void DiscountCard::decrementTurns(){ 
    if (remainingTurns > 0) --remainingTurns; }

// Daftarkan kartu aktif ke player, GameEngine membaca saat menghitung sewa/pajak
void DiscountCard::use(Player& player, GameEngine& engine) {
    
    // player.setActiveDiscountCard(this);
    std::cout << "[DiscountCard] " << player.getUsername()
              << " menggunakan DiscountCard — diskon " << discountPercent
              << "% berlaku selama giliran ini.\n";
}

std::string DiscountCard::getValueString() const {
    return std::to_string(discountPercent);
}


// =============================================================================
// ShieldCard  — kebal semua tagihan/sanksi selama 1 giliran
// =============================================================================

ShieldCard::ShieldCard()
    : SkillCard("ShieldCard",
                "Kebal tagihan atau sanksi apapun selama 1 giliran.",
                SkillCardType::SHIELD) {}

void ShieldCard::use(Player& player, GameEngine& engine) {
    player.setShieldActive(true);
    std::cout << "[ShieldCard] " << player.getUsername()
              << " menggunakan ShieldCard — kebal terhadap tagihan/sanksi giliran ini.\n";
}

std::string ShieldCard::getValueString() const {
    return ""; // ShieldCard tidak memiliki nilai numerik
}


// =============================================================================
// TeleportCard  — pindah ke petak manapun di papan
// =============================================================================

TeleportCard::TeleportCard()
    : SkillCard("TeleportCard",
                "Pindah ke petak manapun di papan permainan.",
                SkillCardType::TELEPORT) {}

void TeleportCard::use(Player& player, GameEngine& engine) {
    std::cout << "[TeleportCard] " << player.getUsername()
              << " menggunakan TeleportCard.\n";
    // GameEngine meminta pemain memilih tujuan lalu memindahkan bidak + proses mendarat
    // engine.handleTeleport(player); nunggguin engine
}

std::string TeleportCard::getValueString() const {
    return ""; // TeleportCard tidak memiliki nilai numerik
}



// LassoCard, tarik pemain lawan yang adad di depan ke posisi kita saat ini


LassoCard::LassoCard()
    : SkillCard("LassoCard",
                "Tarik satu pemain lawan di depan posisimu ke petakmu saat ini.",
                SkillCardType::LASSO) {}

void LassoCard::use(Player& player, GameEngine& engine) {
    std::cout << "[LassoCard] " << player.getUsername()
              << " menggunakan LassoCard.\n";
    // GameEngine mencari target (lawan paling dekat di depan) dan memindahkannya
    // engine.handleLasso(player); NUNGGU ENDRA 
}

std::string LassoCard::getValueString() const {
    return ""; // LassoCard tidak memiliki nilai numerik
}


// DemolitionCard, ancur2in properti lawan

DemolitionCard::DemolitionCard()
    : SkillCard("DemolitionCard",
                "Hancurkan 1 bangunan (rumah/hotel) pada properti milik lawan.",
                SkillCardType::DEMOLITION) {}

void DemolitionCard::use(Player& player, GameEngine& engine) {
    std::cout << "[DemolitionCard] " << player.getUsername()
              << " menggunakan DemolitionCard.\n";
    // GameEngine meminta pemain memilih target properti lawan, lalu menghancurkan bangunan
    // engine.handleDemolition(player); nunggu endra
}

std::string DemolitionCard::getValueString() const {
    return ""; 
}



ActionCard::ActionCard(const std::string& name, const std::string& description)
    : cardName(name), cardDescription(description) {}

ActionCard::~ActionCard() {}

void ActionCard::execute(Player& player, GameEngine& engine) {}

std::string ActionCard::getCardName() const {
    return cardName; 
}
std::string ActionCard::getCardDescription() const {
    return cardDescription;
}


// ChanceCard , abis dipake taro ke tumpukan

ChanceCard::ChanceCard(ChanceCardType cardType)
    : ActionCard(
          cardType == ChanceCardType::NEAREST_STATION ? "Pergi ke Stasiun Terdekat" :
          cardType == ChanceCardType::MOVE_BACK_3      ? "Mundur 3 Petak"            :
                                                         "Masuk Penjara",
          cardType == ChanceCardType::NEAREST_STATION ? "Pergi ke stasiun terdekat." :
          cardType == ChanceCardType::MOVE_BACK_3      ? "Mundur 3 petak."            :
                                                         "Masuk Penjara."
      ),
      type(cardType) {}

ChanceCardType ChanceCard::getType() const { return type; }

void ChanceCard::execute(Player& player, GameEngine& engine) {
    std::cout << "Kartu: \"" << cardDescription << "\"\n";

    switch (type) {
        case ChanceCardType::NEAREST_STATION:
            // Pindah ke stasiun railroad terdekat, searah jarum jam dari posisi saat ini
            // engine.moveToNearestStation(player); lagi2 nunggu endra
            break;

        case ChanceCardType::MOVE_BACK_3:
            // Mundur 3 petak, nilai negatif berarti mundur
            // engine.movePlayerBy(player, -3); lagi2 nunggu endra
            break;

        case ChanceCardType::GO_TO_JAIL:
            player.goToJail();
            break;
    }
}


static const int BIRTHDAY_AMOUNT  = 100;
static const int DOCTOR_AMOUNT    = 700;
static const int ELECTION_AMOUNT  = 200;

CommunityChestCard::CommunityChestCard(CommunityChestCardType cardType)
    : ActionCard(
          cardType == CommunityChestCardType::BIRTHDAY  ? "Hadiah Ulang Tahun" :
          cardType == CommunityChestCardType::DOCTOR     ? "Biaya Dokter"        :
                                                           "Sumbangan Pemilu",
          cardType == CommunityChestCardType::BIRTHDAY  ?
              "Ini adalah hari ulang tahun Anda. Dapatkan M100 dari setiap pemain." :
          cardType == CommunityChestCardType::DOCTOR     ?
              "Biaya dokter. Bayar M700."                                            :
              "Anda mau nyaleg. Bayar M200 kepada setiap pemain."
      ),
      type(cardType) {}

CommunityChestCardType CommunityChestCard::getType() const { return type; }

void CommunityChestCard::execute(Player& player, GameEngine& engine) {
    std::cout << "Kartu: \"" << cardDescription << "\"\n";

    switch (type) {
        // semua pemain bayar BIRTHDAY_AMOUNT ke player ini
        case CommunityChestCardType::BIRTHDAY:
        
            //engine.handleBirthday(player, BIRTHDAY_AMOUNT); lagi2 nunggu endra
            break;

        // pemain ini harus bayar DOCTOR_AMOUNT
        case CommunityChestCardType::DOCTOR:
            try {
                player -= DOCTOR_AMOUNT; // operator-= dari Player.hpp
                std::cout << player.getUsername() << " membayar M" << DOCTOR_AMOUNT
                          << " ke Bank. Sisa Uang = M" << player.getMoney() << ".\n";
            } catch (const NotEnoughMoneyException&) {
                std::cout << "Kamu tidak mampu membayar biaya dokter! (M"
                          << DOCTOR_AMOUNT << ")\n"
                          << "Uang kamu saat ini: M" << player.getMoney() << "\n";
                
                // engine.handleBankruptcy(player, nullptr, DOCTOR_AMOUNT); nunggu endra
            }
            break;

        case CommunityChestCardType::ELECTION:
            // engine.handleElection(player, ELECTION_AMOUNT); nunggu endra
            break;
    }
}