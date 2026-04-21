#pragma once
#include <vector>
#include <string>
#include <map>
using namespace std;

class Tile;
class ConfigLoader;

class Board {
private:
    static Board* instance;
    vector<Tile*> tiles;
    map<std::string, int> tileMap;
    int jailPositionIndex;
    Board();

public:
    friend class ConfigLoader;

    static Board* getInstance();
    ~Board();
    int getTotalTiles() const;
    Tile* getTileAt(int index);
    Tile* getTileByKode(const string& kode);
    int getJailPosition() const;

    void printBoardStatus();
    void resetBoard();
    Tile* getTileAt(int index); 
    Tile* getTileByKode(const std::string& kode);
    int getJailPosition() const;
};