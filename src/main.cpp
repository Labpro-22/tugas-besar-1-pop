// main.cpp, Controller: menghubungkan GameEngine <-> GameWindow

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
#include <iostream>
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
};

// Global state
static Phase phase = Phase::MENU;
static Pending pending = Pending::NONE;
static int lastD1 = 0, lastD2 = 0;
static bool turnJustStarted = true;
static bool gameOverShown = false;

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

// Helperzz

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
    case PlayerStatus::ACTIVE:
        return "ACTIVE";
    case PlayerStatus::JAILED:
        return "JAILED";
    case PlayerStatus::BANKRUPT:
        return "BANKRUPT";
    default:
        return "UNKNOWN";
    }
}

static string propStatusStr(int s) {
    switch (s) {
    case 0:
        return "BANK";
    case 1:
        return "OWNED";
    case 2:
        return "MORTGAGED";
    default:
        return "BANK";
    }
}

static string findOwnerName(int ownerId, const vector<Player *> &players) {
    for (Player *p : players)
        if (p->getId() == ownerId)
            return p->getUsername();
    return "";
}

// Update railroad/utility owned counts for a player
static void updatePropertyCounts(Player *player) {
    int rrCount = 0, utCount = 0;
    for (PropertyTile *prop : player->getOwnedProperties()) {
        if (dynamic_cast<RailroadTile *>(prop))
            rrCount++;
        if (dynamic_cast<UtilityTile *>(prop))
            utCount++;
    }
    for (PropertyTile *prop : player->getOwnedProperties()) {
        RailroadTile *rr = dynamic_cast<RailroadTile *>(prop);
        if (rr)
            rr->setrailroadOwnedCount(rrCount);
        UtilityTile *ut = dynamic_cast<UtilityTile *>(prop);
        if (ut)
            ut->setUtilityOwnedCount(utCount);
    }
}

// Check and set monopoly for a player
static void checkMonopoly(Player *player, Board *board) {
    map<string, int> totalInGroup;
    map<string, int> ownedInGroup;

    for (int i = 1; i <= board->getTotalTiles(); i++) {
        StreetTile *st = dynamic_cast<StreetTile *>(board->getTileAt(i));
        if (st)
            totalInGroup[st->getColorGroup()]++;
    }
    for (PropertyTile *prop : player->getOwnedProperties()) {
        StreetTile *st = dynamic_cast<StreetTile *>(prop);
        if (st)
            ownedInGroup[st->getColorGroup()]++;
    }
    for (PropertyTile *prop : player->getOwnedProperties()) {
        StreetTile *st = dynamic_cast<StreetTile *>(prop);
        if (st) {
            string cg = st->getColorGroup();
            st->setMonopolized(ownedInGroup[cg] == totalInGroup[cg]);
        }
    }
}

// Build GUI

static GameState buildGameState(GameEngine &engine, TransactionLogger &logger) {
    GameState gs;
    Board *board = engine.getBoard();
    auto players = engine.getAllPlayers();

    // Players
    for (int i = 0; i < (int)players.size(); i++) {
        Player *p = players[i];
        PlayerInfo pi;
        pi.username = p->getUsername();
        pi.money = p->getMoney();
        pi.position = p->getPosition() + 1; // GUI uses 1-based
        pi.status = statusStr(p->getStatus());
        pi.jailTurnsLeft = p->getJailTurnsLeft();
        pi.shieldActive = p->isShieldActive();
        pi.color = {0, 0, 0, 255}; // getPlayerColor handles this in GameWindow

        for (const SkillCard *card : p->getHandCards()) {
            CardInfo ci;
            ci.name = card->getCardName();
            ci.description = card->getCardDescription();
            ci.value = card->getValueString();
            pi.handCards.push_back(ci);
        }
        gs.players.push_back(pi);
    }

    // Tiles
    for (int i = 1; i <= board->getTotalTiles(); i++) {
        Tile *tile = board->getTileAt(i);
        if (!tile)
            continue;

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

        StreetTile *st = dynamic_cast<StreetTile *>(tile);
        RailroadTile *rr = dynamic_cast<RailroadTile *>(tile);
        UtilityTile *ut = dynamic_cast<UtilityTile *>(tile);
        PropertyTile *pt = dynamic_cast<PropertyTile *>(tile);

        if (st) {
            ti.tileType = "STREET";
            ti.colorGroup = st->getColorGroup();
            ti.price = st->getPrice();
            ti.propStatus = propStatusStr(st->getStatus());
            ti.ownerName = findOwnerName(st->getOwnerId(), players);
            int rl = st->getRentLevel();
            if (rl == 5) {
                ti.hasHotel = true;
                ti.houseCount = 0;
            } else {
                ti.houseCount = rl;
            }
            ti.festivalMult = 1; // TODO: read actual festival mult
        } else if (rr) {
            ti.tileType = "RAILROAD";
            ti.price = 0;
            ti.propStatus = propStatusStr(rr->getStatus());
            ti.ownerName = findOwnerName(rr->getOwnerId(), players);
        } else if (ut) {
            ti.tileType = "UTILITY";
            ti.price = 0;
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

    // Dice
    gs.dice.dice1 = lastD1;
    gs.dice.dice2 = lastD2;
    gs.dice.isDouble = (lastD1 > 0 && lastD1 == lastD2);
    gs.dice.hasRolled = engine.hasDiceRolled();

    // Turn info
    gs.currentPlayerIndex = engine.getCurrentTurnIdx();
    gs.currentTurn = engine.getCurrentRound();
    gs.maxTurn = ConfigLoader::getInstance()->getMaxTurn();
    gs.gameOver = (phase == Phase::GAME_OVER);
    gs.winnerName = "";

    // Logs (last 10)
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

// Helpers buat auction

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
    // Show first bidder popup
    Player *bidder = auctionCtx.bidders[auctionCtx.currentIdx];
    int minBid = auctionCtx.highestBid + 1;
    string msg = "Properti: " + prop->getName() + " (" + prop->getKode() +
                 ")\n" + "Giliran: " + bidder->getUsername() + "\n" +
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
    ps.options = {"BID +" + to_string(max(50, minBid)),
                  "BID +" + to_string(max(100, minBid + 50)), "PASS"};
    window.showPopup(ps);

    cout << "\n[LELANG] Properti " << prop->getName() << " dilelang!" << endl;
}

static void advanceAuction(int choice, GameEngine &engine, GameWindow &window) {
    Player *bidder = auctionCtx.bidders[auctionCtx.currentIdx];
    int bidAmounts[] = {max(50, auctionCtx.highestBid + 1),
                        max(100, auctionCtx.highestBid + 51)};

    if (choice == 2) {
        // PASS
        auctionCtx.consecutivePasses++;
        cout << "[LELANG] " << bidder->getUsername() << " PASS." << endl;
    } else {
        // BID
        int bidAmount = bidAmounts[choice];
        if (bidAmount <= bidder->getMoney()) {
            auctionCtx.highestBid = bidAmount;
            auctionCtx.highestBidder = bidder;
            auctionCtx.consecutivePasses = 0;
            cout << "[LELANG] " << bidder->getUsername() << " BID "
                 << fmtMoney(bidAmount) << endl;
        } else {
            cout << "[LELANG] Uang tidak cukup! Otomatis PASS." << endl;
            auctionCtx.consecutivePasses++;
        }
    }

    // Check if auction ends: consecutive passes >= bidders.size() - 1
    if (auctionCtx.consecutivePasses >= (int)auctionCtx.bidders.size() - 1 &&
        auctionCtx.highestBidder) {
        // Auction won
        *auctionCtx.highestBidder -= auctionCtx.highestBid;
        auctionCtx.property->setOwnerId(auctionCtx.highestBidder->getId());
        auctionCtx.property->setStatus(1);
        auctionCtx.highestBidder->addProperty(auctionCtx.property);
        updatePropertyCounts(auctionCtx.highestBidder);
        checkMonopoly(auctionCtx.highestBidder, engine.getBoard());

        cout << "[LELANG] Pemenang: " << auctionCtx.highestBidder->getUsername()
             << " dengan harga " << fmtMoney(auctionCtx.highestBid) << endl;

        PopupState ps;
        ps.type = PopupType::AUCTION;
        ps.title = "LELANG SELESAI";
        ps.message = "Pemenang: " + auctionCtx.highestBidder->getUsername() +
                     "\nHarga: " + fmtMoney(auctionCtx.highestBid);
        ps.options = {"OK"};
        window.showPopup(ps);

        auctionCtx.active = false;
        // Will go to POST_ROLL when user clicks OK
        return;
    }

    // If all passed without any bid and only one remains, force them to bid
    if (auctionCtx.consecutivePasses >= (int)auctionCtx.bidders.size() &&
        !auctionCtx.highestBidder) {
        // Force last non-passer — simplified: reset passes
        auctionCtx.consecutivePasses = 0;
    }

    // Next bidder
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

// Tile Effect Processing

static void processTileEffect(GameEngine &engine, GameWindow &window,
                              TransactionLogger &logger) {
    Player *p = engine.getCurrentPlayer();
    Board *board = engine.getBoard();
    Tile *tile = board->getTileAt(p->getPosition() + 1); // Board is 1-indexed
    if (!tile) {
        phase = Phase::POST_ROLL;
        return;
    }

    EffectType effect = tile->onLanded(*p);

    cout << "[INFO] " << p->getUsername() << " mendarat di " << tile->getName()
         << " (" << tile->getKode() << ")" << endl;

    switch (effect) {
    case EffectType::OFFER_BUY: {
        PropertyTile *prop = dynamic_cast<PropertyTile *>(tile);
        if (!prop) {
            phase = Phase::POST_ROLL;
            break;
        }
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
            ps.options = {"Ya, Beli!", "Tidak"};
            window.showPopup(ps);
            pending = Pending::BUY_PROPERTY;
            phase = Phase::EFFECT_PENDING;
        } else {
            cout << "[INFO] Uang tidak cukup untuk membeli. Dilelang!" << endl;
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
            cout << "[BELI] " << prop->getName() << " otomatis menjadi milik "
                 << p->getUsername() << "!" << endl;
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::BUY,
                            prop->getName() + " (Otomatis)");
        }
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::PAY_RENT: {
        PropertyTile *prop = dynamic_cast<PropertyTile *>(tile);
        if (!prop) {
            phase = Phase::POST_ROLL;
            break;
        }

        Player *owner = nullptr;
        for (Player *pl : engine.getAllPlayers())
            if (pl->getId() == prop->getOwnerId()) {
                owner = pl;
                break;
            }

        if (!owner) {
            phase = Phase::POST_ROLL;
            break;
        }

        int diceTotal = lastD1 + lastD2;
        int rent = prop->calcRent(diceTotal);

        // Shield protection
        if (p->isShieldActive()) {
            p->setShieldActive(false);
            cout << "[SHIELD] ShieldCard melindungi dari sewa "
                 << fmtMoney(rent) << "!" << endl;
            rent = 0;
        }
        // Discount
        if (p->getActiveDiscountPercent() > 0) {
            int disc = rent * p->getActiveDiscountPercent() / 100;
            rent -= disc;
            cout << "[DISKON] Diskon " << p->getActiveDiscountPercent()
                 << "% diterapkan! Sewa: " << fmtMoney(rent) << endl;
        }

        if (rent > 0) {
            try {
                *p -= rent;
                *owner += rent;
                cout << "[SEWA] " << p->getUsername() << " bayar "
                     << fmtMoney(rent) << " ke " << owner->getUsername()
                     << endl;
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::RENT,
                                "Bayar " + fmtMoney(rent) + " ke " +
                                    owner->getUsername() + " (" +
                                    prop->getKode() + ")");
            } catch (NotEnoughMoneyException &) {
                // Bangkrut ke pemain lain
                cout << "[BANGKRUT] " << p->getUsername()
                     << " bangkrut! Semua aset ke " << owner->getUsername()
                     << endl;
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
        if (!tax) {
            phase = Phase::POST_ROLL;
            break;
        }
        pendingTax = tax;

        PopupState ps;
        ps.type = PopupType::TAX_CHOICE;
        ps.title = "PAJAK PENGHASILAN";
        ps.message = "Pilih metode pembayaran:\n" + string("1. Bayar flat ") +
                     fmtMoney(tax->getFlatAmount()) + "\n" + "2. Bayar " +
                     to_string((int)tax->getPercentageRate()) +
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
        if (!tax) {
            phase = Phase::POST_ROLL;
            break;
        }

        int amount = tax->getFlatAmount();
        try {
            *p -= amount;
            cout << "[PAJAK] " << p->getUsername() << " bayar PBM "
                 << fmtMoney(amount) << endl;
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::TAX, "PBM " + fmtMoney(amount));
        } catch (NotEnoughMoneyException &) {
            cout << "[BANGKRUT] " << p->getUsername()
                 << " bangkrut karena pajak!" << endl;
            p->declareBankruptcy();
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::BANKRUPT, "Bangkrut PBM");
        }
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::DRAW_CHANCE: {
        cout << "[KARTU] " << p->getUsername() << " mengambil Kartu Kesempatan!"
             << endl;
        // TODO: Implementasi draw dari ChanceDeck (koordinasi dengan Orang 4 -
        // Miguel) Sementara: efek random sederhana
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 2);
        int card = dis(gen);
        if (card == 0) {
            // Pergi ke stasiun terdekat
            cout << "[KESEMPATAN] Pergi ke stasiun terdekat!" << endl;
            int pos = p->getPosition();
            int stations[] = {5, 15, 25,
                              35}; // 0-indexed positions of railroads
            int nearest = stations[0];
            for (int s : stations)
                if (s > pos) {
                    nearest = s;
                    break;
                }
            p->move((nearest - pos + 40) % 40);
        } else if (card == 1) {
            cout << "[KESEMPATAN] Mundur 3 petak!" << endl;
            p->move(37); // 40 - 3 = 37 (mod 40)
        } else {
            cout << "[KESEMPATAN] Masuk Penjara!" << endl;
            p->goToJail();
        }
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::CARD, "Kartu Kesempatan");
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::DRAW_COMMUNITY: {
        cout << "[KARTU] " << p->getUsername() << " mengambil Kartu Dana Umum!"
             << endl;
        // TODO: Implementasi draw dari CommunityDeck (koordinasi dengan Orang 4
        // - Miguel)
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 2);
        int card = dis(gen);
        auto allPlayers = engine.getAllPlayers();
        if (card == 0) {
            // Ulang tahun: dapat M100 dari setiap pemain
            cout << "[DANA UMUM] Selamat ulang tahun! Dapat M100 dari setiap "
                    "pemain."
                 << endl;
            for (Player *other : allPlayers) {
                if (other != p &&
                    other->getStatus() != PlayerStatus::BANKRUPT) {
                    try {
                        *other -= 100;
                        *p += 100;
                    } catch (...) {
                    }
                }
            }
        } else if (card == 1) {
            // Biaya dokter M700
            cout << "[DANA UMUM] Biaya dokter. Bayar M700." << endl;
            try {
                *p -= 700;
            } catch (NotEnoughMoneyException &) {
                cout << "[BANGKRUT] Tidak mampu bayar!" << endl;
                p->declareBankruptcy();
            }
        } else {
            // Nyaleg: bayar M200 ke setiap pemain
            cout << "[DANA UMUM] Mau nyaleg. Bayar M200 kepada setiap pemain."
                 << endl;
            for (Player *other : allPlayers) {
                if (other != p &&
                    other->getStatus() != PlayerStatus::BANKRUPT) {
                    try {
                        *p -= 200;
                        *other += 200;
                    } catch (NotEnoughMoneyException &) {
                        cout << "[BANGKRUT] Tidak mampu bayar!" << endl;
                        p->declareBankruptcy();
                        break;
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
        if (props.empty()) {
            cout << "[FESTIVAL] Kamu tidak punya properti untuk festival."
                 << endl;
            phase = Phase::POST_ROLL;
            break;
        }
        // Build list of owned properties for popup
        string msg =
            "Pilih properti untuk efek festival (sewa x2, 3 giliran):\n";
        vector<string> opts;
        for (auto *prop : props) {
            StreetTile *st = dynamic_cast<StreetTile *>(prop);
            if (st) {
                opts.push_back(st->getKode() + " - " + st->getName());
            }
        }
        if (opts.empty()) {
            cout << "[FESTIVAL] Tidak ada Street untuk festival." << endl;
            phase = Phase::POST_ROLL;
            break;
        }
        PopupState ps;
        ps.type = PopupType::FESTIVAL;
        ps.title = "FESTIVAL";
        ps.message = msg;
        ps.options = opts;
        window.showPopup(ps);
        pending = Pending::FESTIVAL;
        phase = Phase::EFFECT_PENDING;
        break;
    }
    case EffectType::SEND_TO_JAIL: {
        cout << "[PENJARA] " << p->getUsername() << " dikirim ke penjara!"
             << endl;
        p->goToJail();
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::UNKNOWN, "Mendarat di Pergi ke Penjara");
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::AWARD_SALARY: {
        int salary = 200 /* TODO: needs ConfigLoader::getGoSalary() getter */;
        *p += salary;
        cout << "[GO] " << p->getUsername() << " mendarat di GO! Menerima "
             << fmtMoney(salary) << endl;
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

// Popup response handler

static void handlePopupResponse(int choice, GameEngine &engine,
                                GameWindow &window, TransactionLogger &logger) {
    Player *p = engine.getCurrentPlayer();

    // If auction is active, route to auction handler
    if (phase == Phase::AUCTION_ACTIVE) {
        if (!auctionCtx.active) {
            // Auction result OK button
            window.closePopup();
            phase = Phase::POST_ROLL;
            return;
        }
        advanceAuction(choice, engine, window);
        return;
    }

    switch (pending) {
    case Pending::BUY_PROPERTY: {
        window.closePopup();
        if (choice == 0 && pendingProp) {
            // Ya, beli
            try {
                *p -= pendingProp->getPrice();
                pendingProp->setOwnerId(p->getId());
                pendingProp->setStatus(1);
                p->addProperty(pendingProp);
                updatePropertyCounts(p);
                checkMonopoly(p, engine.getBoard());
                cout << "[BELI] " << p->getUsername() << " membeli "
                     << pendingProp->getName() << " seharga "
                     << fmtMoney(pendingProp->getPrice()) << endl;
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::BUY,
                                "Beli " + pendingProp->getName() + " " +
                                    fmtMoney(pendingProp->getPrice()));
            } catch (NotEnoughMoneyException &) {
                cout << "[ERROR] Uang tidak cukup!" << endl;
                startAuction(pendingProp, engine, window);
                pending = Pending::NONE;
                return;
            }
            phase = Phase::POST_ROLL;
        } else {
            // Tidak beli → lelang
            cout << "[INFO] Properti dilelang!" << endl;
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
                amount =
                    (int)(wealth * pendingTax->getPercentageRate() / 100.0);
                cout << "[PAJAK] Kekayaan: " << fmtMoney(wealth) << ", Pajak "
                     << (int)pendingTax->getPercentageRate()
                     << "% = " << fmtMoney(amount) << endl;
            }
            try {
                *p -= amount;
                cout << "[PAJAK] " << p->getUsername() << " bayar PPH "
                     << fmtMoney(amount) << endl;
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::TAX, "PPH " + fmtMoney(amount));
            } catch (NotEnoughMoneyException &) {
                cout << "[BANGKRUT] Tidak mampu bayar pajak!" << endl;
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
            StreetTile *st = dynamic_cast<StreetTile *>(prop);
            if (st) {
                if (streetIdx == choice) {
                    st->setFestivalEffect(2);
                    cout << "[FESTIVAL] Festival aktif di " << st->getName()
                         << "! Sewa x2 selama 3 giliran." << endl;
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
            // Bayar denda
            int fine = 50 /* TODO: needs ConfigLoader::getJailFine() getter */;
            try {
                *p -= fine;
                p->move(0); // Stay, but set status to ACTIVE handled below
                // Need to release from jail — requires setStatus or equivalent
                // Since Player doesn't have setStatus, we work around it
                cout << "[PENJARA] " << p->getUsername() << " bayar denda "
                     << fmtMoney(fine) << " dan bebas!" << endl;
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::UNKNOWN, fmtMoney(fine));
                // Player is still JAILED — rollDice will handle double check
                // We set phase to AWAITING_ACTION so player can roll
                phase = Phase::AWAITING_ACTION;
            } catch (NotEnoughMoneyException &) {
                cout << "[BANGKRUT] Tidak mampu bayar denda!" << endl;
                p->declareBankruptcy();
                phase = Phase::POST_ROLL;
            }
        } else {
            // Coba lempar double
            phase = Phase::AWAITING_ACTION; // Let them roll
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
            cout << "[GADAI] " << prop->getName() << " digadaikan. Terima "
                 << fmtMoney(prop->getmortgageValue()) << endl;
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
            int cost = prop->getPrice(); // Harga beli penuh untuk tebus
            try {
                *p -= cost;
                prop->unmortgage();
                cout << "[TEBUS] " << prop->getName() << " ditebus seharga "
                     << fmtMoney(cost) << endl;
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::UNMORTGAGE, prop->getName());
            } catch (NotEnoughMoneyException &) {
                cout << "[ERROR] Uang tidak cukup!" << endl;
            }
        }
        pending = Pending::NONE;
        break;
    }
    case Pending::BANGUN_COLOR_SELECT: {
        window.closePopup();
        // Build list of streets in selected color group
        bangunCandidates.clear();
        auto props = p->getOwnedProperties();
        // Find all color groups that are monopolized
        map<string, vector<StreetTile *>> monopolized;
        for (auto *prop : props) {
            StreetTile *st = dynamic_cast<StreetTile *>(prop);
            if (st && st->getRentLevel() < 5) { // Not max
                monopolized[st->getColorGroup()].push_back(st);
            }
        }
        // Get the chosen color group
        int idx = 0;
        string chosenColor;
        for (auto &[color, streets] : monopolized) {
            if (idx == choice) {
                chosenColor = color;
                break;
            }
            idx++;
        }

        if (chosenColor.empty()) {
            pending = Pending::NONE;
            break;
        }

        // Find buildable tiles (even distribution rule)
        auto &streets = monopolized[chosenColor];
        int minLevel = 999;
        for (auto *st : streets)
            minLevel = min(minLevel, st->getRentLevel());

        vector<string> opts;
        for (auto *st : streets) {
            if (st->getRentLevel() <= minLevel && st->getRentLevel() < 5) {
                bangunCandidates.push_back(st);
                string label =
                    st->getKode() + " Lv" + to_string(st->getRentLevel());
                if (st->getRentLevel() == 4)
                    label += " -> Hotel " + fmtMoney(st->getHotelCost());
                else
                    label += " -> Rumah " + fmtMoney(st->getHouseCost());
                opts.push_back(label);
            }
        }

        if (opts.empty()) {
            cout << "[BANGUN] Tidak ada yang bisa dibangun." << endl;
            pending = Pending::NONE;
            break;
        }

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
                    cout << "[BANGUN] Hotel dibangun di " << st->getName()
                         << "!" << endl;
                } else {
                    *p -= st->getHouseCost();
                    st->buildHouse();
                    cout << "[BANGUN] Rumah dibangun di " << st->getName()
                         << "! Level: " << st->getRentLevel() << endl;
                }
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::BUILD, st->getName());
            } catch (NotEnoughMoneyException &) {
                cout << "[ERROR] Uang tidak cukup!" << endl;
            }
        }
        pending = Pending::NONE;
        break;
    }
    case Pending::DROP_CARD: {
        window.closePopup();
        if (choice >= 0 && choice < (int)p->getHandCards().size()) {
            cout << "[KARTU] " << p->getHandCards()[choice]->getCardName()
                 << " dibuang." << endl;
            p->removeCard(choice);
        }
        pending = Pending::NONE;
        break;
    }
    default:
        window.closePopup();
        if (phase == Phase::GAME_OVER) {
            // Do nothing, game is over
        } else {
            phase = Phase::POST_ROLL;
        }
        pending = Pending::NONE;
        break;
    }
}

// Cek win condition

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

    // Bankruptcy win
    if (activePlayers == 1 && lastActive) {
        phase = Phase::GAME_OVER;
        PopupState ps;
        ps.type = PopupType::WINNER;
        ps.title = "PERMAINAN SELESAI!";
        ps.message = "Pemenang: " + lastActive->getUsername() +
                     "\n(Semua pemain lain bangkrut)";
        ps.options = {"OK"};
        window.showPopup(ps);
        cout << "\n========================================" << endl;
        cout << "  PEMENANG: " << lastActive->getUsername() << endl;
        cout << "========================================" << endl;
        return true;
    }

    // Max turn win
    if (maxTurn > 0 && engine.getCurrentRound() > maxTurn) {
        phase = Phase::GAME_OVER;
        // Find richest player
        Player *winner = nullptr;
        for (Player *p : players) {
            if (p->getStatus() != PlayerStatus::BANKRUPT) {
                if (!winner || p->getWealth() > winner->getWealth())
                    winner = p;
            }
        }
        string winnerName = winner ? winner->getUsername() : "???";
        PopupState ps;
        ps.type = PopupType::WINNER;
        ps.title = "BATAS GILIRAN TERCAPAI!";
        ps.message =
            "Pemenang: " + winnerName + "\n" +
            (winner ? "Kekayaan: " + fmtMoney(winner->getWealth()) : "");
        ps.options = {"OK"};
        window.showPopup(ps);
        cout << "\n========================================" << endl;
        cout << "  BATAS TURN! PEMENANG: " << winnerName << endl;
        cout << "========================================" << endl;
        return true;
    }

    return false;
}

// MAIN

int main() {
    // ===== 1. SETUP TERMINAL: Menu & Input Pemain =====
    cout << "============================================" << endl;
    cout << "     pOOPs: NIMONSPOLI" << endl;
    cout << "============================================" << endl;
    cout << "1. New Game" << endl;
    cout << "2. Load Game" << endl;
    cout << "Pilihan: ";

    int menuChoice;
    cin >> menuChoice;

    // Load config
    ConfigLoader *config = ConfigLoader::getInstance();
    config->setConfigFilePath("config");
    try {
        config->loadAllConfigs();
        cout << "[CONFIG] Konfigurasi berhasil dimuat." << endl;
    } catch (exception &e) {
        cerr << "[ERROR] Gagal memuat konfigurasi: " << e.what() << endl;
        return 1;
    }

    Board *board = Board::getInstance();
    TransactionLogger logger;
    GameEngine engine(board, &logger);

    vector<string> playerNames;

    if (menuChoice == 1) {
        int numPlayers;
        cout << "Jumlah pemain (2-4): ";
        cin >> numPlayers;
        if (numPlayers < 2)
            numPlayers = 2;
        if (numPlayers > 4)
            numPlayers = 4;

        for (int i = 0; i < numPlayers; i++) {
            string name;
            cout << "Nama pemain " << (i + 1) << ": ";
            cin >> name;
            playerNames.push_back(name);
        }

        engine.startNewGame(playerNames);
        cout << "\n[GAME] Permainan dimulai dengan " << numPlayers << " pemain!"
             << endl;

        // Print turn order
        cout << "[GAME] Urutan giliran: ";
        for (Player *p : engine.getAllPlayers())
            cout << p->getUsername() << " ";
        cout << endl;
    } else {
        cout << "Masukkan nama file save: ";
        string filename;
        cin >> filename;
        engine.loadGame(filename);
        cout << "[LOAD] (TODO: Load game belum terimplementasi di GameEngine)"
             << endl;
        // Fallback: buat game baru
        cout << "Fallback: memulai game baru..." << endl;
        playerNames = {"Player1", "Player2"};
        engine.startNewGame(playerNames);
    }

    // ===== 2. BUAT WINDOW =====
    GameWindow window(1280, 800, "pOOPs: NIMONSPOLI");
    window.init();

    phase = Phase::AWAITING_ACTION;
    turnJustStarted = true;

    // ===== 3. REGISTER CALLBACKS =====

    // LEMPAR DADU
    window.onCommand("LEMPAR_DADU", [&]() {
        if (phase != Phase::AWAITING_ACTION) {
            cout << "[!] Tidak bisa lempar dadu sekarang." << endl;
            return;
        }
        if (engine.hasDiceRolled()) {
            cout << "[!] Dadu sudah dilempar giliran ini." << endl;
            return;
        }

        // Generate dice in controller so we know the values
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(1, 6);
        lastD1 = dis(gen);
        lastD2 = dis(gen);

        try {
            engine.rollDice(lastD1, lastD2);
        } catch (InvalidCommandException &e) {
            cout << "[!] " << e.what() << endl;
            return;
        }

        cout << "\n[DADU] " << engine.getCurrentPlayer()->getUsername() << ": "
             << lastD1 << " + " << lastD2 << " = " << (lastD1 + lastD2) << endl;

        logger.logEvent(engine.getCurrentRound(),
                        engine.getCurrentPlayer()->getUsername(),
                        LogActionType::ROLL,
                        to_string(lastD1) + "+" + to_string(lastD2) + "=" +
                            to_string(lastD1 + lastD2));

        // Check if player went to jail (triple double)
        if (engine.getCurrentPlayer()->getStatus() == PlayerStatus::JAILED) {
            cout << "[PENJARA] Triple double! Masuk penjara." << endl;
            phase = Phase::POST_ROLL;
            return;
        }

        phase = Phase::ROLLED;
        processTileEffect(engine, window, logger);
    });

    // AKHIRI GILIRAN
    window.onCommand("AKHIRI_GILIRAN", [&]() {
        if (phase != Phase::POST_ROLL && phase != Phase::AWAITING_ACTION) {
            if (!engine.hasDiceRolled()) {
                cout << "[!] Lempar dadu dulu sebelum akhiri giliran." << endl;
            } else {
                cout << "[!] Selesaikan aksi yang sedang berjalan." << endl;
            }
            return;
        }
        if (!engine.hasDiceRolled()) {
            cout << "[!] Lempar dadu dulu!" << endl;
            return;
        }

        try {
            engine.endTurn();
        } catch (InvalidCommandException &e) {
            cout << "[!] " << e.what() << endl;
            return;
        } catch (GameStateException &e) {
            // Game over
            cout << "[GAME] " << e.what() << endl;
        }

        lastD1 = 0;
        lastD2 = 0;

        if (checkGameOver(engine, window))
            return;

        turnJustStarted = true;
        phase = Phase::AWAITING_ACTION;
        cout << "\n--- Giliran: " << engine.getCurrentPlayer()->getUsername()
             << " (Turn " << engine.getCurrentRound() << ") ---" << endl;
    });

    // GADAI
    window.onCommand("GADAI", [&]() {
        if (phase != Phase::AWAITING_ACTION && phase != Phase::POST_ROLL)
            return;
        Player *p = engine.getCurrentPlayer();
        gadaiCandidates.clear();
        vector<string> opts;
        for (PropertyTile *prop : p->getOwnedProperties()) {
            if (prop->getStatus() == 1) { // OWNED
                StreetTile *st = dynamic_cast<StreetTile *>(prop);
                if (st && st->hasBuildings())
                    continue; // Harus jual bangunan dulu
                gadaiCandidates.push_back(prop);
                opts.push_back(prop->getKode() + " - Gadai: " +
                               fmtMoney(prop->getmortgageValue()));
            }
        }
        if (opts.empty()) {
            cout << "[GADAI] Tidak ada properti yang bisa digadaikan." << endl;
            return;
        }
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
        if (phase != Phase::AWAITING_ACTION && phase != Phase::POST_ROLL)
            return;
        Player *p = engine.getCurrentPlayer();
        gadaiCandidates.clear();
        vector<string> opts;
        for (PropertyTile *prop : p->getOwnedProperties()) {
            if (prop->getStatus() == 2) { // MORTGAGED
                gadaiCandidates.push_back(prop);
                opts.push_back(prop->getKode() +
                               " - Tebus: " + fmtMoney(prop->getPrice()));
            }
        }
        if (opts.empty()) {
            cout << "[TEBUS] Tidak ada properti yang sedang digadaikan."
                 << endl;
            return;
        }
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
        if (phase != Phase::AWAITING_ACTION && phase != Phase::POST_ROLL)
            return;
        Player *p = engine.getCurrentPlayer();

        // Find monopolized color groups with buildable streets
        map<string, vector<StreetTile *>> buildable;
        for (PropertyTile *prop : p->getOwnedProperties()) {
            StreetTile *st = dynamic_cast<StreetTile *>(prop);
            if (st && st->getRentLevel() < 5) {
                // Check if color group is fully monopolized (all owned by this
                // player)
                buildable[st->getColorGroup()].push_back(st);
            }
        }

        // Filter: only groups that are actually monopolized
        map<string, int> totalInGroup;
        for (int i = 1; i <= board->getTotalTiles(); i++) {
            StreetTile *st = dynamic_cast<StreetTile *>(board->getTileAt(i));
            if (st)
                totalInGroup[st->getColorGroup()]++;
        }

        vector<string> opts;
        vector<string> colorGroups;
        for (auto &[color, streets] : buildable) {
            if ((int)streets.size() == totalInGroup[color]) {
                colorGroups.push_back(color);
                opts.push_back("[" + color + "] " + to_string(streets.size()) +
                               " properti");
            }
        }

        if (opts.empty()) {
            cout << "[BANGUN] Tidak ada color group yang memenuhi syarat."
                 << endl;
            return;
        }

        PopupState ps;
        ps.type = PopupType::BUY_PROPERTY;
        ps.title = "BANGUN";
        ps.message = "Pilih color group:";
        ps.options = opts;
        window.showPopup(ps);
        pending = Pending::BANGUN_COLOR_SELECT;
    });

    // CETAK PAPAN (terminal output)
    window.onCommand("CETAK_PAPAN", [&]() {
        cout << "\n=== PAPAN PERMAINAN ===" << endl;
        board->printBoardStatus();
    });

    // CETAK AKTA (terminal output)
    window.onCommand("CETAK_AKTA", [&]() {
        cout << "\nMasukkan kode petak: ";
        string kode;
        cin >> kode;
        Tile *tile = board->getTileByKode(kode);
        if (!tile) {
            cout << "Petak \"" << kode << "\" tidak ditemukan." << endl;
            return;
        }
        PropertyTile *prop = dynamic_cast<PropertyTile *>(tile);
        if (!prop) {
            cout << "Petak \"" << kode << "\" bukan properti." << endl;
            return;
        }
        cout << "+================================+" << endl;
        cout << "|        AKTA KEPEMILIKAN        |" << endl;
        StreetTile *st = dynamic_cast<StreetTile *>(prop);
        if (st) {
            cout << "|    [" << st->getColorGroup() << "] " << st->getName()
                 << " (" << st->getKode() << ")" << endl;
            cout << "+================================+" << endl;
            cout << "| Harga Beli : " << fmtMoney(st->getPrice()) << endl;
            cout << "| Nilai Gadai: " << fmtMoney(st->getmortgageValue())
                 << endl;
            cout << "| Rumah      : " << fmtMoney(st->getHouseCost()) << endl;
            cout << "| Hotel      : " << fmtMoney(st->getHotelCost()) << endl;
            cout << "| Level      : " << st->getRentLevel() << endl;
        } else {
            cout << "|    " << prop->getName() << " (" << prop->getKode() << ")"
                 << endl;
            cout << "+================================+" << endl;
            cout << "| Nilai Gadai: " << fmtMoney(prop->getmortgageValue())
                 << endl;
        }
        cout << "| Status     : " << propStatusStr(prop->getStatus()) << endl;
        if (prop->getOwnerId() != -1)
            cout << "| Pemilik    : "
                 << findOwnerName(prop->getOwnerId(), engine.getAllPlayers())
                 << endl;
        cout << "+================================+" << endl;
    });

    // CETAK PROPERTI (terminal output)
    window.onCommand("CETAK_PROPERTI", [&]() {
        Player *p = engine.getCurrentPlayer();
        cout << "\n=== Properti Milik: " << p->getUsername() << " ===" << endl;
        if (p->getOwnedProperties().empty()) {
            cout << "Kamu belum memiliki properti apapun." << endl;
            return;
        }
        for (PropertyTile *prop : p->getOwnedProperties()) {
            StreetTile *st = dynamic_cast<StreetTile *>(prop);
            cout << "  - " << prop->getName() << " (" << prop->getKode() << ") "
                 << propStatusStr(prop->getStatus());
            if (st) {
                cout << " [" << st->getColorGroup() << "] Lv"
                     << st->getRentLevel();
            }
            cout << endl;
        }
        cout << "Total kekayaan: " << fmtMoney(p->getWealth()) << endl;
    });

    // CETAK LOG (terminal output)
    window.onCommand("CETAK_LOG", [&]() {
        auto logs = logger.getLogs(-1);
        cout << "\n=== Log Transaksi ===" << endl;
        for (auto &le : logs) {
            cout << "[Turn " << le.turn << "] " << le.username << " | "
                 << actionTypeToString(le.actionType) << " | " << le.detail
                 << endl;
        }
    });

    // SIMPAN
    window.onCommand("SIMPAN", [&]() {
        cout << "\nMasukkan nama file: ";
        string filename;
        cin >> filename;
        engine.saveGame(filename);
        cout << "[SAVE] (TODO: Save game di GameEngine)" << endl;
    });

    // MUAT
    window.onCommand("MUAT", [&]() {
        cout << "[MUAT] Muat hanya bisa dilakukan sebelum permainan dimulai."
             << endl;
    });

    // GUNAKAN KEMAMPUAN (kartu di tangan - index 0, 1, 2)
    for (int ci = 0; ci < 3; ci++) {
        window.onCommand("GUNAKAN_KEMAMPUAN_" + to_string(ci), [&, ci]() {
            if (phase != Phase::AWAITING_ACTION) {
                cout << "[!] Kartu hanya bisa digunakan sebelum lempar dadu."
                     << endl;
                return;
            }
            if (engine.hasDiceRolled()) {
                cout << "[!] Kartu hanya bisa digunakan SEBELUM lempar dadu."
                     << endl;
                return;
            }
            Player *p = engine.getCurrentPlayer();
            if (p->getHasUsedCardThisTurn()) {
                cout << "[!] Sudah menggunakan kartu giliran ini." << endl;
                return;
            }
            auto &cards = p->getHandCards();
            if (ci >= (int)cards.size())
                return;

            SkillCard *card = cards[ci];
            cout << "[KARTU] Menggunakan " << card->getCardName() << "!"
                 << endl;
            card->use(*p, engine);
            p->setHasUsedCardThisTurn(true);
            p->removeCard(ci);
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::CARD, card->getCardName());
        });
    }

    // POPUP RESPONSE
    window.onPopupOption(
        [&](int idx) { handlePopupResponse(idx, engine, window, logger); });

    // MAIN
    cout << "\n--- Giliran: " << engine.getCurrentPlayer()->getUsername()
         << " (Turn " << engine.getCurrentRound() << ") ---" << endl;

    while (window.isRunning()) {
        // Handle turn start logic
        if (turnJustStarted && phase == Phase::AWAITING_ACTION) {
            turnJustStarted = false;
            Player *p = engine.getCurrentPlayer();

            if (p->getStatus() == PlayerStatus::BANKRUPT) {
                // Skip bankrupt player
                try {
                    engine.endTurn();
                } catch (...) {
                }
                if (checkGameOver(engine, window))
                    continue;
                turnJustStarted = true;
                continue;
            }

            // Reset discount from previous turn
            p->setActiveDiscountPercent(0);

            // TODO: Draw skill card (koordinasi Orang 4 - Miguel)
            // Sementara skip card draw

            // Check if jailed → show jail popup
            if (p->getStatus() == PlayerStatus::JAILED) {
                int fine =
                    50 /* TODO: needs ConfigLoader::getJailFine() getter */;
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

        // Update GUI state
        GameState gs = buildGameState(engine, logger);
        window.updateState(gs);
        window.tick();
    }

    cout << "\nTerima kasih telah bermain pOOPs: NIMONSPOLI!" << endl;
    return 0;
}
