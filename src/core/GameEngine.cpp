#include "../../include/core/ConfigLoader.hpp"
#include "../../include/core/Exceptions.hpp"
#include "../../include/core/GameEngine.hpp"
#include "../../include/core/Player.hpp"
#include "../../include/core/TransactionLogger.hpp"
#include "../../include/models/Board.hpp"
#include "../../include/models/Card.hpp"
#include "../../include/models/PropertyTile.hpp"
#include "../../include/models/Tile.hpp"

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

    // Gerakkan pemain berdasarkan total dadu
    activePlayer->move(diceTotal);
    diceRolled = true;
}

void GameEngine::executeTileAction() {
    if (!diceRolled) {
        throw InvalidCommandException("Lempar dadu terlebih dahulu");
    }

    Player *activePlayer = players[currentTurnIdx];
    Tile *landedTile = board->getTileAt(activePlayer->getPosition());
    if (landedTile) {
        EffectType effect = landedTile->onLanded(*activePlayer);

        // TODO: IMPLEMENTASI executeTileAction, ini banyak banget CUOKK
        switch (effect) {
        case EffectType::OFFER_BUY:
            // Implementasi interaksi UI prompt pembelian, jika ditolak ->
            // lelang
            break;
        case EffectType::PAY_RENT:
        case EffectType::PAY_TAX_FLAT:
            // Cek status kebangkrutan dll.
            break;
        default:
            break;
        }
    }
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
