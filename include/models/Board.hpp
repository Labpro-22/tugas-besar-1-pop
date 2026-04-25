#pragma once
#include <map>
#include <string>
#include <vector>
using namespace std;

class Tile;
class ConfigLoader;

class Board {
  private:
    static Board *instance;
    vector<Tile *> tiles;
    map<std::string, int> tileMap;
    int jailPositionIndex;
    Board();

  public:
    friend class ConfigLoader;

    static Board *getInstance();
    ~Board();
    int getTotalTiles() const;
    Tile *getTileAt(int index);
    Tile *getTileByKode(const string &kode);
    vector<Tile *> getTileByColorGroup(const string &colorGroup);
    int getJailPosition() const;

    void printBoardStatus();
    void resetBoard();
};
