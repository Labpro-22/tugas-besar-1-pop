#include "../../include/core/Enums.hpp"

std::string actionTypeToString(LogActionType type) {
    switch (type) {
        case LogActionType::ROLL: return "ROLL";
        case LogActionType::BUY: return "BUY";
        case LogActionType::RENT: return "RENT";
        case LogActionType::TAX: return "TAX";
        case LogActionType::BUILD: return "BUILD";
        case LogActionType::SELL: return "SELL";
        case LogActionType::MORTGAGE: return "MORTGAGE";
        case LogActionType::UNMORTGAGE: return "UNMORTGAGE";
        case LogActionType::CARD: return "CARD";
        case LogActionType::RAILROAD: return "RAILROAD";
        case LogActionType::UTILITY: return "UTILITY";
        case LogActionType::AUCTION: return "AUCTION";
        case LogActionType::FESTIVAL: return "FESTIVAL";
        case LogActionType::BANKRUPT: return "BANKRUPT";
        case LogActionType::SAVE: return "SAVE";
        case LogActionType::LOAD: return "LOAD";
        case LogActionType::DOUBLE_ROLL: return "DOUBLE_ROLL";
        case LogActionType::GO: return "GO";
        default: return "UNKNOWN";
    }
}

LogActionType stringToActionType(const std::string& str) {
    if (str == "GO") return LogActionType::GO;
    if (str == "ROLL") return LogActionType::ROLL;
    if (str == "BUY") return LogActionType::BUY;
    if (str == "RENT") return LogActionType::RENT;
    if (str == "TAX") return LogActionType::TAX;
    if (str == "BUILD") return LogActionType::BUILD;
    if (str == "SELL") return LogActionType::SELL;
    if (str == "MORTGAGE") return LogActionType::MORTGAGE;
    if (str == "UNMORTGAGE") return LogActionType::UNMORTGAGE;
    if (str == "CARD") return LogActionType::CARD;
    if (str == "RAILROAD") return LogActionType::RAILROAD;
    if (str == "UTILITY") return LogActionType::UTILITY;
    if (str == "AUCTION") return LogActionType::AUCTION;
    if (str == "FESTIVAL") return LogActionType::FESTIVAL;
    if (str == "BANKRUPT") return LogActionType::BANKRUPT;
    if (str == "SAVE") return LogActionType::SAVE;
    if (str == "LOAD") return LogActionType::LOAD;
    if (str == "DOUBLE_ROLL") return LogActionType::DOUBLE_ROLL;
    return LogActionType::UNKNOWN;
}
