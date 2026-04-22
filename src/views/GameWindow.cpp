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
  boardRect = {(float)BOARD_X, (float)BOARD_Y, (float)BOARD_W, (float)BOARD_H};
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

  std::string tBig =
      std::to_string(state.currentTurn) + " / " + std::to_string(state.maxTurn);
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
    drawPixelText(tile.kode, (int)rect.x + 4, (int)rect.y + 4, 6, C_TEXT_DIM);
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
    Color rowBg = isActive
                      ? C_ACCENT_BG
                      : (isBankrupt ? Color{230, 225, 210, 255} : C_PANEL_ALT);
    DrawRectangle(sx + 6, ry + 3, sw - 12, rowH, C_SHADOW);
    DrawRectangle(sx + 12, ry, sw - 24, rowH, rowBg);
    DrawRectangleLinesEx(
        {(float)(sx + 12), (float)ry, (float)(sw - 24), (float)rowH},
        isActive ? 2 : 1, C_BORDER);

    drawPion(gAssets, sx + 18, ry + rowH / 2 - 8, 16, getPlayerColor(i));
    Color nc = isActive ? Color{29, 93, 39, 255}
                        : (isBankrupt ? Color{130, 120, 100, 255} : C_TEXT);
    drawPixelText("P" + std::to_string(i + 1) + " " + truncate(p.username, 8),
                  sx + 44, ry + 11, 10, nc);
    std::string ms = formatMoney(p.money);
    int ms_w = MeasureText(ms.c_str(), 10);
    drawPixelText(ms, sx + sw - 24 - ms_w - 44, ry + 11, 10,
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
  drawPixelText("GILIRAN: " + active, sx + 40, by + 13, 10, {29, 93, 39, 255});
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
      drawPixelText("= " + std::to_string(d1 + d2), dx + 86, diceY + 18, 10,
                    C_TEXT);
    else
      drawPixelText("-.-", dx + 4, diceY + 18, 10, C_TEXT_DIM);
  }

  drawHandCards();
  drawLog();

  {
    int sh2 = (int)sidebarRect.height;
    int footY = sy + sh2 - 82;
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
    std::string label, iconKey, cmd;
    Color bg, fg;
  };
  static const std::vector<CmdBtn> buttons = {
      {"LEMPAR\nDADU", "cmd_lempar_dadu", "LEMPAR_DADU", C_BTN_BG, C_BTN_TEXT},
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
      Rectangle iconRect = {bRect.x + (bRect.width - iconSz) / 2.0f, iconY,
                            iconSz, iconSz};
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
