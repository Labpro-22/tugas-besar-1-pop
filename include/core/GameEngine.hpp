#ifndef GAME_ENGINE_HPP
#define GAME_ENGINE_HPP

#include "../models/Card.hpp"
#include "Enums.hpp"
#include <map>
#include <string>
#include <vector>

class Player;
class Board;
class TransactionLogger;
class PropertyTile;

class GameEngine {
  private:
    Board *board;
    TransactionLogger *logger;
    std::vector<Player *> player;

    // NOTE: TUNGGU IMPLEMENTASI MIGUEL, MASIH RANCU BANGET MAU DIBAWA KEMANA
    // CardDeck<SkillCard> *skillDeck;          // Kartu Kemampuan Spesial
    // CardDeck<SkillCard> *chanceDeck;         // Kartu Kesempatan
    // CardDeck<SkillCard> *communityChestDeck; // Kartu Dana Umum

    // Jumlah ronde yang sudah dilalui, harus < MAX_TURN (dari file konfigurasi)
    int roundCount;

    // Indeks ke vektor player, menandakan giliran pemain mana
    int currentTurn;

    // Jumlah petak yang dilewati = dice1 + dice2
    // Streak jika dice1 == dice2
    int dice1;
    int dice2;

    // Bernilai 0 setiap awal giliran, setiap dice1 == dice2, doubleStreak++
    int doubleStreak;

  public:
    GameEngine(Board* board, TransactionLogger* logger);
    ~GameEngine();

    // Permainan baru. Input jumlah pemain, nama pemain. Acak giliran. 
    // SYARAT: config sudah diload oleh config loader.
    void startNewGame();

    // Permainan berdasarkan saved game sebelumnya. 
    // SYARAT: sudah diload oleh config loader.
    void startLoadedGame();

    // Proses pengocokan dadu hingga doubleStreak = 3, atau dice1 != dice2
    void rollDice();
    
    // proses korupsi dadu (ATUR MANUAL HEHE), exception kalau d1, d2 > 6, d1, d2 < 0
    void rollDice(int d1, int d2);

    // Total nilai dadu terakhir
    int getDiceTotal() const;

    // True jika dice1 == dice2
    bool isDouble() const;
 
    // NOTE: PANGGIL FUNGSI FESTIVAL, belum tau implementasinya
    // Urutan: Festival, Bagi kartu spesial, reset giliran
    void executeTurn();

    // Pindahkan currentTurn ke pemain ACTIVE selanjutnya, BANKRUPT gak dihitung
    // Tambahkan roundCount, setelah currentTurn, kembali ke indeks 1
    void advanceTurn();


    // Cek apakah kondisi saat ini sudah ada yang memenangkan pertandingan, dipanggil di setiap akhir giliran.
    void checkWinCondition();
};

#endif
