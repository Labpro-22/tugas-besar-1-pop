#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <string>


const int maxPlayer = 4;

// INI BUAT VARIABELS/ATRIBUT YG SCOPENYA UDAH TAU DAN BIAR BEST PRACTICE. JANGAN DI DEFINE DI HPP MASING2! (pake ini ya enumz)
enum class PlayerStatus{
    ACTIVE,
    JAILED,
    BANKRUPT
};

enum class PropertyStatus{
    BANK,
    OWNED,
    MORTGAGED
};

enum class CardTileType{
    CHANCE,
    COMMUNITY_CHEST
};

enum class TaxType{
    PPH,
    PBM
};

enum class SpecialTileType{
    GO,
    JAIL,
    FREE_PARKING,
    GO_TO_JAIL
};

enum class ChanceCardType{
    NEAREST_STATION,
    MOVE_BACK_3,
    GO_TO_JAIL
};

enum class CommunityChestCardType{
    BIRTHDAY,
    DOCTOR,
    ELECTION
};

enum class SkillCardType{
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

enum class LogActionType { // Plis ini beda sama type" yang di class lain yak, ini khusus event yang sedang terjadi aja bukan semacam state suatu objek
    ROLL, BUY, RENT, TAX, BUILD, SELL, MORTGAGE, 
    UNMORTGAGE, CARD, RAILROAD, UTILITY, AUCTION, 
    FESTIVAL, BANKRUPT, SAVE, LOAD, DOUBLE_ROLL, GO, UNKNOWN
};

std::string actionTypeToString(LogActionType type);
LogActionType stringToActionType(const std::string& str);


#endif
