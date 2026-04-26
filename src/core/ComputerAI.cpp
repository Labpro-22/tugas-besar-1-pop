#include "../../include/core/ComputerAI.hpp"
#include "../../include/core/Enums.hpp"
#include "../../include/core/Player.hpp"
#include "../../include/models/ActionTile.hpp"
#include "../../include/models/Board.hpp"
#include "../../include/models/Card.hpp"
#include "../../include/models/PropertyTile.hpp"
#include "../../include/models/Tile.hpp"

#include <algorithm>
#include <climits>
#include <map>

// ---- Singleton RNG ----

std::mt19937 &ComputerAI::getRng() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

// ---- decideBuyProperty ----

int ComputerAI::decideBuyProperty(const Player *player, const PropertyTile *prop,
                                   Difficulty diff) {
    if (!player || !prop) return 1;

    if (diff == Difficulty::HARD) {
        // Selalu beli jika mampu
        return (player->getMoney() >= prop->getPrice()) ? 0 : 1;
    } else {
        // EASY: 60% kemungkinan membeli
        std::uniform_int_distribution<> d(0, 9);
        return (d(getRng()) < 6) ? 0 : 1;
    }
}

// ---- decideTaxChoice ----

int ComputerAI::decideTaxChoice(const Player *player, const TaxTile *tax,
                                 Difficulty diff) {
    if (!tax || !player) return 0;

    if (diff == Difficulty::HARD) {
        // Hitung mana yang lebih murah
        int flatAmt = tax->getFlatAmount();
        int wealth  = tax->computeNetWorth(*player);
        int pctAmt  = static_cast<int>(wealth * tax->getPercentageRate() / 100.0);
        return (flatAmt <= pctAmt) ? 0 : 1;
    } else {
        // EASY: selalu pilih flat (lebih sederhana)
        return 0;
    }
}

// ---- decideFestival ----

int ComputerAI::decideFestival(const Player *player, Difficulty diff) {
    if (!player) return 0;

    const auto &props = player->getOwnedProperties();
    int streetIdx = 0;
    int bestIdx   = 0;
    int bestRent  = -1;

    for (const auto *prop : props) {
        if (!prop->isStreet()) continue;

        if (diff == Difficulty::HARD) {
            // Pilih properti dengan sewa tertinggi (dadu rata-rata = 7)
            int rent = prop->calcRent(7);
            if (rent > bestRent) {
                bestRent = rent;
                bestIdx  = streetIdx;
            }
        } else {
            // EASY: pilih yang pertama
            bestIdx = 0;
            break;
        }
        streetIdx++;
    }
    return bestIdx;
}

// ---- decideJailChoice ----

int ComputerAI::decideJailChoice(const Player *player, int fine, Difficulty diff) {
    if (!player) return 1;

    if (diff == Difficulty::HARD) {
        // Bayar denda jika punya cukup uang (ingin terus bergerak)
        return (player->getMoney() >= fine * 4) ? 0 : 1;
    } else {
        // EASY: selalu coba lempar dadu
        return 1;
    }
}

// ---- decideAuction ----

int ComputerAI::decideAuction(const Player *bidder, const PropertyTile *prop,
                               int /*currentBid*/, int bid1, int bid2,
                               Difficulty diff) {
    if (!bidder || !prop) return 2;

    if (diff == Difficulty::HARD) {
        // Tawar hingga 70% dari uang yang dimiliki
        int maxBid = static_cast<int>(bidder->getMoney() * 0.70);
        if (bid1 > maxBid || bid1 > bidder->getMoney()) return 2; // pass
        if (bid2 <= maxBid && bid2 <= bidder->getMoney()) return 1; // tawar tinggi
        return 0; // tawar rendah
    } else {
        // EASY: 30% kemungkinan tawar minimum, sisanya pass
        std::uniform_int_distribution<> d(0, 9);
        if (d(getRng()) < 3 && bid1 <= bidder->getMoney()) return 0;
        return 2;
    }
}

// ---- decideDropCard ----

int ComputerAI::decideDropCard(const Player *player, const SkillCard *newCard,
                                Difficulty diff) {
    if (!player) return 0;
    const auto &cards = player->getHandCards();
    if (cards.empty()) return 0;

    if (diff == Difficulty::HARD) {
        // Prioritas: Shield > Teleport > Move > Demolition > Discount > Lasso
        auto priority = [](SkillCardType t) -> int {
            switch (t) {
            case SkillCardType::SHIELD:      return 6;
            case SkillCardType::TELEPORT:    return 5;
            case SkillCardType::MOVE:        return 4;
            case SkillCardType::DEMOLITION:  return 3;
            case SkillCardType::DISCOUNT:    return 2;
            case SkillCardType::LASSO:       return 1;
            default:                          return 0;
            }
        };

        // Abaikan newCard — kita harus membuang kartu yang sudah ada di tangan
        (void)newCard;

        int minPrio = INT_MAX;
        int minIdx  = 0;
        for (int i = 0; i < static_cast<int>(cards.size()); i++) {
            int p = priority(cards[i]->getSkillType());
            if (p < minPrio) {
                minPrio = p;
                minIdx  = i;
            }
        }
        return minIdx;
    } else {
        // EASY: buang kartu pertama
        return 0;
    }
}

// ---- decideBuildTarget ----

std::string ComputerAI::decideBuildTarget(const Player *player, Board *board,
                                           Difficulty diff) {
    if (!player || !board || diff == Difficulty::EASY) return "";

    // Hitung total petak per color group di papan
    std::map<std::string, int> totalInGroup;
    for (int i = 1; i <= board->getTotalTiles(); i++) {
        Tile *t = board->getTileAt(i);
        if (t && t->isStreet())
            totalInGroup[t->getColorGroup()]++;
    }

    // Kumpulkan properti street milik pemain per color group
    std::map<std::string, std::vector<const StreetTile *>> ownedInGroup;
    for (const PropertyTile *prop : player->getOwnedProperties()) {
        if (!prop->isStreet()) continue;
        if (prop->getStatus() != 1) continue; // skip digadaikan
        const StreetTile *st = static_cast<const StreetTile *>(prop);
        ownedInGroup[st->getColorGroup()].push_back(st);
    }

    // Untuk setiap color group yang dimonopoli, cari target bangun
    for (const auto &[color, streets] : ownedInGroup) {
        auto it = totalInGroup.find(color);
        if (it == totalInGroup.end()) continue;
        if (static_cast<int>(streets.size()) != it->second) continue; // belum monopoli

        // Cari level bangunan minimum di group ini
        int minLevel = INT_MAX;
        for (const StreetTile *st : streets) {
            if (st->getStatus() != 1) continue;
            minLevel = std::min(minLevel, st->getRentLevel());
        }
        if (minLevel == INT_MAX || minLevel >= 5) continue; // semua sudah hotel

        // Pilih properti dengan level terendah yang mampu dibayar
        for (const StreetTile *st : streets) {
            if (st->getStatus() != 1) continue;
            if (st->getRentLevel() != minLevel) continue; // patuhi aturan merata
            if (st->getRentLevel() >= 5) continue;        // sudah hotel

            // Pastikan ada cadangan uang setelah membangun
            int cost = (st->getRentLevel() == 4) ? st->getHotelCost()
                                                  : st->getHouseCost();
            if (player->getMoney() > cost + 200) // simpan cadangan M200
                return st->getKode();
        }
    }
    return "";
}
