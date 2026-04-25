#include "../../include/core/GameEngine.hpp"
#include "../../include/core/Auction.hpp"
#include "../../include/core/ConfigLoader.hpp"
#include "../../include/core/Enums.hpp"
#include "../../include/core/Exceptions.hpp"
#include "../../include/core/Player.hpp"
#include "../../include/core/TransactionLogger.hpp"
#include "../../include/models/ActionTile.hpp"
#include "../../include/models/Board.hpp"
#include "../../include/models/Card.hpp"
#include "../../include/models/PropertyTile.hpp"
#include "../../include/models/Tile.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

using namespace std;

// Konstruktor menginisialisasi state awal permainan
GameEngine::GameEngine(Board *b, TransactionLogger *l)
    : board(b), saveLoadManager(nullptr), logger(l), skillDeck(nullptr),
      chanceDeck(nullptr), communityDeck(nullptr), currentTurnIdx(0),
      roundCount(0), diceRolled(false), turnEnded(false),
      lastRollWasDouble(false) {}

// Destruktor bertanggung jawab menghapus semua pointer Player yang di-alokasi
GameEngine::~GameEngine() {
    for (Player *p : players) {
        delete p;
    }
    players.clear();
    delete chanceDeck;
    delete communityDeck;
    delete skillDeck;
}

void GameEngine::initDecks() {

    skillDeck = new CardDeck<SkillCard>();
    for (int i = 0; i < 4; i++)
        skillDeck->addCard(new MoveCard());
    for (int i = 0; i < 3; i++)
        skillDeck->addCard(new DiscountCard());
    for (int i = 0; i < 2; i++)
        skillDeck->addCard(new ShieldCard());
    for (int i = 0; i < 2; i++)
        skillDeck->addCard(new TeleportCard());
    for (int i = 0; i < 2; i++)
        skillDeck->addCard(new LassoCard());
    for (int i = 0; i < 2; i++)
        skillDeck->addCard(new DemolitionCard());
    skillDeck->shuffle();

    chanceDeck = new CardDeck<ActionCard>();
    chanceDeck->addCard(new ChanceCard(ChanceCardType::NEAREST_STATION));
    chanceDeck->addCard(new ChanceCard(ChanceCardType::MOVE_BACK_3));
    chanceDeck->addCard(new ChanceCard(ChanceCardType::GO_TO_JAIL));
    chanceDeck->shuffle();

    communityDeck = new CardDeck<ActionCard>();
    communityDeck->addCard(
        new CommunityChestCard(CommunityChestCardType::BIRTHDAY));
    communityDeck->addCard(
        new CommunityChestCard(CommunityChestCardType::DOCTOR));
    communityDeck->addCard(
        new CommunityChestCard(CommunityChestCardType::ELECTION));
    communityDeck->shuffle();
}

void GameEngine::startNewGame(const std::vector<std::string> &playerNames) {
    // 1. Reset state jika sebelumnya sudah ada permainan
    for (Player *p : players)
        delete p;
    players.clear();

    roundCount = 1;
    currentTurnIdx = 0;
    diceRolled = false;
    turnEnded = false;
    lastRollWasDouble = false;

    // 2. (Re)inisialisasi deck kartu
    delete skillDeck;    skillDeck    = nullptr;
    delete chanceDeck;   chanceDeck   = nullptr;
    delete communityDeck; communityDeck = nullptr;
    initDecks();

    int initialMoney = ConfigLoader::getInstance()->getInitialMoney();

    for (const string &name : playerNames) {
        players.push_back(new Player(name, initialMoney));
    }

    random_device rd;
    mt19937 g(rd());
    shuffle(players.begin(), players.end(), g);

    for (int i = 0; i < (int)players.size(); i++)
        players[i]->setId(i);

    // Kartu dibagikan di awal giliran masing-masing pemain (lihat main.cpp)

    if (logger)
        logger->logEvent(0, "SYSTEM", LogActionType::LOAD, "Game Started");
}

void GameEngine::saveGame(const std::string &filePath) {
    std::ofstream out(filePath);
    if (!out.is_open()) {
        throw std::runtime_error("Tidak bisa membuka file untuk simpan: " + filePath);
    }

    out << "NIMONSPOLI_SAVE_V1\n";
    out << "GAME " << currentTurnIdx << " " << roundCount << "\n";

    for (Player *p : players) {
        int statusInt = 0;
        if (p->getStatus() == PlayerStatus::JAILED) statusInt = 1;
        else if (p->getStatus() == PlayerStatus::BANKRUPT) statusInt = 2;
        out << "PLAYER "
            << p->getId() << " "
            << p->getUsername() << " "
            << p->getMoney() << " "
            << p->getPosition() << " "
            << statusInt << " "
            << p->getJailTurnsLeft() << " "
            << (p->isShieldActive() ? 1 : 0) << " "
            << p->getActiveDiscountPercent() << "\n";

        for (SkillCard *sc : p->getHandCards()) {
            std::string typeName;
            switch (sc->getSkillType()) {
                case SkillCardType::MOVE:       typeName = "MOVE";       break;
                case SkillCardType::DISCOUNT:   typeName = "DISCOUNT";   break;
                case SkillCardType::SHIELD:     typeName = "SHIELD";     break;
                case SkillCardType::TELEPORT:   typeName = "TELEPORT";   break;
                case SkillCardType::LASSO:      typeName = "LASSO";      break;
                case SkillCardType::DEMOLITION: typeName = "DEMOLITION"; break;
                default:                        typeName = "UNKNOWN";    break;
            }
            std::string val = sc->getValueString();
            out << "SKILLCARD " << p->getId() << " " << typeName
                << " " << (val.empty() ? "0" : val) << "\n";
        }
    }

    int totalTiles = board->getTotalTiles();
    for (int i = 1; i <= totalTiles; i++) {
        Tile *tile = board->getTileAt(i);
        if (!tile) continue;
        if (!tile->isProperty()) continue;
        PropertyTile *prop = static_cast<PropertyTile *>(tile);

        int rentLevel = tile->getRentLevel();
        int festMult = 1;
        bool monopolized = false;

        // Determine isMonopolized by checking if all tiles in the same color
        // group belong to the same owner
        if (tile->isStreet() && prop->getOwnerId() != -1) {
            std::string cg = tile->getColorGroup();
            auto groupTiles = board->getTileByColorGroup(cg);
            bool allSame = true;
            for (Tile *gt : groupTiles) {
                if (!gt->isProperty() ||
                    static_cast<PropertyTile *>(gt)->getOwnerId() != prop->getOwnerId()) {
                    allSame = false;
                    break;
                }
            }
            monopolized = allSame;
        }

        out << "PROPERTY "
            << tile->getKode() << " "
            << prop->getOwnerId() << " "
            << prop->getStatus() << " "
            << rentLevel << " "
            << festMult << " "
            << (monopolized ? 1 : 0) << "\n";
    }

    out << "END\n";
    out.close();

    if (logger)
        logger->logEvent(roundCount, "SYSTEM", LogActionType::SAVE, filePath);
}

void GameEngine::loadGame(const std::string &filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        throw std::runtime_error("File tidak ditemukan: " + filePath);
    }

    std::string line;
    std::getline(in, line);
    if (line != "NIMONSPOLI_SAVE_V1") {
        throw std::runtime_error("Format file save tidak valid.");
    }

    // --- Parse GAME line ---
    int savedTurnIdx = 0, savedRound = 1;
    std::getline(in, line);
    {
        std::istringstream ss(line);
        std::string tag;
        ss >> tag >> savedTurnIdx >> savedRound;
    }

    // 1. Reset semua tile board ke kondisi awal
    int totalTiles = board->getTotalTiles();
    for (int i = 1; i <= totalTiles; i++) {
        Tile *tile = board->getTileAt(i);
        if (!tile) continue;
        if (!tile->isProperty()) continue;
        PropertyTile *prop = static_cast<PropertyTile *>(tile);
        prop->setOwnerId(-1);
        prop->setStatus(0);
        if (tile->isStreet()) {
            StreetTile *st = static_cast<StreetTile *>(tile);
            st->demolish(); st->setMonopolized(false); st->clearFestivalEffect();
        }
        if (tile->isRailroad())
            static_cast<RailroadTile *>(tile)->setrailroadOwnedCount(1);
        if (tile->isUtility())
            static_cast<UtilityTile *>(tile)->setUtilityOwnedCount(1);
    }

    // 2. Hapus semua player lama
    for (Player *p : players) delete p;
    players.clear();

    int initialMoney = ConfigLoader::getInstance()->getInitialMoney();

    // 3. Parse dan terapkan setiap baris langsung (tanpa buffer sementara)
    //    Format file menjamin PLAYER selalu sebelum SKILLCARD dan PROPERTY.
    while (std::getline(in, line)) {
        if (line == "END") break;
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "PLAYER") {
            int id, money, position, statusInt, jailTurns, shieldInt, discount;
            std::string username;
            ss >> id >> username >> money >> position
               >> statusInt >> jailTurns >> shieldInt >> discount;

            Player *p = new Player(username, initialMoney);
            p->setId(id);
            p->setMoney(money);
            p->setPosition(position);
            PlayerStatus ps = PlayerStatus::ACTIVE;
            if (statusInt == 1) ps = PlayerStatus::JAILED;
            else if (statusInt == 2) ps = PlayerStatus::BANKRUPT;
            p->setStatus(ps);
            p->setJailTurnsLeft(jailTurns);
            p->setShieldActive(shieldInt != 0);
            p->setActiveDiscountPercent(discount);
            players.push_back(p);

        } else if (tag == "SKILLCARD") {
            int playerId, value;
            std::string typeName;
            ss >> playerId >> typeName >> value;

            SkillCard *card = nullptr;
            if (typeName == "MOVE")            card = new MoveCard(value);
            else if (typeName == "DISCOUNT")   card = new DiscountCard(value);
            else if (typeName == "SHIELD")     card = new ShieldCard();
            else if (typeName == "TELEPORT")   card = new TeleportCard();
            else if (typeName == "LASSO")      card = new LassoCard();
            else if (typeName == "DEMOLITION") card = new DemolitionCard();

            if (card) {
                bool handled = false;
                for (Player *p : players) {
                    if (p->getId() == playerId) {
                        try { p->addCard(card); } catch (...) { delete card; }
                        handled = true;
                        break;
                    }
                }
                if (!handled) delete card;
            }

        } else if (tag == "PROPERTY") {
            std::string kode;
            int ownerId, propStatus, rentLevel, festMult, monoInt;
            ss >> kode >> ownerId >> propStatus >> rentLevel >> festMult >> monoInt;

            Tile *tile = board->getTileByKode(kode);
            if (!tile) continue;
            if (!tile->isProperty()) continue;
            PropertyTile *prop = static_cast<PropertyTile *>(tile);

            prop->setOwnerId(ownerId);
            prop->setStatus(propStatus);

            if (tile->isStreet()) {
                StreetTile *st = static_cast<StreetTile *>(tile);
                st->setMonopolized(monoInt != 0);
                if (rentLevel > 0) {
                    for (int h = 0; h < rentLevel && h < 4; h++)
                        st->buildHouse();
                    if (rentLevel == 5)
                        st->buildHotel();
                }
                if (festMult > 1)
                    st->setFestivalEffect(festMult);
            }
            if (tile->isRailroad() && ownerId != -1) {
                RailroadTile *rr = static_cast<RailroadTile *>(tile);
                int cnt = 0;
                for (int i = 1; i <= totalTiles; i++) {
                    Tile *t2 = board->getTileAt(i);
                    if (t2 && t2->isRailroad() &&
                        static_cast<RailroadTile *>(t2)->getOwnerId() == ownerId) cnt++;
                }
                rr->setrailroadOwnedCount(cnt > 0 ? cnt : 1);
            }
            if (tile->isUtility() && ownerId != -1) {
                UtilityTile *ut = static_cast<UtilityTile *>(tile);
                int cnt = 0;
                for (int i = 1; i <= totalTiles; i++) {
                    Tile *t2 = board->getTileAt(i);
                    if (t2 && t2->isUtility() &&
                        static_cast<UtilityTile *>(t2)->getOwnerId() == ownerId) cnt++;
                }
                ut->setUtilityOwnedCount(cnt > 0 ? cnt : 1);
            }
        }
    }
    in.close();

    // 4. Bangun ulang daftar ownedProperties dari state board
    for (int i = 1; i <= totalTiles; i++) {
        Tile *tile = board->getTileAt(i);
        if (!tile || !tile->isProperty()) continue;
        PropertyTile *prop = static_cast<PropertyTile *>(tile);
        if (prop->getOwnerId() == -1) continue;
        for (Player *p : players) {
            if (p->getId() == prop->getOwnerId()) {
                p->addProperty(prop);
                break;
            }
        }
    }

    // 5. Restore game state
    currentTurnIdx = savedTurnIdx;
    roundCount = savedRound;
    diceRolled = false;
    turnEnded = false;
    lastRollWasDouble = false;

    if (logger)
        logger->logEvent(roundCount, "SYSTEM", LogActionType::LOAD, filePath);
}

const std::vector<SkillCard *> &GameEngine::getSkillDeckCards() const {
    if (skillDeck)
        return skillDeck->getDeck();
    static std::vector<SkillCard *> empty;
    return empty;
}

SkillCard *GameEngine::drawSkillCard() {
    if (!skillDeck) return nullptr;
    if (skillDeck->isEmpty()) skillDeck->reshuffleDiscard();
    if (skillDeck->isEmpty()) return nullptr;
    return skillDeck->draw();
}

void GameEngine::discardSkillCard(SkillCard *card) {
    if (skillDeck && card) skillDeck->discard(card);
}

ActionCard *GameEngine::drawChanceCard() {
    if (!chanceDeck) return nullptr;
    if (chanceDeck->isEmpty()) chanceDeck->reshuffleDiscard();
    if (chanceDeck->isEmpty()) return nullptr;
    return chanceDeck->draw();
}

void GameEngine::discardChanceCard(ActionCard *card) {
    if (chanceDeck && card) chanceDeck->discard(card);
}

ActionCard *GameEngine::drawCommunityCard() {
    if (!communityDeck) return nullptr;
    if (communityDeck->isEmpty()) communityDeck->reshuffleDiscard();
    if (communityDeck->isEmpty()) return nullptr;
    return communityDeck->draw();
}

void GameEngine::discardCommunityCard(ActionCard *card) {
    if (communityDeck && card) communityDeck->discard(card);
}

void GameEngine::rollDice(int d1, int d2) {
    if (diceRolled) {
        throw InvalidCommandException("Dadu sudah dikocok pada giliran ini");
    }

    int finalD1 = d1;
    int finalD2 = d2;

    if (d1 == 0 && d2 == 0) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(1, 6);
        finalD1 = dis(gen);
        finalD2 = dis(gen);
    } else if (d1 < 1 || d1 > 6 || d2 < 1 || d2 > 6) {
        throw InvalidCommandException("Nilai dadu tidak valid");
    }

    int diceTotal = finalD1 + finalD2;
    Player *activePlayer = players[currentTurnIdx];

    std::cout << "\n[DADU] " << activePlayer->getUsername()
              << " melempar dadu: " << finalD1 << " + " << finalD2 << " = "
              << diceTotal << "\n";

    // Cek Status Penjara
    if (activePlayer->getStatus() == PlayerStatus::JAILED) {
        if (finalD1 == finalD2) {
            std::cout << "[PENJARA] Anda mendapat angka ganda (Double)! Anda "
                         "bebas dari penjara.\n";
            Tile *jailTile = board->getTileAt(board->getJailPosition());
            if (jailTile && jailTile->isJailTile())
                static_cast<JailTile *>(jailTile)->releasePlayer(*activePlayer);
        } else {
            std::cout << "[PENJARA] Bukan double. Anda tetap di penjara.\n";
            activePlayer->decrementJailTurn();
            diceRolled = true;
            return; // Giliran berakhir tanpa gerak
        }
    }

    // Gerakkan pemain berdasarkan total dadu
    int oldPos = activePlayer->getPosition();
    activePlayer->move(diceTotal);
    diceRolled = true;

    // Cek melewati Start/GO (Mendapatkan gaji)
    if (oldPos + diceTotal >= board->getTotalTiles() && oldPos != 0) {
        Tile *goTile = board->getTileAt(0);
        if (goTile && goTile->isGoTile()) {
            GoTile *go = static_cast<GoTile *>(goTile);
            go->awardSalary(*activePlayer);
            std::cout
                << "[INFO] Anda melewati petak GO! Menerima gaji sebesar Rp"
                << go->getSalary() << ".\n";
            if (logger)
                logger->logEvent(roundCount, activePlayer->getUsername(),
                                 LogActionType::GO, "Lewat GO");
        }
    }

    // Khusus UtilityTile, simpan nilai dadu terakhir untuk kalkulasi sewa
    Tile *landedTile = board->getTileAt(activePlayer->getPosition());
    if (landedTile && landedTile->isUtility()) {
        static_cast<UtilityTile *>(landedTile)->setLastDiceRoll(diceTotal);
    }

    if (finalD1 == finalD2) {
        activePlayer->incrementDoubleStreak();
        // Aturan standar: 3 kali double berturut-turut -> penjara
        if (activePlayer->getDoubleStreak() >= 3) {
            activePlayer->goToJail();
            diceRolled = true;       // Giliran selesai, bisa endTurn
            lastRollWasDouble = false; // Jangan beri bonus turn setelah masuk penjara
        } else {
            lastRollWasDouble = true; // Double biasa, beri giliran bonus
        }
    } else {
        // TODO:
        // Jika tidak double, kita perlu memastikan bahwa double streak
        // diabaikan saat endTurn. Kita bisa asumsikan Player::resetTurnFlags()
        // akan dihandle di endTurn, tapi untuk flag kita simpan informasi bahwa
        // roll terakhir bukan double. Karena kita tidak memodifikasi
        // GameEngine.hpp, kita asumsikan getDoubleStreak menandakan jumlah
        // kesempatan roll, bukan hasil roll. Idealnya jika roll bukan double,
        // streak seharusnya 0. Kita biarkan sesuai implementasi Player (jika
        // tidak ada resetter spesifik, endTurn akan memanggil resetTurnFlags).
    }
}

void GameEngine::executeTileAction() {
    if (!diceRolled) {
        throw InvalidCommandException("Lempar dadu terlebih dahulu");
    }

    Player *activePlayer = players[currentTurnIdx];
    Tile *landedTile = board->getTileAt(activePlayer->getPosition());
    if (landedTile) {
        EffectType effect = landedTile->onLanded(*activePlayer);

        switch (effect) {
        case EffectType::OFFER_BUY: {
            if (!landedTile->isProperty()) break;
            PropertyTile *prop = static_cast<PropertyTile *>(landedTile);
                if (activePlayer->getMoney() >= prop->getPrice()) {
                    std::cout
                        << "\n[PILIHAN] " << activePlayer->getUsername()
                        << " mendarat di " << prop->getName() << ".\n"
                        << "Harga properti: Rp" << prop->getPrice() << "\n"
                        << "Uang Anda saat ini: Rp" << activePlayer->getMoney()
                        << "\n"
                        << "Apakah Anda ingin membeli properti ini? (Y/N): ";
                    char choice;
                    std::cin >> choice;

                    if (choice == 'Y' || choice == 'y') {
                        *activePlayer -= prop->getPrice();
                        prop->setOwnerId(activePlayer->getId());
                        prop->setStatus(1); // 1 = OWNED
                        activePlayer->addProperty(prop);
                        std::cout << "Selamat! Anda berhasil membeli "
                                  << prop->getName() << ".\n";
                        if (logger)
                            logger->logEvent(
                                roundCount, activePlayer->getUsername(),
                                LogActionType::BUY, prop->getName());
                    } else {
                        std::cout << "Anda menolak membeli. Properti akan "
                                     "dilelang!\n";
                        std::vector<Player *> activeBidders;
                        for (Player *p : players) {
                            if (p->getStatus() != PlayerStatus::BANKRUPT)
                                activeBidders.push_back(p);
                        }
                        Auction auction(prop, activeBidders);
                        auction.run();
                    }
                } else {
                    std::cout << "\n[INFO] Uang Anda tidak cukup untuk membeli "
                              << prop->getName()
                              << ". Properti otomatis dilelang!\n";
                    std::vector<Player *> activeBidders;
                    for (Player *p : players) {
                        if (p->getStatus() != PlayerStatus::BANKRUPT)
                            activeBidders.push_back(p);
                    }
                    Auction auction(prop, activeBidders);
                    auction.run();
                }
            break;
        }
        case EffectType::AUTO_ACQUIRE: {
            if (!landedTile->isProperty()) break;
            PropertyTile *prop = static_cast<PropertyTile *>(landedTile);
            if (prop->getOwnerId() == -1) {
                prop->setOwnerId(activePlayer->getId());
                prop->setStatus(1);
                activePlayer->addProperty(prop);
                if (logger)
                    logger->logEvent(roundCount, activePlayer->getUsername(),
                                     LogActionType::BUY,
                                     prop->getName() + " (Auto)");
            }
            break;
        }
        case EffectType::PAY_RENT: {
            if (!landedTile->isProperty()) break;
            PropertyTile *prop = (PropertyTile *)landedTile;
            if (prop->getOwnerId() != -1 &&
                prop->getOwnerId() != activePlayer->getId()) {
                // Cari owner
                Player *owner = nullptr;
                for (Player *p : players) {
                    if (p->getId() == prop->getOwnerId()) {
                        owner = p;
                        break;
                    }
                }
                if (owner) {
                    int rentAmount = prop->calcRent();

                    // Cek efek dari skill card (Shield atau Discount)
                    if (activePlayer->isShieldActive()) {
                        activePlayer->setShieldActive(false);
                        rentAmount = 0;
                        if (logger)
                            logger->logEvent(roundCount,
                                             activePlayer->getUsername(),
                                             LogActionType::CARD,
                                             "Shield menahan bayar sewa");
                    } else if (activePlayer->getActiveDiscountPercent() > 0) {
                        rentAmount =
                            rentAmount *
                            (100 - activePlayer->getActiveDiscountPercent()) /
                            100;
                    }

                    try {
                        *activePlayer -= rentAmount;
                        *owner += rentAmount;
                        if (logger)
                            logger->logEvent(roundCount,
                                             activePlayer->getUsername(),
                                             LogActionType::RENT,
                                             "Ke " + owner->getUsername());
                    } catch (NotEnoughMoneyException &e) {
                        std::cout
                            << "[BANGKRUT] " << activePlayer->getUsername()
                            << " bangkrut karena tidak bisa membayar sewa!\n";
                        handleBankruptcy(activePlayer, owner);
                        if (logger)
                            logger->logEvent(
                                roundCount, activePlayer->getUsername(),
                                LogActionType::BANKRUPT,
                                "Bangkrut sewa ke " + owner->getUsername());
                    }
                }
            }
            break;
        }
        case EffectType::PAY_TAX_FLAT: {
            if (!landedTile->isTaxTile()) break;
            TaxTile *tax = static_cast<TaxTile *>(landedTile);
                try {
                    *activePlayer -= tax->getFlatAmount();
                    if (logger)
                        logger->logEvent(roundCount,
                                         activePlayer->getUsername(),
                                         LogActionType::TAX, tax->getName());
                } catch (NotEnoughMoneyException &e) {
                    std::cout
                        << "[BANGKRUT] " << activePlayer->getUsername()
                        << " bangkrut karena tidak bisa membayar pajak flat!\n";
                    handleBankruptcy(activePlayer, nullptr);
                }
            break;
        }
        case EffectType::PAY_TAX_CHOICE: {
            if (!landedTile->isTaxTile()) break;
            TaxTile *tax = static_cast<TaxTile *>(landedTile);
                int flat = tax->getFlatAmount();
                int pct = static_cast<int>(tax->computeNetWorth(*activePlayer) *
                                           tax->getPercentageRate() / 100.0);

                std::cout << "\n[PAJAK] " << activePlayer->getUsername()
                          << " mendarat di " << tax->getName() << ".\n"
                          << "Silakan pilih metode pembayaran pajak:\n"
                          << "1. Bayar nominal tetap (Rp" << flat << ")\n"
                          << "2. Bayar persentase " << tax->getPercentageRate()
                          << "% dari total kekayaan (Rp" << pct << ")\n"
                          << "Pilihan (1/2): ";
                int choice;
                std::cin >> choice;

                int amount = (choice == 1) ? flat : pct;
                try {
                    *activePlayer -= amount;
                    std::cout << "Anda membayar pajak sebesar Rp" << amount
                              << ".\n";
                    if (logger)
                        logger->logEvent(roundCount,
                                         activePlayer->getUsername(),
                                         LogActionType::TAX, tax->getName());
                } catch (NotEnoughMoneyException &e) {
                    std::cout << "[BANGKRUT] " << activePlayer->getUsername()
                              << " bangkrut karena tidak bisa membayar pajak "
                                 "persentase!\n";
                    handleBankruptcy(activePlayer, nullptr);
                }
            break;
        }
        case EffectType::START_AUCTION: {
            if (!landedTile->isProperty()) break;
            PropertyTile *prop = static_cast<PropertyTile *>(landedTile);
            if (prop->getOwnerId() == -1) {
                std::vector<Player *> activeBidders;
                for (Player *p : players) {
                    if (p->getStatus() != PlayerStatus::BANKRUPT)
                        activeBidders.push_back(p);
                }
                Auction auction(prop, activeBidders);
                auction.run();
            }
            break;
        }
        case EffectType::DRAW_CHANCE: {
            if (!chanceDeck)
                break;
            ActionCard *card = chanceDeck->draw();
            std::cout << "\n[KESEMPATAN] " << activePlayer->getUsername()
                      << " mengambil kartu Kesempatan!\n";
            ActionCardEffect cardEffect = card->execute(*activePlayer, *this);
            chanceDeck->discard(card);
            handleActionCardEffect(*activePlayer, cardEffect);
            break;
        }
        case EffectType::DRAW_COMMUNITY: {
            if (!communityDeck)
                break;
            ActionCard *card = communityDeck->draw();
            std::cout << "\n[DANA UMUM] " << activePlayer->getUsername()
                      << " mengambil kartu Dana Umum!\n";
            ActionCardEffect cardEffect = card->execute(*activePlayer, *this);
            communityDeck->discard(card);
            handleActionCardEffect(*activePlayer, cardEffect);
            break;
        }
        case EffectType::FESTIVAL_TRIGGER: {
            std::cout << "\n[FESTIVAL] Anda mendarat di petak Festival!\n"
                      << "Pilih properti yang Anda miliki untuk "
                         "dilipatgandakan harga sewanya (x2).\n";

            auto props = activePlayer->getOwnedProperties();
            if (props.empty()) {
                std::cout << "Sayang sekali, Anda belum memiliki properti "
                             "satupun untuk diadakan festival.\n";
            } else {
                for (size_t i = 0; i < props.size(); i++) {
                    std::cout << i + 1 << ". " << props[i]->getName() << " ("
                              << props[i]->getKode() << ")\n";
                }
                std::cout << "Masukkan nomor properti (1-" << props.size()
                          << "): ";
                int fChoice;
                std::cin >> fChoice;
                if (fChoice >= 1 && fChoice <= (int)props.size()) {
                    PropertyTile *sel = props[fChoice - 1];
                    if (sel->isStreet()) {
                        StreetTile *street = static_cast<StreetTile *>(sel);
                        street->setFestivalEffect(2); // Set multiplier
                        std::cout << "Festival berhasil diadakan di "
                                  << street->getName()
                                  << "! Harga sewa naik 2x lipat.\n";
                    } else {
                        std::cout << "Festival hanya bisa diadakan di properti "
                                     "jalan (Street).\n";
                    }
                }
            }
            break;
        }
        default:
            break;
        }
    }
}
void GameEngine::handleActionCardEffect(Player &player,
                                        ActionCardEffect effect) {
    switch (effect) {
    case ActionCardEffect::CHANCE_NEAREST_STATION: {
        int pos = player.getPosition();
        for (int i = 1; i <= 40; i++) {
            int checkIdx = (pos + i) % 40 + 1;
            Tile *t = board->getTileAt(checkIdx);
            if (t && t->isRailroad()) {
                player.move(i);
                std::cout << player.getUsername()
                          << " maju ke stasiun terdekat: " << t->getName()
                          << ".\n";
                break;
            }
        }
        break;
    }

    case ActionCardEffect::CHANCE_MOVE_BACK_3:
        player.move(-3);
        std::cout << player.getUsername() << " mundur 3 petak.\n";
        break;

    case ActionCardEffect::CHANCE_GO_TO_JAIL:
        player.goToJail();
        std::cout << player.getUsername() << " masuk penjara!\n";
        break;

    case ActionCardEffect::COMMUNITY_BIRTHDAY:
        for (Player *p : players) {
            if (p->getUsername() == player.getUsername())
                continue;
            if (p->getStatus() == PlayerStatus::BANKRUPT)
                continue;
            try {
                *p -= 100;
                player += 100;
                std::cout << p->getUsername()
                          << " membayar M100 hadiah ulang tahun ke "
                          << player.getUsername() << ".\n";
            } catch (NotEnoughMoneyException &) {
                handleBankruptcy(p, &player);
            }
        }
        break;

    case ActionCardEffect::COMMUNITY_DOCTOR:
        try {
            player -= 700;
            std::cout << player.getUsername()
                      << " membayar M700 biaya dokter ke Bank."
                      << " Sisa: Rp" << player.getMoney() << ".\n";
        } catch (NotEnoughMoneyException &) {
            std::cout << "[BANGKRUT] Tidak bisa membayar biaya dokter!\n";
            handleBankruptcy(&player, nullptr);
        }
        break;

    case ActionCardEffect::COMMUNITY_ELECTION:
        for (Player *p : players) {
            if (p->getUsername() == player.getUsername())
                continue;
            if (p->getStatus() == PlayerStatus::BANKRUPT)
                continue;
            try {
                player -= 200;
                *p += 200;
                std::cout << player.getUsername()
                          << " membayar M200 sumbangan pemilu ke "
                          << p->getUsername() << ".\n";
            } catch (NotEnoughMoneyException &) {
                std::cout
                    << "[BANGKRUT] Tidak bisa membayar sumbangan pemilu!\n";
                handleBankruptcy(&player, p);
                return;
            }
        }
        break;

    default:
        break;
    }
}

void GameEngine::useSkillCard(int cardIdx) {
    Player *activePlayer = players[currentTurnIdx];
    auto cards = activePlayer->getHandCards();

    if (cardIdx < 0 || cardIdx >= (int)cards.size()) {
        std::cout << "Index kartu tidak valid.\n";
        return;
    }

    if (activePlayer->getHasUsedCardThisTurn()) {
        std::cout << "Kamu sudah menggunakan kartu pada giliran ini!\n";
        return;
    }

    if (diceRolled) {
        std::cout
            << "Kartu kemampuan hanya bisa digunakan SEBELUM melempar dadu.\n";
        return;
    }

    SkillCard *card = cards[cardIdx];
    SkillCardEffect effect = card->use(*activePlayer, *this);
    activePlayer->removeCard(cardIdx);
    activePlayer->setHasUsedCardThisTurn(true);

    std::cout << "Anda menggunakan kartu: " << card->getCardName() << "!\n";
    if (logger)
        logger->logEvent(roundCount, activePlayer->getUsername(),
                         LogActionType::CARD, card->getCardName());

    switch (effect) {
    case SkillCardEffect::MOVE: {
        int steps = card->getSteps();
        if (steps > 0) {
            activePlayer->move(steps);
            std::cout << activePlayer->getUsername() << " maju "
                      << steps << " petak.\n";
        }
        break;
    }

    case SkillCardEffect::DISCOUNT: {
        int pct = card->getDiscountPercent();
        if (pct > 0) {
            activePlayer->setActiveDiscountPercent(pct);
            std::cout << "Diskon " << pct
                      << "% aktif untuk giliran ini.\n";
        }
        break;
    }

    case SkillCardEffect::SHIELD:
        activePlayer->setShieldActive(true);
        std::cout << "Shield aktif! Kebal tagihan giliran ini.\n";
        break;

    case SkillCardEffect::TELEPORT: {
        std::cout << "Masukkan kode petak tujuan: ";
        std::string kode;
        std::cin >> kode;
        Tile *target = board->getTileByKode(kode);
        if (!target) {
            std::cout << "Petak tidak ditemukan!\n";
            break;
        }
        int dest = target->getIndex() - 1;
        int curr = activePlayer->getPosition();
        int steps = (dest - curr + 40) % 40;
        activePlayer->move(steps);
        std::cout << activePlayer->getUsername() << " teleport ke "
                  << target->getName() << ".\n";
        break;
    }

    case SkillCardEffect::LASSO: {
        int myPos = activePlayer->getPosition();
        Player *target = nullptr;
        int minDist = 40;
        for (Player *p : players) {
            if (p->getUsername() == activePlayer->getUsername())
                continue;
            if (p->getStatus() == PlayerStatus::BANKRUPT)
                continue;
            int dist = (p->getPosition() - myPos + 40) % 40;
            if (dist > 0 && dist < minDist) {
                minDist = dist;
                target = p;
            }
        }
        if (!target) {
            std::cout << "Tidak ada lawan di depan.\n";
            break;
        }
        int pullSteps = (myPos - target->getPosition() + 40) % 40;
        target->move(pullSteps);
        std::cout << target->getUsername() << " ditarik ke posisi "
                  << activePlayer->getUsername() << ".\n";
        break;
    }

    case SkillCardEffect::DEMOLITION: {
        std::vector<StreetTile *> targets;
        for (Player *p : players) {
            if (p->getUsername() == activePlayer->getUsername())
                continue;
            if (p->getStatus() == PlayerStatus::BANKRUPT)
                continue;
            for (PropertyTile *prop : p->getOwnedProperties()) {
                if (prop->isStreet() && prop->hasBuildings())
                    targets.push_back(static_cast<StreetTile *>(prop));
            }
        }
        if (targets.empty()) {
            std::cout << "Tidak ada properti lawan yang memiliki bangunan.\n";
            break;
        }
        std::cout << "Pilih properti untuk dihancurkan:\n";
        for (int i = 0; i < (int)targets.size(); i++) {
            std::cout << i + 1 << ". " << targets[i]->getName() << " ["
                      << targets[i]->getColorGroup() << "]\n";
        }
        int choice;
        std::cin >> choice;
        if (choice >= 1 && choice <= (int)targets.size()) {
            targets[choice - 1]->demolish();
            std::cout << targets[choice - 1]->getName()
                      << " bangunannya berhasil dihancurkan!\n";
        }
        break;
    }

    default:
        break;
    }

    // Kembalikan kartu ke discard pile skill deck
    if (skillDeck)
        skillDeck->discard(card);
}

void GameEngine::handleBankruptcy(Player *bankruptPlayer, Player *creditor) {
    bankruptPlayer->declareBankruptcy();
    auto properties = bankruptPlayer->getOwnedProperties();

    for (PropertyTile *prop : properties) {
        if (creditor) {
            prop->setOwnerId(creditor->getId());
            creditor->addProperty(prop);
        } else {
            prop->setOwnerId(-1);
            prop->setStatus(0); // Kembali ke BANK
            if (prop->isStreet() && prop->hasBuildings())
                static_cast<StreetTile *>(prop)->demolish();
        }
    }

    for (PropertyTile *prop : properties) {
        bankruptPlayer->removeProperty(prop);
    }

    std::cout << "Semua aset milik " << bankruptPlayer->getUsername()
              << " telah "
              << (creditor ? "diserahkan ke " + creditor->getUsername()
                           : "disita oleh Bank")
              << ".\n";
}

void GameEngine::sellBuilding(const std::string &colorGroup) {
    vector<Tile *> curr = board->getTileByColorGroup(colorGroup);
    cout << "Daftar bangunan di Color Group [" << colorGroup << "]" << endl;
    int i = 1;
    for (auto v : curr) {
        int level = v->getRentLevel();
        cout << i << ". " << v->getName() << "(" << v->getKode() << ") - ";
        if (level <= 4)
            cout << level << " rumah";
        else
            cout << "Hotel";
        cout << " -> Nilai jual bangunan: " << v->calcValue();
        cout << endl;
    }
    for (auto v : curr) {
        v->demolish();
        // TODO : handle perpindahan duit
        players[v->getOwnerId()] += v->calcValue();
    }
}

void GameEngine::buyBuilding(const std::string &propertyCode) {
    Player *activePlayer = players[currentTurnIdx];
    Tile *tile = board->getTileByKode(propertyCode);
    if (!tile || !tile->isStreet() || tile->getOwnerId() != activePlayer->getId()) {
        std::cout << "Properti tidak valid atau bukan milik Anda.\n";
        return;
    }
    StreetTile *street = static_cast<StreetTile *>(tile);

    try {
        if (street->getRentLevel() < 4) { // Max 4 rumah
            // Even-building rule: semua properti satu warna harus punya rumah
            // yang sama sebelum bisa menambah 1 lagi di properti ini
            auto groupTiles = board->getTileByColorGroup(street->getColorGroup());
            for (Tile *gt : groupTiles) {
                if (gt == tile) continue;
                if (gt->isStreet() && gt->getRentLevel() < street->getRentLevel()) {
                    StreetTile *gs = static_cast<StreetTile *>(gt);
                    std::cout << "Aturan bangun merata: " << gs->getName()
                              << " harus dibangun terlebih dahulu.\n";
                    return;
                }
            }
            *activePlayer -= street->getHouseCost();
            street->buildHouse();
            std::cout << "Berhasil membangun rumah di " << street->getName()
                      << "!\n";
        } else if (street->getRentLevel() ==
                   4) { // Level 4 berarti sudah 4 rumah, bisa bangun hotel
            *activePlayer -= street->getHotelCost();
            street->buildHotel();
            std::cout << "Berhasil membangun hotel di " << street->getName()
                      << "!\n";
        } else {
            std::cout << "Properti sudah mencapai level bangunan maksimal!\n";
        }
    } catch (NotEnoughMoneyException &e) {
        std::cout << "Uang Anda tidak cukup untuk membangun di sini.\n";
    }
}

void GameEngine::mortgageProperty(const std::string &propertyCode) {
    Player *activePlayer = players[currentTurnIdx];
    Tile *tile = board->getTileByKode(propertyCode);
    if (!tile || !tile->isProperty()) {
        std::cout << "Properti tidak valid atau bukan milik Anda.\n";
        return;
    }
    PropertyTile *prop = static_cast<PropertyTile *>(tile);
    if (prop->getOwnerId() != activePlayer->getId()) {
        std::cout << "Properti tidak valid atau bukan milik Anda.\n";
        return;
    }

    if (prop->getStatus() == 2) { // 2 = MORTGAGED
        std::cout << "Properti ini sudah dalam status tergadai.\n";
        return;
    }

    prop->mortgage();
    *activePlayer += prop->getmortgageValue();
    std::cout << prop->getName() << " berhasil digadaikan. Anda menerima Rp"
              << prop->getmortgageValue() << ".\n";
}

void GameEngine::unmortgageProperty(const std::string &propertyCode) {
    Player *activePlayer = players[currentTurnIdx];
    Tile *tile = board->getTileByKode(propertyCode);
    if (!tile || !tile->isProperty()) {
        std::cout << "Properti tidak valid atau bukan milik Anda.\n";
        return;
    }
    PropertyTile *prop = static_cast<PropertyTile *>(tile);
    if (prop->getOwnerId() != activePlayer->getId()) {
        std::cout << "Properti tidak valid atau bukan milik Anda.\n";
        return;
    }

    if (prop->getStatus() != 2) {
        std::cout << "Properti ini tidak sedang digadaikan.\n";
        return;
    }

    int cost = prop->getmortgageValue() +
               (prop->getmortgageValue() * 10 / 100); // Bunga 10%
    try {
        *activePlayer -= cost;
        prop->unmortgage();
        std::cout << prop->getName() << " berhasil ditebus dengan biaya Rp"
                  << cost << ".\n";
    } catch (NotEnoughMoneyException &e) {
        std::cout << "Uang tidak cukup untuk menebus properti (butuh Rp" << cost
                  << ").\n";
    }
}

void GameEngine::endTurn() {
    if (!diceRolled) {
        throw InvalidCommandException(
            "Anda harus melempar dadu sebelum mengakhir giliran.");
    }

    Player *activePlayer = players[currentTurnIdx];

    if (lastRollWasDouble &&
        activePlayer->getStatus() != PlayerStatus::JAILED) {
        // Roll double → dapat giliran bonus, belum pindah ke pemain berikutnya
        diceRolled = false;
        lastRollWasDouble = false;
        turnEnded = false;

        if (logger)
            logger->logEvent(roundCount, activePlayer->getUsername(),
                             LogActionType::DOUBLE_ROLL,
                             "Giliran bonus karena double!");
    } else {
        lastRollWasDouble = false;
        turnEnded = true;
        activePlayer->resetTurnFlags();
        nextTurn();
    }
}

void GameEngine::nextTurn() {
    int nextIdx = (currentTurnIdx + 1) % players.size();

    // Skip pemain bangkrut
    while (players[nextIdx]->getStatus() == PlayerStatus::BANKRUPT) {
        nextIdx = (nextIdx + 1) % players.size();
        if (nextIdx == currentTurnIdx)
            break; // Jika semua bangkrut, game over
    }

    if (nextIdx < currentTurnIdx) {
        roundCount++; // Increment ronde jika balik ke pemain pertama
    }

    currentTurnIdx = nextIdx;
    diceRolled = false;
    turnEnded = false;

    checkWinCondition();
}

void GameEngine::checkWinCondition() {
    int activePlayers = 0;
    for (Player *p : players) {
        if (p->getStatus() != PlayerStatus::BANKRUPT)
            activePlayers++;
    }

    // Jika sisa 1 orang
    if (activePlayers == 1) {
        throw GameStateException(
            "Permainan Selesai! Hanya tersisa satu pemain.");
    }

    // Jika turn maksimum tercapai
    int maxTurn = ConfigLoader::getInstance()->getMaxTurn();
    if (maxTurn > 0 && roundCount > maxTurn) {
        throw GameStateException("Batas Max Turn Tercapai!");
    }
}

Player *GameEngine::getCurrentPlayer() const {
    if (players.empty() || currentTurnIdx < 0 ||
        currentTurnIdx >= (int)players.size())
        return nullptr;
    return players[currentTurnIdx];
}
