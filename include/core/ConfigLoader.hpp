#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include "Tile.hpp"
using namespace std;

class ConfigLoader {
private:
    static ConfigLoader* instance;
    ConfigLoader();

    ifstream currentFile;
    string currentLine;
    int currentParsedIndex;
    string configFilePath;
    bool isConfigValid;

    map<int, int> railroadRentMap;
    map<int, int> utilityMultiplierMap;
    int taxPPHFlat;
    double taxPPHPercent;
    int taxPBMFlat;
    int goSalary;
    int jailFine;

    int maxTurn;
    int initialMoney;

    class TileDefinition {
    public:
        int index;
        string kode;
        string nama;
        string tipeStr;

        TileDefinition(int index, const string& kode,
                       const string& nama, const string& tipeStr);

        int getIndex() const;
        string getKode() const;
        string getNama() const;
        string getTipeStr() const;
    };
    vector<TileDefinition> layoutDefinitions;

    void readRailroadConfig();
    void readUtilityConfig();
    void readTaxConfig();
    void readSpecialConfig();
    void readMiscConfig();

    void buildLayoutFromHardcode();
    void buildLayoutFromFile();

    Tile* instantiateTile(const TileDefinition& def);
    void linkSpecialTiles();

public:
    static ConfigLoader* getInstance();
    ~ConfigLoader();

    void loadAllConfigs();
    void parsePropertyConfig();
    void parseActionTileConfig();
    void validateBoardLayout();
    void setConfigFilePath(const string& path);
    bool getIsConfigValid() const;

    // Getter untuk GameEngine setelah loadAllConfigs() selesai
    int getMaxTurn() const;
    int getInitialMoney() const;
};