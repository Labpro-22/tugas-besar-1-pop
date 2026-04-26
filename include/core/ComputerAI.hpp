#ifndef COMPUTER_AI_HPP
#define COMPUTER_AI_HPP

#include <random>
#include <string>

// Forward declarations
class Player;
class PropertyTile;
class TaxTile;
class SkillCard;
class Board;

// ============================================================
// ComputerAI — Decision engine untuk pemain komputer (COM)
//
// Difficulty::EASY : Keputusan biasa/acak (default untuk pemain "COM")
// Difficulty::HARD : Keputusan optimal/strategis (untuk pemain "god")
// ============================================================
class ComputerAI {
  public:
    enum class Difficulty { EASY, HARD };

    // Putuskan apakah membeli properti atau lewatkan ke lelang.
    // Returns: 0 = beli, 1 = lewatkan/lelang
    static int decideBuyProperty(const Player *player, const PropertyTile *prop,
                                 Difficulty diff);

    // Putuskan metode pembayaran pajak PPH.
    // Returns: 0 = flat, 1 = persentase kekayaan
    static int decideTaxChoice(const Player *player, const TaxTile *tax,
                               Difficulty diff);

    // Putuskan properti mana yang mendapat efek festival.
    // Returns: indeks di antara properti street milik pemain
    static int decideFestival(const Player *player, Difficulty diff);

    // Putuskan cara keluar dari penjara.
    // Returns: 0 = bayar denda, 1 = lempar dadu
    static int decideJailChoice(const Player *player, int fine, Difficulty diff);

    // Putuskan tawaran lelang.
    // Returns: 0 = bid rendah, 1 = bid tinggi, 2 = pass
    static int decideAuction(const Player *bidder, const PropertyTile *prop,
                             int currentBid, int bid1, int bid2, Difficulty diff);

    // Putuskan kartu mana yang dibuang saat tangan penuh.
    // Returns: indeks kartu di tangan pemain (0–2)
    static int decideDropCard(const Player *player, const SkillCard *newCard,
                              Difficulty diff);

    // Tentukan target properti untuk dibangun (hanya untuk HARD mode).
    // Returns: kode properti, atau string kosong jika tidak perlu membangun
    static std::string decideBuildTarget(const Player *player, Board *board,
                                        Difficulty diff);

  private:
    static std::mt19937 &getRng();
};

#endif
