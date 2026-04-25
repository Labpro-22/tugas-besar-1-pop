#ifndef GAME_WINDOW_HPP
#define GAME_WINDOW_HPP

#include <functional>
#include <map>
#include <raylib.h>
#include <string>
#include <vector>

struct CardInfo {
    std::string name;
    std::string description;
    std::string value;
};

struct PlayerInfo {
    std::string username;
    int money;
    int position;
    std::string status;
    int jailTurnsLeft;
    bool shieldActive;
    std::vector<CardInfo> handCards;
    Color color;
};

struct TileInfo {
    int index;
    std::string kode;
    std::string name;
    std::string tileType;

    std::string colorGroup;
    int price;
    std::string ownerName;
    std::string propStatus;
    int houseCount;
    bool hasHotel;
    int festivalMult;
};

struct LogInfo {
    int turn;
    std::string username;
    std::string actionType;
    std::string detail;
};

struct DiceInfo {
    int dice1;
    int dice2;
    bool isDouble;
    bool hasRolled;
};

struct GameState {
    std::vector<PlayerInfo> players;
    std::vector<TileInfo> tiles;
    std::vector<LogInfo> logs;
    int currentPlayerIndex;
    int currentTurn;
    int maxTurn;
    DiceInfo dice;
    bool gameOver;
    std::string winnerName;
};

enum class PopupType {
    NONE,
    BUY_PROPERTY,
    TAX_CHOICE,
    AUCTION,
    FESTIVAL,
    LIQUIDATION,
    JAIL,
    DROP_CARD,
    WINNER,
    SAVE,
};

struct PopupState {
    PopupType type = PopupType::NONE;
    std::string title;
    std::string message;

    std::vector<std::string> options;
    int selectedIndex = -1;
};

class GameWindow {
  public:
    GameWindow(int width, int height, const std::string &title);
    ~GameWindow();

    void init();

    void updateState(const GameState &state);

    void showPopup(const PopupState &popup);

    void closePopup();

    bool isRunning() const;

    void tick();

    void onCommand(const std::string &name, std::function<void()> callback);

    void onPopupOption(std::function<void(int index)> callback);

  private:
    int screenW, screenH;
    Rectangle boardRect;
    Rectangle sidebarRect;
    Rectangle cmdBarRect;

    GameState state;
    PopupState popup;

    std::map<std::string, std::function<void()>> commandCallbacks;
    std::function<void(int)> popupCallback;

    void drawBoard();
    void drawTile(const TileInfo &tile, Rectangle rect);
    void drawPlayerPawns();
    void drawSidebar();
    void drawPlayerList();
    void drawHandCards();
    void drawLog();
    void drawCommandBar();
    void drawPopup();

    Color getTileColor(const std::string &colorGroup) const;
    Color getPlayerColor(int playerIndex) const;
    Rectangle getTileRect(int tileIndex) const;
    void drawPixelText(const std::string &text, int x, int y, int size,
                       Color color);
    void drawButton(const std::string &label, Rectangle rect, Color bg,
                    Color fg, bool hovered);
    bool isHovered(Rectangle rect) const;
};

#endif
