#ifndef GAME_ENGINE_HPP
#define GAME_ENGINE_HPP

#include <random>
#include <string>
#include <vector>

// Forward Declarations - Hindari include di header untuk mencegah circular
// dependency
class Player;
class Board;
class SaveLoadManager;
class TransactionLogger;
class Auction;
class SkillCard;
template <typename T> class CardDeck;

class GameEngine {
  private:
    // State Game
    std::vector<Player *> players;
    Board *board;
    SaveLoadManager *saveLoadManager;
    TransactionLogger *logger;
    CardDeck<SkillCard> *skillDeck; // Sesuaikan dengan tipe dari Miguel
    CardDeck<ActionCard> *chanceDeck;
    CardDeck<ActionCard> *communityDeck;

    int currentTurnIdx; // Indeks pemain aktif (0-3)
    int roundCount;     // Jumlah putaran yang sudah berjalan
    bool diceRolled;
    bool turnEnded;

    // Utilitas Internal
    void checkWinCondition();
    void nextTurn();
    void handleBankruptcy(Player *bankruptPlayer, Player *creditor = nullptr);
    void runAuction(PropertyTile *prop);
    void handleActionCardEffect(Player &player, ActionCardEffect effect);
    void initDecks();

  public:
    GameEngine(Board *board, TransactionLogger *logger);
    ~GameEngine();

    void startNewGame(const std::vector<std::string> &playerNames);
    void loadGame(const std::string &filePath);
    void saveGame(const std::string &filePath);

    void rollDice(int d1 = 0, int d2 = 0);
    void executeTileAction();
    void endTurn();

    void buyBuilding(const std::string &propertyCode);
    void sellBuilding(const std::string &colorGroup);
    void mortgageProperty(const std::string &propertyCode);
    void unmortgageProperty(const std::string &propertyCode);
    void useSkillCard(int cardIdx);

    Player *getCurrentPlayer() const;
    std::vector<Player *> getAllPlayers() const { return players; }
    Board *getBoard() const { return board; }
    int getCurrentRound() const { return roundCount; }
    int getCurrentTurnIdx() const { return currentTurnIdx; }

    bool hasDiceRolled() const { return diceRolled; }
    bool isTurnEnded() const { return turnEnded; }
    const std::vector<SkillCard *> &getSkillDeckCards() const;
};

#endif
