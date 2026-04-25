#include "../include/models/Board.hpp"
#include "../include/models/Tile.hpp"
#include <iostream>

Board *Board::instance = nullptr;

Board::Board() : jailPositionIndex(-1) {}

Board::~Board() {
    for (Tile *tile : tiles) {
        delete tile;
    }
    tiles.clear();
    tileMap.clear();

    instance = nullptr;
}

Board *Board::getInstance() {
    if (instance == nullptr) {
        instance = new Board();
    }
    return instance;
}

int Board::getTotalTiles() const { return static_cast<int>(tiles.size()); }

Tile *Board::getTileAt(int index) {
    if (index < 1 || index > static_cast<int>(tiles.size())) {
        return nullptr;
    }
    return tiles[index - 1];
}

Tile *Board::getTileByKode(const std::string &kode) {
    auto it = tileMap.find(kode);
    if (it == tileMap.end()) {
        return nullptr;
    }
    return getTileAt(it->second);
}

std::vector<Tile *> Board::getTileByColorGroup(const std::string &colorGroup) {
    std::vector<Tile *> colorGroupTiles;
    for (Tile *tile : tiles) {
        if (tile->getColorGroup() == colorGroup) {
            colorGroupTiles.push_back(tile);
        }
    }
    return colorGroupTiles;
}

int Board::getJailPosition() const { return jailPositionIndex; }

void Board::resetBoard() {
    for (Tile *tile : tiles) {
        delete tile;
    }
    tiles.clear();
    tileMap.clear();
    jailPositionIndex = -1;
}

void Board::printBoardStatus() {
    cout << "=== Board Status Debug ===" << std::endl;
    cout << "Total tiles: " << getTotalTiles() << std::endl;
    cout << "Jail position: " << jailPositionIndex << std::endl;
    cout << std::endl;

    for (int i = 1; i <= getTotalTiles(); i++) {
        Tile *tile = getTileAt(i);
        if (tile != nullptr) {
            std::cout << "[" << i << "] " << tile->getKode() << " - "
                      << tile->getName() << std::endl;
        } else {
            std::cout << "[" << i << "] KOSONG (nullptr)" << std::endl;
        }
    }
}