#include "../../include/core/Exceptions.hpp"
#include "../../include/core/GameEngine.hpp"
#include "../../include/core/Player.hpp"
#include "../../include/models/Card.hpp"
#include <iostream>
#include <random>

static int randomInt(int lo, int hi) {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

SkillCard::SkillCard(const std::string &name, const std::string &description,
                     SkillCardType type)
    : cardName(name), cardDescription(description), skillType(type) {}

SkillCard::~SkillCard() {}

SkillCardEffect SkillCard::use(Player &player, GameEngine &engine) {
    return SkillCardEffect::NONE;
}

std::string SkillCard::getCardName() const { return cardName; }
std::string SkillCard::getCardDescription() const { return cardDescription; }
SkillCardType SkillCard::getSkillType() const { return skillType; }

MoveCard::MoveCard()
    : SkillCard(
          "MoveCard",
          "Maju sejumlah petak. Langkah ditentukan acak saat kartu didapat.",
          SkillCardType::MOVE),
      steps(randomInt(1, 6)) {}

MoveCard::MoveCard(int savedSteps)
    : SkillCard(
          "MoveCard",
          "Maju sejumlah petak. Langkah ditentukan acak saat kartu didapat.",
          SkillCardType::MOVE),
      steps(savedSteps) {}

int MoveCard::getSteps() const { return steps; }

SkillCardEffect MoveCard::use(Player &player, GameEngine &engine) {
    return SkillCardEffect::MOVE;
}

std::string MoveCard::getValueString() const { return std::to_string(steps); }

static int randomDiscountPercent() { return randomInt(1, 5) * 10; }

DiscountCard::DiscountCard()
    : SkillCard("DiscountCard",
                "Diskon persentase acak pada sewa/pajak selama 1 giliran.",
                SkillCardType::DISCOUNT),
      discountPercent(randomDiscountPercent()), remainingTurns(1) {}

DiscountCard::DiscountCard(int savedPercent, int savedTurns)
    : SkillCard("DiscountCard",
                "Diskon persentase acak pada sewa/pajak selama 1 giliran.",
                SkillCardType::DISCOUNT),
      discountPercent(savedPercent), remainingTurns(savedTurns) {}

int DiscountCard::getDiscountPercent() const { return discountPercent; }
int DiscountCard::getRemainingTurns() const { return remainingTurns; }
void DiscountCard::decrementTurns() {
    if (remainingTurns > 0)
        --remainingTurns;
}

SkillCardEffect DiscountCard::use(Player &player, GameEngine &engine) {
    return SkillCardEffect::DISCOUNT;
}

std::string DiscountCard::getValueString() const {
    return std::to_string(discountPercent);
}

ShieldCard::ShieldCard()
    : SkillCard("ShieldCard",
                "Kebal tagihan atau sanksi apapun selama 1 giliran.",
                SkillCardType::SHIELD) {}

SkillCardEffect ShieldCard::use(Player &player, GameEngine &engine) {
    return SkillCardEffect::SHIELD;
}

std::string ShieldCard::getValueString() const { return ""; }

TeleportCard::TeleportCard()
    : SkillCard("TeleportCard", "Pindah ke petak manapun di papan permainan.",
                SkillCardType::TELEPORT) {}

SkillCardEffect TeleportCard::use(Player &player, GameEngine &engine) {
    return SkillCardEffect::TELEPORT;
}

std::string TeleportCard::getValueString() const { return ""; }

LassoCard::LassoCard()
    : SkillCard(
          "LassoCard",
          "Tarik satu pemain lawan di depan posisimu ke petakmu saat ini.",
          SkillCardType::LASSO) {}

SkillCardEffect LassoCard::use(Player &player, GameEngine &engine) {
    return SkillCardEffect::LASSO;
}

std::string LassoCard::getValueString() const { return ""; }

DemolitionCard::DemolitionCard()
    : SkillCard("DemolitionCard",
                "Hancurkan 1 bangunan (rumah/hotel) pada properti milik lawan.",
                SkillCardType::DEMOLITION) {}

SkillCardEffect DemolitionCard::use(Player &player, GameEngine &engine) {
    return SkillCardEffect::DEMOLITION;
}

std::string DemolitionCard::getValueString() const { return ""; }

ActionCard::ActionCard(const std::string &name, const std::string &description)
    : cardName(name), cardDescription(description) {}

ActionCard::~ActionCard() {}

ActionCardEffect ActionCard::execute(Player &player, GameEngine &engine) {
    return ActionCardEffect::NONE;
}

std::string ActionCard::getCardName() const { return cardName; }
std::string ActionCard::getCardDescription() const { return cardDescription; }

ChanceCard::ChanceCard(ChanceCardType cardType)
    : ActionCard(cardType == ChanceCardType::NEAREST_STATION
                     ? "Pergi ke Stasiun Terdekat"
                 : cardType == ChanceCardType::MOVE_BACK_3 ? "Mundur 3 Petak"
                                                           : "Masuk Penjara",
                 cardType == ChanceCardType::NEAREST_STATION
                     ? "Pergi ke stasiun terdekat."
                 : cardType == ChanceCardType::MOVE_BACK_3 ? "Mundur 3 petak."
                                                           : "Masuk Penjara."),
      type(cardType) {}

ChanceCardType ChanceCard::getType() const { return type; }

ActionCardEffect ChanceCard::execute(Player &player, GameEngine &engine) {
    switch (type) {
    case ChanceCardType::NEAREST_STATION:
        return ActionCardEffect::CHANCE_NEAREST_STATION;
    case ChanceCardType::MOVE_BACK_3:
        return ActionCardEffect::CHANCE_MOVE_BACK_3;
    case ChanceCardType::GO_TO_JAIL:
        return ActionCardEffect::CHANCE_GO_TO_JAIL;
    }
    return ActionCardEffect::NONE;
}

CommunityChestCard::CommunityChestCard(CommunityChestCardType cardType)
    : ActionCard(
          cardType == CommunityChestCardType::BIRTHDAY ? "Hadiah Ulang Tahun"
          : cardType == CommunityChestCardType::DOCTOR ? "Biaya Dokter"
                                                       : "Sumbangan Pemilu",
          cardType == CommunityChestCardType::BIRTHDAY
              ? "Ini adalah hari ulang tahun Anda. Dapatkan M100 dari setiap "
                "pemain."
          : cardType == CommunityChestCardType::DOCTOR
              ? "Biaya dokter. Bayar M700."
              : "Anda mau nyaleg. Bayar M200 kepada setiap pemain."),
      type(cardType) {}

CommunityChestCardType CommunityChestCard::getType() const { return type; }

ActionCardEffect CommunityChestCard::execute(Player &player,
                                             GameEngine &engine) {
    switch (type) {
    case CommunityChestCardType::BIRTHDAY:
        return ActionCardEffect::COMMUNITY_BIRTHDAY;
    case CommunityChestCardType::DOCTOR:
        return ActionCardEffect::COMMUNITY_DOCTOR;
    case CommunityChestCardType::ELECTION:
        return ActionCardEffect::COMMUNITY_ELECTION;
    }
    return ActionCardEffect::NONE;
}
