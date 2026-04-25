#ifndef GAME_WINDOW_HPP
#define GAME_WINDOW_HPP

#include <functional>
#include <map>
#include <raylib.h>
#include <string>
#include <vector>

// ---- Data structs passed from controller to window ----

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

// ---- Popup system ----

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

// ---- App screens ----

enum class AppScreen {
    MAIN_MENU,
    PLAYER_SETUP,
    PLAYING,
};

// ---- Internal UI state ----

struct MenuState {
    int numPlayers = 2;
    std::vector<std::string> playerNames = {"", "", "", ""};
    int activeField = 0;
};

struct TextInputState {
    bool active = false;
    bool isSave = true; // true=save, false=load
    std::string buffer;
    std::string title;
    std::string hint;
    std::string errorMsg;
};

struct PropertyInfoState {
    bool active = false;
    bool justOpened = false;
    std::string title;
    std::vector<std::string> lines;
};

// ---- GameWindow class ----

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

    // Command callbacks (game actions)
    void onCommand(const std::string &name, std::function<void()> callback);
    void onPopupOption(std::function<void(int index)> callback);

    // Screen management
    void setScreen(AppScreen screen);
    AppScreen getScreen() const;

    // Menu/setup callbacks
    void onNewGame(std::function<void(int, std::vector<std::string>)> cb);
    void onLoadGame(std::function<void(std::string)> cb);
    void onSaveGame(std::function<void(std::string)> cb);
    void onExitGame(std::function<void()> cb);

    // GUI dialogs
    void showTextInput(const std::string &title, const std::string &hint,
                       bool isSave);
    void showPropertyInfo(const std::string &title,
                          const std::vector<std::string> &lines);
    void closePropertyInfo();
    void setTextInputError(const std::string &msg);

  private:
    int screenW, screenH;
    Rectangle boardRect;
    Rectangle sidebarRect;
    Rectangle cmdBarRect;

    GameState state;
    PopupState popup;

    AppScreen currentScreen = AppScreen::MAIN_MENU;
    MenuState menuState;
    TextInputState textInput;
    PropertyInfoState propInfo;
    bool exitRequested = false;

    std::map<std::string, std::function<void()>> commandCallbacks;
    std::function<void(int)> popupCallback;
    std::function<void(int, std::vector<std::string>)> newGameCallback;
    std::function<void(std::string)> loadGameCallback;
    std::function<void(std::string)> saveGameCallback;
    std::function<void()> exitGameCallback;

    // Draw methods - game
    void drawBoard();
    void drawTile(const TileInfo &tile, Rectangle rect);
    void drawPlayerPawns();
    void drawSidebar();
    void drawPlayerList();
    void drawHandCards();
    void drawLog();
    void drawCommandBar();
    void drawPopup();

    // Draw methods - screens
    void drawMainMenu();
    void drawPlayerSetup();
    void drawTextInputOverlay();
    void drawPropertyInfoOverlay();

    // Input helper
    void handleKeyboardTextInput(std::string &buffer, int maxLen = 24);

    // Utilities
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
