#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <string>

const int maxPlayer = 4;

// INI BUAT VARIABELS/ATRIBUT YG SCOPENYA UDAH TAU DAN BIAR BEST PRACTICE.
// JANGAN DI DEFINE DI HPP MASING2! (pake ini ya enumz)
enum class PlayerStatus { ACTIVE, JAILED, BANKRUPT };

enum class PropertyStatus { BANK, OWNED, MORTGAGED };

enum class CardTileType { CHANCE, COMMUNITY_CHEST };

enum class TaxType { PPH, PBM };

enum class SpecialTileType { GO, JAIL, FREE_PARKING, GO_TO_JAIL };

enum class ChanceCardType { NEAREST_STATION, MOVE_BACK_3, GO_TO_JAIL };

enum class CommunityChestCardType { BIRTHDAY, DOCTOR, ELECTION };

enum class SkillCardType {
    MOVE,
    DISCOUNT,
    SHIELD,
    TELEPORT,
    LASSO,
    DEMOLITION
};

enum class GameErrorCode {
    NOT_ENOUGH_MONEY = 1,
    MAX_CARD_LIMIT = 2,
    INVALID_COMMAND = 3,
    INVALID_PROPERTY = 4,
    GAME_STATE_ERROR = 5
};

enum class LogActionType { // Plis ini beda sama type" yang di class lain yak,
                           // ini khusus event yang sedang terjadi aja bukan
                           // semacam state suatu objek
    ROLL,
    BUY,
    RENT,
    TAX,
    BUILD,
    SELL,
    MORTGAGE,
    UNMORTGAGE,
    CARD,
    RAILROAD,
    UTILITY,
    AUCTION,
    FESTIVAL,
    BANKRUPT,
    SAVE,
    LOAD,
    DOUBLE_ROLL,
    GO,
    UNKNOWN
};

std::string actionTypeToString(LogActionType type);
LogActionType stringToActionType(const std::string &str);

enum class SkillCardEffect {
    NONE,
    MOVE,
    DISCOUNT,
    SHIELD,
    TELEPORT,
    LASSO,
    DEMOLITION
};

enum class ActionCardEffect {
    NONE,
    CHANCE_NEAREST_STATION,
    CHANCE_MOVE_BACK_3,
    CHANCE_GO_TO_JAIL,
    COMMUNITY_BIRTHDAY,
    COMMUNITY_DOCTOR,
    COMMUNITY_ELECTION
};

enum class EffectType {
    NONE, // Tidak ada efek (FreeParkingTile)

    // --- Properti ---
    OFFER_BUY,          // Tawarkan pembelian street ke pemain aktif
    AUTO_ACQUIRE,       // Kepemilikan otomatis tanpa beli (Railroad / Utility)
    PAY_RENT,           // Pemain harus bayar sewa ke owner
    ALREADY_OWNED_SELF, // Pemain mendarat di propertinya sendiri, tidak ada
                        // aksi

    // --- Pajak ---
    PAY_TAX_FLAT,   // Bayar pajak flat langsung (PBM)
    PAY_TAX_CHOICE, // Pemain pilih flat atau persentase (PPH)

    // --- Kartu ---
    DRAW_CHANCE,    // Ambil kartu Kesempatan
    DRAW_COMMUNITY, // Ambil kartu Dana Umum

    // --- Festival ---
    FESTIVAL_TRIGGER, // Pemain pilih properti untuk efek festival

    // --- Spesial ---
    AWARD_SALARY,  // Berikan gaji Go ke pemain
    SEND_TO_JAIL,  // Pindahkan pemain ke penjara
    JAIL_TURN,     // Pemain sedang di penjara, proses giliran penjara
    JUST_VISITING, // Pemain mampir di penjara, tidak ada aksi
    START_AUCTION, // Mulai lelang properti ini

    // --- Jail Outcomes ---
    JAIL_PAID_FINE,         // Pemain berhasil bayar denda dan bebas
    JAIL_ROLLED_DOUBLE,     // Pemain roll double dan bebas
    JAIL_ROLLED_NO_DOUBLE,  // Pemain roll tapi tidak double, tetap di jail
    JAIL_FORCED_BANKRUPTCY, // Pemain bangkrut karena tidak bisa bayar denda
};
#endif
