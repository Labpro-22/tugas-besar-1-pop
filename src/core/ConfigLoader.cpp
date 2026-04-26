#include "../../include/core/ConfigLoader.hpp"
#include "../../include/models/ActionTile.hpp"
#include "../../include/models/Board.hpp"
#include "../../include/models/PropertyTile.hpp"
#include "../../include/models/Tile.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace std;

ConfigLoader *ConfigLoader::instance = nullptr;

ConfigLoader::TileDefinition::TileDefinition(int index, const string &kode,
                                             const string &nama,
                                             const string &tipeStr)
    : index(index), kode(kode), nama(nama), tipeStr(tipeStr) {}

int ConfigLoader::TileDefinition::getIndex() const { return index; }
string ConfigLoader::TileDefinition::getKode() const { return kode; }
string ConfigLoader::TileDefinition::getNama() const { return nama; }
string ConfigLoader::TileDefinition::getTipeStr() const { return tipeStr; }

ConfigLoader::ConfigLoader()
    : currentParsedIndex(0), configFilePath(""), isConfigValid(false),
      taxPPHFlat(0), taxPPHPercent(0.0), taxPBMFlat(0), goSalary(0),
      jailFine(0), maxTurn(0), initialMoney(0) {}

ConfigLoader::~ConfigLoader() {
    if (currentFile.is_open()) {
        currentFile.close();
    }
    instance = nullptr;
}

ConfigLoader *ConfigLoader::getInstance() {
    if (instance == nullptr) {
        instance = new ConfigLoader();
    }
    return instance;
}

void ConfigLoader::setConfigFilePath(const string &path) {
    configFilePath = path;
}

bool ConfigLoader::getIsConfigValid() const { return isConfigValid; }

int ConfigLoader::getMaxTurn() const { return maxTurn; }

int ConfigLoader::getInitialMoney() const { return initialMoney; }

void ConfigLoader::readRailroadConfig() {
    string filePath = configFilePath + "/railroad.txt";
    currentFile.open(filePath);

    if (!currentFile.is_open()) {
        throw runtime_error("Gagal membuka file: " + filePath);
    }

    getline(currentFile, currentLine);

    int jumlah, biaya;
    while (currentFile >> jumlah >> biaya) {
        railroadRentMap[jumlah] = biaya;
    }

    currentFile.close();
    currentFile.clear();
}

void ConfigLoader::readUtilityConfig() {
    string filePath = configFilePath + "/utility.txt";
    currentFile.open(filePath);

    if (!currentFile.is_open()) {
        throw runtime_error("Gagal membuka file: " + filePath);
    }

    getline(currentFile, currentLine); // skip header

    int jumlah, pengali;
    while (currentFile >> jumlah >> pengali) {
        utilityMultiplierMap[jumlah] = pengali;
    }

    currentFile.close();
    currentFile.clear();
}

void ConfigLoader::readTaxConfig() {
    string filePath = configFilePath + "/tax.txt";
    currentFile.open(filePath);

    if (!currentFile.is_open()) {
        throw runtime_error("Gagal membuka file: " + filePath);
    }

    getline(currentFile, currentLine); // skip header
    currentFile >> taxPPHFlat >> taxPPHPercent >> taxPBMFlat;

    currentFile.close();
    currentFile.clear();
}

void ConfigLoader::readSpecialConfig() {
    string filePath = configFilePath + "/special.txt";
    currentFile.open(filePath);

    if (!currentFile.is_open()) {
        throw runtime_error("Gagal membuka file: " + filePath);
    }

    getline(currentFile, currentLine); // skip header
    currentFile >> goSalary >> jailFine;

    currentFile.close();
    currentFile.clear();
}

void ConfigLoader::readMiscConfig() {
    string filePath = configFilePath + "/misc.txt";
    currentFile.open(filePath);

    if (!currentFile.is_open()) {
        throw runtime_error("Gagal membuka file: " + filePath);
    }

    getline(currentFile, currentLine); // skip header
    currentFile >> maxTurn >> initialMoney;

    currentFile.close();
    currentFile.clear();
}

void ConfigLoader::buildLayoutFromHardcode() {
    layoutDefinitions.clear();
    layoutDefinitions.push_back(
        TileDefinition(1, "GO", "Petak Mulai", "SPECIAL_GO"));
    layoutDefinitions.push_back(TileDefinition(2, "GRT", "Garut", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(3, "DNU", "Dana Umum", "CARD_COMMUNITY"));
    layoutDefinitions.push_back(
        TileDefinition(4, "TSK", "Tasikmalaya", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(5, "PPH", "Pajak Penghasilan", "TAX_PPH"));
    layoutDefinitions.push_back(
        TileDefinition(6, "GBR", "Stasiun Gambir", "RAILROAD"));
    layoutDefinitions.push_back(TileDefinition(7, "BGR", "Bogor", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(8, "FES", "Festival", "FESTIVAL"));
    layoutDefinitions.push_back(TileDefinition(9, "DPK", "Depok", "STREET"));
    layoutDefinitions.push_back(TileDefinition(10, "BKS", "Bekasi", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(11, "PEN", "Penjara", "SPECIAL_JAIL"));
    layoutDefinitions.push_back(
        TileDefinition(12, "MGL", "Magelang", "STREET"));
    layoutDefinitions.push_back(TileDefinition(13, "PLN", "PLN", "UTILITY"));
    layoutDefinitions.push_back(TileDefinition(14, "SOL", "Solo", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(15, "YOG", "Yogyakarta", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(16, "STB", "Stasiun Bandung", "RAILROAD"));
    layoutDefinitions.push_back(TileDefinition(17, "MAL", "Malang", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(18, "DNU", "Dana Umum", "CARD_COMMUNITY"));
    layoutDefinitions.push_back(
        TileDefinition(19, "SMG", "Semarang", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(20, "SBY", "Surabaya", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(21, "BBP", "Bebas Parkir", "SPECIAL_FREEPARKING"));
    layoutDefinitions.push_back(
        TileDefinition(22, "MKS", "Makassar", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(23, "KSP", "Kesempatan", "CARD_CHANCE"));
    layoutDefinitions.push_back(
        TileDefinition(24, "BLP", "Balikpapan", "STREET"));
    layoutDefinitions.push_back(TileDefinition(25, "MND", "Manado", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(26, "TUG", "Stasiun Tugu", "RAILROAD"));
    layoutDefinitions.push_back(
        TileDefinition(27, "PLB", "Palembang", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(28, "PKB", "Pekanbaru", "STREET"));
    layoutDefinitions.push_back(TileDefinition(29, "PAM", "PAM", "UTILITY"));
    layoutDefinitions.push_back(TileDefinition(30, "MED", "Medan", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(31, "PPJ", "Pergi ke Penjara", "SPECIAL_GOTOJAIL"));
    layoutDefinitions.push_back(TileDefinition(32, "BDG", "Bandung", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(33, "DEN", "Denpasar", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(34, "FES", "Festival", "FESTIVAL"));
    layoutDefinitions.push_back(TileDefinition(35, "MTR", "Mataram", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(36, "GUB", "Stasiun Gubeng", "RAILROAD"));
    layoutDefinitions.push_back(
        TileDefinition(37, "KSP", "Kesempatan", "CARD_CHANCE"));
    layoutDefinitions.push_back(TileDefinition(38, "JKT", "Jakarta", "STREET"));
    layoutDefinitions.push_back(
        TileDefinition(39, "PBM", "Pajak Barang Mewah", "TAX_PBM"));
    layoutDefinitions.push_back(
        TileDefinition(40, "IKN", "Ibu Kota Nusantara", "STREET"));
}

void ConfigLoader::buildLayoutFromFile() {
    // TODO: implementasi bonus Papan Dinamis
    throw runtime_error("buildLayoutFromFile() belum diimplementasi.");
}

Tile *ConfigLoader::instantiateTile(const TileDefinition &def) {
    const string t = def.getTipeStr();

    if (t == "CARD_CHANCE") {
        return new CardTile(def.getIndex(), def.getKode(), def.getNama(),
                            "CHANCE");
    }
    if (t == "CARD_COMMUNITY") {
        return new CardTile(def.getIndex(), def.getKode(), def.getNama(),
                            "COMMUNITY_CHEST");
    }
    if (t == "TAX_PPH") {
        return new TaxTile(def.getIndex(), def.getKode(), def.getNama(), "PPH",
                           taxPPHFlat, taxPPHPercent);
    }
    if (t == "TAX_PBM") {
        return new TaxTile(def.getIndex(), def.getKode(), def.getNama(), "PBM",
                           taxPBMFlat);
    }
    if (t == "FESTIVAL") {
        return new FestivalTile(def.getIndex(), def.getKode(), def.getNama());
    }
    if (t == "SPECIAL_GO") {
        return new GoTile(def.getIndex(), def.getKode(), def.getNama(),
                          goSalary);
    }
    if (t == "SPECIAL_JAIL") {
        return new JailTile(def.getIndex(), def.getKode(), def.getNama(),
                            jailFine);
    }
    if (t == "SPECIAL_FREEPARKING") {
        return new FreeParkingTile(def.getIndex(), def.getKode(),
                                   def.getNama());
    }
    if (t == "SPECIAL_GOTOJAIL") {
        return new GoToJailTile(def.getIndex(), def.getKode(), def.getNama());
    }

    return nullptr;
}

void ConfigLoader::linkSpecialTiles() {
    Board *board = Board::getInstance();

    JailTile *jailTile = nullptr;
    GoToJailTile *goToJailTile = nullptr;

    for (int i = 1; i <= board->getTotalTiles(); i++) {
        Tile *tile = board->getTileAt(i);
        if (tile == nullptr)
            continue;

        if (jailTile == nullptr && tile->isJailTile())
            jailTile = static_cast<JailTile *>(tile);

        if (goToJailTile == nullptr && tile->isGoToJailTile())
            goToJailTile = static_cast<GoToJailTile *>(tile);

        if (jailTile != nullptr && goToJailTile != nullptr)
            break;
    }

    if (jailTile == nullptr) {
        throw runtime_error(
            "validateBoardLayout: Tidak ditemukan JailTile di Board.");
    }
    if (goToJailTile == nullptr) {
        throw runtime_error(
            "validateBoardLayout: Tidak ditemukan GoToJailTile di Board.");
    }

    goToJailTile->setJailTile(jailTile);
}

void ConfigLoader::parsePropertyConfig() {
    string filePath = configFilePath + "/property.txt";
    currentFile.open(filePath);

    if (!currentFile.is_open()) {
        throw runtime_error("Gagal membuka file: " + filePath);
    }

    Board *board = Board::getInstance();

    getline(currentFile, currentLine); // skip header

    while (getline(currentFile, currentLine)) {
        if (currentLine.empty())
            continue;

        istringstream ss(currentLine);

        int id;
        string kode, nama, jenis, warna;
        int hargaLahan, nilaiGadai;

        ss >> id >> kode >> nama >> jenis >> warna >> hargaLahan >> nilaiGadai;

        Tile *newTile = nullptr;

        if (jenis == "STREET") {
            int upgRumah, upgHotel;
            ss >> upgRumah >> upgHotel;

            vector<int> rentTable;
            int rent;
            while (ss >> rent) {
                rentTable.push_back(rent);
            }

            newTile = new StreetTile(id, kode, nama, warna, hargaLahan,
                                     nilaiGadai, rentTable, upgRumah, upgHotel);
        } else if (jenis == "RAILROAD") {
            newTile =
                new RailroadTile(id, kode, nama, nilaiGadai, railroadRentMap);
        } else if (jenis == "UTILITY") {
            newTile = new UtilityTile(id, kode, nama, nilaiGadai,
                                      utilityMultiplierMap);
        } else {
            // Jenis tidak dikenali — skip dengan pesan peringatan
            cerr << "PERINGATAN: Jenis properti tidak dikenali: " << jenis
                 << " (kode: " << kode << ")" << endl;
            continue;
        }

        if (id < 1) {
            delete newTile;
            throw runtime_error("ID properti tidak valid: " + to_string(id));
        }

        if (static_cast<int>(board->tiles.size()) < id) {
            board->tiles.resize(id, nullptr);
        }

        board->tiles[id - 1] = newTile;
        board->tileMap[kode] = id;
    }

    currentFile.close();
    currentFile.clear();
}

void ConfigLoader::parseActionTileConfig() {
    Board *board = Board::getInstance();

    int totalTiles = static_cast<int>(layoutDefinitions.size());
    if (static_cast<int>(board->tiles.size()) < totalTiles) {
        board->tiles.resize(totalTiles, nullptr);
    }

    for (const TileDefinition &def : layoutDefinitions) {
        const string t = def.getTipeStr();
        if (t == "STREET" || t == "RAILROAD" || t == "UTILITY") {
            continue;
        }

        Tile *newTile = instantiateTile(def);

        if (newTile == nullptr) {
            throw runtime_error("Gagal menginstansiasi tile: " + def.getKode() +
                                " tipe: " + def.getTipeStr());
        }

        board->tiles[def.getIndex() - 1] = newTile;

        if (board->tileMap.find(def.getKode()) == board->tileMap.end()) {
            board->tileMap[def.getKode()] = def.getIndex();
        }

        if (def.getTipeStr() == "SPECIAL_JAIL") {
            board->jailPositionIndex = def.getIndex();
        }
    }
}

void ConfigLoader::validateBoardLayout() {
    Board *board = Board::getInstance();
    int total = board->getTotalTiles();

    if (total == 0) {
        throw runtime_error(
            "validateBoardLayout: Board kosong, tidak ada tile.");
    }

    // Cek tidak ada slot nullptr
    for (int i = 1; i <= total; i++) {
        if (board->getTileAt(i) == nullptr) {
            throw runtime_error("validateBoardLayout: Slot kosong di indeks " +
                                to_string(i));
        }
    }

    // Cek jailPositionIndex sudah diset
    if (board->getJailPosition() == -1) {
        throw runtime_error(
            "validateBoardLayout: JailTile tidak ditemukan di Board.");
    }

    isConfigValid = true;
}

void ConfigLoader::loadAllConfigs() {
    if (configFilePath.empty()) {
        throw runtime_error("loadAllConfigs: configFilePath belum diset. "
                            "Panggil setConfigFilePath() terlebih dahulu.");
    }

    isConfigValid = false;

    readRailroadConfig();
    readUtilityConfig();
    readTaxConfig();
    readSpecialConfig();
    readMiscConfig();
    buildLayoutFromHardcode();
    parsePropertyConfig();
    parseActionTileConfig();
    linkSpecialTiles();
    validateBoardLayout();
}
