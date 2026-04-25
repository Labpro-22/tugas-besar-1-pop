#ifndef FORMATTER_HPP
#define FORMATTER_HPP

#include "../core/Player.hpp"
#include "../core/TransactionLogger.hpp" //jujur gatau tar ditaro dimana, placeholder dulu aje
#include "../models/Board.hpp" //jujur gatau tar ditaro dimana, placeholder dulu aje
#include "../models/PropertyTile.hpp"
#include <string>
#include <vector>

class Formatter {
  public:
    static void printBoard(Board &board, const std::vector<Player> &players,
                           int currentTurn, int maxTurn);
    static void printAkta(const PropertyTile &property);
    static void printProperti(const Player &player);
    static void printLog(const TransactionLogger &logger, int limit = -1);
    static void printPanelLikuidasi(const Player &player, int debt);
    static void printBayarSewa(const Player &payer, const Player &owner,
                               const PropertyTile &property, int rentAmount);
    static void printBayarPajak(const Player &player, int amount);
    static void printAuction(const PropertyTile &property,
                             const Player &highestBidder, int highestBid);
    static void printFestival(const PropertyTile &property, int oldRent,
                              int newRent, int duration);
    static void printHandCards(const Player &player);
    static void printWinner(const std::vector<Player> &players,
                            bool isBankruptcy);
    static void printBankrupt(const Player &player,
                              const std::string &creditor);
};

#endif
