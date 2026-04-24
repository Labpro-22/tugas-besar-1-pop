#include "../../include/core/ConfigLoader.hpp"
#include "../../include/core/Exceptions.hpp"
#include "../../include/core/GameEngine.hpp"
#include "../../include/core/Player.hpp"
#include "../../include/core/TransactionLogger.hpp"
#include "../../include/models/Board.hpp"
#include "../../include/models/Card.hpp"
#include "../../include/models/PropertyTile.hpp"
#include "../../include/models/Tile.hpp"
#include "../../include/core/Auction.hpp"
#include "../../include/models/ActionTile.hpp"

#include <algorithm>
#include <iostream>
#include <random>

using namespace std;

// Konstruktor menginisialisasi state awal permainan
GameEngine::GameEngine(Board *b, TransactionLogger *l)
    : board(b), saveLoadManager(nullptr), logger(l), skillDeck(nullptr),
      currentTurnIdx(0), roundCount(0), diceRolled(false), turnEnded(false) {}

// Destruktor bertanggung jawab menghapus semua pointer Player yang di-alokasi
GameEngine::~GameEngine() {
    for (Player *p : players) {
        delete p;
    }
    players.clear();
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

    int initialMoney = ConfigLoader::getInstance()->getInitialMoney();

    for (const string &name : playerNames) {
        players.push_back(new Player(name, initialMoney));
    }
    
    random_device rd;
    mt19937 g(rd());
    shuffle(players.begin(), players.end(), g);    

    if (logger)
        logger->logEvent(0, "SYSTEM", LogActionType::LOAD, "Game Started");
}

void GameEngine::loadGame(const std::string &filePath) {
    // TODO: Implementasi load game menggunakan saveLoadManager
}

void GameEngine::saveGame(const std::string &filePath) {
    // TODO: Implementasi save game menggunakan saveLoadManager
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
    
    std::cout << "\n[DADU] " << activePlayer->getUsername() << " melempar dadu: " 
              << finalD1 << " + " << finalD2 << " = " << diceTotal << "\n";

    // Cek Status Penjara
    if (activePlayer->getStatus() == PlayerStatus::JAILED) {
        if (finalD1 == finalD2) {
            std::cout << "[PENJARA] Anda mendapat angka ganda (Double)! Anda bebas dari penjara.\n";
            Tile* jailTile = board->getTileAt(board->getJailPosition());
            JailTile* jt = dynamic_cast<JailTile*>(jailTile);
            if (jt) jt->releasePlayer(*activePlayer);
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
        Tile* goTile = board->getTileAt(0);
        GoTile* go = dynamic_cast<GoTile*>(goTile);
        if (go) {
            go->awardSalary(*activePlayer);
            std::cout << "[INFO] Anda melewati petak GO! Menerima gaji sebesar Rp" << go->getSalary() << ".\n";
            if (logger) logger->logEvent(roundCount, activePlayer->getUsername(), LogActionType::GOTILE, "Lewat GO");
        }
    }
    
    // Khusus UtilityTile, simpan nilai dadu terakhir untuk kalkulasi sewa
    Tile *landedTile = board->getTileAt(activePlayer->getPosition());
    if (landedTile) {
        UtilityTile *util = dynamic_cast<UtilityTile*>(landedTile);
        if (util) {
            util->setLastDiceRoll(diceTotal);
        }
    }

    if (finalD1 == finalD2) {
        activePlayer->incrementDoubleStreak();
        // Aturan standar: 3 kali double berturut-turut -> penjara
        if (activePlayer->getDoubleStreak() >= 3) {
            activePlayer->goToJail();
            diceRolled = false; // Memaksa endTurn
        }
    } else {
        // Jika tidak double, kita perlu memastikan bahwa double streak diabaikan saat endTurn.
        // Kita bisa asumsikan Player::resetTurnFlags() akan dihandle di endTurn, 
        // tapi untuk flag kita simpan informasi bahwa roll terakhir bukan double.
        // Karena kita tidak memodifikasi GameEngine.hpp, kita asumsikan getDoubleStreak
        // menandakan jumlah kesempatan roll, bukan hasil roll. 
        // Idealnya jika roll bukan double, streak seharusnya 0.
        // Kita biarkan sesuai implementasi Player (jika tidak ada resetter spesifik, 
        // endTurn akan memanggil resetTurnFlags).
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
            PropertyTile *prop = dynamic_cast<PropertyTile*>(landedTile);
            if (prop) {
                if (activePlayer->getMoney() >= prop->getPrice()) {
                    std::cout << "\n[PILIHAN] " << activePlayer->getUsername() 
                              << " mendarat di " << prop->getName() << ".\n"
                              << "Harga properti: Rp" << prop->getPrice() << "\n"
                              << "Uang Anda saat ini: Rp" << activePlayer->getMoney() << "\n"
                              << "Apakah Anda ingin membeli properti ini? (Y/N): ";
                    char choice;
                    std::cin >> choice;
                    
                    if (choice == 'Y' || choice == 'y') {
                        *activePlayer -= prop->getPrice();
                        prop->setOwnerId(activePlayer->getId());
                        prop->setStatus(1); // 1 = OWNED
                        activePlayer->addProperty(prop);
                        std::cout << "Selamat! Anda berhasil membeli " << prop->getName() << ".\n";
                        if (logger) logger->logEvent(roundCount, activePlayer->getUsername(), LogActionType::BUY, prop->getName());
                    } else {
                        std::cout << "Anda menolak membeli. Properti akan dilelang!\n";
                        std::vector<Player*> activeBidders;
                        for (Player* p : players) {
                            if (p->getStatus() != PlayerStatus::BANKRUPT) activeBidders.push_back(p);
                        }
                        Auction auction(prop, activeBidders);
                        auction.run();
                    }
                } else {
                    std::cout << "\n[INFO] Uang Anda tidak cukup untuk membeli " << prop->getName() << ". Properti otomatis dilelang!\n";
                    std::vector<Player*> activeBidders;
                    for (Player* p : players) {
                        if (p->getStatus() != PlayerStatus::BANKRUPT) activeBidders.push_back(p);
                    }
                    Auction auction(prop, activeBidders);
                    auction.run();
                }
            }
            break;
        }
        case EffectType::AUTO_ACQUIRE: {
            PropertyTile *prop = dynamic_cast<PropertyTile*>(landedTile);
            if (prop && prop->getOwnerId() == -1) {
                prop->setOwnerId(activePlayer->getId());
                prop->setStatus(1);
                activePlayer->addProperty(prop);
                if (logger) logger->logEvent(roundCount, activePlayer->getUsername(), LogActionType::BUY, prop->getName() + " (Auto)");
            }
            break;
        }
        case EffectType::PAY_RENT: {
            PropertyTile *prop = dynamic_cast<PropertyTile*>(landedTile);
            if (prop && prop->getOwnerId() != -1 && prop->getOwnerId() != activePlayer->getId()) {
                // Cari owner
                Player *owner = nullptr;
                for (Player *p : players) {
                    if (p->getId() == prop->getOwnerId()) {
                        owner = p; break;
                    }
                }
                if (owner) {
                    int rentAmount = prop->calcRent();
                    
                    // Cek efek dari skill card (Shield atau Discount)
                    if (activePlayer->isShieldActive()) {
                        activePlayer->setShieldActive(false);
                        rentAmount = 0;
                        if (logger) logger->logEvent(roundCount, activePlayer->getUsername(), LogActionType::CARD, "Shield menahan bayar sewa");
                    } else if (activePlayer->getActiveDiscountPercent() > 0) {
                        rentAmount = rentAmount * (100 - activePlayer->getActiveDiscountPercent()) / 100;
                    }

                    try {
                        *activePlayer -= rentAmount;
                        *owner += rentAmount;
                        if (logger) logger->logEvent(roundCount, activePlayer->getUsername(), LogActionType::RENT, "Ke " + owner->getUsername());
                    } catch (NotEnoughMoneyException &e) {
                        std::cout << "[BANGKRUT] " << activePlayer->getUsername() << " bangkrut karena tidak bisa membayar sewa!\n";
                        handleBankruptcy(activePlayer, owner);
                        if (logger) logger->logEvent(roundCount, activePlayer->getUsername(), LogActionType::BANKRUPT, "Bangkrut sewa ke " + owner->getUsername());
                    }
                }
            }
            break;
        }
        case EffectType::PAY_TAX_FLAT: {
            TaxTile *tax = dynamic_cast<TaxTile*>(landedTile);
            if (tax) {
                try {
                    *activePlayer -= tax->getFlatAmount();
                    if (logger) logger->logEvent(roundCount, activePlayer->getUsername(), LogActionType::TAX, tax->getName());
                } catch (NotEnoughMoneyException &e) {
                    std::cout << "[BANGKRUT] " << activePlayer->getUsername() << " bangkrut karena tidak bisa membayar pajak flat!\n";
                    handleBankruptcy(activePlayer, nullptr);
                }
            }
            break;
        }
        case EffectType::PAY_TAX_CHOICE: {
            TaxTile *tax = dynamic_cast<TaxTile*>(landedTile);
            if (tax) {
                int flat = tax->getFlatAmount();
                int pct = static_cast<int>(tax->computeNetWorth(*activePlayer) * tax->getPercentageRate() / 100.0);
                
                std::cout << "\n[PAJAK] " << activePlayer->getUsername() 
                          << " mendarat di " << tax->getName() << ".\n"
                          << "Silakan pilih metode pembayaran pajak:\n"
                          << "1. Bayar nominal tetap (Rp" << flat << ")\n"
                          << "2. Bayar persentase " << tax->getPercentageRate() << "% dari total kekayaan (Rp" << pct << ")\n"
                          << "Pilihan (1/2): ";
                int choice;
                std::cin >> choice;
                
                int amount = (choice == 1) ? flat : pct;
                try {
                    *activePlayer -= amount;
                    std::cout << "Anda membayar pajak sebesar Rp" << amount << ".\n";
                    if (logger) logger->logEvent(roundCount, activePlayer->getUsername(), LogActionType::TAX, tax->getName());
                } catch (NotEnoughMoneyException &e) {
                    std::cout << "[BANGKRUT] " << activePlayer->getUsername() << " bangkrut karena tidak bisa membayar pajak persentase!\n";
                    handleBankruptcy(activePlayer, nullptr);
                }
            }
            break;
        }
        case EffectType::START_AUCTION: {
            PropertyTile *prop = dynamic_cast<PropertyTile*>(landedTile);
            if (prop && prop->getOwnerId() == -1) {
                std::vector<Player*> activeBidders;
                for (Player* p : players) {
                    if (p->getStatus() != PlayerStatus::BANKRUPT) activeBidders.push_back(p);
                }
                Auction auction(prop, activeBidders);
                auction.run();
            }
            break;
        }
        case EffectType::DRAW_CHANCE:
        case EffectType::DRAW_COMMUNITY: {
            std::cout << "\n[KARTU] " << activePlayer->getUsername() << " mengambil kartu " 
                      << (effect == EffectType::DRAW_CHANCE ? "Kesempatan (Chance)" : "Dana Umum (Community Chest)") << "!\n";
            std::cout << "(Mekanisme penarikan kartu aksi akan disinkronkan sepenuhnya setelah Deck Kartu Action dibuat)\n";
            break;
        }
        case EffectType::FESTIVAL_TRIGGER: {
            std::cout << "\n[FESTIVAL] Anda mendarat di petak Festival!\n"
                      << "Pilih properti yang Anda miliki untuk dilipatgandakan harga sewanya (x2).\n";
            
            auto props = activePlayer->getOwnedProperties();
            if (props.empty()) {
                std::cout << "Sayang sekali, Anda belum memiliki properti satupun untuk diadakan festival.\n";
            } else {
                for (size_t i = 0; i < props.size(); i++) {
                    std::cout << i + 1 << ". " << props[i]->getName() << " (" << props[i]->getKode() << ")\n";
                }
                std::cout << "Masukkan nomor properti (1-" << props.size() << "): ";
                int fChoice;
                std::cin >> fChoice;
                if (fChoice >= 1 && fChoice <= (int)props.size()) {
                    StreetTile* street = dynamic_cast<StreetTile*>(props[fChoice-1]);
                    if (street) {
                        street->setFestivalEffect(2); // Set multiplier
                        std::cout << "Festival berhasil diadakan di " << street->getName() << "! Harga sewa naik 2x lipat.\n";
                    } else {
                        std::cout << "Festival hanya bisa diadakan di properti jalan (Street).\n";
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

void GameEngine::handleBankruptcy(Player* bankruptPlayer, Player* creditor) {
    bankruptPlayer->declareBankruptcy();
    auto properties = bankruptPlayer->getOwnedProperties();
    
    for (PropertyTile* prop : properties) {
        if (creditor) {
            prop->setOwnerId(creditor->getId());
            creditor->addProperty(prop);
        } else {
            prop->setOwnerId(-1);
            prop->setStatus(0); // Kembali ke BANK
            StreetTile* street = dynamic_cast<StreetTile*>(prop);
            if (street && street->hasBuildings()) {
                street->demolish(); 
            }
        }
    }
    
    std::cout << "Semua aset milik " << bankruptPlayer->getUsername() << " telah " 
              << (creditor ? "diserahkan ke " + creditor->getUsername() : "disita oleh Bank") << ".\n";
}

void GameEngine::sellBuilding(const std::string& colorGroup){
    vector<Tile*> curr = board->getTileByColorGroup(colorGroup);
    Player* currentPlayer = players[currentTurnIdx];
    cout << "Daftar bangunan di Color Group [" << colorGroup << "]" << endl;
    int i = 1;
    for(auto v : curr){
        int level = v->getRentLevel();
        cout << i << ". " << v->getName() << "(" << v->getKode() << ") - ";
        if(level <= 4) cout << level << " rumah";
        else cout << "Hotel";
        cout << " -> Nilai jual bangunan: " << v->calcValue();
        cout << endl;
    }
    for(auto v : curr){
        v->demolish();
        // TODO : handle perpindahan duit
        players[v->getOwnerId()] += v->calcValue();
    }
}

void GameEngine::buyBuilding(const std::string& propertyCode) {
    Player* activePlayer = players[currentTurnIdx];
    Tile* tile = board->getTileByKode(propertyCode);
    StreetTile* street = dynamic_cast<StreetTile*>(tile);
    
    if (!street || street->getOwnerId() != activePlayer->getId()) {
        std::cout << "Properti tidak valid atau bukan milik Anda.\n";
        return;
    }
    
    try {
        if (street->getRentLevel() < 4) { // Max 4 rumah
            *activePlayer -= street->getHouseCost();
            street->buildHouse();
            std::cout << "Berhasil membangun rumah di " << street->getName() << "!\n";
        } else if (street->getRentLevel() == 4) { // Level 4 berarti sudah 4 rumah, bisa bangun hotel
            *activePlayer -= street->getHotelCost();
            street->buildHotel();
            std::cout << "Berhasil membangun hotel di " << street->getName() << "!\n";
        } else {
            std::cout << "Properti sudah mencapai level bangunan maksimal!\n";
        }
    } catch (NotEnoughMoneyException& e) {
        std::cout << "Uang Anda tidak cukup untuk membangun di sini.\n";
    }
}

void GameEngine::mortgageProperty(const std::string& propertyCode) {
    Player* activePlayer = players[currentTurnIdx];
    Tile* tile = board->getTileByKode(propertyCode);
    PropertyTile* prop = dynamic_cast<PropertyTile*>(tile);
    
    if (!prop || prop->getOwnerId() != activePlayer->getId()) {
        std::cout << "Properti tidak valid atau bukan milik Anda.\n";
        return;
    }
    
    if (prop->getStatus() == 2) { // 2 = MORTGAGED
        std::cout << "Properti ini sudah dalam status tergadai.\n";
        return;
    }
    
    prop->mortgage(); 
    *activePlayer += prop->getmortgageValue();
    std::cout << prop->getName() << " berhasil digadaikan. Anda menerima Rp" << prop->getmortgageValue() << ".\n";
}

void GameEngine::unmortgageProperty(const std::string& propertyCode) {
    Player* activePlayer = players[currentTurnIdx];
    Tile* tile = board->getTileByKode(propertyCode);
    PropertyTile* prop = dynamic_cast<PropertyTile*>(tile);
    
    if (!prop || prop->getOwnerId() != activePlayer->getId()) {
        std::cout << "Properti tidak valid atau bukan milik Anda.\n";
        return;
    }
    
    if (prop->getStatus() != 2) {
        std::cout << "Properti ini tidak sedang digadaikan.\n";
        return;
    }
    
    int cost = prop->getmortgageValue() + (prop->getmortgageValue() * 10 / 100); // Bunga 10%
    try {
        *activePlayer -= cost;
        prop->unmortgage();
        std::cout << prop->getName() << " berhasil ditebus dengan biaya Rp" << cost << ".\n";
    } catch (NotEnoughMoneyException& e) {
        std::cout << "Uang tidak cukup untuk menebus properti (butuh Rp" << cost << ").\n";
    }
}

void GameEngine::useSkillCard(int cardIdx) {
    Player* activePlayer = players[currentTurnIdx];
    auto cards = activePlayer->getHandCards();
    if (cardIdx < 0 || cardIdx >= (int)cards.size()) {
        std::cout << "Index kartu tidak valid.\n";
        return;
    }
    
    SkillCard* card = cards[cardIdx];
    card->use(*activePlayer, *this); // Efek kartu diaplikasikan
    activePlayer->removeCard(cardIdx);
    std::cout << "Anda menggunakan kartu: " << card->getCardName() << "!\n";
}

void GameEngine::endTurn() {
    if (!diceRolled) {
        throw InvalidCommandException(
            "Anda harus melempar dadu sebelum mengakhir giliran.");
    }

    Player *activePlayer = players[currentTurnIdx];

    if (activePlayer->getDoubleStreak() > 0 &&
        activePlayer->getStatus() != PlayerStatus::JAILED) {
        diceRolled = false;
        turnEnded = false;

        if (logger)
            logger->logEvent(roundCount, activePlayer->getUsername(),
                             LogActionType::DOUBLE_ROLL,
                             "Player mendapatkan giliran tambahan!");
    } else {
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
