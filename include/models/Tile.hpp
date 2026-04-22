#pragma once
#include <string>
#include "../core/Player.hpp"
using namespace std;
#include <bits/stdc++.h>
enum class EffectType {
    NONE,               // Tidak ada efek (FreeParkingTile)
 
    // Properti 
    OFFER_BUY,          // Tawarkan pembelian street ke pemain aktif
    AUTO_ACQUIRE,       // Kepemilikan otomatis tanpa beli (Railroad / Utility)
    PAY_RENT,           // Pemain harus bayar sewa ke owner
    ALREADY_OWNED_SELF, // Pemain mendarat di propertinya sendiri, tidak ada aksi
 
    // Pajak
    PAY_TAX_FLAT,       // Bayar pajak flat langsung (PBM)
    PAY_TAX_CHOICE,     // Pemain pilih flat atau persentase (PPH)
 
    // Kartu 
    DRAW_CHANCE,        // Ambil kartu Kesempatan
    DRAW_COMMUNITY,     // Ambil kartu Dana Umum
 
    // Festival 
    FESTIVAL_TRIGGER,   // Pemain pilih properti untuk efek festival
 
    // Spesial 
    AWARD_SALARY,       // Berikan gaji Go ke pemain
    SEND_TO_JAIL,       // Pindahkan pemain ke penjara
    JAIL_TURN,          // Pemain sedang di penjara, proses giliran penjara
    JUST_VISITING,      // Pemain mampir di penjara, tidak ada aksi
    START_AUCTION,      // Mulai lelang properti ini
};
class Tile {
protected:
    int id;
    std::string kode;
    std::string name;
public:
    Tile(int id, string kode, string name);
    virtual EffectType onLanded(Player& player) = 0; 
    virtual ~Tile();
    int getIndex() const;
    string getKode() const;
    string getName() const;
};
