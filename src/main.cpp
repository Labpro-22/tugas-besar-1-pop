// main.cpp — Controller: menghubungkan GameEngine <-> GameWindow (fully GUI)

#include "../include/core/ConfigLoader.hpp"
#include "../include/core/Exceptions.hpp"
#include "../include/core/GameEngine.hpp"
#include "../include/core/Player.hpp"
#include "../include/core/TransactionLogger.hpp"
#include "../include/models/ActionTile.hpp"
#include "../include/models/Board.hpp"
#include "../include/models/Card.hpp"
#include "../include/models/PropertyTile.hpp"
#include "../include/models/Tile.hpp"
#include "../include/views/GameWindow.hpp"

#include <algorithm>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

enum class Phase {
    MENU,
    AWAITING_ACTION, // Pemain bisa roll, use card, build, gadai, dll
    ROLLED,          // Dadu sudah dilempar, efek tile sedang diproses
    EFFECT_PENDING,  // Menunggu popup response (beli/tax/festival/dll)
    POST_ROLL,       // Setelah efek selesai, bisa build/gadai/end turn
    AUCTION_ACTIVE,  // Sedang dalam proses lelang
    GAME_OVER,
};

enum class Pending {
    NONE,
    BUY_PROPERTY,
    TAX_CHOICE,
    FESTIVAL,
    JAIL_CHOICE,
    DROP_CARD,
    GADAI_SELECT,
    TEBUS_SELECT,
    BANGUN_COLOR_SELECT,
    BANGUN_TILE_SELECT,
    SAVE_CONFIRM,
    EXIT_CONFIRM,
    AKTA_SELECT,      // Pilih properti untuk cetak akta
    PAPAN_INFO,       // Info papan permainan
    PROPERTI_INFO,    // Info properti pemain
};

// Global state
static Phase phase = Phase::MENU;
static Pending pending = Pending::NONE;
static int lastD1 = 0, lastD2 = 0;
static bool turnJustStarted = true;
static bool gameStarted = false;

// Pending context
static PropertyTile *pendingProp = nullptr;
static TaxTile *pendingTax = nullptr;

// Auction state
struct AuctionCtx {
    PropertyTile *property = nullptr;
    vector<Player *> bidders;
    int currentIdx = 0;
    int highestBid = 0;
    Player *highestBidder = nullptr;
    int consecutivePasses = 0;
    bool active = false;
};
static AuctionCtx auctionCtx;

// Bangun state
static vector<StreetTile *> bangunCandidates;
static string bangunColorGroup;

// Gadai/Tebus candidates
static vector<PropertyTile *> gadaiCandidates;

// Akta select candidates (all board properties)
static vector<PropertyTile *> aktaCandidates;

// ---- Formatters ----

static string fmtMoney(int amount) {
    string s = to_string(amount);
    int pos = (int)s.length() - 3;
    while (pos > 0) {
        s.insert(pos, ".");
        pos -= 3;
    }
    return "M" + s;
}

static string statusStr(PlayerStatus s) {
    switch (s) {
    case PlayerStatus::ACTIVE:   return "ACTIVE";
    case PlayerStatus::JAILED:   return "JAILED";
    case PlayerStatus::BANKRUPT: return "BANKRUPT";
    default:                     return "UNKNOWN";
    }
}

static string propStatusStr(int s) {
    switch (s) {
    case 0: return "BANK";
    case 1: return "OWNED";
    case 2: return "MORTGAGED";
    default: return "BANK";
    }
}

static string findOwnerName(int ownerId, const vector<Player *> &players) {
    for (Player *p : players)
        if (p->getId() == ownerId)
            return p->getUsername();
    return "";
}

static void updatePropertyCounts(Player *player) {
    int rrCount = 0, utCount = 0;
    for (PropertyTile *prop : player->getOwnedProperties()) {
        if (dynamic_cast<RailroadTile *>(prop)) rrCount++;
        if (dynamic_cast<UtilityTile *>(prop))  utCount++;
    }
    for (PropertyTile *prop : player->getOwnedProperties()) {
        if (auto *rr = dynamic_cast<RailroadTile *>(prop))
            rr->setrailroadOwnedCount(rrCount);
        if (auto *ut = dynamic_cast<UtilityTile *>(prop))
            ut->setUtilityOwnedCount(utCount);
    }
}

static void checkMonopoly(Player *player, Board *board) {
    map<string, int> totalInGroup;
    map<string, int> ownedInGroup;
    for (int i = 1; i <= board->getTotalTiles(); i++) {
        if (auto *st = dynamic_cast<StreetTile *>(board->getTileAt(i)))
            totalInGroup[st->getColorGroup()]++;
    }
    for (PropertyTile *prop : player->getOwnedProperties()) {
        if (auto *st = dynamic_cast<StreetTile *>(prop))
            ownedInGroup[st->getColorGroup()]++;
    }
    for (PropertyTile *prop : player->getOwnedProperties()) {
        if (auto *st = dynamic_cast<StreetTile *>(prop)) {
            string cg = st->getColorGroup();
            st->setMonopolized(ownedInGroup[cg] == totalInGroup[cg]);
        }
    }
}

// ---- Build GUI GameState ----

static GameState buildGameState(GameEngine &engine, TransactionLogger &logger) {
    GameState gs;
    Board *board = engine.getBoard();
    auto players = engine.getAllPlayers();

    for (int i = 0; i < (int)players.size(); i++) {
        Player *p = players[i];
        PlayerInfo pi;
        pi.username = p->getUsername();
        pi.money = p->getMoney();
        pi.position = p->getPosition() + 1;
        pi.status = statusStr(p->getStatus());
        pi.jailTurnsLeft = p->getJailTurnsLeft();
        pi.shieldActive = p->isShieldActive();
        pi.color = {0, 0, 0, 255};
        for (const SkillCard *card : p->getHandCards()) {
            CardInfo ci;
            ci.name = card->getCardName();
            ci.description = card->getCardDescription();
            ci.value = card->getValueString();
            pi.handCards.push_back(ci);
        }
        gs.players.push_back(pi);
    }

    for (int i = 1; i <= board->getTotalTiles(); i++) {
        Tile *tile = board->getTileAt(i);
        if (!tile) continue;

        TileInfo ti;
        ti.index = i;
        ti.kode = tile->getKode();
        ti.name = tile->getName();
        ti.price = 0;
        ti.houseCount = 0;
        ti.hasHotel = false;
        ti.festivalMult = 1;
        ti.colorGroup = "";
        ti.ownerName = "";
        ti.propStatus = "BANK";

        if (auto *st = dynamic_cast<StreetTile *>(tile)) {
            ti.tileType = "STREET";
            ti.colorGroup = st->getColorGroup();
            ti.price = st->getPrice();
            ti.propStatus = propStatusStr(st->getStatus());
            ti.ownerName = findOwnerName(st->getOwnerId(), players);
            int rl = st->getRentLevel();
            if (rl == 5) { ti.hasHotel = true; ti.houseCount = 0; }
            else          { ti.houseCount = rl; }
        } else if (auto *rr = dynamic_cast<RailroadTile *>(tile)) {
            ti.tileType = "RAILROAD";
            ti.propStatus = propStatusStr(rr->getStatus());
            ti.ownerName = findOwnerName(rr->getOwnerId(), players);
        } else if (auto *ut = dynamic_cast<UtilityTile *>(tile)) {
            ti.tileType = "UTILITY";
            ti.propStatus = propStatusStr(ut->getStatus());
            ti.ownerName = findOwnerName(ut->getOwnerId(), players);
        } else if (dynamic_cast<CardTile *>(tile)) {
            ti.tileType = "CARD";
        } else if (dynamic_cast<TaxTile *>(tile)) {
            ti.tileType = "TAX";
        } else if (dynamic_cast<FestivalTile *>(tile)) {
            ti.tileType = "FESTIVAL";
        } else {
            ti.tileType = "SPECIAL";
        }
        gs.tiles.push_back(ti);
    }

    gs.dice.dice1 = lastD1;
    gs.dice.dice2 = lastD2;
    gs.dice.isDouble = (lastD1 > 0 && lastD1 == lastD2);
    gs.dice.hasRolled = engine.hasDiceRolled();
    gs.currentPlayerIndex = engine.getCurrentTurnIdx();
    gs.currentTurn = engine.getCurrentRound();
    gs.maxTurn = ConfigLoader::getInstance()->getMaxTurn();
    gs.gameOver = (phase == Phase::GAME_OVER);
    gs.winnerName = "";

    auto logEntries = logger.getLogs(10);
    for (auto &le : logEntries) {
        LogInfo li;
        li.turn = le.turn;
        li.username = le.username;
        li.actionType = actionTypeToString(le.actionType);
        li.detail = le.detail;
        gs.logs.push_back(li);
    }
    return gs;
}

// ---- Auction helpers ----

static void startAuction(PropertyTile *prop, GameEngine &engine,
                         GameWindow &window) {
    auctionCtx.property = prop;
    auctionCtx.bidders.clear();
    auctionCtx.highestBid = 0;
    auctionCtx.highestBidder = nullptr;
    auctionCtx.consecutivePasses = 0;
    auctionCtx.active = true;

    auto players = engine.getAllPlayers();
    int startIdx = (engine.getCurrentTurnIdx() + 1) % players.size();
    for (int i = 0; i < (int)players.size(); i++) {
        int idx = (startIdx + i) % players.size();
        if (players[idx]->getStatus() != PlayerStatus::BANKRUPT)
            auctionCtx.bidders.push_back(players[idx]);
    }
    auctionCtx.currentIdx = 0;

    if (auctionCtx.bidders.empty()) {
        auctionCtx.active = false;
        phase = Phase::POST_ROLL;
        return;
    }

    phase = Phase::AUCTION_ACTIVE;
    Player *bidder = auctionCtx.bidders[auctionCtx.currentIdx];
    int minBid = auctionCtx.highestBid + 1;
    string msg = "Properti: " + prop->getName() + " (" + prop->getKode() + ")\n" +
                 "Giliran: " + bidder->getUsername() + "\n" +
                 "Bid tertinggi: " +
                 (auctionCtx.highestBidder
                      ? fmtMoney(auctionCtx.highestBid) + " (" +
                            auctionCtx.highestBidder->getUsername() + ")"
                      : "Belum ada") +
                 "\n" + "Uang: " + fmtMoney(bidder->getMoney());

    PopupState ps;
    ps.type = PopupType::AUCTION;
    ps.title = "LELANG: " + prop->getKode();
    ps.message = msg;
    ps.options = {"BID " + fmtMoney(max(50, minBid)),
                  "BID " + fmtMoney(max(100, minBid + 50)), "PASS"};
    window.showPopup(ps);
}

static void advanceAuction(int choice, GameEngine &engine, GameWindow &window) {
    Player *bidder = auctionCtx.bidders[auctionCtx.currentIdx];
    int bidAmounts[] = {max(50, auctionCtx.highestBid + 1),
                        max(100, auctionCtx.highestBid + 51)};

    if (choice == 2) {
        auctionCtx.consecutivePasses++;
    } else {
        int bidAmount = bidAmounts[choice];
        if (bidAmount <= bidder->getMoney()) {
            auctionCtx.highestBid = bidAmount;
            auctionCtx.highestBidder = bidder;
            auctionCtx.consecutivePasses = 0;
        } else {
            auctionCtx.consecutivePasses++;
        }
    }

    if (auctionCtx.consecutivePasses >= (int)auctionCtx.bidders.size() - 1 &&
        auctionCtx.highestBidder) {
        *auctionCtx.highestBidder -= auctionCtx.highestBid;
        auctionCtx.property->setOwnerId(auctionCtx.highestBidder->getId());
        auctionCtx.property->setStatus(1);
        auctionCtx.highestBidder->addProperty(auctionCtx.property);
        updatePropertyCounts(auctionCtx.highestBidder);
        checkMonopoly(auctionCtx.highestBidder, engine.getBoard());

        PopupState ps;
        ps.type = PopupType::AUCTION;
        ps.title = "LELANG SELESAI";
        ps.message = "Pemenang: " + auctionCtx.highestBidder->getUsername() +
                     "\nHarga: " + fmtMoney(auctionCtx.highestBid);
        ps.options = {"OK"};
        window.showPopup(ps);

        auctionCtx.active = false;
        return;
    }

    if (auctionCtx.consecutivePasses >= (int)auctionCtx.bidders.size() &&
        !auctionCtx.highestBidder) {
        // Semua pemain pass tanpa ada yang bid — lelang berakhir tanpa pemenang
        PopupState ps;
        ps.type = PopupType::AUCTION;
        ps.title = "LELANG BERAKHIR";
        ps.message = "Tidak ada penawar.\nProperti " +
                     auctionCtx.property->getName() + " tidak terjual.";
        ps.options = {"OK"};
        window.showPopup(ps);
        auctionCtx.active = false;
        return;
    }

    auctionCtx.currentIdx =
        (auctionCtx.currentIdx + 1) % auctionCtx.bidders.size();
    Player *nextBidder = auctionCtx.bidders[auctionCtx.currentIdx];
    int minBid = auctionCtx.highestBid + 1;
    string msg = "Properti: " + auctionCtx.property->getName() + "\n" +
                 "Giliran: " + nextBidder->getUsername() + "\n" +
                 "Bid tertinggi: " +
                 (auctionCtx.highestBidder
                      ? fmtMoney(auctionCtx.highestBid) + " (" +
                            auctionCtx.highestBidder->getUsername() + ")"
                      : "Belum ada") +
                 "\n" + "Uang: " + fmtMoney(nextBidder->getMoney());

    PopupState ps;
    ps.type = PopupType::AUCTION;
    ps.title = "LELANG: " + auctionCtx.property->getKode();
    ps.message = msg;
    ps.options = {"BID " + fmtMoney(max(50, minBid)),
                  "BID " + fmtMoney(max(100, minBid + 50)), "PASS"};
    window.showPopup(ps);
}

// ---- Tile Effect Processing ----

static void processTileEffect(GameEngine &engine, GameWindow &window,
                              TransactionLogger &logger) {
    Player *p = engine.getCurrentPlayer();
    Board *board = engine.getBoard();
    Tile *tile = board->getTileAt(p->getPosition() + 1);
    if (!tile) { phase = Phase::POST_ROLL; return; }

    EffectType effect = tile->onLanded(*p);

    switch (effect) {
    case EffectType::OFFER_BUY: {
        PropertyTile *prop = dynamic_cast<PropertyTile *>(tile);
        if (!prop) { phase = Phase::POST_ROLL; break; }
        pendingProp = prop;

        if (p->getMoney() >= prop->getPrice()) {
            PopupState ps;
            ps.type = PopupType::BUY_PROPERTY;
            ps.title = "BELI PROPERTI";
            ps.message = "Kamu mendarat di " + prop->getName() + " (" +
                         prop->getKode() + ")\n" +
                         "Harga: " + fmtMoney(prop->getPrice()) + "\n" +
                         "Uang kamu: " + fmtMoney(p->getMoney()) + "\n" +
                         "Beli properti ini?";
            ps.options = {"Ya, Beli!", "Tidak (Lelang)"};
            window.showPopup(ps);
            pending = Pending::BUY_PROPERTY;
            phase = Phase::EFFECT_PENDING;
        } else {
            startAuction(prop, engine, window);
        }
        break;
    }
    case EffectType::AUTO_ACQUIRE: {
        PropertyTile *prop = dynamic_cast<PropertyTile *>(tile);
        if (prop && prop->getOwnerId() == -1) {
            prop->setOwnerId(p->getId());
            prop->setStatus(1);
            p->addProperty(prop);
            updatePropertyCounts(p);
            checkMonopoly(p, board);
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::BUY, prop->getName() + " (Otomatis)");
        }
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::PAY_RENT: {
        PropertyTile *prop = dynamic_cast<PropertyTile *>(tile);
        if (!prop) { phase = Phase::POST_ROLL; break; }

        Player *owner = nullptr;
        for (Player *pl : engine.getAllPlayers())
            if (pl->getId() == prop->getOwnerId()) { owner = pl; break; }
        if (!owner) { phase = Phase::POST_ROLL; break; }

        int diceTotal = lastD1 + lastD2;
        int rent = prop->calcRent(diceTotal);

        if (p->isShieldActive()) {
            p->setShieldActive(false);
            rent = 0;
        }
        if (p->getActiveDiscountPercent() > 0) {
            int disc = rent * p->getActiveDiscountPercent() / 100;
            rent -= disc;
        }

        if (rent > 0) {
            try {
                *p -= rent;
                *owner += rent;
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::RENT,
                                "Bayar " + fmtMoney(rent) + " ke " +
                                    owner->getUsername() + " (" +
                                    prop->getKode() + ")");
            } catch (NotEnoughMoneyException &) {
                int sisaUang = p->getMoney();
                *owner += sisaUang;
                for (PropertyTile *op : p->getOwnedProperties()) {
                    op->setOwnerId(owner->getId());
                    owner->addProperty(op);
                }
                p->declareBankruptcy();
                updatePropertyCounts(owner);
                checkMonopoly(owner, board);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::BANKRUPT,
                                "Bangkrut ke " + owner->getUsername());
            }
        }
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::PAY_TAX_CHOICE: {
        TaxTile *tax = dynamic_cast<TaxTile *>(tile);
        if (!tax) { phase = Phase::POST_ROLL; break; }
        pendingTax = tax;

        PopupState ps;
        ps.type = PopupType::TAX_CHOICE;
        ps.title = "PAJAK PENGHASILAN";
        ps.message = "Pilih metode pembayaran:\n" +
                     string("1. Bayar flat ") + fmtMoney(tax->getFlatAmount()) + "\n" +
                     "2. Bayar " + to_string((int)tax->getPercentageRate()) +
                     "% dari kekayaan\n" +
                     "(Pilih SEBELUM menghitung kekayaan!)";
        ps.options = {"Flat " + fmtMoney(tax->getFlatAmount()), "Persentase"};
        window.showPopup(ps);
        pending = Pending::TAX_CHOICE;
        phase = Phase::EFFECT_PENDING;
        break;
    }
    case EffectType::PAY_TAX_FLAT: {
        TaxTile *tax = dynamic_cast<TaxTile *>(tile);
        if (!tax) { phase = Phase::POST_ROLL; break; }
        int amount = tax->getFlatAmount();
        try {
            *p -= amount;
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::TAX, "PBM " + fmtMoney(amount));
        } catch (NotEnoughMoneyException &) {
            p->declareBankruptcy();
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::BANKRUPT, "Bangkrut PBM");
        }
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::DRAW_CHANCE: {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 2);
        int card = dis(gen);
        if (card == 0) {
            int pos = p->getPosition();
            int stations[] = {5, 15, 25, 35};
            int nearest = stations[0];
            for (int s : stations)
                if (s > pos) { nearest = s; break; }
            p->move((nearest - pos + 40) % 40);
        } else if (card == 1) {
            p->move(37);
        } else {
            p->goToJail();
        }
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::CARD, "Kartu Kesempatan");
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::DRAW_COMMUNITY: {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 2);
        int card = dis(gen);
        auto allPlayers = engine.getAllPlayers();
        if (card == 0) {
            for (Player *other : allPlayers) {
                if (other != p && other->getStatus() != PlayerStatus::BANKRUPT) {
                    try { *other -= 100; *p += 100; } catch (...) {}
                }
            }
        } else if (card == 1) {
            try { *p -= 700; }
            catch (NotEnoughMoneyException &) { p->declareBankruptcy(); }
        } else {
            for (Player *other : allPlayers) {
                if (other != p && other->getStatus() != PlayerStatus::BANKRUPT) {
                    try { *p -= 200; *other += 200; }
                    catch (NotEnoughMoneyException &) {
                        p->declareBankruptcy(); break;
                    }
                }
            }
        }
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::CARD, "Kartu Dana Umum");
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::FESTIVAL_TRIGGER: {
        auto props = p->getOwnedProperties();
        vector<string> opts;
        for (auto *prop : props) {
            if (auto *st = dynamic_cast<StreetTile *>(prop))
                opts.push_back(st->getKode() + " - " + st->getName());
        }
        if (opts.empty()) { phase = Phase::POST_ROLL; break; }
        PopupState ps;
        ps.type = PopupType::FESTIVAL;
        ps.title = "FESTIVAL";
        ps.message = "Pilih properti untuk efek festival (sewa x2, 3 giliran):";
        ps.options = opts;
        window.showPopup(ps);
        pending = Pending::FESTIVAL;
        phase = Phase::EFFECT_PENDING;
        break;
    }
    case EffectType::SEND_TO_JAIL: {
        p->goToJail();
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::UNKNOWN, "Mendarat di Pergi ke Penjara");
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::AWARD_SALARY: {
        int salary = 200;
        *p += salary;
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::UNKNOWN, "Mendarat di GO");
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::ALREADY_OWNED_SELF:
    case EffectType::NONE:
    case EffectType::JUST_VISITING:
    default:
        phase = Phase::POST_ROLL;
        break;
    }
}

// ---- Popup response handler ----

static bool exitConfirmPending = false;

static void handlePopupResponse(int choice, GameEngine &engine,
                                GameWindow &window, TransactionLogger &logger) {
    // -1 = user cancelled via list popup BATAL button
    if (choice < 0) {
        window.closePopup();
        pending = Pending::NONE;
        return;
    }

    Player *p = gameStarted ? engine.getCurrentPlayer() : nullptr;

    // Exit confirmation (SAVE popup type repurposed for exit confirm)
    if (pending == Pending::EXIT_CONFIRM) {
        window.closePopup();
        if (choice == 0) {
            // User confirmed exit — set flag, main loop will break
            exitConfirmPending = true;
        }
        pending = Pending::NONE;
        return;
    }

    // Auction routing
    if (phase == Phase::AUCTION_ACTIVE) {
        if (!auctionCtx.active) {
            window.closePopup();
            phase = Phase::POST_ROLL;
            return;
        }
        advanceAuction(choice, engine, window);
        return;
    }

    if (!p) { window.closePopup(); return; }

    switch (pending) {
    case Pending::BUY_PROPERTY: {
        window.closePopup();
        if (choice == 0 && pendingProp) {
            try {
                *p -= pendingProp->getPrice();
                pendingProp->setOwnerId(p->getId());
                pendingProp->setStatus(1);
                p->addProperty(pendingProp);
                updatePropertyCounts(p);
                checkMonopoly(p, engine.getBoard());
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::BUY,
                                "Beli " + pendingProp->getName() + " " +
                                    fmtMoney(pendingProp->getPrice()));
            } catch (NotEnoughMoneyException &) {
                startAuction(pendingProp, engine, window);
                pending = Pending::NONE;
                return;
            }
            phase = Phase::POST_ROLL;
        } else {
            startAuction(pendingProp, engine, window);
        }
        pending = Pending::NONE;
        break;
    }
    case Pending::TAX_CHOICE: {
        window.closePopup();
        if (pendingTax) {
            int amount;
            if (choice == 0) {
                amount = pendingTax->getFlatAmount();
            } else {
                int wealth = pendingTax->computeNetWorth(*p);
                amount = (int)(wealth * pendingTax->getPercentageRate() / 100.0);
            }
            try {
                *p -= amount;
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::TAX, "PPH " + fmtMoney(amount));
            } catch (NotEnoughMoneyException &) {
                p->declareBankruptcy();
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::BANKRUPT, "Bangkrut PPH");
            }
        }
        pending = Pending::NONE;
        phase = Phase::POST_ROLL;
        break;
    }
    case Pending::FESTIVAL: {
        window.closePopup();
        auto props = p->getOwnedProperties();
        int streetIdx = 0;
        for (auto *prop : props) {
            if (auto *st = dynamic_cast<StreetTile *>(prop)) {
                if (streetIdx == choice) {
                    st->setFestivalEffect(2);
                    logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                    LogActionType::FESTIVAL, st->getName());
                    break;
                }
                streetIdx++;
            }
        }
        pending = Pending::NONE;
        phase = Phase::POST_ROLL;
        break;
    }
    case Pending::JAIL_CHOICE: {
        window.closePopup();
        if (choice == 0) {
            int fine = 50;
            try {
                *p -= fine;
                p->setStatus(PlayerStatus::ACTIVE);
                p->setJailTurnsLeft(0);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::UNKNOWN, fmtMoney(fine));
                phase = Phase::AWAITING_ACTION;
            } catch (NotEnoughMoneyException &) {
                p->declareBankruptcy();
                phase = Phase::POST_ROLL;
            }
        } else {
            phase = Phase::AWAITING_ACTION;
        }
        pending = Pending::NONE;
        break;
    }
    case Pending::GADAI_SELECT: {
        window.closePopup();
        if (choice >= 0 && choice < (int)gadaiCandidates.size()) {
            PropertyTile *prop = gadaiCandidates[choice];
            prop->mortgage();
            *p += prop->getmortgageValue();
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::MORTGAGE, prop->getName());
        }
        pending = Pending::NONE;
        break;
    }
    case Pending::TEBUS_SELECT: {
        window.closePopup();
        if (choice >= 0 && choice < (int)gadaiCandidates.size()) {
            PropertyTile *prop = gadaiCandidates[choice];
            int cost = prop->getPrice();
            try {
                *p -= cost;
                prop->unmortgage();
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::UNMORTGAGE, prop->getName());
            } catch (NotEnoughMoneyException &) {
                // Uang tidak cukup, batal
            }
        }
        pending = Pending::NONE;
        break;
    }
    case Pending::BANGUN_COLOR_SELECT: {
        window.closePopup();
        bangunCandidates.clear();
        auto props = p->getOwnedProperties();
        map<string, vector<StreetTile *>> monopolized;
        for (auto *prop : props) {
            if (auto *st = dynamic_cast<StreetTile *>(prop))
                if (st->getRentLevel() < 5)
                    monopolized[st->getColorGroup()].push_back(st);
        }
        int idx = 0;
        string chosenColor;
        for (auto &[color, streets] : monopolized) {
            if (idx == choice) { chosenColor = color; break; }
            idx++;
        }
        if (chosenColor.empty()) { pending = Pending::NONE; break; }

        auto &streets = monopolized[chosenColor];
        int minLevel = 999;
        for (auto *st : streets) minLevel = min(minLevel, st->getRentLevel());

        vector<string> opts;
        for (auto *st : streets) {
            if (st->getRentLevel() <= minLevel && st->getRentLevel() < 5) {
                bangunCandidates.push_back(st);
                string label = st->getKode() + " Lv" + to_string(st->getRentLevel());
                if (st->getRentLevel() == 4)
                    label += " -> Hotel " + fmtMoney(st->getHotelCost());
                else
                    label += " -> Rumah " + fmtMoney(st->getHouseCost());
                opts.push_back(label);
            }
        }
        if (opts.empty()) { pending = Pending::NONE; break; }

        PopupState ps;
        ps.type = PopupType::BUY_PROPERTY;
        ps.title = "BANGUN - " + chosenColor;
        ps.message = "Pilih properti untuk dibangun:";
        ps.options = opts;
        window.showPopup(ps);
        pending = Pending::BANGUN_TILE_SELECT;
        break;
    }
    case Pending::BANGUN_TILE_SELECT: {
        window.closePopup();
        if (choice >= 0 && choice < (int)bangunCandidates.size()) {
            StreetTile *st = bangunCandidates[choice];
            try {
                if (st->getRentLevel() == 4) {
                    *p -= st->getHotelCost();
                    st->buildHotel();
                } else {
                    *p -= st->getHouseCost();
                    st->buildHouse();
                }
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::BUILD, st->getName());
            } catch (NotEnoughMoneyException &) {
                // Uang tidak cukup
            }
        }
        pending = Pending::NONE;
        break;
    }
    case Pending::DROP_CARD: {
        window.closePopup();
        if (choice >= 0 && choice < (int)p->getHandCards().size())
            p->removeCard(choice);
        pending = Pending::NONE;
        break;
    }
    case Pending::AKTA_SELECT: {
        window.closePopup();
        if (choice >= 0 && choice < (int)aktaCandidates.size()) {
            PropertyTile *prop = aktaCandidates[choice];
            vector<string> lines;
            lines.push_back("+================================+");
            lines.push_back("  AKTA KEPEMILIKAN");

            if (auto *st = dynamic_cast<StreetTile *>(prop)) {
                lines.push_back("  [" + st->getColorGroup() + "] " + st->getName() + " (" + st->getKode() + ")");
                lines.push_back("+================================+");
                lines.push_back("  Harga Beli : " + fmtMoney(st->getPrice()));
                lines.push_back("  Nilai Gadai: " + fmtMoney(st->getmortgageValue()));
                lines.push_back("  Biaya Rumah: " + fmtMoney(st->getHouseCost()));
                lines.push_back("  Biaya Hotel: " + fmtMoney(st->getHotelCost()));
                lines.push_back("  Level Bgn  : " + to_string(st->getRentLevel()));
            } else {
                lines.push_back("  " + prop->getName() + " (" + prop->getKode() + ")");
                lines.push_back("+================================+");
                lines.push_back("  Nilai Gadai: " + fmtMoney(prop->getmortgageValue()));
            }
            lines.push_back("  Status     : " + propStatusStr(prop->getStatus()));
            if (prop->getOwnerId() != -1) {
                lines.push_back("  Pemilik    : " +
                                findOwnerName(prop->getOwnerId(), engine.getAllPlayers()));
            }
            window.showPropertyInfo("AKTA: " + prop->getKode(), lines);
        }
        pending = Pending::NONE;
        break;
    }
    default:
        window.closePopup();
        if (phase != Phase::GAME_OVER)
            phase = Phase::POST_ROLL;
        pending = Pending::NONE;
        break;
    }
}

// ---- Win condition check ----

static bool checkGameOver(GameEngine &engine, GameWindow &window) {
    auto players = engine.getAllPlayers();
    int activePlayers = 0;
    Player *lastActive = nullptr;
    for (Player *p : players) {
        if (p->getStatus() != PlayerStatus::BANKRUPT) {
            activePlayers++;
            lastActive = p;
        }
    }

    int maxTurn = ConfigLoader::getInstance()->getMaxTurn();

    if (activePlayers == 1 && lastActive) {
        phase = Phase::GAME_OVER;
        PopupState ps;
        ps.type = PopupType::WINNER;
        ps.title = "PERMAINAN SELESAI!";
        ps.message = "Pemenang: " + lastActive->getUsername() +
                     "\n(Semua pemain lain bangkrut)";
        ps.options = {"OK"};
        window.showPopup(ps);
        return true;
    }

    if (maxTurn > 0 && engine.getCurrentRound() > maxTurn) {
        phase = Phase::GAME_OVER;
        Player *winner = nullptr;
        for (Player *p : players) {
            if (p->getStatus() != PlayerStatus::BANKRUPT)
                if (!winner || p->getWealth() > winner->getWealth())
                    winner = p;
        }
        string winnerName = winner ? winner->getUsername() : "???";
        PopupState ps;
        ps.type = PopupType::WINNER;
        ps.title = "BATAS GILIRAN TERCAPAI!";
        ps.message = "Pemenang: " + winnerName + "\n" +
                     (winner ? "Kekayaan: " + fmtMoney(winner->getWealth()) : "");
        ps.options = {"OK"};
        window.showPopup(ps);
        return true;
    }
    return false;
}

// ============================================================
// MAIN
// ============================================================

int main() {
    // Load config first (before window - needed for engine setup)
    ConfigLoader *config = ConfigLoader::getInstance();
    config->setConfigFilePath("config");
    try {
        config->loadAllConfigs();
    } catch (exception &e) {
        // Config load failed - we'll show a blank game, not fatal for window
        (void)e;
    }

    Board *board = Board::getInstance();
    TransactionLogger logger;
    GameEngine engine(board, &logger);

    // Create window (starts in MAIN_MENU screen)
    GameWindow window(1280, 800, "pOOPs: NIMONSPOLI");
    window.init();

    // ---- Register menu / lifecycle callbacks ----

    window.onNewGame([&](int numPlayers, vector<string> names) {
        (void)numPlayers;
        engine.startNewGame(names);
        gameStarted = true;
        window.setScreen(AppScreen::PLAYING);
        phase = Phase::AWAITING_ACTION;
        turnJustStarted = true;
        lastD1 = 0; lastD2 = 0;
    });

    window.onLoadGame([&](string filename) {
        // Try to load; on fail, show error in text input
        try {
            engine.loadGame(filename);
            gameStarted = true;
            window.setScreen(AppScreen::PLAYING);
            phase = Phase::AWAITING_ACTION;
            turnJustStarted = true;
            lastD1 = 0; lastD2 = 0;
        } catch (...) {
            window.setTextInputError("Gagal memuat file: " + filename);
        }
    });

    window.onSaveGame([&](string filename) {
        if (!gameStarted) return;
        try {
            engine.saveGame(filename);
        } catch (exception &e) {
            window.setTextInputError(string("Gagal simpan: ") + e.what());
        }
    });

    window.onExitGame([&]() {
        // handled via exitConfirmPending
    });

    // ---- Register game action callbacks ----

    // LEMPAR DADU
    window.onCommand("LEMPAR_DADU", [&]() {
        if (!gameStarted || phase != Phase::AWAITING_ACTION) return;
        if (engine.hasDiceRolled()) return;

        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(1, 6);
        lastD1 = dis(gen);
        lastD2 = dis(gen);

        try {
            engine.rollDice(lastD1, lastD2);
        } catch (InvalidCommandException &) {
            return;
        }

        logger.logEvent(engine.getCurrentRound(),
                        engine.getCurrentPlayer()->getUsername(),
                        LogActionType::ROLL,
                        to_string(lastD1) + "+" + to_string(lastD2) + "=" +
                            to_string(lastD1 + lastD2));

        if (engine.getCurrentPlayer()->getStatus() == PlayerStatus::JAILED) {
            phase = Phase::POST_ROLL;
            return;
        }

        phase = Phase::ROLLED;
        processTileEffect(engine, window, logger);
    });

    // AKHIRI GILIRAN
    window.onCommand("AKHIRI_GILIRAN", [&]() {
        if (!gameStarted) return;
        if (phase != Phase::POST_ROLL && phase != Phase::AWAITING_ACTION) return;
        if (!engine.hasDiceRolled()) return;

        try {
            engine.endTurn();
        } catch (InvalidCommandException &) {
            return;
        } catch (GameStateException &) {}

        lastD1 = 0; lastD2 = 0;
        if (checkGameOver(engine, window)) return;
        turnJustStarted = true;
        phase = Phase::AWAITING_ACTION;
    });

    // GADAI
    window.onCommand("GADAI", [&]() {
        if (!gameStarted) return;
        if (phase != Phase::AWAITING_ACTION && phase != Phase::POST_ROLL) return;
        Player *p = engine.getCurrentPlayer();
        gadaiCandidates.clear();
        vector<string> opts;
        for (PropertyTile *prop : p->getOwnedProperties()) {
            if (prop->getStatus() == 1) {
                if (auto *st = dynamic_cast<StreetTile *>(prop))
                    if (st->hasBuildings()) continue;
                gadaiCandidates.push_back(prop);
                opts.push_back(prop->getKode() + " - Gadai: " +
                               fmtMoney(prop->getmortgageValue()));
            }
        }
        if (opts.empty()) return;
        PopupState ps;
        ps.type = PopupType::LIQUIDATION;
        ps.title = "GADAI PROPERTI";
        ps.message = "Pilih properti untuk digadaikan:";
        ps.options = opts;
        window.showPopup(ps);
        pending = Pending::GADAI_SELECT;
    });

    // TEBUS
    window.onCommand("TEBUS", [&]() {
        if (!gameStarted) return;
        if (phase != Phase::AWAITING_ACTION && phase != Phase::POST_ROLL) return;
        Player *p = engine.getCurrentPlayer();
        gadaiCandidates.clear();
        vector<string> opts;
        for (PropertyTile *prop : p->getOwnedProperties()) {
            if (prop->getStatus() == 2) {
                gadaiCandidates.push_back(prop);
                opts.push_back(prop->getKode() + " - Tebus: " +
                               fmtMoney(prop->getPrice()));
            }
        }
        if (opts.empty()) return;
        PopupState ps;
        ps.type = PopupType::LIQUIDATION;
        ps.title = "TEBUS PROPERTI";
        ps.message = "Pilih properti untuk ditebus:";
        ps.options = opts;
        window.showPopup(ps);
        pending = Pending::TEBUS_SELECT;
    });

    // BANGUN
    window.onCommand("BANGUN", [&]() {
        if (!gameStarted) return;
        if (phase != Phase::AWAITING_ACTION && phase != Phase::POST_ROLL) return;
        Player *p = engine.getCurrentPlayer();

        map<string, vector<StreetTile *>> buildable;
        for (PropertyTile *prop : p->getOwnedProperties()) {
            if (auto *st = dynamic_cast<StreetTile *>(prop))
                if (st->getRentLevel() < 5)
                    buildable[st->getColorGroup()].push_back(st);
        }

        map<string, int> totalInGroup;
        for (int i = 1; i <= board->getTotalTiles(); i++) {
            if (auto *st = dynamic_cast<StreetTile *>(board->getTileAt(i)))
                totalInGroup[st->getColorGroup()]++;
        }

        vector<string> opts;
        for (auto &[color, streets] : buildable) {
            if ((int)streets.size() == totalInGroup[color])
                opts.push_back("[" + color + "] " + to_string(streets.size()) + " properti");
        }
        if (opts.empty()) return;

        PopupState ps;
        ps.type = PopupType::BUY_PROPERTY;
        ps.title = "BANGUN";
        ps.message = "Pilih color group:";
        ps.options = opts;
        window.showPopup(ps);
        pending = Pending::BANGUN_COLOR_SELECT;
    });

    // CETAK PAPAN — show board summary in GUI overlay
    window.onCommand("CETAK_PAPAN", [&]() {
        if (!gameStarted) return;
        vector<string> lines;
        lines.push_back("=== STATUS PAPAN PERMAINAN ===");
        lines.push_back("Turn: " + to_string(engine.getCurrentRound()) + " / " +
                        to_string(ConfigLoader::getInstance()->getMaxTurn()));
        lines.push_back("");
        for (int i = 1; i <= board->getTotalTiles(); i++) {
            Tile *tile = board->getTileAt(i);
            if (!tile) continue;
            if (auto *pt = dynamic_cast<PropertyTile *>(tile)) {
                string ownerStr = pt->getOwnerId() == -1 ? "BANK" :
                    findOwnerName(pt->getOwnerId(), engine.getAllPlayers());
                lines.push_back("[" + to_string(i) + "] " + tile->getKode() +
                                " - " + propStatusStr(pt->getStatus()) +
                                " (" + ownerStr + ")");
            }
        }
        window.showPropertyInfo("PAPAN PERMAINAN", lines);
    });

    // CETAK AKTA — show popup to select property, then show deed
    window.onCommand("CETAK_AKTA", [&]() {
        if (!gameStarted) return;
        aktaCandidates.clear();
        vector<string> opts;
        for (int i = 1; i <= board->getTotalTiles(); i++) {
            Tile *tile = board->getTileAt(i);
            if (auto *pt = dynamic_cast<PropertyTile *>(tile)) {
                aktaCandidates.push_back(pt);
                string label = pt->getKode() + " - " + pt->getName();
                string ownerStr = pt->getOwnerId() == -1 ? "" :
                    " [" + findOwnerName(pt->getOwnerId(), engine.getAllPlayers()) + "]";
                opts.push_back(label + ownerStr);
            }
        }
        if (opts.empty()) return;
        PopupState ps;
        ps.type = PopupType::LIQUIDATION;
        ps.title = "PILIH PROPERTI - CETAK AKTA";
        ps.message = "Pilih properti untuk melihat akta kepemilikan:";
        ps.options = opts;
        window.showPopup(ps);
        pending = Pending::AKTA_SELECT;
    });

    // CETAK PROPERTI — show current player's properties in GUI overlay
    window.onCommand("CETAK_PROPERTI", [&]() {
        if (!gameStarted) return;
        Player *p = engine.getCurrentPlayer();
        vector<string> lines;
        lines.push_back("Pemain: " + p->getUsername());
        lines.push_back("Uang  : " + fmtMoney(p->getMoney()));
        lines.push_back("Total Kekayaan: " + fmtMoney(p->getWealth()));
        lines.push_back("---");
        if (p->getOwnedProperties().empty()) {
            lines.push_back("(Belum memiliki properti)");
        } else {
            for (PropertyTile *prop : p->getOwnedProperties()) {
                string info = prop->getKode() + " - " + prop->getName() +
                              " [" + propStatusStr(prop->getStatus()) + "]";
                if (auto *st = dynamic_cast<StreetTile *>(prop))
                    info += " Lv" + to_string(st->getRentLevel());
                lines.push_back(info);
            }
        }
        window.showPropertyInfo("PROPERTI: " + p->getUsername(), lines);
    });

    // KELUAR — show exit confirmation popup
    window.onCommand("KELUAR", [&]() {
        PopupState ps;
        ps.type = PopupType::SAVE;
        ps.title = "KELUAR DARI GAME";
        ps.message = "Yakin ingin keluar?\nProgress yang belum disimpan\nakan hilang!";
        ps.options = {"Ya, Keluar", "Batal"};
        window.showPopup(ps);
        pending = Pending::EXIT_CONFIRM;
    });

    // CETAK LOG — show full log in GUI overlay
    window.onCommand("CETAK_LOG", [&]() {
        if (!gameStarted) return;
        auto logs = logger.getLogs(-1);
        vector<string> lines;
        lines.push_back("=== LOG TRANSAKSI (" + to_string(logs.size()) + " entri) ===");
        int from = (int)logs.size() > 15 ? (int)logs.size() - 15 : 0;
        for (int i = from; i < (int)logs.size(); i++) {
            auto &le = logs[i];
            lines.push_back("[T" + to_string(le.turn) + "] " +
                            le.username + " | " + le.detail);
        }
        if (logs.size() > 15) lines.push_back("... (hanya 15 terakhir ditampilkan)");
        window.showPropertyInfo("LOG TRANSAKSI", lines);
    });

    // SIMPAN — show GUI text input dialog
    window.onCommand("SIMPAN", [&]() {
        if (!gameStarted) return;
        window.showTextInput("SIMPAN PERMAINAN", "Path file (contoh: saves/game1):", true);
    });

    // MUAT — show GUI text input dialog
    window.onCommand("MUAT", [&]() {
        window.showTextInput("MUAT PERMAINAN", "Path file (contoh: saves/game1):", false);
    });

    // GUNAKAN KEMAMPUAN (kartu di tangan)
    for (int ci = 0; ci < 3; ci++) {
        window.onCommand("GUNAKAN_KEMAMPUAN_" + to_string(ci), [&, ci]() {
            if (!gameStarted || phase != Phase::AWAITING_ACTION) return;
            if (engine.hasDiceRolled()) return;
            Player *p = engine.getCurrentPlayer();
            if (p->getHasUsedCardThisTurn()) return;
            auto &cards = p->getHandCards();
            if (ci >= (int)cards.size()) return;
            SkillCard *card = cards[ci];
            card->use(*p, engine);
            p->setHasUsedCardThisTurn(true);
            p->removeCard(ci);
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::CARD, card->getCardName());
        });
    }

    // POPUP RESPONSE
    window.onPopupOption([&](int idx) {
        // Handle exit confirmation (popup of type SAVE with title "KELUAR DARI GAME")
        if (pending == Pending::NONE && idx == 0) {
            // Could be exit confirm or winner OK — check popup state
        }
        handlePopupResponse(idx, engine, window, logger);
    });

    // ============================================================
    // MAIN LOOP
    // ============================================================

    while (window.isRunning()) {
        // Handle exit confirmation flag set in handlePopupResponse
        if (exitConfirmPending) {
            // Force window close by breaking the loop
            break;
        }

        if (window.getScreen() == AppScreen::PLAYING && gameStarted) {
            // Turn start logic
            if (turnJustStarted && phase == Phase::AWAITING_ACTION) {
                turnJustStarted = false;
                Player *p = engine.getCurrentPlayer();

                if (p->getStatus() == PlayerStatus::BANKRUPT) {
                    try { engine.endTurn(); } catch (...) {}
                    if (checkGameOver(engine, window)) {
                        GameState gs = buildGameState(engine, logger);
                        window.updateState(gs);
                        window.tick();
                        continue;
                    }
                    turnJustStarted = true;
                    continue;
                }

                // Reset discount
                p->setActiveDiscountPercent(0);

                // Jail popup
                if (p->getStatus() == PlayerStatus::JAILED) {
                    int fine = 50;
                    PopupState ps;
                    ps.type = PopupType::JAIL;
                    ps.title = "PENJARA";
                    ps.message = p->getUsername() + " sedang di penjara.\n" +
                                 "Sisa giliran percobaan: " +
                                 to_string(p->getJailTurnsLeft()) + "\n" +
                                 "Bayar denda " + fmtMoney(fine) +
                                 " atau coba lempar double?";
                    ps.options = {"Bayar Denda " + fmtMoney(fine), "Lempar Dadu"};
                    window.showPopup(ps);
                    pending = Pending::JAIL_CHOICE;
                    phase = Phase::EFFECT_PENDING;
                }
            }

            GameState gs = buildGameState(engine, logger);
            window.updateState(gs);
        }

        window.tick();
    }

    return 0;
}
