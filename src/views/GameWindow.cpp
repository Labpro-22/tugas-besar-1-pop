#include "../../include/views/GameWindow.hpp"
#include <algorithm>
#include <raylib.h>
#include <sstream>
#include <unordered_map>

static const int SCREEN_W = 1280;
static const int SCREEN_H = 800;
static const int BOARD_X = 16;
static const int BOARD_Y = 16;
static const int BOARD_W = 820;
static const int BOARD_H = 680;
static const int CMD_X = 16;
static const int CMD_Y = 704;
static const int CMD_W = 820;
static const int CMD_H = 80;
static const int SIDE_X = 852;
static const int SIDE_Y = 16;
static const int SIDE_W = 412;
static const int SIDE_H = 768;
static const int CORNER_SIZE = 80;

static const Color C_BG = {235, 230, 214, 255};
static const Color C_PANEL = {250, 246, 234, 255};
static const Color C_PANEL_ALT = {239, 233, 215, 255};
static const Color C_BORDER = {58, 42, 24, 255};
static const Color C_TEXT = {43, 32, 22, 255};
static const Color C_TEXT_DIM = {107, 86, 56, 255};
static const Color C_ACCENT = {47, 138, 62, 255};
static const Color C_ACCENT_BG = {200, 228, 198, 255};
static const Color C_GOOD = {31, 122, 51, 255};
static const Color C_WARN = {201, 138, 11, 255};
static const Color C_DANGER = {194, 33, 33, 255};
static const Color C_BTN_BG = {58, 42, 24, 255};
static const Color C_BTN_TEXT = {243, 229, 191, 255};
static const Color C_BOARD_FELT = {215, 232, 207, 255};
static const Color C_TILE_FACE = {223, 232, 207, 255};
static const Color C_SHADOW = {0, 0, 0, 60};

static Color colorGroupToColor(const std::string &g) {
    if (g == "COKLAT")
        return {138, 74, 30, 255};
    if (g == "BIRU_MUDA")
        return {116, 195, 234, 255};
    if (g == "MERAH_MUDA")
        return {216, 83, 156, 255};
    if (g == "ORANGE")
        return {231, 135, 28, 255};
    if (g == "MERAH")
        return {212, 42, 42, 255};
    if (g == "KUNING")
        return {242, 206, 12, 255};
    if (g == "HIJAU")
        return {47, 160, 85, 255};
    if (g == "BIRU_TUA")
        return {27, 70, 183, 255};
    if (g == "ABU_ABU")
        return {143, 142, 138, 255};
    return {128, 110, 84, 255};
}

static Color playerIndexToColor(int i) {
    switch (i) {
    case 0:
        return {214, 56, 56, 255};
    case 1:
        return {38, 89, 204, 255};
    case 2:
        return {38, 153, 64, 255};
    case 3:
        return {204, 140, 12, 255};
    default:
        return {100, 100, 100, 255};
    }
}

static std::string formatMoney(int amount) {
    std::string s = std::to_string(amount);
    int pos = (int)s.length() - 3;
    while (pos > 0) {
        s.insert(pos, ".");
        pos -= 3;
    }
    return "M" + s;
}

static std::string truncate(const std::string &s, int maxLen) {
    if ((int)s.length() <= maxLen)
        return s;
    return s.substr(0, maxLen - 2) + "..";
}

static Color tint(Color c, int d) {
    auto cl = [](int v) {
        return (unsigned char)std::max(0, std::min(255, v));
    };
    return {cl(c.r + d), cl(c.g + d), cl(c.b + d), c.a};
}

struct AssetCache {
    std::unordered_map<std::string, Texture2D> tex;
    bool loaded = false;

    Texture2D *get(const std::string &key) {
        auto it = tex.find(key);
        return it == tex.end() ? nullptr : &it->second;
    }

    void loadPng(const std::string &key, const char *path) {
        if (!FileExists(path))
            return;
        Image img = LoadImage(path);
        if (!img.data)
            return;
        Texture2D t = LoadTextureFromImage(img);

        SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
        UnloadImage(img);
        tex[key] = t;
    }

    void loadAll() {
        if (loaded)
            return;

        for (int i = 1; i <= 6; i++) {
            loadPng("dice_" + std::to_string(i),
                    ("assets/Dice" + std::to_string(i) + ".png").c_str());
        }

        loadPng("pion", "assets/Pion.png");

        loadPng("cmd_lempar_dadu", "assets/LemparDadu.png");
        loadPng("cmd_beli", "assets/Beli.png");
        loadPng("cmd_gadai", "assets/Gadai.png");
        loadPng("cmd_tebus", "assets/Tebus.png");
        loadPng("cmd_bangun", "assets/Bangun.png");
        loadPng("cmd_cetak_akta", "assets/CetakAkta.png");
        loadPng("cmd_cetak_papan", "assets/CetakPapan.png");
        loadPng("cmd_cetak_properti", "assets/CetakProperti.png");
        loadPng("cmd_akhiri", "assets/AkhiriGiliran.png");
        loaded = true;
    }

    void unloadAll() {
        for (auto &kv : tex)
            UnloadTexture(kv.second);
        tex.clear();
        loaded = false;
    }
};
static AssetCache gAssets;
static int popupScrollOffset = 0; // scroll offset for list popups

static void drawIconInRect(Texture2D *tex, Rectangle rect, Color tintColor) {
    if (!tex)
        return;
    float scale = std::min(rect.width / (float)tex->width,
                           rect.height / (float)tex->height);
    float dw = tex->width * scale;
    float dh = tex->height * scale;
    DrawTextureEx(
        *tex,
        {rect.x + (rect.width - dw) / 2.0f, rect.y + (rect.height - dh) / 2.0f},
        0.0f, scale, tintColor);
}

static void drawDie(AssetCache &assets, int x, int y, int size, int value,
                    Color tintColor) {
    if (value < 1 || value > 6)
        return;
    Texture2D *tex = assets.get("dice_" + std::to_string(value));
    if (tex) {
        drawIconInRect(tex, {(float)x, (float)y, (float)size, (float)size},
                       tintColor);
    } else {

        DrawRectangle(x, y, size, size, WHITE);
        DrawRectangleLinesEx({(float)x, (float)y, (float)size, (float)size}, 2,
                             {58, 42, 24, 255});
        bool p[9] = {};
        switch (value) {
        case 1:
            p[4] = 1;
            break;
        case 2:
            p[0] = p[8] = 1;
            break;
        case 3:
            p[0] = p[4] = p[8] = 1;
            break;
        case 4:
            p[0] = p[2] = p[6] = p[8] = 1;
            break;
        case 5:
            p[0] = p[2] = p[4] = p[6] = p[8] = 1;
            break;
        case 6:
            p[0] = p[2] = p[3] = p[5] = p[6] = p[8] = 1;
            break;
        }
        int pad = size / 5, cell = (size - 2 * pad) / 3;
        for (int i = 0; i < 9; i++)
            if (p[i])
                DrawCircle(x + pad + (i % 3) * cell + cell / 2,
                           y + pad + (i / 3) * cell + cell / 2, size / 10,
                           {43, 32, 22, 255});
    }
}

static void drawPion(AssetCache &assets, int x, int y, int size,
                     Color playerColor) {
    Texture2D *tex = assets.get("pion");
    if (tex) {
        drawIconInRect(tex, {(float)x, (float)y, (float)size, (float)size},
                       playerColor);
    } else {

        int r = size / 2;
        DrawCircle(x + r, y + r, (float)r, playerColor);
        DrawCircleLines(x + r, y + r, (float)r, {58, 42, 24, 255});
    }
}

GameWindow::GameWindow(int width, int height, const std::string &title)
    : screenW(width), screenH(height) {
    boardRect = {(float)BOARD_X, (float)BOARD_Y, (float)BOARD_W,
                 (float)BOARD_H};
    sidebarRect = {(float)SIDE_X, (float)SIDE_Y, (float)SIDE_W, (float)SIDE_H};
    cmdBarRect = {(float)CMD_X, (float)CMD_Y, (float)CMD_W, (float)CMD_H};
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
}

GameWindow::~GameWindow() {
    gAssets.unloadAll();
    CloseWindow();
}

void GameWindow::init() { gAssets.loadAll(); }

void GameWindow::updateState(const GameState &s) { state = s; }
void GameWindow::showPopup(const PopupState &p) {
    popup = p;
    popupScrollOffset = 0;
}
void GameWindow::closePopup() { popup.type = PopupType::NONE; }
bool GameWindow::isRunning() const {
    return !WindowShouldClose() && !exitRequested;
}
void GameWindow::onCommand(const std::string &n, std::function<void()> cb) {
    commandCallbacks[n] = cb;
}
void GameWindow::onPopupOption(std::function<void(int)> cb) {
    popupCallback = cb;
}
void GameWindow::setScreen(AppScreen screen) { currentScreen = screen; }
AppScreen GameWindow::getScreen() const { return currentScreen; }
void GameWindow::onNewGame(
    std::function<void(int, std::vector<std::string>)> cb) {
    newGameCallback = cb;
}
void GameWindow::onLoadGame(std::function<void(std::string)> cb) {
    loadGameCallback = cb;
}
void GameWindow::onSaveGame(std::function<void(std::string)> cb) {
    saveGameCallback = cb;
}
void GameWindow::onExitGame(std::function<void()> cb) {
    exitGameCallback = cb;
}
void GameWindow::showTextInput(const std::string &title,
                               const std::string &hint, bool isSave) {
    textInput.active = true;
    textInput.isSave = isSave;
    textInput.buffer = "";
    textInput.title = title;
    textInput.hint = hint;
    textInput.errorMsg = "";
}
void GameWindow::showPropertyInfo(const std::string &title,
                                  const std::vector<std::string> &lines) {
    propInfo.active = true;
    propInfo.justOpened = true;
    propInfo.title = title;
    propInfo.lines = lines;
}
void GameWindow::closePropertyInfo() { propInfo.active = false; }
void GameWindow::setTextInputError(const std::string &msg) {
    textInput.errorMsg = msg;
}

void GameWindow::tick() {
    BeginDrawing();
    ClearBackground(C_BG);

    switch (currentScreen) {
    case AppScreen::MAIN_MENU:
        drawMainMenu();
        break;
    case AppScreen::PLAYER_SETUP:
        drawPlayerSetup();
        break;
    case AppScreen::PLAYING:
        drawBoard();
        drawSidebar();
        drawCommandBar();
        if (popup.type != PopupType::NONE)
            drawPopup();
        if (propInfo.active)
            drawPropertyInfoOverlay();
        break;
    }

    if (textInput.active)
        drawTextInputOverlay();

    EndDrawing();
}

void GameWindow::drawPixelText(const std::string &text, int x, int y, int size,
                               Color color) {
    DrawText(text.c_str(), x, y, size, color);
}

bool GameWindow::isHovered(Rectangle r) const {
    return CheckCollisionPointRec(GetMousePosition(), r);
}

void GameWindow::drawButton(const std::string &label, Rectangle rect, Color bg,
                            Color fg, bool hovered) {
    DrawRectangleRec({rect.x + 3, rect.y + 3, rect.width, rect.height},
                     C_SHADOW);
    DrawRectangleRec(rect, hovered ? tint(bg, 15) : bg);
    DrawRectangleLinesEx(rect, 2, C_BORDER);
    int fs = 10;
    std::string l1 = label, l2;
    size_t nl = label.find('\n');
    if (nl != std::string::npos) {
        l1 = label.substr(0, nl);
        l2 = label.substr(nl + 1);
    }
    auto tw = [&](const std::string &s) { return MeasureText(s.c_str(), fs); };
    if (l2.empty()) {
        drawPixelText(l1, (int)(rect.x + (rect.width - tw(l1)) / 2),
                      (int)(rect.y + (rect.height - fs) / 2), fs, fg);
    } else {
        drawPixelText(l1, (int)(rect.x + (rect.width - tw(l1)) / 2),
                      (int)(rect.y + rect.height / 2 - fs - 2), fs, fg);
        drawPixelText(l2, (int)(rect.x + (rect.width - tw(l2)) / 2),
                      (int)(rect.y + rect.height / 2 + 2), fs, fg);
    }
}

Color GameWindow::getTileColor(const std::string &c) const {
    return colorGroupToColor(c);
}
Color GameWindow::getPlayerColor(int i) const { return playerIndexToColor(i); }

// ---- Keyboard text input helper ----

void GameWindow::handleKeyboardTextInput(std::string &buffer, int maxLen) {
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125 && (int)buffer.size() < maxLen) {
            buffer += (char)key;
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !buffer.empty()) {
        buffer.pop_back();
    }
}

// ---- MAIN MENU SCREEN ----

void GameWindow::drawMainMenu() {
    DrawRectangle(0, 0, screenW, screenH, C_BOARD_FELT);
    DrawRectangle(0, 0, screenW, 12, C_BTN_BG);
    DrawRectangle(0, screenH - 12, screenW, 12, C_BTN_BG);
    DrawRectangle(0, 0, 12, screenH, C_BTN_BG);
    DrawRectangle(screenW - 12, 0, 12, screenH, C_BTN_BG);

    float cw = 500.0f, ch = 460.0f;
    float cx = (screenW - cw) / 2.0f;
    float cy = (screenH - ch) / 2.0f;

    DrawRectangleRec({cx + 10, cy + 10, cw, ch}, {0, 0, 0, 100});
    DrawRectangleRec({cx, cy, cw, ch}, C_PANEL);
    DrawRectangleLinesEx({cx, cy, cw, ch}, 4, C_BORDER);

    DrawRectangle((int)cx, (int)cy, (int)cw, 88, C_BTN_BG);
    DrawRectangleLinesEx({cx, cy, cw, 88.0f}, 2, C_BORDER);
    DrawRectangle((int)cx, (int)cy, (int)cw, 8, {166, 38, 26, 255});

    const char *title1 = "pOOPs: NIMONSPOLI";
    int tw1 = MeasureText(title1, 26);
    drawPixelText(title1, (int)(cx + (cw - tw1) / 2), (int)cy + 18, 26, C_BTN_TEXT);

    const char *title2 = "Board Game Monopoli ala Indonesia!";
    int tw2 = MeasureText(title2, 10);
    drawPixelText(title2, (int)(cx + (cw - tw2) / 2), (int)cy + 58, 10,
                  {216, 200, 154, 255});
    drawPixelText("v1.0", (int)(cx + cw - 48), (int)cy + 10, 8, {122, 104, 64, 255});

    float btnW = cw - 80.0f;
    float btnX = cx + 40.0f;
    float btnH = 54.0f;

    // NEW GAME
    float y1 = cy + 110;
    Rectangle r1 = {btnX, y1, btnW, btnH};
    bool h1 = isHovered(r1);
    DrawRectangleRec({r1.x + 5, r1.y + 5, r1.width, r1.height}, {0, 0, 0, 80});
    DrawRectangleRec(r1, h1 ? tint(C_ACCENT, 25) : C_ACCENT);
    DrawRectangleLinesEx(r1, 3, C_BORDER);
    const char *l1 = "MULAI PERMAINAN BARU";
    int lw1 = MeasureText(l1, 14);
    drawPixelText(l1, (int)(r1.x + (r1.width - lw1) / 2),
                  (int)(r1.y + (r1.height - 14) / 2), 14, WHITE);
    if (h1 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = AppScreen::PLAYER_SETUP;
        menuState.activeField = 0;
    }

    // LOAD GAME
    float y2 = cy + 182;
    Rectangle r2 = {btnX, y2, btnW, btnH};
    bool h2 = isHovered(r2);
    DrawRectangleRec({r2.x + 5, r2.y + 5, r2.width, r2.height}, {0, 0, 0, 80});
    DrawRectangleRec(r2, h2 ? tint(C_BTN_BG, 30) : C_BTN_BG);
    DrawRectangleLinesEx(r2, 3, C_BORDER);
    const char *l2 = "MUAT PERMAINAN TERSIMPAN";
    int lw2 = MeasureText(l2, 14);
    drawPixelText(l2, (int)(r2.x + (r2.width - lw2) / 2),
                  (int)(r2.y + (r2.height - 14) / 2), 14, C_BTN_TEXT);
    if (h2 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        textInput.active = true;
        textInput.isSave = false;
        textInput.buffer = "";
        textInput.title = "MUAT PERMAINAN";
        textInput.hint = "Masukkan path file (contoh: saves/game1):";
        textInput.errorMsg = "";
    }

    // EXIT
    float y3 = cy + 254;
    Rectangle r3 = {btnX, y3, btnW, btnH};
    bool h3 = isHovered(r3);
    DrawRectangleRec({r3.x + 5, r3.y + 5, r3.width, r3.height}, {0, 0, 0, 80});
    DrawRectangleRec(r3, h3 ? Color{220, 50, 50, 255} : C_DANGER);
    DrawRectangleLinesEx(r3, 3, C_BORDER);
    const char *l3 = "KELUAR";
    int lw3 = MeasureText(l3, 14);
    drawPixelText(l3, (int)(r3.x + (r3.width - lw3) / 2),
                  (int)(r3.y + (r3.height - 14) / 2), 14, WHITE);
    if (h3 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        exitRequested = true;
    }

    drawPixelText("Tugas Besar 1 - IF2210 Pemrograman Berorientasi Objek",
                  (int)(cx + 20), (int)(cy + ch - 26), 8, C_TEXT_DIM);
}

// ---- PLAYER SETUP SCREEN ----

void GameWindow::drawPlayerSetup() {
    DrawRectangle(0, 0, screenW, screenH, C_BOARD_FELT);
    DrawRectangle(0, 0, screenW, 12, C_BTN_BG);
    DrawRectangle(0, screenH - 12, screenW, 12, C_BTN_BG);
    DrawRectangle(0, 0, 12, screenH, C_BTN_BG);
    DrawRectangle(screenW - 12, 0, 12, screenH, C_BTN_BG);

    float cw = 540.0f, ch = 520.0f;
    float cx = (screenW - cw) / 2.0f;
    float cy = (screenH - ch) / 2.0f;

    DrawRectangleRec({cx + 10, cy + 10, cw, ch}, {0, 0, 0, 100});
    DrawRectangleRec({cx, cy, cw, ch}, C_PANEL);
    DrawRectangleLinesEx({cx, cy, cw, ch}, 4, C_BORDER);

    DrawRectangle((int)cx, (int)cy, (int)cw, 64, C_BTN_BG);
    DrawRectangle((int)cx, (int)cy, (int)cw, 8, {166, 38, 26, 255});
    const char *hdr = "PENGATURAN PEMAIN";
    int hw = MeasureText(hdr, 18);
    drawPixelText(hdr, (int)(cx + (cw - hw) / 2), (int)cy + 22, 18, C_BTN_TEXT);

    drawPixelText("JUMLAH PEMAIN:", (int)cx + 30, (int)cy + 82, 10, C_TEXT_DIM);
    for (int n = 2; n <= 4; n++) {
        float bx = cx + 180 + (n - 2) * 88.0f;
        Rectangle nr = {bx, cy + 76, 72, 32};
        bool sel = (menuState.numPlayers == n);
        bool hov = isHovered(nr);
        DrawRectangleRec(nr, sel ? C_ACCENT : (hov ? C_ACCENT_BG : C_PANEL_ALT));
        DrawRectangleLinesEx(nr, sel ? 3.0f : 1.0f, C_BORDER);
        std::string ns = std::to_string(n) + " Pemain";
        int nw = MeasureText(ns.c_str(), 9);
        drawPixelText(ns, (int)(nr.x + (nr.width - nw) / 2), (int)(nr.y + 11), 9,
                      sel ? WHITE : C_TEXT);
        if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            menuState.numPlayers = n;
        }
    }

    DrawRectangle((int)cx + 20, (int)cy + 120, (int)cw - 40, 1, C_BORDER);

    for (int i = 0; i < menuState.numPlayers; i++) {
        float fy = cy + 136 + i * 70.0f;
        bool isActive = (menuState.activeField == i);
        Color pColor = playerIndexToColor(i);
        DrawCircle((int)(cx + 38), (int)(fy + 22), 10, pColor);
        DrawCircleLines((int)(cx + 38), (int)(fy + 22), 10, C_BORDER);

        std::string labelStr = "Pemain " + std::to_string(i + 1);
        drawPixelText(labelStr, (int)cx + 54, (int)fy + 8, 9, C_TEXT_DIM);

        Rectangle inputRect = {cx + 54, fy + 22, cw - 90, 34};
        Color borderCol = isActive ? C_ACCENT : C_BORDER;
        float borderThick = isActive ? 3.0f : 1.0f;
        DrawRectangleRec(inputRect, isActive ? Color{248, 252, 248, 255} : WHITE);
        DrawRectangleLinesEx(inputRect, borderThick, borderCol);

        std::string displayText = menuState.playerNames[i];
        bool showCursor = isActive && ((int)(GetTime() * 2) % 2 == 0);
        std::string shown = displayText + (showCursor ? "|" : "");
        if (menuState.playerNames[i].empty() && !showCursor) {
            drawPixelText(("Nama pemain " + std::to_string(i + 1)),
                          (int)inputRect.x + 10, (int)inputRect.y + 11, 10,
                          C_TEXT_DIM);
        } else {
            drawPixelText(shown, (int)inputRect.x + 10, (int)inputRect.y + 11,
                          10, C_TEXT);
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && isHovered(inputRect)) {
            menuState.activeField = i;
        }
    }

    if (menuState.activeField >= 0 &&
        menuState.activeField < menuState.numPlayers) {
        handleKeyboardTextInput(menuState.playerNames[menuState.activeField], 16);
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_TAB)) {
            menuState.activeField = (menuState.activeField + 1) % menuState.numPlayers;
        }
    }

    bool allFilled = true;
    for (int i = 0; i < menuState.numPlayers; i++)
        if (menuState.playerNames[i].empty()) { allFilled = false; break; }

    if (!allFilled) {
        drawPixelText("Isi semua nama pemain untuk memulai.",
                      (int)(cx + 30), (int)(cy + ch - 96), 9, C_WARN);
    }

    float sbY = cy + ch - 76;
    float sbW = (cw - 100) * 0.65f;
    Rectangle startBtn = {cx + 40, sbY, sbW, 44};
    bool shov = isHovered(startBtn) && allFilled;
    Color sbCol = allFilled ? (shov ? tint(C_ACCENT, 20) : C_ACCENT)
                            : Color{160, 160, 155, 255};
    DrawRectangleRec({startBtn.x + 4, startBtn.y + 4, startBtn.width, startBtn.height},
                     {0, 0, 0, 80});
    DrawRectangleRec(startBtn, sbCol);
    DrawRectangleLinesEx(startBtn, 3, C_BORDER);
    const char *startLabel = "MULAI GAME!";
    int slw = MeasureText(startLabel, 14);
    drawPixelText(startLabel, (int)(startBtn.x + (startBtn.width - slw) / 2),
                  (int)(startBtn.y + (startBtn.height - 14) / 2), 14, WHITE);
    if (allFilled && shov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        std::vector<std::string> names;
        for (int i = 0; i < menuState.numPlayers; i++)
            names.push_back(menuState.playerNames[i]);
        if (newGameCallback)
            newGameCallback(menuState.numPlayers, names);
    }

    float backX = cx + 40 + sbW + 16;
    float backW = cw - 80 - sbW - 16;
    Rectangle backBtn = {backX, sbY, backW, 44};
    bool bhov = isHovered(backBtn);
    DrawRectangleRec({backBtn.x + 4, backBtn.y + 4, backBtn.width, backBtn.height},
                     {0, 0, 0, 80});
    DrawRectangleRec(backBtn, bhov ? tint(C_PANEL_ALT, 15) : C_PANEL_ALT);
    DrawRectangleLinesEx(backBtn, 2, C_BORDER);
    const char *backLabel = "KEMBALI";
    int blw = MeasureText(backLabel, 10);
    drawPixelText(backLabel, (int)(backBtn.x + (backBtn.width - blw) / 2),
                  (int)(backBtn.y + (backBtn.height - 10) / 2), 10, C_TEXT);
    if (bhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        currentScreen = AppScreen::MAIN_MENU;
        for (auto &n : menuState.playerNames) n = "";
        menuState.activeField = 0;
    }
}

// ---- TEXT INPUT OVERLAY ----

void GameWindow::drawTextInputOverlay() {
    if (!textInput.active) return;

    DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 160});

    float pw = 460.0f, ph = 230.0f;
    float px = (screenW - pw) / 2.0f, py = (screenH - ph) / 2.0f;

    DrawRectangleRec({px + 8, py + 8, pw, ph}, {0, 0, 0, 100});
    DrawRectangleRec({px, py, pw, ph}, C_PANEL);
    DrawRectangleLinesEx({px, py, pw, ph}, 3, C_BORDER);

    Color hdrCol = textInput.isSave ? C_WARN : C_BTN_BG;
    DrawRectangle((int)px, (int)py, (int)pw, 48, hdrCol);
    int htw = MeasureText(textInput.title.c_str(), 13);
    drawPixelText(textInput.title, (int)(px + (pw - htw) / 2), (int)py + 17, 13, WHITE);

    drawPixelText(textInput.hint, (int)px + 20, (int)py + 62, 9, C_TEXT_DIM);

    Rectangle inputRect = {px + 20, py + 82, pw - 40, 38};
    DrawRectangleRec(inputRect, WHITE);
    DrawRectangleLinesEx(inputRect, 2, C_ACCENT);

    bool cursor = ((int)(GetTime() * 2) % 2 == 0);
    std::string shown = textInput.buffer + (cursor ? "|" : "");
    if (textInput.buffer.empty() && !cursor) {
        drawPixelText("contoh: saves/game1 atau /path/ke/file",
                      (int)inputRect.x + 10, (int)inputRect.y + 12, 10, C_TEXT_DIM);
    } else {
        drawPixelText(shown, (int)inputRect.x + 10, (int)inputRect.y + 12, 10, C_TEXT);
    }

    handleKeyboardTextInput(textInput.buffer, 64);

    if (!textInput.errorMsg.empty()) {
        drawPixelText(textInput.errorMsg, (int)px + 20, (int)py + 126, 9, C_DANGER);
    }

    float bh = 36.0f, bw = (pw - 56.0f) / 2.0f;
    Rectangle confirmBtn = {px + 20, py + ph - bh - 14, bw, bh};
    Rectangle cancelBtn  = {px + 36 + bw, py + ph - bh - 14, bw, bh};

    bool chov  = isHovered(confirmBtn) && !textInput.buffer.empty();
    bool cahov = isHovered(cancelBtn);

    Color confirmCol = !textInput.buffer.empty()
                           ? (chov ? tint(C_ACCENT, 20) : C_ACCENT)
                           : Color{150, 150, 150, 255};
    DrawRectangleRec(confirmBtn, confirmCol);
    DrawRectangleLinesEx(confirmBtn, 2, C_BORDER);
    const char *confLabel = textInput.buffer.empty() ? "(kosong)" : "KONFIRMASI";
    int clw = MeasureText(confLabel, 10);
    drawPixelText(confLabel, (int)(confirmBtn.x + (confirmBtn.width - clw) / 2),
                  (int)(confirmBtn.y + (confirmBtn.height - 10) / 2), 10, WHITE);

    DrawRectangleRec(cancelBtn, cahov ? tint(C_PANEL_ALT, 15) : C_PANEL_ALT);
    DrawRectangleLinesEx(cancelBtn, 2, C_BORDER);
    const char *cancelLabel = "BATAL";
    int calw = MeasureText(cancelLabel, 10);
    drawPixelText(cancelLabel, (int)(cancelBtn.x + (cancelBtn.width - calw) / 2),
                  (int)(cancelBtn.y + (cancelBtn.height - 10) / 2), 10, C_TEXT);

    auto confirm = [&]() {
        std::string fn = textInput.buffer;
        bool isSave = textInput.isSave;
        textInput.active = false;
        textInput.buffer = "";
        textInput.errorMsg = "";
        if (isSave) { if (saveGameCallback) saveGameCallback(fn); }
        else         { if (loadGameCallback) loadGameCallback(fn); }
    };
    auto cancel = [&]() {
        textInput.active = false;
        textInput.buffer = "";
        textInput.errorMsg = "";
    };

    if (IsKeyPressed(KEY_ENTER) && !textInput.buffer.empty()) { confirm(); return; }
    if (chov  && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { confirm(); return; }
    if (cahov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { cancel();  return; }
}

// ---- PROPERTY INFO OVERLAY ----

void GameWindow::drawPropertyInfoOverlay() {
    if (!propInfo.active) return;

    DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 150});

    float pw = 400.0f;
    float lineH = 20.0f;
    float ph = 60.0f + propInfo.lines.size() * lineH + 54.0f;
    float px = (screenW - pw) / 2.0f, py = (screenH - ph) / 2.0f;

    DrawRectangleRec({px + 8, py + 8, pw, ph}, {0, 0, 0, 100});
    DrawRectangleRec({px, py, pw, ph}, C_PANEL);
    DrawRectangleLinesEx({px, py, pw, ph}, 3, C_BORDER);

    DrawRectangle((int)px, (int)py, (int)pw, 48, C_BTN_BG);
    int htw = MeasureText(propInfo.title.c_str(), 12);
    drawPixelText(propInfo.title, (int)(px + (pw - htw) / 2), (int)py + 17, 12,
                  C_BTN_TEXT);

    for (int i = 0; i < (int)propInfo.lines.size(); i++) {
        int ly = (int)(py + 58 + i * lineH);
        drawPixelText(propInfo.lines[i], (int)px + 20, ly, 9, C_TEXT);
    }

    float btnY = py + ph - 42;
    Rectangle closeBtn = {px + 20, btnY, pw - 40, 32};
    bool chov = isHovered(closeBtn);
    DrawRectangleRec(closeBtn, chov ? tint(C_BTN_BG, 20) : C_BTN_BG);
    DrawRectangleLinesEx(closeBtn, 2, C_BORDER);
    const char *clbl = "TUTUP";
    int clw = MeasureText(clbl, 10);
    drawPixelText(clbl, (int)(closeBtn.x + (closeBtn.width - clw) / 2),
                  (int)(closeBtn.y + (closeBtn.height - 10) / 2), 10, C_BTN_TEXT);

    // Skip click-detection on the first frame to avoid same-frame glitch
    if (propInfo.justOpened) { propInfo.justOpened = false; return; }
    if (chov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        propInfo.active = false;
    }
}

Rectangle GameWindow::getTileRect(int tileIndex) const {
    float bx = boardRect.x, by = boardRect.y, bw = boardRect.width,
          bh = boardRect.height;
    float cs = CORNER_SIZE;
    float tw = (bw - 2 * cs) / 9.0f, th = (bh - 2 * cs) / 9.0f;
    int i = ((tileIndex - 1) + 40) % 40;
    if (i == 0)
        return {bx + bw - cs, by + bh - cs, cs, cs};
    if (i >= 1 && i <= 9)
        return {bx + bw - cs - i * tw, by + bh - cs, tw, cs};
    if (i == 10)
        return {bx, by + bh - cs, cs, cs};
    if (i >= 11 && i <= 19)
        return {bx, by + bh - cs - (i - 10) * th, cs, th};
    if (i == 20)
        return {bx, by, cs, cs};
    if (i >= 21 && i <= 29)
        return {bx + cs + (i - 21) * tw, by, tw, cs};
    if (i == 30)
        return {bx + bw - cs, by, cs, cs};
    if (i >= 31 && i <= 39)
        return {bx + bw - cs, by + cs + (i - 31) * th, cs, th};
    return {bx, by, tw, th};
}

void GameWindow::drawBoard() {
    DrawRectangleRec(boardRect, C_BOARD_FELT);
    DrawRectangleLinesEx(boardRect, 3, C_BORDER);

    float cx = boardRect.x + CORNER_SIZE, cy = boardRect.y + CORNER_SIZE;
    float cw = boardRect.width - 2.0f * CORNER_SIZE,
          ch = boardRect.height - 2.0f * CORNER_SIZE;
    DrawRectangle((int)cx, (int)cy, (int)cw, (int)ch, C_TILE_FACE);
    DrawRectangleLinesEx({cx, cy, cw, ch}, 1,
                         {C_BORDER.r, C_BORDER.g, C_BORDER.b, 45});

    int lx = (int)(cx + cw / 2), ly = (int)(cy + ch / 2);
    int w_title = MeasureText("pOOPs: NIMONSPOLI", 22);
    drawPixelText("pOOPs: NIMONSPOLI", lx - w_title / 2, ly - 110, 22,
                  {166, 38, 26, 255});
    drawPixelText("TURN", lx - MeasureText("TURN", 10) / 2, ly - 70, 10,
                  C_TEXT_DIM);

    std::string tBig = std::to_string(state.currentTurn) + " / " +
                       std::to_string(state.maxTurn);
    int w_tbig = MeasureText(tBig.c_str(), 38);
    drawPixelText(tBig, lx - w_tbig / 2, ly - 40, 38, C_TEXT);

    if (!state.players.empty()) {
        std::string gir =
            "GILIRAN > " + state.players[state.currentPlayerIndex].username;
        int w_gir = MeasureText(gir.c_str(), 9);
        drawPixelText(gir, lx - w_gir / 2, ly + 20, 9, C_TEXT_DIM);
    }

    int d1 = state.dice.dice1, d2 = state.dice.dice2;
    if (d1 > 0 && d2 > 0) {
        drawDie(gAssets, lx - 50, ly + 46, 36, d1, C_TEXT);
        drawDie(gAssets, lx + 14, ly + 46, 36, d2, C_TEXT);
        std::string diceSum = "= " + std::to_string(d1 + d2);
        drawPixelText(diceSum, lx + 50 + 5, ly + 54, 10, C_TEXT);
    }

    for (int idx = 1; idx <= 40; idx++) {
        const TileInfo *tp = nullptr;
        for (const auto &t : state.tiles)
            if (t.index == idx) {
                tp = &t;
                break;
            }
        if (tp)
            drawTile(*tp, getTileRect(idx));
    }
    drawPlayerPawns();
}

void GameWindow::drawTile(const TileInfo &tile, Rectangle rect) {
    float cs = (float)CORNER_SIZE;
    bool isCorner = (rect.width >= cs - 1 && rect.height >= cs - 1);
    bool isSideBtm = (!isCorner && rect.height >= cs - 1 &&
                      rect.y > boardRect.y + boardRect.height / 2);
    bool isSideTop = (!isCorner && rect.height >= cs - 1 &&
                      rect.y < boardRect.y + boardRect.height / 2);
    bool isSideLft = (!isCorner && rect.width >= cs - 1 &&
                      rect.x < boardRect.x + boardRect.width / 2);
    bool isSideRgt = (!isCorner && rect.width >= cs - 1 &&
                      rect.x > boardRect.x + boardRect.width / 2);

    DrawRectangleRec(rect, C_TILE_FACE);

    if (isCorner) {
        const char *l1 = "", *l2 = "";
        Color col = C_TEXT;
        if (tile.kode == "GO") {
            l1 = "COLLECT";
            l2 = "M200";
            col = {47, 138, 62, 255};
        } else if (tile.kode == "PEN") {
            l1 = "JUST";
            l2 = "VISITING";
            col = C_TEXT;
        } else if (tile.kode == "BBP") {
            l1 = "FREE";
            l2 = "PARKING";
            col = C_TEXT;
        } else if (tile.kode == "PPJ") {
            l1 = "GO TO";
            l2 = "JAIL";
            col = C_DANGER;
        }

        if (tile.kode == "PEN") {
            DrawRectangle((int)rect.x, (int)rect.y, (int)rect.width / 2,
                          (int)rect.height, {231, 135, 28, 180});
        }

        int mx = (int)(rect.x + rect.width / 2);
        int my = (int)(rect.y + rect.height / 2);
        if (l1[0]) {
            int w_l1 = MeasureText(l1, 8);
            int w_l2 = MeasureText(l2, 8);
            drawPixelText(l1, mx - w_l1 / 2, my - 16, 8, col);
            drawPixelText(l2, mx - w_l2 / 2, my + 2, 8, col);
        }
        drawPixelText(tile.kode, (int)rect.x + 4, (int)rect.y + 4, 6,
                      C_TEXT_DIM);
        DrawRectangleLinesEx(rect, 2, C_BORDER);
        return;
    }

    if (!tile.colorGroup.empty()) {
        Color sc = getTileColor(tile.colorGroup);
        float ss = 18.0f;
        Rectangle sr;
        if (isSideBtm)
            sr = {rect.x, rect.y, rect.width, ss};
        else if (isSideTop)
            sr = {rect.x, rect.y + rect.height - ss, rect.width, ss};
        else if (isSideLft)
            sr = {rect.x + rect.width - ss, rect.y, ss, rect.height};
        else
            sr = {rect.x, rect.y, ss, rect.height};
        DrawRectangleRec(sr, sc);
        DrawRectangleLinesEx(sr, 1, C_BORDER);
    }

    int fs = 7;
    int tx = (int)rect.x + 3, ty = (int)rect.y + 3;
    if (isSideBtm)
        ty = (int)rect.y + 22;
    else if (isSideRgt)
        tx = (int)rect.x + 22;

    drawPixelText(tile.kode, tx, ty, fs, C_TEXT);

    const char *secLabel = nullptr;
    Color secColor = C_TEXT_DIM;
    if (tile.kode == "KSP") {
        secLabel = "?";
        secColor = C_WARN;
    } else if (tile.kode == "DNU") {
        secLabel = "CC";
        secColor = {38, 89, 204, 255};
    } else if (tile.kode == "PPH") {
        secLabel = "TAX";
        secColor = C_DANGER;
    } else if (tile.kode == "PBM") {
        secLabel = "TAX";
        secColor = C_DANGER;
    } else if (tile.kode == "FES") {
        secLabel = "FES";
        secColor = C_WARN;
    } else if (tile.kode == "GBR" || tile.kode == "STB" || tile.kode == "TUG" ||
               tile.kode == "GUB") {
        secLabel = "ST";
        secColor = C_TEXT_DIM;
    } else if (tile.kode == "PLN" || tile.kode == "PAM") {
        secLabel = "UTL";
        secColor = C_TEXT_DIM;
    }

    if (secLabel)
        drawPixelText(secLabel, tx, ty + fs + 2, fs, secColor);

    if (tile.price > 0) {
        std::string ps = "M" + std::to_string(tile.price);
        int py = (int)(rect.y + rect.height) - fs - 3;
        if (isSideTop)
            py = ty + fs + (secLabel ? fs + 4 : 2);
        drawPixelText(ps, tx, py, fs - 1, C_GOOD);
    }

    if (tile.propStatus == "MORTGAGED")
        DrawRectangleRec(rect, {C_WARN.r, C_WARN.g, C_WARN.b, 55});

    if (tile.hasHotel) {
        int hx = 0, hy = 0, hw = 0, hh = 0;
        if (isSideBtm) {
            hx = (int)rect.x + 2;
            hy = (int)rect.y + 2;
            hw = (int)rect.width - 4;
            hh = 10;
        } else if (isSideTop) {
            hx = (int)rect.x + 2;
            hy = (int)rect.y + (int)rect.height - 12;
            hw = (int)rect.width - 4;
            hh = 10;
        } else if (isSideLft) {
            hx = (int)rect.x + (int)rect.width - 12;
            hy = (int)rect.y + 2;
            hw = 10;
            hh = (int)rect.height - 4;
        } else if (isSideRgt) {
            hx = (int)rect.x + 2;
            hy = (int)rect.y + 2;
            hw = 10;
            hh = (int)rect.height - 4;
        }
        DrawRectangle(hx, hy, hw, hh, C_DANGER);
        DrawRectangleLines(hx, hy, hw, hh, C_BORDER);
    } else
        for (int h = 0; h < tile.houseCount && h < 4; h++) {
            int hx = 0, hy = 0;
            if (isSideBtm) {
                hx = (int)rect.x + 2 + h * 10;
                hy = (int)rect.y + 2;
            } else if (isSideTop) {
                hx = (int)rect.x + 2 + h * 10;
                hy = (int)rect.y + (int)rect.height - 11;
            } else if (isSideLft) {
                hx = (int)rect.x + (int)rect.width - 11;
                hy = (int)rect.y + 2 + h * 10;
            } else if (isSideRgt) {
                hx = (int)rect.x + 2;
                hy = (int)rect.y + 2 + h * 10;
            }
            DrawRectangle(hx, hy, 8, 8, C_GOOD);
            DrawRectangleLines(hx, hy, 8, 8, {13, 72, 24, 255});
        }

    if (tile.propStatus == "MORTGAGED") {
        int mx = (int)rect.x + (int)rect.width - 15, my = (int)rect.y + 2;
        DrawRectangle(mx, my, 13, 9, C_WARN);
        DrawRectangleLines(mx, my, 13, 9, C_BORDER);
        drawPixelText("M", mx + 3, my + 1, 7, WHITE);
    }

    if (!tile.ownerName.empty() && tile.propStatus == "OWNED") {
        int oi = -1;
        for (int i = 0; i < (int)state.players.size(); i++)
            if (state.players[i].username == tile.ownerName) {
                oi = i;
                break;
            }
        if (oi >= 0) {
            Color oc = getPlayerColor(oi);
            int dx = 0, dy = 0;
            if (isSideBtm) {
                dx = (int)rect.x + (int)rect.width - 8;
                dy = (int)rect.y + 24;
            } else if (isSideTop) {
                dx = (int)rect.x + (int)rect.width - 8;
                dy = (int)rect.y + (int)rect.height - 24;
            } else if (isSideLft) {
                dx = (int)rect.x + (int)rect.width - 24;
                dy = (int)rect.y + 8;
            } else if (isSideRgt) {
                dx = (int)rect.x + 24;
                dy = (int)rect.y + 8;
            }
            DrawCircle(dx, dy, 5, oc);
            DrawCircleLines(dx, dy, 5, C_BORDER);
        }
    }

    DrawRectangleLinesEx(rect, 1, {C_BORDER.r, C_BORDER.g, C_BORDER.b, 110});
}

void GameWindow::drawPlayerPawns() {
    std::map<int, std::vector<int>> posMap;
    for (int i = 0; i < (int)state.players.size(); i++)
        if (state.players[i].status != "BANKRUPT")
            posMap[state.players[i].position].push_back(i);

    for (auto &[pos, indices] : posMap) {
        int sp = (pos >= 1 && pos <= 40) ? pos : 1;
        Rectangle r = getTileRect(sp);
        for (int j = 0; j < (int)indices.size(); j++) {
            Color pc = getPlayerColor(indices[j]);

            float ox = (j % 2) * 20.0f, oy = (j / 2) * 20.0f;
            int px = (int)(r.x + r.width - 20 - ox);
            int py = (int)(r.y + r.height - 20 - oy);
            drawPion(gAssets, px, py, 18, pc);
        }
    }
}

void GameWindow::drawPlayerList() {
    int sx = (int)sidebarRect.x, sy = (int)sidebarRect.y,
        sw = (int)sidebarRect.width;
    int startY = sy + 64 + 3 + 40 + 2 + 10;
    drawPixelText("PEMAIN", sx + 14, startY, 8, C_TEXT_DIM);
    int rowH = 36, gap = 6;
    int maxRows = std::min(4, (int)state.players.size());
    for (int i = 0; i < maxRows; i++) {
        const PlayerInfo &p = state.players[i];
        int ry = startY + 14 + i * (rowH + gap);
        bool isActive = (i == state.currentPlayerIndex);
        bool isBankrupt = (p.status == "BANKRUPT");
        Color rowBg =
            isActive ? C_ACCENT_BG
                     : (isBankrupt ? Color{230, 225, 210, 255} : C_PANEL_ALT);
        DrawRectangle(sx + 6, ry + 3, sw - 12, rowH, C_SHADOW);
        DrawRectangle(sx + 12, ry, sw - 24, rowH, rowBg);
        DrawRectangleLinesEx(
            {(float)(sx + 12), (float)ry, (float)(sw - 24), (float)rowH},
            isActive ? 2 : 1, C_BORDER);

        drawPion(gAssets, sx + 18, ry + rowH / 2 - 8, 16, getPlayerColor(i));
        Color nc = isActive ? Color{29, 93, 39, 255}
                            : (isBankrupt ? Color{130, 120, 100, 255} : C_TEXT);
        drawPixelText("P" + std::to_string(i + 1) + " " +
                          truncate(p.username, 8),
                      sx + 44, ry + 11, 10, nc);
        std::string ms = formatMoney(p.money);
        int ms_w = MeasureText(ms.c_str(), 10);
        drawPixelText(ms, sx + sw - 24 - ms_w - 44, ry + 11, 10,
                      isBankrupt ? Color{140, 125, 100, 255} : C_GOOD);
        drawPixelText("@" + std::to_string(p.position), sx + sw - 42, ry + 13,
                      8, C_TEXT_DIM);
        if (p.status == "JAILED") {
            int bx = sx + 170, by = ry + 9;
            std::string jlbl = "JAIL(" + std::to_string(p.jailTurnsLeft) + ")";
            int jlw = MeasureText(jlbl.c_str(), 7) + 10;
            DrawRectangle(bx, by, jlw, 18, C_DANGER);
            DrawRectangleLines(bx, by, jlw, 18, C_BORDER);
            drawPixelText(jlbl, bx + 5, by + 5, 7, WHITE);
        } else if (p.status == "BANKRUPT") {
            int bx = sx + 150, by = ry + 9;
            DrawRectangle(bx, by, 74, 18, {90, 90, 90, 255});
            DrawRectangleLines(bx, by, 74, 18, C_BORDER);
            drawPixelText("BANGKRUT", bx + 5, by + 5, 7, WHITE);
        }
    }
}

void GameWindow::drawHandCards() {
    int sx = (int)sidebarRect.x, sy = (int)sidebarRect.y,
        sw = (int)sidebarRect.width;
    int players = (int)state.players.size();
    int y = sy + 64 + 3 + 40 + 2 + 10 + 14 + players * (36 + 6) + 8 + 40 + 10;
    drawPixelText("KARTU DI TANGAN", sx + 14, y, 8, C_TEXT_DIM);
    y += 14;
    if (state.players.empty())
        return;
    const PlayerInfo &active = state.players[state.currentPlayerIndex];
    if (active.handCards.empty()) {
        drawPixelText("(kosong)", sx + 14, y + 8, 8, C_TEXT_DIM);
        return;
    }
    static const Color pips[] = {
        {128, 51, 230, 255}, {38, 140, 89, 255}, {204, 128, 25, 255}};
    int cardH = 40, gap = 6;
    for (int i = 0; i < (int)active.handCards.size() && i < 3; i++) {
        const CardInfo &card = active.handCards[i];
        int ry = y + i * (cardH + gap);
        DrawRectangle(sx + 15, ry + 3, sw - 24, cardH, C_SHADOW);
        DrawRectangle(sx + 12, ry, sw - 24, cardH, C_PANEL_ALT);
        DrawRectangleLinesEx(
            {(float)(sx + 12), (float)ry, (float)(sw - 24), (float)cardH}, 2,
            C_BORDER);
        DrawRectangle(sx + 12, ry, 8, cardH, pips[i % 3]);
        drawPixelText(truncate(card.name, 14), sx + 30, ry + 8, 9, C_TEXT);
        drawPixelText(truncate(card.description, 28), sx + 30, ry + 22, 8,
                      C_TEXT_DIM);
        Rectangle ur = {(float)(sx + sw - 24 - 72), (float)(ry + 6), 66.0f,
                        (float)(cardH - 12)};
        bool hov = isHovered(ur);
        drawButton("PAKAI", ur, C_BTN_BG, C_BTN_TEXT, hov);
        if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            auto it =
                commandCallbacks.find("GUNAKAN_KEMAMPUAN_" + std::to_string(i));
            if (it != commandCallbacks.end())
                it->second();
        }
    }
}

void GameWindow::drawLog() {
    int sx = (int)sidebarRect.x, sy = (int)sidebarRect.y,
        sw = (int)sidebarRect.width;
    int numCards =
        state.players.empty()
            ? 0
            : std::min(3, (int)state.players[state.currentPlayerIndex]
                              .handCards.size());
    int handH = numCards > 0 ? numCards * (40 + 6) : 20;
    int players = (int)state.players.size();
    int y = sy + 64 + 3 + 40 + 2 + 10 + 14 + players * (36 + 6) + 8 + 40 + 10 +
            14 + handH + 8;
    drawPixelText("LOG TERAKHIR", sx + 14, y, 8, C_TEXT_DIM);
    y += 14;
    int show = std::min(5, (int)state.logs.size());
    int from = (int)state.logs.size() - show;
    for (int i = 0; i < show; i++) {
        const LogInfo &log = state.logs[from + i];
        int ry = y + i * (22 + 2);
        bool lat = (i == show - 1);
        if (lat) {
            DrawRectangle(sx + 12, ry, sw - 24, 22, C_ACCENT_BG);
            DrawRectangle(sx + 12, ry, 3, 22, C_ACCENT);
        }
        drawPixelText("T" + std::to_string(log.turn), sx + 20, ry + 7, 9,
                      lat ? C_ACCENT : C_TEXT_DIM);
        std::string logLine = "[" + log.actionType + "] " + log.username + ": " + log.detail;
        drawPixelText(truncate(logLine, 34), sx + 50,
                      ry + 7, 8, lat ? C_TEXT : C_TEXT_DIM);
    }
}

void GameWindow::drawSidebar() {
    DrawRectangleRec(sidebarRect, C_PANEL);
    DrawRectangleLinesEx(sidebarRect, 3, C_BORDER);
    int sx = (int)sidebarRect.x, sy = (int)sidebarRect.y,
        sw = (int)sidebarRect.width;

    DrawRectangle(sx, sy, sw, 64, C_BTN_BG);
    drawPixelText("pOOPs: NIMONSPOLI", sx + 14, sy + 12, 16, C_BTN_TEXT);
    drawPixelText("TURN " + std::to_string(state.currentTurn) + " / " +
                      std::to_string(state.maxTurn),
                  sx + 14, sy + 40, 9, {216, 200, 154, 255});
    drawPixelText("v1.0", sx + sw - 40, sy + 8, 7, {122, 104, 64, 255});
    DrawRectangle(sx, sy + 64, sw, 3, C_BORDER);

    int by = sy + 67, bh = 40;
    DrawRectangle(sx, by, sw, bh, C_ACCENT_BG);
    std::string active = state.players.empty()
                             ? "?"
                             : state.players[state.currentPlayerIndex].username;
    Color ac = state.players.empty() ? C_ACCENT
                                     : getPlayerColor(state.currentPlayerIndex);

    drawPion(gAssets, sx + 14, by + bh / 2 - 10, 20, ac);
    drawPixelText("GILIRAN: " + active, sx + 40, by + 13, 10,
                  {29, 93, 39, 255});
    DrawRectangle(sx, by + bh, sw, 2, C_BORDER);

    drawPlayerList();

    {
        int players2 = (int)state.players.size();
        int diceY = sy + 64 + 3 + 40 + 2 + 10 + 14 + players2 * (36 + 6) + 8;
        int h = 48;
        DrawRectangle(sx + 15, diceY + 3, sw - 24, h, C_SHADOW);
        DrawRectangle(sx + 12, diceY, sw - 24, h, C_PANEL_ALT);
        DrawRectangleLinesEx(
            {(float)(sx + 12), (float)diceY, (float)(sw - 24), (float)h}, 2,
            C_BORDER);
        drawPixelText("DADU", sx + 22, diceY + 18, 8, C_TEXT_DIM);
        int d1 = state.dice.dice1, d2 = state.dice.dice2;
        int dx = sx + sw - 24 - 110;
        if (d1 > 0)
            drawDie(gAssets, dx, diceY + 6, 36, d1, C_TEXT);
        if (d2 > 0)
            drawDie(gAssets, dx + 42, diceY + 6, 36, d2, C_TEXT);
        if (d1 > 0 && d2 > 0)
            drawPixelText("= " + std::to_string(d1 + d2), dx + 86, diceY + 18,
                          10, C_TEXT);
        else
            drawPixelText("-.-", dx + 4, diceY + 18, 10, C_TEXT_DIM);
    }

    drawHandCards();
    drawLog();

    {
        int sh2 = (int)sidebarRect.height;
        int footY = sy + sh2 - 82;
        DrawRectangle(sx, footY, sw, 2, C_BORDER);
        Rectangle logBtn = {(float)(sx + 14), (float)(footY + 8),
                            (float)(sw - 28), 28.0f};
        bool lhov = isHovered(logBtn);
        drawButton(">> CETAK LOG LENGKAP", logBtn, C_PANEL_ALT, C_TEXT_DIM,
                   lhov);
        if (lhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            auto it = commandCallbacks.find("CETAK_LOG");
            if (it != commandCallbacks.end())
                it->second();
        }
        int saveY = footY + 42;
        float totalW = (float)(sw - 28);
        float gap3 = 4.0f;
        float bw3 = (totalW - 2 * gap3) / 3.0f;
        Rectangle sr = {(float)(sx + 14), (float)saveY, bw3, 30.0f};
        Rectangle lr = {(float)(sx + 14) + bw3 + gap3, (float)saveY, bw3, 30.0f};
        Rectangle er = {(float)(sx + 14) + 2 * (bw3 + gap3), (float)saveY, bw3, 30.0f};
        drawButton("SIMPAN", sr, {244, 223, 170, 255}, C_WARN, isHovered(sr));
        drawButton("MUAT", lr, C_PANEL_ALT, C_TEXT, isHovered(lr));
        drawButton("KELUAR", er, C_DANGER, WHITE, isHovered(er));
        if (isHovered(sr) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            auto it = commandCallbacks.find("SIMPAN");
            if (it != commandCallbacks.end()) it->second();
        }
        if (isHovered(lr) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            auto it = commandCallbacks.find("MUAT");
            if (it != commandCallbacks.end()) it->second();
        }
        if (isHovered(er) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            auto it = commandCallbacks.find("KELUAR");
            if (it != commandCallbacks.end()) it->second();
        }
    }
}

void GameWindow::drawCommandBar() {
    DrawRectangleRec(cmdBarRect, C_PANEL);
    DrawRectangleLinesEx(cmdBarRect, 3, C_BORDER);
    int cx = (int)cmdBarRect.x, cy = (int)cmdBarRect.y;
    drawPixelText("PERINTAH", cx + 12, cy + 6, 8, C_TEXT_DIM);

    struct CmdBtn {
        std::string label, iconKey, cmd;
        Color bg, fg;
    };
    static const std::vector<CmdBtn> buttons = {
        {"LEMPAR\nDADU", "cmd_lempar_dadu", "LEMPAR_DADU", C_BTN_BG,
         C_BTN_TEXT},
        {"BELI", "cmd_beli", "BELI", C_PANEL_ALT, C_TEXT},
        {"GADAI", "cmd_gadai", "GADAI", C_PANEL_ALT, C_TEXT},
        {"TEBUS", "cmd_tebus", "TEBUS", C_PANEL_ALT, C_TEXT},
        {"BANGUN", "cmd_bangun", "BANGUN", C_PANEL_ALT, C_TEXT},
        {"CETAK\nPAPAN", "cmd_cetak_papan", "CETAK_PAPAN", C_PANEL_ALT, C_TEXT},
        {"CETAK\nAKTA", "cmd_cetak_akta", "CETAK_AKTA", C_PANEL_ALT, C_TEXT},
        {"CETAK\nPROPERTI", "cmd_cetak_properti", "CETAK_PROPERTI", C_PANEL_ALT,
         C_TEXT},
        {"AKHIRI\nGILIRAN",
         "cmd_akhiri",
         "AKHIRI_GILIRAN",
         {106, 22, 22, 255},
         {255, 214, 214, 255}},
    };

    float gap = 8.0f, btnH = 56.0f, startX = cx + 12.0f, btnY = cy + 20.0f;
    int nb = (int)buttons.size();
    float btnW = (CMD_W - 24.0f - (nb - 1) * gap) / (float)nb;

    for (int i = 0; i < nb; i++) {
        const CmdBtn &b = buttons[i];
        Rectangle bRect = {startX + i * (btnW + gap), btnY, btnW, btnH};
        bool hov = isHovered(bRect);

        DrawRectangleRec({bRect.x + 3, bRect.y + 3, bRect.width, bRect.height},
                         C_SHADOW);
        DrawRectangleRec(bRect, hov ? tint(b.bg, 15) : b.bg);
        DrawRectangleLinesEx(bRect, 2, C_BORDER);

        Texture2D *iconTex = gAssets.get(b.iconKey);
        float iconY = bRect.y + 5.0f;
        if (iconTex) {
            float iconSz = std::min(20.0f, btnW * 0.5f);
            Rectangle iconRect = {bRect.x + (bRect.width - iconSz) / 2.0f,
                                  iconY, iconSz, iconSz};
            drawIconInRect(iconTex, iconRect, b.fg);
        }

        std::string l1 = b.label, l2;
        size_t nl = b.label.find('\n');
        if (nl != std::string::npos) {
            l1 = b.label.substr(0, nl);
            l2 = b.label.substr(nl + 1);
        }
        int fs = 6;
        int tw1 = MeasureText(l1.c_str(), fs);
        if (l2.empty()) {
            drawPixelText(l1, (int)(bRect.x + (bRect.width - tw1) / 2),
                          (int)(bRect.y + 36), fs, b.fg);
        } else {
            int tw2 = MeasureText(l2.c_str(), fs);
            drawPixelText(l1, (int)(bRect.x + (bRect.width - tw1) / 2),
                          (int)(bRect.y + 30), fs, b.fg);
            drawPixelText(l2, (int)(bRect.x + (bRect.width - tw2) / 2),
                          (int)(bRect.y + 39), fs, b.fg);
        }

        if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            auto it = commandCallbacks.find(b.cmd);
            if (it != commandCallbacks.end())
                it->second();
        }
    }
}

void GameWindow::drawPopup() {
    bool useList = (int)popup.options.size() > 3;

    DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 150});
    float pw = useList ? 460.0f : 380.0f;
    float ph = useList ? 480.0f : 260.0f;
    float px = (screenW - pw) / 2.0f, py = (screenH - ph) / 2.0f;
    Rectangle popRect = {px, py, pw, ph};
    DrawRectangleRec({px + 6, py + 6, pw, ph}, {0, 0, 0, 120});
    DrawRectangleRec(popRect, C_PANEL);
    DrawRectangleLinesEx(popRect, 3, C_BORDER);

    Color hc = C_BTN_BG;
    if (popup.type == PopupType::BUY_PROPERTY) hc = {27, 70, 183, 255};
    if (popup.type == PopupType::AUCTION)      hc = C_WARN;
    if (popup.type == PopupType::WINNER)       hc = C_ACCENT;
    if (popup.type == PopupType::JAIL)         hc = C_DANGER;

    DrawRectangle((int)px, (int)py, (int)pw, 44, hc);
    drawPixelText(popup.title, (int)px + 14, (int)py + 16, 12, WHITE);

    if (!useList) {
        drawPixelText(popup.message, (int)px + 14, (int)py + 60, 10, C_TEXT);
        float obh = 34;
        float obw = (pw - 28 - (popup.options.size() - 1) * 8.0f) /
                    std::max(1, (int)popup.options.size());
        for (int i = 0; i < (int)popup.options.size(); i++) {
            Rectangle or_ = {px + 14.0f + i * (obw + 8), py + ph - obh - 14,
                              obw, obh};
            bool hov = isHovered(or_);
            drawButton(popup.options[i], or_,
                       i == 0 ? C_BTN_BG : C_PANEL_ALT,
                       i == 0 ? C_BTN_TEXT : C_TEXT, hov);
            if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                popup.selectedIndex = i;
                if (popupCallback) popupCallback(i);
            }
        }
    } else {
        // Scrollable list popup
        drawPixelText(popup.message, (int)px + 14, (int)py + 55, 9, C_TEXT_DIM);

        int visItems = 8;
        float itemH = 32.0f;
        float listY = py + 78;
        float listH = visItems * itemH;

        // Mouse wheel scroll
        float wheel = GetMouseWheelMove();
        popupScrollOffset -= (int)wheel;
        int maxScroll = std::max(0, (int)popup.options.size() - visItems);
        popupScrollOffset = std::max(0, std::min(popupScrollOffset, maxScroll));

        // Clip drawing to list area
        BeginScissorMode((int)(px + 8), (int)listY, (int)(pw - 16), (int)listH);
        for (int i = 0; i < (int)popup.options.size(); i++) {
            int vi = i - popupScrollOffset;
            if (vi < 0 || vi >= visItems) continue;
            float iy = listY + vi * itemH;
            Rectangle ir = {px + 8, iy, pw - 16, itemH - 2};
            bool hov = isHovered(ir);
            DrawRectangleRec(ir, hov ? C_ACCENT_BG : (i % 2 == 0 ? C_PANEL : C_PANEL_ALT));
            DrawRectangleLinesEx(ir, 1, C_BORDER);
            drawPixelText(truncate(popup.options[i], 44), (int)(px + 18),
                          (int)(iy + (itemH - 2 - 9) / 2), 9, C_TEXT);
            if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                popup.selectedIndex = i;
                if (popupCallback) popupCallback(i);
            }
        }
        EndScissorMode();

        // Scroll indicator
        if (maxScroll > 0) {
            drawPixelText("^ scroll mouse ^", (int)(px + pw - 120),
                          (int)(listY + listH + 4), 7, C_TEXT_DIM);
        }

        // BATAL button
        float bBY = py + ph - 42;
        Rectangle bBtnR = {px + 14, bBY, pw - 28, 32};
        bool bHov = isHovered(bBtnR);
        DrawRectangleRec(bBtnR, bHov ? tint(C_PANEL_ALT, 20) : C_PANEL_ALT);
        DrawRectangleLinesEx(bBtnR, 2, C_BORDER);
        const char *bLbl = "BATAL";
        int bLw = MeasureText(bLbl, 10);
        drawPixelText(bLbl, (int)(bBtnR.x + (bBtnR.width - bLw) / 2),
                      (int)(bBtnR.y + (bBtnR.height - 10) / 2), 10, C_TEXT);
        if (bHov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (popupCallback) popupCallback(-1);
        }
    }
}
