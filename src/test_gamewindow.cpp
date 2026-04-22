#include "../include/views/GameWindow.hpp"
#include <iostream>
#include <vector>

GameState createDummyGameState() {
  GameState state;

  PlayerInfo p1;
  p1.username = "Player1";
  p1.money = 2000;
  p1.position = 1;
  p1.status = "ACTIVE";
  p1.jailTurnsLeft = 0;
  p1.shieldActive = false;
  p1.color = {217, 38, 38, 255};

  CardInfo card1;
  card1.name = "MoveCard";
  card1.description = "Maju 5 petak";
  card1.value = "5";
  p1.handCards.push_back(card1);

  CardInfo card2;
  card2.name = "DiscountCard";
  card2.description = "Diskon 30%";
  card2.value = "30 1";
  p1.handCards.push_back(card2);

  state.players.push_back(p1);

  PlayerInfo p2;
  p2.username = "Player2";
  p2.money = 1500;
  p2.position = 5;
  p2.status = "ACTIVE";
  p2.jailTurnsLeft = 0;
  p2.shieldActive = false;
  p2.color = {38, 89, 204, 255};

  CardInfo card3;
  card3.name = "ShieldCard";
  card3.description = "Kebal 1 giliran";
  card3.value = "";
  p2.handCards.push_back(card3);

  state.players.push_back(p2);

  PlayerInfo p3;
  p3.username = "Player3";
  p3.money = 1800;
  p3.position = 15;
  p3.status = "JAILED";
  p3.jailTurnsLeft = 2;
  p3.shieldActive = false;
  p3.color = {38, 153, 64, 255};
  state.players.push_back(p3);

  PlayerInfo p4;
  p4.username = "Player4";
  p4.money = 500;
  p4.position = 20;
  p4.status = "BANKRUPT";
  p4.jailTurnsLeft = 0;
  p4.shieldActive = false;
  p4.color = {204, 140, 12, 255};
  state.players.push_back(p4);

  TileInfo goTile;
  goTile.index = 1;
  goTile.kode = "GO";
  goTile.name = "GO";
  goTile.tileType = "SPECIAL";
  goTile.colorGroup = "";
  goTile.price = 0;
  goTile.ownerName = "";
  goTile.propStatus = "BANK";
  goTile.houseCount = 0;
  goTile.hasHotel = false;
  goTile.festivalMult = 1;
  state.tiles.push_back(goTile);

  for (int i = 2; i <= 9; i++) {
    TileInfo tile;
    tile.index = i;
    tile.kode = "ST" + std::to_string(i);
    tile.name = "Street " + std::to_string(i);
    tile.tileType = "STREET";

    std::vector<std::string> colors = {"COKLAT", "BIRU_MUDA", "MERAH_MUDA",
                                       "ORANGE", "MERAH",     "KUNING",
                                       "HIJAU"};
    tile.colorGroup = colors[(i - 2) % colors.size()];

    tile.price = 50 + (i - 2) * 20;
    tile.propStatus = (i % 3 == 0) ? "OWNED" : "BANK";
    tile.ownerName = (i % 3 == 0) ? "Player1" : "";
    tile.houseCount = (i % 3 == 0) ? (i % 5) : 0;
    tile.hasHotel = false;
    tile.festivalMult = 1;
    state.tiles.push_back(tile);
  }

  TileInfo jailTile;
  jailTile.index = 10;
  jailTile.kode = "JAIL";
  jailTile.name = "Penjara";
  jailTile.tileType = "SPECIAL";
  jailTile.colorGroup = "";
  jailTile.price = 0;
  jailTile.ownerName = "";
  jailTile.propStatus = "BANK";
  jailTile.houseCount = 0;
  jailTile.hasHotel = false;
  jailTile.festivalMult = 1;
  state.tiles.push_back(jailTile);

  for (int i = 11; i <= 40; i++) {
    TileInfo tile;
    tile.index = i;
    tile.kode = "T" + std::to_string(i);
    tile.name = "Tile " + std::to_string(i);

    if (i % 5 == 0) {
      tile.tileType = "RAILROAD";
      tile.price = 200;
      tile.propStatus = (i % 2 == 0) ? "OWNED" : "BANK";
      tile.ownerName = (i % 2 == 0) ? "Player2" : "";
    } else if (i % 7 == 0) {
      tile.tileType = "UTILITY";
      tile.price = 150;
      tile.propStatus = "BANK";
      tile.ownerName = "";
    } else if (i % 11 == 0) {
      tile.tileType = "TAX";
      tile.price = 0;
      tile.propStatus = "BANK";
      tile.ownerName = "";
    } else if (i == 20) {
      tile.tileType = "SPECIAL";
      tile.kode = "PARKIR";
      tile.name = "Parkir Gratis";
      tile.price = 0;
      tile.propStatus = "BANK";
      tile.ownerName = "";
    } else if (i == 30) {
      tile.tileType = "SPECIAL";
      tile.kode = "PENJARA_MASUK";
      tile.name = "Pergi ke Penjara";
      tile.price = 0;
      tile.propStatus = "BANK";
      tile.ownerName = "";
    } else {
      tile.tileType = "STREET";
      std::vector<std::string> colors = {"COKLAT", "BIRU_MUDA", "MERAH_MUDA",
                                         "ORANGE", "MERAH",     "KUNING",
                                         "HIJAU",  "BIRU_TUA",  "ABU_ABU"};
      tile.colorGroup = colors[i % colors.size()];
      tile.price = 100 + (i * 10);
      tile.propStatus = (i % 4 == 0) ? "MORTGAGED" : "BANK";
      tile.ownerName = "";
    }

    tile.colorGroup = (tile.tileType == "STREET" && tile.colorGroup.empty())
                          ? "ABU_ABU"
                          : tile.colorGroup;
    tile.houseCount = 0;
    tile.hasHotel = false;
    tile.festivalMult = 1;
    state.tiles.push_back(tile);
  }

  state.currentPlayerIndex = 0;
  state.currentTurn = 5;
  state.maxTurn = 20;
  state.gameOver = false;
  state.winnerName = "";

  state.dice.dice1 = 3;
  state.dice.dice2 = 4;
  state.dice.isDouble = false;
  state.dice.hasRolled = true;

  LogInfo log1;
  log1.turn = 1;
  log1.username = "Player1";
  log1.actionType = "ROLL_DICE";
  log1.detail = "Lempar dadu: 5+2=7";
  state.logs.push_back(log1);

  LogInfo log2;
  log2.turn = 2;
  log2.username = "Player2";
  log2.actionType = "BUY_PROPERTY";
  log2.detail = "Membeli properti ST5";
  state.logs.push_back(log2);

  LogInfo log3;
  log3.turn = 3;
  log3.username = "Player1";
  log3.actionType = "PAY_RENT";
  log3.detail = "Bayar sewa M150 ke Player2";
  state.logs.push_back(log3);

  LogInfo log4;
  log4.turn = 4;
  log4.username = "Player3";
  log4.actionType = "JAIL";
  log4.detail = "Masuk penjara";
  state.logs.push_back(log4);

  LogInfo log5;
  log5.turn = 5;
  log5.username = "Player1";
  log5.actionType = "USE_CARD";
  log5.detail = "Gunakan MoveCard +5";
  state.logs.push_back(log5);

  return state;
}

void handleCommand(const std::string &cmd) {
  std::cout << "Command pressed: " << cmd << std::endl;
}

void handlePopupOption(int index) {
  std::cout << "Popup option selected: " << index << std::endl;
}

int main() {
  std::cout << "=== GameWindow Test Driver ===" << std::endl;
  std::cout << "Creating GameWindow with dummy data..." << std::endl;

  GameWindow window(1280, 800, "pOOPs: NIMONSPOLI - Test Driver");
  window.init();

  GameState dummyState = createDummyGameState();
  window.updateState(dummyState);

  window.onCommand("LEMPAR_DADU", []() { handleCommand("LEMPAR_DADU"); });
  window.onCommand("BELI", []() { handleCommand("BELI"); });
  window.onCommand("GADAI", []() { handleCommand("GADAI"); });
  window.onCommand("TEBUS", []() { handleCommand("TEBUS"); });
  window.onCommand("BANGUN", []() { handleCommand("BANGUN"); });
  window.onCommand("CETAK_PAPAN", []() { handleCommand("CETAK_PAPAN"); });
  window.onCommand("CETAK_AKTA", []() { handleCommand("CETAK_AKTA"); });
  window.onCommand("CETAK_PROPERTI", []() { handleCommand("CETAK_PROPERTI"); });
  window.onCommand("AKHIRI_GILIRAN", []() { handleCommand("AKHIRI_GILIRAN"); });

  window.onPopupOption([](int idx) { handlePopupOption(idx); });

  std::cout << "Window created. Starting main loop..." << std::endl;
  std::cout << "Press ESC or close window to exit." << std::endl;

  while (window.isRunning()) {
    window.tick();
  }

  std::cout << "Test completed. Window closed." << std::endl;
  return 0;
}
