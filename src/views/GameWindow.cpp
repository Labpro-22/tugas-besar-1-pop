#include "../../include/views/GameWindow.hpp"
#include <algorithm>
#include <cstring>
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
  auto cl = [](int v) { return (unsigned char)std::max(0, std::min(255, v)); };
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
    SetTextureFilter(t, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    tex[key] = t;
  }
  void loadAll() {
    if (loaded)
      return;
    loadPng("tile_go", "assets/tile_go.png");
    loadPng("tile_jail", "assets/tile_jail.png");
    loadPng("tile_free_parking", "assets/tile_free_parking.png");
    loadPng("tile_go_to_jail", "assets/tile_go_to_jail.png");
    loadPng("tile_chance", "assets/tile_chance.png");
    loadPng("tile_community_chest", "assets/tile_community_chest.png");
    loadPng("tile_income_tax", "assets/tile_income_tax.png");
    loadPng("tile_luxury_tax", "assets/tile_luxury_tax.png");
    loadPng("tile_station", "assets/tile_station.png");
    loadPng("tile_electric", "assets/tile_electric.png");
    loadPng("tile_waterworks", "assets/tile_waterworks.png");
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

static void drawMiniDie(Font &font, bool fontLoaded, int x, int y, int value) {
  const int sz = 32;
  DrawRectangle(x, y, sz, sz, WHITE);
  DrawRectangleLinesEx({(float)x, (float)y, (float)sz, (float)sz}, 2,
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
  int pad = 6, cell = (sz - 2 * pad) / 3;
  for (int i = 0; i < 9; i++) {
    if (!p[i])
      continue;
    DrawCircle(x + pad + (i % 3) * cell + cell / 2,
               y + pad + (i / 3) * cell + cell / 2, 3, {43, 32, 22, 255});
  }
}

static void drawChessPawn(int x, int y, Color fill) {
  Color brd = {58, 42, 24, 255};
  DrawCircle(x + 10, y + 5, 4, fill);
  DrawCircleLines(x + 10, y + 5, 4, brd);
  Vector2 v1 = {(float)(x + 5), (float)(y + 18)};
  Vector2 v2 = {(float)(x + 15), (float)(y + 18)};
  Vector2 v3 = {(float)(x + 10), (float)(y + 9)};
  DrawTriangle(v1, v3, v2, fill);
  DrawTriangleLines(v1, v3, v2, brd);
  DrawRectangle(x + 3, y + 18, 14, 4, fill);
  DrawRectangleLines(x + 3, y + 18, 14, 4, brd);
}

static void drawIcon(const std::string &kind, int x, int y, Color c) {
  if (kind == "dice") {
    DrawRectangleLines(x, y, 20, 20, c);
    DrawCircle(x + 6, y + 6, 2, c);
    DrawCircle(x + 14, y + 14, 2, c);
    DrawCircle(x + 14, y + 6, 2, c);
    DrawCircle(x + 6, y + 14, 2, c);
  } else if (kind == "buy") {
    DrawRectangleLines(x + 3, y + 6, 14, 12, c);
    DrawLine(x + 6, y + 6, x + 8, y + 2, c);
    DrawLine(x + 14, y + 6, x + 12, y + 2, c);
    DrawLine(x + 8, y + 2, x + 12, y + 2, c);
  } else if (kind == "mort") {
    DrawRectangleLines(x + 3, y + 4, 14, 14, c);
    DrawText("M", x + 7, y + 7, 8, c);
  } else if (kind == "redeem") {
    DrawRectangleLines(x + 3, y + 4, 14, 14, c);
    DrawLine(x + 6, y + 15, x + 9, y + 18, c);
    DrawLine(x + 9, y + 18, x + 15, y + 12, c);
  } else if (kind == "build") {
    DrawRectangleLines(x + 2, y + 10, 7, 8, c);
    DrawRectangleLines(x + 11, y + 5, 7, 13, c);
  } else if (kind == "board") {
    DrawRectangleLines(x + 2, y + 2, 16, 16, c);
    DrawLine(x + 2, y + 8, x + 18, y + 8, c);
    DrawLine(x + 8, y + 2, x + 8, y + 18, c);
  } else if (kind == "deed") {
    DrawRectangleLines(x + 4, y + 2, 13, 16, c);
    DrawLine(x + 7, y + 9, x + 14, y + 9, c);
    DrawLine(x + 7, y + 13, x + 14, y + 13, c);
  } else if (kind == "prop") {
    DrawLine(x + 2, y + 10, x + 10, y + 3, c);
    DrawLine(x + 10, y + 3, x + 18, y + 10, c);
    DrawRectangleLines(x + 4, y + 9, 12, 10, c);
  } else if (kind == "end") {
    DrawCircleLines(x + 10, y + 10, 8, c);
    DrawLine(x + 6, y + 6, x + 14, y + 14, c);
    DrawLine(x + 14, y + 6, x + 6, y + 14, c);
  }
}

GameWindow::GameWindow(int width, int height, const std::string &title)
    : screenW(width), screenH(height), fontLoaded(false) {
  boardRect = {(float)BOARD_X, (float)BOARD_Y, (float)BOARD_W, (float)BOARD_H};
  sidebarRect = {(float)SIDE_X, (float)SIDE_Y, (float)SIDE_W, (float)SIDE_H};
  cmdBarRect = {(float)CMD_X, (float)CMD_Y, (float)CMD_W, (float)CMD_H};
  InitWindow(width, height, title.c_str());
  SetTargetFPS(60);
}

GameWindow::~GameWindow() {
  if (fontLoaded)
    UnloadFont(pixelFont);
  gAssets.unloadAll();
  CloseWindow();
}

void GameWindow::init() {
  if (FileExists("assets/PressStart2P.ttf")) {
    pixelFont = LoadFontEx("assets/PressStart2P.ttf", 32, nullptr, 0);
    SetTextureFilter(pixelFont.texture, TEXTURE_FILTER_POINT);
    fontLoaded = true;
  }
  gAssets.loadAll();
}

void GameWindow::updateState(const GameState &s) { state = s; }
void GameWindow::showPopup(const PopupState &p) { popup = p; }
void GameWindow::closePopup() { popup.type = PopupType::NONE; }
bool GameWindow::isRunning() const { return !WindowShouldClose(); }
void GameWindow::onCommand(const std::string &n, std::function<void()> cb) {
  commandCallbacks[n] = cb;
}
void GameWindow::onPopupOption(std::function<void(int)> cb) {
  popupCallback = cb;
}

void GameWindow::tick() {
  BeginDrawing();
  ClearBackground(C_BG);
  drawBoard();
  drawSidebar();
  drawCommandBar();
  if (popup.type != PopupType::NONE)
    drawPopup();
  EndDrawing();
}

void GameWindow::drawPixelText(const std::string &text, int x, int y, int size,
                               Color color) {
  if (fontLoaded)
    DrawTextEx(pixelFont, text.c_str(), {(float)x, (float)y}, (float)size, 1.0f,
               color);
  else
    DrawText(text.c_str(), x, y, size, color);
}

bool GameWindow::isHovered(Rectangle r) const {
  return CheckCollisionPointRec(GetMousePosition(), r);
}

void GameWindow::drawButton(const std::string &label, Rectangle rect, Color bg,
                            Color fg, bool hovered) {
  DrawRectangleRec({rect.x + 3, rect.y + 3, rect.width, rect.height}, C_SHADOW);
  DrawRectangleRec(rect, hovered ? tint(bg, 15) : bg);
  DrawRectangleLinesEx(rect, 2, C_BORDER);
  int fs = 10;
  std::string l1 = label, l2;
  size_t nl = label.find('\n');
  if (nl != std::string::npos) {
    l1 = label.substr(0, nl);
    l2 = label.substr(nl + 1);
  }
  auto tw = [&](const std::string &s) { return (int)s.size() * fs; };
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
  drawPixelText("NIMONSPOLI", lx - 110, ly - 110, 22, {166, 38, 26, 255});
  drawPixelText("TURN", lx - 24, ly - 70, 10, C_TEXT_DIM);

  std::string tBig =
      std::to_string(state.currentTurn) + " / " + std::to_string(state.maxTurn);
  drawPixelText(tBig, lx - (int)tBig.size() * 38 / 2, ly - 40, 38, C_TEXT);

  if (!state.players.empty()) {
    std::string gir =
        "GILIRAN > " + state.players[state.currentPlayerIndex].username;
    drawPixelText(gir, lx - (int)gir.size() * 9 / 2, ly + 20, 9, C_TEXT_DIM);
  }

  int d1 = state.dice.dice1, d2 = state.dice.dice2;
  if (d1 > 0 && d2 > 0) {
    drawMiniDie(pixelFont, fontLoaded, lx - 50, ly + 46, d1);
    drawMiniDie(pixelFont, fontLoaded, lx - 10, ly + 46, d2);
    drawPixelText("= " + std::to_string(d1 + d2), lx + 30, ly + 54, 10, C_TEXT);
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

  if (isCorner) {
    const char *key = nullptr;
    if (tile.kode == "GO")
      key = "tile_go";
    else if (tile.kode == "PEN" || tile.kode == "PENJARA")
      key = "tile_jail";
    else if (tile.kode == "BBP")
      key = "tile_free_parking";
    else if (tile.kode == "PPJ")
      key = "tile_go_to_jail";

    Texture2D *tex = key ? gAssets.get(key) : nullptr;
    if (tex) {
      DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height}, rect,
                     {0, 0}, 0.0f, WHITE);
    } else {
      DrawRectangleRec(rect, C_TILE_FACE);
      drawPixelText(tile.kode, (int)rect.x + 6, (int)rect.y + 30, 8, C_TEXT);
    }
    DrawRectangleLinesEx(rect, 1, C_BORDER);
    return;
  }

  const char *illKey = nullptr;
  if (tile.kode == "KSP")
    illKey = "tile_chance";
  else if (tile.kode == "DNU")
    illKey = "tile_community_chest";
  else if (tile.kode == "PPH")
    illKey = "tile_income_tax";
  else if (tile.kode == "PBM")
    illKey = "tile_luxury_tax";
  else if (tile.kode == "GBR" || tile.kode == "STB" || tile.kode == "TUG" ||
           tile.kode == "GUB")
    illKey = "tile_station";
  else if (tile.kode == "PLN")
    illKey = "tile_electric";
  else if (tile.kode == "PAM")
    illKey = "tile_waterworks";

  else if (tile.kode.find("CHANCE") != std::string::npos ||
           tile.kode.find("KSP") != std::string::npos)
    illKey = "tile_chance";
  else if (tile.kode.find("DNU") != std::string::npos ||
           tile.kode.find("DANA") != std::string::npos)
    illKey = "tile_community_chest";

  Texture2D *illTex = illKey ? gAssets.get(illKey) : nullptr;

  DrawRectangleRec(rect, C_TILE_FACE);

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

  if (illTex) {
    float pad = 3, nameH = 16;
    Rectangle face = {rect.x + pad, rect.y + pad, rect.width - 2 * pad,
                      rect.height - 2 * pad};
    if (isSideBtm) {
      face.y += nameH;
      face.height -= nameH;
    } else if (isSideTop) {
      face.height -= nameH;
    } else if (isSideLft) {
      face.width -= nameH;
    } else if (isSideRgt) {
      face.x += nameH;
      face.width -= nameH;
    }
    float asp = (float)illTex->width / illTex->height;
    float dw = face.width, dh = face.height;
    if (face.width / face.height > asp)
      dw = face.height * asp;
    else
      dh = face.width / asp;
    DrawTexturePro(*illTex, {0, 0, (float)illTex->width, (float)illTex->height},
                   {face.x + (face.width - dw) / 2,
                    face.y + (face.height - dh) / 2, dw, dh},
                   {0, 0}, 0.0f, WHITE);
  }

  if (tile.propStatus == "MORTGAGED")
    DrawRectangleRec(rect, {C_WARN.r, C_WARN.g, C_WARN.b, 55});

  int fs = 7, tx = (int)rect.x + 4, ty = (int)rect.y + 4;
  if (isSideBtm)
    ty = (int)rect.y + 22;
  else if (isSideRgt)
    tx = (int)rect.x + 22;
  drawPixelText(tile.kode, tx, ty, fs, C_TEXT);

  if (tile.price > 0) {
    int py = (int)(rect.y + rect.height) - fs - 5;
    if (isSideTop)
      py = (int)rect.y + 4 + fs + 3;
    drawPixelText("M" + std::to_string(tile.price), tx, py, fs - 1, C_GOOD);
  }

  if (tile.hasHotel) {
    if (isSideBtm)
      DrawRectangle((int)rect.x + 3, (int)rect.y + 3, (int)rect.width - 6, 12,
                    C_DANGER);
    else if (isSideTop)
      DrawRectangle((int)rect.x + 3, (int)rect.y + (int)rect.height - 15,
                    (int)rect.width - 6, 12, C_DANGER);
    else if (isSideLft)
      DrawRectangle((int)rect.x + (int)rect.width - 15, (int)rect.y + 3, 12,
                    (int)rect.height - 6, C_DANGER);
    else if (isSideRgt)
      DrawRectangle((int)rect.x + 3, (int)rect.y + 3, 12, (int)rect.height - 6,
                    C_DANGER);
  } else
    for (int h = 0; h < tile.houseCount && h < 4; h++) {
      int hx = 0, hy = 0;
      if (isSideBtm) {
        hx = (int)rect.x + 3 + h * 11;
        hy = (int)rect.y + 4;
      } else if (isSideTop) {
        hx = (int)rect.x + 3 + h * 11;
        hy = (int)rect.y + (int)rect.height - 13;
      } else if (isSideLft) {
        hx = (int)rect.x + (int)rect.width - 13;
        hy = (int)rect.y + 3 + h * 11;
      } else if (isSideRgt) {
        hx = (int)rect.x + 4;
        hy = (int)rect.y + 3 + h * 11;
      }
      DrawRectangle(hx, hy, 9, 9, C_GOOD);
      DrawRectangleLines(hx, hy, 9, 9, {13, 72, 24, 255});
    }

  if (tile.propStatus == "MORTGAGED") {
    int mx = (int)rect.x + (int)rect.width - 16, my = (int)rect.y + 2;
    DrawRectangle(mx, my, 14, 10, C_WARN);
    DrawRectangleLines(mx, my, 14, 10, C_BORDER);
    drawPixelText("M", mx + 3, my + 2, 7, WHITE);
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
        dx = (int)rect.x + (int)rect.width - 10;
        dy = (int)rect.y + 28;
      } else if (isSideTop) {
        dx = (int)rect.x + (int)rect.width - 10;
        dy = (int)rect.y + (int)rect.height - 28;
      } else if (isSideLft) {
        dx = (int)rect.x + (int)rect.width - 28;
        dy = (int)rect.y + 10;
      } else if (isSideRgt) {
        dx = (int)rect.x + 28;
        dy = (int)rect.y + 10;
      }
      DrawCircle(dx, dy, 6, oc);
      DrawCircleLines(dx, dy, 6, C_BORDER);
    }
  }

  DrawRectangleLinesEx(rect, 1, {C_BORDER.r, C_BORDER.g, C_BORDER.b, 120});
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
      float ox = (j % 2) * 13.0f, oy = (j / 2) * 14.0f;
      drawChessPawn((int)(r.x + r.width - 22 - ox),
                    (int)(r.y + r.height - 26 - oy),
                    getPlayerColor(indices[j]));
    }
  }
}

void GameWindow::drawPlayerList() {
  int sx = (int)sidebarRect.x, sy = (int)sidebarRect.y,
      sw = (int)sidebarRect.width;
  int startY = sy + 64 + 3 + 40 + 2 + 10;
  drawPixelText("PEMAIN", sx + 14, startY, 8, C_TEXT_DIM);
  int rowH = 36, gap = 6, maxRows = std::min(4, (int)state.players.size());
  for (int i = 0; i < maxRows; i++) {
    const PlayerInfo &p = state.players[i];
    int ry = startY + 14 + i * (rowH + gap);
    bool isActive = (i == state.currentPlayerIndex);
    bool isBankrupt = (p.status == "BANKRUPT");
    Color rowBg = isActive
                      ? C_ACCENT_BG
                      : (isBankrupt ? Color{230, 225, 210, 255} : C_PANEL_ALT);
    DrawRectangle(sx + 6, ry + 3, sw - 12, rowH, C_SHADOW);
    DrawRectangle(sx + 12, ry, sw - 24, rowH, rowBg);
    DrawRectangleLinesEx(
        {(float)(sx + 12), (float)ry, (float)(sw - 24), (float)rowH},
        isActive ? 2 : 1, C_BORDER);
    Color pc = getPlayerColor(i);
    DrawCircle(sx + 28, ry + rowH / 2, 7, pc);
    DrawCircleLines(sx + 28, ry + rowH / 2, 7, C_BORDER);
    Color nc = isActive ? Color{29, 93, 39, 255}
                        : (isBankrupt ? Color{130, 120, 100, 255} : C_TEXT);
    drawPixelText("P" + std::to_string(i + 1) + " " + truncate(p.username, 8),
                  sx + 44, ry + 11, 10, nc);
    std::string ms = formatMoney(p.money);
    drawPixelText(ms, sx + sw - 24 - (int)ms.size() * 10 - 44, ry + 11, 10,
                  isBankrupt ? Color{140, 125, 100, 255} : C_GOOD);
    drawPixelText("@" + std::to_string(p.position), sx + sw - 42, ry + 13, 8,
                  C_TEXT_DIM);
    if (p.status == "JAILED") {
      int bx = sx + 180, by = ry + 9;
      DrawRectangle(bx, by, 44, 18, C_DANGER);
      DrawRectangleLines(bx, by, 44, 18, C_BORDER);
      drawPixelText("JAIL", bx + 6, by + 5, 8, WHITE);
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
      auto it = commandCallbacks.find("GUNAKAN_KEMAMPUAN_" + std::to_string(i));
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
          : std::min(
                3,
                (int)state.players[state.currentPlayerIndex].handCards.size());
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
    drawPixelText(truncate(log.username + " " + log.detail, 30), sx + 50,
                  ry + 7, 8, lat ? C_TEXT : C_TEXT_DIM);
  }
}

void GameWindow::drawSidebar() {
  DrawRectangleRec(sidebarRect, C_PANEL);
  DrawRectangleLinesEx(sidebarRect, 3, C_BORDER);
  int sx = (int)sidebarRect.x, sy = (int)sidebarRect.y,
      sw = (int)sidebarRect.width;

  DrawRectangle(sx, sy, sw, 64, C_BTN_BG);
  drawPixelText("NIMONSPOLI", sx + 14, sy + 12, 16, C_BTN_TEXT);
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
  DrawCircle(sx + 22, by + bh / 2, 8, ac);
  DrawCircleLines(sx + 22, by + bh / 2, 8, C_BORDER);
  drawPixelText("GILIRAN: " + active, sx + 40, by + 13, 10, {29, 93, 39, 255});
  DrawRectangle(sx, by + bh, sw, 2, C_BORDER);

  drawPlayerList();

  {
    int players = (int)state.players.size();
    int diceY = sy + 64 + 3 + 40 + 2 + 10 + 14 + players * (36 + 6) + 8;
    int h = 40;
    DrawRectangle(sx + 15, diceY + 3, sw - 24, h, C_SHADOW);
    DrawRectangle(sx + 12, diceY, sw - 24, h, C_PANEL_ALT);
    DrawRectangleLinesEx(
        {(float)(sx + 12), (float)diceY, (float)(sw - 24), (float)h}, 2,
        C_BORDER);
    drawPixelText("DADU", sx + 22, diceY + 14, 8, C_TEXT_DIM);

    int d1 = state.dice.dice1, d2 = state.dice.dice2;
    int dx = sx + sw - 24 - 90;
    if (d1 > 0)
      drawMiniDie(pixelFont, fontLoaded, dx, diceY + 4, d1);
    if (d2 > 0)
      drawMiniDie(pixelFont, fontLoaded, dx + 38, diceY + 4, d2);
    if (d1 > 0 && d2 > 0)
      drawPixelText("= " + std::to_string(d1 + d2), dx + 78, diceY + 14, 10,
                    C_TEXT);
    else
      drawPixelText("- . -", dx, diceY + 14, 10, C_TEXT_DIM);
  }

  drawHandCards();
  drawLog();

  {
    int sh = (int)sidebarRect.height;
    int footY = sy + sh - 82;
    DrawRectangle(sx, footY, sw, 2, C_BORDER);
    Rectangle logBtn = {(float)(sx + 14), (float)(footY + 8), (float)(sw - 28),
                        28.0f};
    bool lhov = isHovered(logBtn);
    drawButton(">> CETAK LOG LENGKAP", logBtn, C_PANEL_ALT, C_TEXT_DIM, lhov);
    if (lhov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      auto it = commandCallbacks.find("CETAK_LOG");
      if (it != commandCallbacks.end())
        it->second();
    }
    int saveY = footY + 42;
    float hw = ((float)sw - 28 - 8) / 2.0f;
    Rectangle sr = {(float)(sx + 14), (float)saveY, hw, 30.0f};
    Rectangle lr = {(float)(sx + 14) + hw + 8, (float)saveY, hw, 30.0f};
    drawButton("[ SIMPAN ]", sr, {244, 223, 170, 255}, C_WARN, isHovered(sr));
    drawButton("[ MUAT ]", lr, C_PANEL_ALT, C_TEXT, isHovered(lr));
    if (isHovered(sr) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      auto it = commandCallbacks.find("SIMPAN");
      if (it != commandCallbacks.end())
        it->second();
    }
    if (isHovered(lr) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      auto it = commandCallbacks.find("MUAT");
      if (it != commandCallbacks.end())
        it->second();
    }
  }
}

void GameWindow::drawCommandBar() {
  DrawRectangleRec(cmdBarRect, C_PANEL);
  DrawRectangleLinesEx(cmdBarRect, 3, C_BORDER);
  int cx = (int)cmdBarRect.x, cy = (int)cmdBarRect.y;
  drawPixelText("PERINTAH", cx + 12, cy + 6, 8, C_TEXT_DIM);

  struct CmdBtn {
    std::string label, icon, cmd;
    Color bg, fg;
  };
  static const std::vector<CmdBtn> buttons = {
      {"LEMPAR\nDADU", "dice", "LEMPAR_DADU", C_BTN_BG, C_BTN_TEXT},
      {"BELI", "buy", "BELI", C_PANEL_ALT, C_TEXT},
      {"GADAI", "mort", "GADAI", C_PANEL_ALT, C_TEXT},
      {"TEBUS", "redeem", "TEBUS", C_PANEL_ALT, C_TEXT},
      {"BANGUN", "build", "BANGUN", C_PANEL_ALT, C_TEXT},
      {"CETAK\nPAPAN", "board", "CETAK_PAPAN", C_PANEL_ALT, C_TEXT},
      {"CETAK\nAKTA", "deed", "CETAK_AKTA", C_PANEL_ALT, C_TEXT},
      {"CETAK\nPROPERTI", "prop", "CETAK_PROPERTI", C_PANEL_ALT, C_TEXT},
      {"AKHIRI\nGILIRAN",
       "end",
       "AKHIRI_GILIRAN",
       {106, 22, 22, 255},
       {255, 214, 214, 255}},
  };

  float gap = 8, btnH = 56, startX = cx + 12.0f, btnY = cy + 20.0f;
  int nb = (int)buttons.size();
  float btnW = (CMD_W - 24.0f - (nb - 1) * gap) / (float)nb;

  for (int i = 0; i < nb; i++) {
    const CmdBtn &b = buttons[i];
    Rectangle br = {startX + i * (btnW + gap), btnY, btnW, btnH};
    bool hov = isHovered(br);
    DrawRectangleRec({br.x + 3, br.y + 3, br.width, br.height}, C_SHADOW);
    DrawRectangleRec(br, hov ? tint(b.bg, 15) : b.bg);
    DrawRectangleLinesEx(br, 2, C_BORDER);
    drawIcon(b.icon, (int)(br.x + br.width / 2 - 10), (int)br.y + 6, b.fg);
    std::string l1 = b.label, l2;
    size_t nl = b.label.find('\n');
    if (nl != std::string::npos) {
      l1 = b.label.substr(0, nl);
      l2 = b.label.substr(nl + 1);
    }
    int fs = 8, tw1 = (int)l1.size() * fs;
    if (l2.empty()) {
      drawPixelText(l1, (int)(br.x + (br.width - tw1) / 2),
                    (int)(br.y + br.height - fs - 8), fs, b.fg);
    } else {
      int tw2 = (int)l2.size() * fs;
      drawPixelText(l1, (int)(br.x + (br.width - tw1) / 2),
                    (int)(br.y + br.height - 2 * fs - 10), fs, b.fg);
      drawPixelText(l2, (int)(br.x + (br.width - tw2) / 2),
                    (int)(br.y + br.height - fs - 4), fs, b.fg);
    }
    if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      auto it = commandCallbacks.find(b.cmd);
      if (it != commandCallbacks.end())
        it->second();
    }
  }
}

void GameWindow::drawPopup() {
  DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 150});
  float pw = 380, ph = 260, px = (screenW - pw) / 2.0f,
        py = (screenH - ph) / 2.0f;
  Rectangle popRect = {px, py, pw, ph};
  DrawRectangleRec({px + 6, py + 6, pw, ph}, {0, 0, 0, 120});
  DrawRectangleRec(popRect, C_PANEL);
  DrawRectangleLinesEx(popRect, 3, C_BORDER);

  Color hc = C_BTN_BG;
  if (popup.type == PopupType::BUY_PROPERTY)
    hc = {27, 70, 183, 255};
  if (popup.type == PopupType::AUCTION)
    hc = C_WARN;
  if (popup.type == PopupType::WINNER)
    hc = C_ACCENT;
  if (popup.type == PopupType::JAIL)
    hc = C_DANGER;

  DrawRectangle((int)px, (int)py, (int)pw, 44, hc);
  drawPixelText(popup.title, (int)px + 14, (int)py + 16, 12, WHITE);
  drawPixelText(popup.message, (int)px + 14, (int)py + 60, 10, C_TEXT);

  float obh = 34, obw = (pw - 28 - (popup.options.size() - 1) * 8.0f) /
                        std::max(1, (int)popup.options.size());
  for (int i = 0; i < (int)popup.options.size(); i++) {
    Rectangle or_ = {px + 14.0f + i * (obw + 8), py + ph - obh - 14, obw, obh};
    bool hov = isHovered(or_);
    drawButton(popup.options[i], or_, i == 0 ? C_BTN_BG : C_PANEL_ALT,
               i == 0 ? C_BTN_TEXT : C_TEXT, hov);
    if (hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      popup.selectedIndex = i;
      if (popupCallback)
        popupCallback(i);
    }
  }
  drawPixelText("[ESC] tutup", (int)px + (int)pw - 100, (int)py + (int)ph - 18,
                7, C_TEXT_DIM);
  if (IsKeyPressed(KEY_ESCAPE))
    closePopup();
}
