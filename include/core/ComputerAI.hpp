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

class ComputerAI {
public:
  enum class Difficulty { EASY, HARD };
  static int decideBuyProperty(const Player *player, const PropertyTile *prop,
                               Difficulty diff);
  static int decideTaxChoice(const Player *player, const TaxTile *tax,
                             Difficulty diff);

  static int decideFestival(const Player *player, Difficulty diff);

  static int decideJailChoice(const Player *player, int fine, Difficulty diff);

  static int decideAuction(const Player *bidder, const PropertyTile *prop,
                           int currentBid, int bid1, int bid2, Difficulty diff);

  static int decideDropCard(const Player *player, const SkillCard *newCard,
                            Difficulty diff);

  static std::string decideBuildTarget(const Player *player, Board *board,
                                       Difficulty diff);

private:
  static std::mt19937 &getRng();
};

#endif
