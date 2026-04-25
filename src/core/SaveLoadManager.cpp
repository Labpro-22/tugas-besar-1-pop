#include "../../include/core/SaveLoadManager.hpp"
#include "../../include/core/Enums.hpp"
#include "../../include/core/GameEngine.hpp"
#include "../../include/core/Player.hpp"
#include "../../include/core/TransactionLogger.hpp"
#include "../../include/models/Board.hpp"
#include "../../include/models/Card.hpp"
#include "../../include/models/PropertyTile.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

// PASTI MASIH FAIL SOALNYA BELOM ADA GETTER SETTER DARI YANG LAIN
bool SaveLoadManager::fileExists(const std::string &fileName) {
    std::ifstream f(fileName);
    return f.good();
}

bool SaveLoadManager::saveGame(const std::string &fileName,
                               const GameEngine &engine) {
    std::ofstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Gagal membuka file untuk disimpan: " << fileName << "\n";
        return false;
    }

    file << engine.getCurrentTurn() << " " << engine.getMaxTurn() << "\n";

    savePlayers(file, engine);
    saveProperties(file, engine);
    saveDeck(file, engine);
    saveLogs(file, engine);

    file.close();
    return true;
}

bool SaveLoadManager::loadGame(const std::string &fileName,
                               GameEngine &engine) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "File tidak ditemukan: " << fileName << "\n";
        return false;
    }

    int currentTurn, maxTurn;
    if (!(file >> currentTurn >> maxTurn)) {
        std::cerr << "Format file rusak (baris pertama).\n";
        return false;
    }
    engine.setCurrentTurn(currentTurn);
    engine.setMaxTurn(maxTurn);

    bool ok = true;
    ok = ok && loadPlayers(file, engine);
    ok = ok && loadProperties(file, engine);
    ok = ok && loadDeck(file, engine);
    ok = ok && loadLogs(file, engine);

    file.close();
    return ok;
}

void SaveLoadManager::savePlayers(std::ofstream &file,
                                  const GameEngine &engine) {
    const auto &players = engine.getPlayers();

    file << players.size() << "\n";

    for (const Player *p : players) {
        std::string statusStr;
        switch (p->getStatus()) {
        case PlayerStatus::ACTIVE:
            statusStr = "ACTIVE";
            break;
        case PlayerStatus::JAILED:
            statusStr = "JAILED";
            break;
        case PlayerStatus::BANKRUPT:
            statusStr = "BANKRUPT";
            break;
        }
        file << p->getUsername() << " " << p->getMoney() << " "
             << p->getPosition() << " " << statusStr << "\n";

        const auto &cards = p->getHandCards();
        file << cards.size() << "\n";

        for (const SkillCard *card : cards) {
            std::string val = card->getValueString();
            file << card->getCardName();
            if (!val.empty())
                file << " " << val;
            file << "\n";
        }
    }

    const auto &order = engine.getTurnOrder();
    for (int i = 0; i < (int)order.size(); i++) {
        if (i > 0)
            file << " ";
        file << order[i];
    }
    file << "\n";

    file << engine.getActivePlayerUsername() << "\n";
}

void SaveLoadManager::saveProperties(std::ofstream &file,
                                     const GameEngine &engine) {
    const Board *board = engine.getBoard();
    if (!board)
        return;

    std::vector<const PropertyTile *> props = board->getAllProperties();

    file << props.size() << "\n";

    for (const PropertyTile *prop : props) {
        std::string jenis;
        std::string nBangunan = "0";
        int fmult = 1;
        int fdur = 0;

        const StreetTile *street = dynamic_cast<const StreetTile *>(prop);
        const RailroadTile *rr = dynamic_cast<const RailroadTile *>(prop);
        const UtilityTile *util = dynamic_cast<const UtilityTile *>(prop);

        if (street) {
            jenis = "street";
            fmult = street->getFestivalMultiplier();
            fdur = street->getFestivalDuration();
            if (street->hasHotel()) {
                nBangunan = "H";
            } else {
                nBangunan = std::to_string(street->getHouseCount());
            }
        } else if (rr) {
            jenis = "railroad";
        } else if (util) {
            jenis = "utility";
        } else {
            jenis = "unknown";
        }

        std::string statusStr;
        switch (prop->getPropertyStatus()) {
        case PropertyStatus::BANK:
            statusStr = "BANK";
            break;
        case PropertyStatus::OWNED:
            statusStr = "OWNED";
            break;
        case PropertyStatus::MORTGAGED:
            statusStr = "MORTGAGED";
            break;
        }

        std::string ownerStr = "BANK";
        const Player *owner = prop->getOwner();
        if (owner)
            ownerStr = owner->getUsername();

        file << prop->getKode() << " " << jenis << " " << ownerStr << " "
             << statusStr << " " << fmult << " " << fdur << " " << nBangunan
             << "\n";
    }
}

void SaveLoadManager::saveDeck(std::ofstream &file, const GameEngine &engine) {
    const auto &deckCards = engine.getSkillDeckCards();

    file << deckCards.size() << "\n";
    for (const SkillCard *card : deckCards) {
        file << card->getCardName() << "\n";
    }
}

void SaveLoadManager::saveLogs(std::ofstream &file, const GameEngine &engine) {
    const TransactionLogger *logger = engine.getLogger();
    if (!logger) {
        file << "0\n";
        return;
    }

    file << logger->serialize();
}

bool SaveLoadManager::loadPlayers(std::ifstream &file, GameEngine &engine) {
    int jumlahPemain;
    if (!(file >> jumlahPemain))
        return false;

    for (int i = 0; i < jumlahPemain; i++) {
        std::string username, statusStr;
        int money, position;

        if (!(file >> username >> money >> position >> statusStr))
            return false;

        Player *p = engine.getPlayerByUsername(username);
        if (!p) {
            continue;
        }
        p->setMoney(money);
        p->setPosition(position);

        if (statusStr == "ACTIVE")
            p->setStatus(PlayerStatus::ACTIVE);
        else if (statusStr == "JAILED")
            p->setStatus(PlayerStatus::JAILED);
        else if (statusStr == "BANKRUPT")
            p->setStatus(PlayerStatus::BANKRUPT);

        int jumlahKartu;
        if (!(file >> jumlahKartu))
            return false;

        for (int j = 0; j < jumlahKartu; j++) {
            std::string line;
            if (j == 0)
                std::getline(file, line);
            std::getline(file, line);

            std::istringstream iss(line);
            std::string cardName;
            iss >> cardName;

            SkillCard *card = nullptr;

            if (cardName == "MoveCard") {
                int steps = 0;
                iss >> steps;
                card = new MoveCard(steps);
            } else if (cardName == "DiscountCard") {
                int pct = 0, dur = 1;
                iss >> pct >> dur;
                card = new DiscountCard(pct, dur);
            } else if (cardName == "ShieldCard") {
                card = new ShieldCard();
            } else if (cardName == "TeleportCard") {
                card = new TeleportCard();
            } else if (cardName == "LassoCard") {
                card = new LassoCard();
            } else if (cardName == "DemolitionCard") {
                card = new DemolitionCard();
            }

            if (card)
                p->addCard(card);
        }
    }

    std::string orderLine;
    std::getline(file >> std::ws, orderLine);
    std::istringstream orderStream(orderLine);
    std::vector<std::string> order;
    std::string uname;
    while (orderStream >> uname)
        order.push_back(uname);
    engine.setTurnOrder(order);

    std::string activeUsername;
    if (!(file >> activeUsername))
        return false;
    engine.setActivePlayer(activeUsername);

    return true;
}

bool SaveLoadManager::loadProperties(std::ifstream &file, GameEngine &engine) {
    int jumlahProperti;
    if (!(file >> jumlahProperti))
        return false;

    Board *board = engine.getBoard();
    if (!board)
        return false;

    for (int i = 0; i < jumlahProperti; i++) {
        std::string kode, jenis, ownerStr, statusStr, nBangunanStr;
        int fmult, fdur;

        if (!(file >> kode >> jenis >> ownerStr >> statusStr >> fmult >> fdur >>
              nBangunanStr))
            return false;

        Tile *tile = board->getTileByKode(kode);
        if (!tile)
            continue;

        PropertyTile *prop = dynamic_cast<PropertyTile *>(tile);
        if (!prop)
            continue;

        PropertyStatus status;
        if (statusStr == "BANK")
            status = PropertyStatus::BANK;
        else if (statusStr == "OWNED")
            status = PropertyStatus::OWNED;
        else if (statusStr == "MORTGAGED")
            status = PropertyStatus::MORTGAGED;
        else
            status = PropertyStatus::BANK;
        prop->setPropertyStatus(status);

        if (ownerStr != "BANK") {
            Player *owner = engine.getPlayerByUsername(ownerStr);
            if (owner)
                prop->setOwner(owner);
        } else {
            prop->setOwner(nullptr);
        }

        StreetTile *street = dynamic_cast<StreetTile *>(prop);
        if (street) {
            street->setFestivalMultiplier(fmult);
            street->setFestivalDuration(fdur);

            if (nBangunanStr == "H") {
                street->setHotel(true);
                street->setHouseCount(0);
            } else {
                street->setHotel(false);
                street->setHouseCount(std::stoi(nBangunanStr));
            }
        }
    }

    return true;
}

bool SaveLoadManager::loadDeck(std::ifstream &file, GameEngine &engine) {
    int jumlahKartu;
    if (!(file >> jumlahKartu))
        return false;

    std::vector<SkillCard *> cards;
    for (int i = 0; i < jumlahKartu; i++) {
        std::string cardName;
        if (!(file >> cardName))
            return false;

        SkillCard *card = nullptr;
        if (cardName == "MoveCard")
            card = new MoveCard();
        else if (cardName == "DiscountCard")
            card = new DiscountCard();
        else if (cardName == "ShieldCard")
            card = new ShieldCard();
        else if (cardName == "TeleportCard")
            card = new TeleportCard();
        else if (cardName == "LassoCard")
            card = new LassoCard();
        else if (cardName == "DemolitionCard")
            card = new DemolitionCard();

        if (card)
            cards.push_back(card);
    }

    engine.setSkillDeckCards(cards);
    return true;
}

bool SaveLoadManager::loadLogs(std::ifstream &file, GameEngine &engine) {
    TransactionLogger *logger = engine.getLogger();
    if (!logger)
        return false;

    std::ostringstream oss;
    oss << file.rdbuf();
    logger->deserialize(oss.str());

    return true;
}
