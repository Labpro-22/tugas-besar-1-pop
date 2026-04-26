// main.cpp — Controller: menghubungkan GameEngine <-> GameWindow (fully GUI)

#include "../include/core/ComputerAI.hpp"
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
#include <cctype>
#include <map>
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
    AKTA_SELECT,         // Pilih properti untuk cetak akta
    PAPAN_INFO,          // Info papan permainan
    PROPERTI_INFO,       // Info properti pemain
    TELEPORT_SELECT,     // Pilih tujuan TeleportCard
    LASSO_SELECT,        // Pilih target LassoCard
    DEMOLITION_SELECT,   // Pilih properti untuk DemolitionCard
};

// Global state
static Phase phase = Phase::MENU;
static Pending pending = Pending::NONE;
static int lastD1 = 0, lastD2 = 0;
static bool turnJustStarted = true;
static bool gameStarted = false;

// ---- COM (Computer Player) state ----
static std::map<int, ComputerAI::Difficulty> comPlayers; // player ID → difficulty

// Forward declaration — definisi ada di bagian COM helpers
static bool isComPlayer(int playerId);
static int comDelayCounter = 0;
static const int COM_DELAY = 50; // ~0.83 detik pada 60fps sebelum COM bertindak

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

// SkillCard interactive state
static SkillCard *pendingSkillCard = nullptr;
static SkillCard *pendingNewCard = nullptr;   // kartu yang belum bisa masuk (tangan penuh)
static bool newPlayerTurnStarted = false;     // true hanya saat giliran pemain baru (bukan bonus double)

// Festival duration tracker (tile index 1-based → sisa giliran)
static map<int, int> festivalDurations;
static vector<Player *> lassoTargets;
static vector<StreetTile *> demolitionCandidates;

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
        if (prop->isRailroad()) rrCount++;
        if (prop->isUtility())  utCount++;
    }
    for (PropertyTile *prop : player->getOwnedProperties()) {
        if (prop->isRailroad())
            static_cast<RailroadTile *>(prop)->setrailroadOwnedCount(rrCount);
        if (prop->isUtility())
            static_cast<UtilityTile *>(prop)->setUtilityOwnedCount(utCount);
    }
}

static void checkMonopoly(Player *player, Board *board) {
    map<string, int> totalInGroup;
    map<string, int> ownedInGroup;
    for (int i = 1; i <= board->getTotalTiles(); i++) {
        Tile *t = board->getTileAt(i);
        if (t && t->isStreet())
            totalInGroup[t->getColorGroup()]++;
    }
    for (PropertyTile *prop : player->getOwnedProperties()) {
        if (prop->isStreet())
            ownedInGroup[prop->getColorGroup()]++;
    }
    for (PropertyTile *prop : player->getOwnedProperties()) {
        if (prop->isStreet()) {
            string cg = prop->getColorGroup();
            static_cast<StreetTile *>(prop)->setMonopolized(
                ownedInGroup[cg] == totalInGroup[cg]);
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
        pi.hasUsedCardThisTurn = p->getHasUsedCardThisTurn();
        pi.isCom = isComPlayer(p->getId());
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

        ti.tileType = tile->getTileCategory();

        if (tile->isProperty()) {
            PropertyTile *prop = static_cast<PropertyTile *>(tile);
            ti.price       = prop->getPrice();
            ti.propStatus  = propStatusStr(prop->getStatus());
            ti.ownerName   = findOwnerName(prop->getOwnerId(), players);
            ti.festivalMult = prop->getFestivalMultiplier(); // baca multiplier nyata via virtual
        }
        if (tile->isStreet()) {
            ti.colorGroup = tile->getColorGroup();
            int rl = tile->getRentLevel();
            if (rl == 5) { ti.hasHotel = true; ti.houseCount = 0; }
            else          { ti.houseCount = rl; }
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

    // Phase string untuk GUI (button disabled state dll)
    switch (phase) {
    case Phase::AWAITING_ACTION:  gs.currentPhase = "AWAITING_ACTION"; break;
    case Phase::ROLLED:           gs.currentPhase = "ROLLED"; break;
    case Phase::EFFECT_PENDING:   gs.currentPhase = "EFFECT_PENDING"; break;
    case Phase::POST_ROLL:        gs.currentPhase = "POST_ROLL"; break;
    case Phase::AUCTION_ACTIVE:   gs.currentPhase = "AUCTION_ACTIVE"; break;
    case Phase::GAME_OVER:        gs.currentPhase = "GAME_OVER"; break;
    default:                      gs.currentPhase = "MENU"; break;
    }
    gs.isComTurn = gameStarted && engine.getCurrentPlayer() &&
                   isComPlayer(engine.getCurrentPlayer()->getId());

    gs.canBuyCurrentTile = false;
    if (gameStarted) {
        Player *cp = engine.getCurrentPlayer();
        if (cp) {
            Tile *ct = board->getTileAt(cp->getPosition() + 1);
            if (ct && ct->isProperty()) {
                PropertyTile *pt = static_cast<PropertyTile *>(ct);
                gs.canBuyCurrentTile = (pt->getStatus() == 0 && pt->getOwnerId() == -1);
            }
        }
    }

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
        pendingProp = nullptr;
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
        pendingProp = nullptr;
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
        pendingProp = nullptr;
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
        if (!tile->isProperty()) { phase = Phase::POST_ROLL; break; }
        PropertyTile *prop = static_cast<PropertyTile *>(tile);
        pendingProp = prop;
        // Tidak auto-popup: biarkan pemain tekan BELI, atau AKHIRI_GILIRAN untuk lelang
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::AUTO_ACQUIRE: {
        if (!tile->isProperty()) { phase = Phase::POST_ROLL; break; }
        PropertyTile *prop = static_cast<PropertyTile *>(tile);
        if (prop->getOwnerId() == -1) {
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
        if (!tile->isProperty()) { phase = Phase::POST_ROLL; break; }
        PropertyTile *prop = static_cast<PropertyTile *>(tile);

        Player *owner = nullptr;
        for (Player *pl : engine.getAllPlayers())
            if (pl->getId() == prop->getOwnerId()) { owner = pl; break; }
        if (!owner) { phase = Phase::POST_ROLL; break; }

        // Shield memblokir sewa (QnA Q45)
        if (p->isShieldActive()) {
            p->setShieldActive(false);
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::RENT, "Sewa DICEGAH oleh Shield (" + prop->getKode() + ")");
            phase = Phase::POST_ROLL;
            break;
        }

        int diceTotal = lastD1 + lastD2;
        int rent = prop->calcRent(diceTotal);

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
                // Salin dulu sebelum modifikasi vektor
                auto bkProps = vector<PropertyTile*>(
                    p->getOwnedProperties().begin(), p->getOwnedProperties().end());
                for (PropertyTile *op : bkProps) {
                    op->setOwnerId(owner->getId());
                    op->setStatus(1);
                    owner->addProperty(op);
                    p->removeProperty(op);
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
        if (!tile->isTaxTile()) { phase = Phase::POST_ROLL; break; }
        TaxTile *tax = static_cast<TaxTile *>(tile);

        // Shield memblokir pajak (QnA Q45)
        if (p->isShieldActive()) {
            p->setShieldActive(false);
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::TAX, "Pajak PPH DICEGAH oleh Shield");
            phase = Phase::POST_ROLL;
            break;
        }

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
        if (!tile->isTaxTile()) { phase = Phase::POST_ROLL; break; }
        TaxTile *tax = static_cast<TaxTile *>(tile);

        // Shield memblokir pajak (QnA Q45)
        if (p->isShieldActive()) {
            p->setShieldActive(false);
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::TAX, "Pajak PBM DICEGAH oleh Shield");
            phase = Phase::POST_ROLL;
            break;
        }

        int amount = tax->getFlatAmount();
        try {
            *p -= amount;
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::TAX, "PBM " + fmtMoney(amount));
        } catch (NotEnoughMoneyException &) {
            // Reset properti ke bank sebelum bangkrut
            auto bkProps = vector<PropertyTile*>(
                p->getOwnedProperties().begin(), p->getOwnedProperties().end());
            for (PropertyTile *op : bkProps) {
                op->setOwnerId(-1);
                op->setStatus(0);
                if (op->isStreet()) static_cast<StreetTile*>(op)->demolish();
                p->removeProperty(op);
            }
            p->declareBankruptcy();
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::BANKRUPT, "Bangkrut PBM");
        }
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::DRAW_CHANCE: {
        ActionCard *drawn = engine.drawChanceCard();
        if (!drawn) { phase = Phase::POST_ROLL; break; }
        string cardName = drawn->getCardName();
        ActionCardEffect eff = drawn->execute(*p, engine);
        engine.discardChanceCard(drawn);

        if (eff == ActionCardEffect::CHANCE_NEAREST_STATION) {
            int pos = p->getPosition();
            int stations[] = {5, 15, 25, 35};
            int nearest = stations[0];
            for (int s : stations)
                if (s > pos) { nearest = s; break; }
            p->move((nearest - pos + 40) % 40);
        } else if (eff == ActionCardEffect::CHANCE_MOVE_BACK_3) {
            p->move(37); // 37 ≡ -3 mod 40
        } else if (eff == ActionCardEffect::CHANCE_GO_TO_JAIL) {
            p->goToJail();
        }
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::CARD, "Kesempatan: " + cardName);
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::DRAW_COMMUNITY: {
        ActionCard *drawn = engine.drawCommunityCard();
        if (!drawn) { phase = Phase::POST_ROLL; break; }
        string cardName = drawn->getCardName();
        ActionCardEffect eff = drawn->execute(*p, engine);
        engine.discardCommunityCard(drawn);

        auto allPlayers = engine.getAllPlayers();
        if (eff == ActionCardEffect::COMMUNITY_BIRTHDAY) {
            // Ulang tahun: terima M100 dari tiap pemain lain
            // Shield tidak melindungi pemain lain yang membayar birthday
            for (Player *other : allPlayers) {
                if (other != p && other->getStatus() != PlayerStatus::BANKRUPT) {
                    try { *other -= 100; *p += 100; } catch (...) {}
                }
            }
        } else if (eff == ActionCardEffect::COMMUNITY_DOCTOR) {
            // Dokter: bayar M700 — Shield melindungi (QnA Q45)
            if (p->isShieldActive()) {
                p->setShieldActive(false);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::CARD, "Dana Umum Dokter DICEGAH oleh Shield");
            } else {
                try { *p -= 700; }
                catch (NotEnoughMoneyException &) {
                    auto bkProps = vector<PropertyTile*>(
                        p->getOwnedProperties().begin(), p->getOwnedProperties().end());
                    for (PropertyTile *op : bkProps) {
                        op->setOwnerId(-1); op->setStatus(0);
                        if (op->isStreet()) static_cast<StreetTile*>(op)->demolish();
                        p->removeProperty(op);
                    }
                    p->declareBankruptcy();
                }
            }
        } else if (eff == ActionCardEffect::COMMUNITY_ELECTION) {
            // Pemilu: bayar M200 ke tiap pemain lain — Shield melindungi (QnA Q45)
            if (p->isShieldActive()) {
                p->setShieldActive(false);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::CARD, "Dana Umum Pemilu DICEGAH oleh Shield");
            } else {
                bool bankrupt = false;
                for (Player *other : allPlayers) {
                    if (other != p && other->getStatus() != PlayerStatus::BANKRUPT) {
                        try { *p -= 200; *other += 200; }
                        catch (NotEnoughMoneyException &) {
                            auto bkProps = vector<PropertyTile*>(
                                p->getOwnedProperties().begin(), p->getOwnedProperties().end());
                            for (PropertyTile *op : bkProps) {
                                op->setOwnerId(-1); op->setStatus(0);
                                if (op->isStreet()) static_cast<StreetTile*>(op)->demolish();
                                p->removeProperty(op);
                            }
                            p->declareBankruptcy();
                            bankrupt = true;
                            break;
                        }
                    }
                }
                (void)bankrupt;
            }
        }
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::CARD, "Dana Umum: " + cardName);
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::FESTIVAL_TRIGGER: {
        auto props = p->getOwnedProperties();
        vector<string> opts;
        for (auto *prop : props) {
            if (!prop->isStreet()) continue; // Railroad/Utility butuh perubahan model
            int curMult = prop->getFestivalMultiplier();
            if (curMult >= 8) continue; // sudah maks
            string label = prop->getKode() + " - " + prop->getName();
            if (curMult > 1) label += " (saat ini x" + to_string(curMult) + " → x" + to_string(curMult * 2) + ")";
            else             label += " (x2, 3 giliran)";
            opts.push_back(label);
        }
        if (opts.empty()) { phase = Phase::POST_ROLL; break; }
        PopupState ps;
        ps.type = PopupType::FESTIVAL;
        ps.title = "FESTIVAL";
        ps.message = "Pilih properti untuk efek festival (stackable x2→x4→x8, 3 giliran):";
        ps.options = opts;
        window.showPopup(ps);
        pending = Pending::FESTIVAL;
        phase = Phase::EFFECT_PENDING;
        break;
    }
    case EffectType::SEND_TO_JAIL: {
        // Shield memblokir masuk penjara (QnA Q45)
        if (p->isShieldActive()) {
            p->setShieldActive(false);
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::UNKNOWN, "Masuk Penjara DICEGAH oleh Shield");
            phase = Phase::POST_ROLL;
            break;
        }
        p->goToJail();
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::UNKNOWN, "Masuk penjara (Pergi ke Penjara)");
        phase = Phase::POST_ROLL;
        break;
    }
    case EffectType::AWARD_SALARY: {
        int salary = 200;
        *p += salary;
        logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                        LogActionType::GO, "Mendarat di GO, terima M200");
        // Dapat kartu skill saat melewati/mendarat di GO
        SkillCard *drawn = engine.drawSkillCard();
        if (drawn) {
            try {
                p->addCard(drawn);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::CARD, "Dapat kartu: " + drawn->getCardName());
            } catch (...) {
                // Tangan penuh (3 kartu), kembalikan ke deck
                engine.discardSkillCard(drawn);
            }
        }
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
            pendingProp = nullptr;
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
                // Reset properti ke bank sebelum bangkrut
                auto bkProps = vector<PropertyTile*>(
                    p->getOwnedProperties().begin(), p->getOwnedProperties().end());
                for (PropertyTile *op : bkProps) {
                    op->setOwnerId(-1); op->setStatus(0);
                    if (op->isStreet()) static_cast<StreetTile*>(op)->demolish();
                    p->removeProperty(op);
                }
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
        // Bangun ulang daftar yang sama seperti saat popup dibuat (Street, maks <8)
        int selectIdx = 0;
        for (auto *prop : props) {
            if (!prop->isStreet()) continue;
            int curMult = prop->getFestivalMultiplier();
            if (curMult >= 8) continue;
            if (selectIdx == choice) {
                StreetTile *st = static_cast<StreetTile*>(prop);
                st->setFestivalEffect(2); // stacking: 1→2→4→8
                festivalDurations[prop->getIndex()] = 3; // 3 giliran
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::FESTIVAL,
                                prop->getName() + " (x" + to_string(st->getFestivalMultiplier()) + ")");
                break;
            }
            selectIdx++;
        }
        pending = Pending::NONE;
        phase = Phase::POST_ROLL;
        break;
    }
    case Pending::JAIL_CHOICE: {
        window.closePopup();
        int fine = ConfigLoader::getInstance()->getJailFine();
        if (choice == 0) {
            try {
                *p -= fine;
                p->setStatus(PlayerStatus::ACTIVE);
                p->setJailTurnsLeft(0);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::UNKNOWN, "Bayar denda penjara " + fmtMoney(fine));
                phase = Phase::AWAITING_ACTION; // bisa lempar dadu setelah bebas
            } catch (NotEnoughMoneyException &) {
                auto bkProps = vector<PropertyTile*>(
                    p->getOwnedProperties().begin(), p->getOwnedProperties().end());
                for (PropertyTile *op : bkProps) {
                    op->setOwnerId(-1); op->setStatus(0);
                    if (op->isStreet()) static_cast<StreetTile*>(op)->demolish();
                    p->removeProperty(op);
                }
                p->declareBankruptcy();
                phase = Phase::POST_ROLL;
            }
        } else {
            // Lempar dadu — coba double untuk keluar penjara
            random_device rd2;
            mt19937 gen2(rd2());
            uniform_int_distribution<> dis2(1, 6);
            lastD1 = dis2(gen2);
            lastD2 = dis2(gen2);
            try {
                engine.rollDice(lastD1, lastD2);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::ROLL,
                                to_string(lastD1) + "+" + to_string(lastD2) +
                                    "=" + to_string(lastD1 + lastD2));
                if (engine.getCurrentPlayer()->getStatus() == PlayerStatus::JAILED) {
                    // Non-double: tetap di penjara, kurangi sisa percobaan
                    p->decrementJailTurn();
                    phase = Phase::POST_ROLL;
                } else {
                    // Double: bebas dari penjara, proses petak
                    phase = Phase::ROLLED;
                    processTileEffect(engine, window, logger);
                }
            } catch (...) {
                phase = Phase::AWAITING_ACTION;
            }
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
            // Biaya tebus = nilai gadai + 10% (spec)
            int cost = static_cast<int>(prop->getmortgageValue() * 1.1);
            try {
                *p -= cost;
                prop->unmortgage();
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::UNMORTGAGE,
                                prop->getName() + " " + fmtMoney(cost));
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
            if (prop->isStreet() && prop->getRentLevel() < 5)
                monopolized[prop->getColorGroup()].push_back(
                    static_cast<StreetTile *>(prop));
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
        // Jika drop dipicu oleh kartu awal giliran, masukkan kartu baru
        if (pendingNewCard) {
            try {
                p->addCard(pendingNewCard);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::CARD,
                                "Dapat kartu awal giliran: " + pendingNewCard->getCardName());
            } catch (...) {
                engine.discardSkillCard(pendingNewCard);
            }
            pendingNewCard = nullptr;
            // Cek jail popup yang tadi ditunda
            if (p->getStatus() == PlayerStatus::JAILED) {
                int fine = ConfigLoader::getInstance()->getJailFine();
                if (p->getJailTurnsLeft() == 0) {
                    // Wajib bayar
                    try {
                        *p -= fine;
                        p->setStatus(PlayerStatus::ACTIVE);
                        p->setJailTurnsLeft(0);
                    } catch (NotEnoughMoneyException &) {
                        p->declareBankruptcy();
                    }
                } else {
                    PopupState ps;
                    ps.type = PopupType::JAIL;
                    ps.title = "PENJARA";
                    ps.message = p->getUsername() + " sedang di penjara.\n" +
                                 "Sisa percobaan: " + to_string(p->getJailTurnsLeft()) +
                                 " giliran\nBayar denda " + fmtMoney(fine) +
                                 " atau coba lempar double?";
                    ps.options = {"Bayar Denda " + fmtMoney(fine), "Lempar Dadu"};
                    window.showPopup(ps);
                    pending = Pending::JAIL_CHOICE;
                    phase = Phase::EFFECT_PENDING;
                    return;
                }
            }
        }
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

            if (prop->isStreet()) {
                lines.push_back("  [" + prop->getColorGroup() + "] " + prop->getName() + " (" + prop->getKode() + ")");
                lines.push_back("+================================+");
                lines.push_back("  Harga Beli : " + fmtMoney(prop->getPrice()));
                lines.push_back("  Nilai Gadai: " + fmtMoney(prop->getmortgageValue()));
                lines.push_back("  Biaya Rumah: " + fmtMoney(prop->getHouseCost()));
                lines.push_back("  Biaya Hotel: " + fmtMoney(prop->getHotelCost()));
                lines.push_back("  Level Bgn  : " + to_string(prop->getRentLevel()));
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
    case Pending::TELEPORT_SELECT: {
        window.closePopup();
        if (choice >= 0 && choice < engine.getBoard()->getTotalTiles() && pendingSkillCard) {
            // choice 0 = tile 1 = posisi 0
            int oldPos = p->getPosition();
            p->setPosition(choice);

            // Award gaji GO jika melewati GO (posisi baru < posisi lama, artinya wrap)
            if (choice < oldPos) {
                int salary = ConfigLoader::getInstance()->getGoSalary();
                *p += salary;
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::GO,
                                "TeleportCard melewati GO, terima " + fmtMoney(salary));
            }

            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::CARD,
                            "TeleportCard → petak " + to_string(choice + 1));
            auto &cards = p->getHandCards();
            for (int i = 0; i < (int)cards.size(); i++) {
                if (cards[i] == pendingSkillCard) { p->removeCard(i); break; }
            }
            p->setHasUsedCardThisTurn(true);
            pendingSkillCard = nullptr;
            engine.markDiceRolled();
            phase = Phase::ROLLED;
            processTileEffect(engine, window, logger);
        }
        pending = Pending::NONE;
        break;
    }
    case Pending::LASSO_SELECT: {
        window.closePopup();
        if (choice >= 0 && choice < (int)lassoTargets.size() && pendingSkillCard) {
            Player *target = lassoTargets[choice];
            int myPos = p->getPosition();
            target->setPosition(myPos);
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::CARD,
                            "LassoCard: " + target->getUsername() +
                                " ditarik ke petak " + to_string(myPos + 1));
            auto &cards = p->getHandCards();
            for (int i = 0; i < (int)cards.size(); i++) {
                if (cards[i] == pendingSkillCard) { p->removeCard(i); break; }
            }
            p->setHasUsedCardThisTurn(true);
            pendingSkillCard = nullptr;
        }
        lassoTargets.clear();
        pending = Pending::NONE;
        break;
    }
    case Pending::DEMOLITION_SELECT: {
        window.closePopup();
        if (choice >= 0 && choice < (int)demolitionCandidates.size() && pendingSkillCard) {
            StreetTile *st = demolitionCandidates[choice];
            st->removeOneBuilding();
            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                            LogActionType::CARD,
                            "DemolitionCard: bangunan di " + st->getKode() + " dihancurkan");
            auto &cards = p->getHandCards();
            for (int i = 0; i < (int)cards.size(); i++) {
                if (cards[i] == pendingSkillCard) { p->removeCard(i); break; }
            }
            p->setHasUsedCardThisTurn(true);
            pendingSkillCard = nullptr;
        }
        demolitionCandidates.clear();
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

        // Kumpulkan pemain aktif dan urutkan: kekayaan → jumlah properti → jumlah kartu
        vector<Player*> active;
        for (Player *p : players)
            if (p->getStatus() != PlayerStatus::BANKRUPT) active.push_back(p);

        sort(active.begin(), active.end(), [](Player *a, Player *b) {
            if (a->getWealth() != b->getWealth())
                return a->getWealth() > b->getWealth();
            if (a->getOwnedProperties().size() != b->getOwnedProperties().size())
                return a->getOwnedProperties().size() > b->getOwnedProperties().size();
            return a->getHandCards().size() > b->getHandCards().size();
        });

        string title, msg;
        if (active.size() > 1) {
            Player *top = active[0];
            // Cek seri: semua pemain aktif sama di semua tiebreaker
            bool seri = true;
            for (Player *p : active) {
                if (p->getWealth() != top->getWealth() ||
                    p->getOwnedProperties().size() != top->getOwnedProperties().size() ||
                    p->getHandCards().size() != top->getHandCards().size()) {
                    seri = false; break;
                }
            }
            if (seri) {
                title = "SERI!";
                msg = "Semua pemain aktif seimbang — semua menang!\n\nKlasemen:";
            } else {
                title = "BATAS GILIRAN TERCAPAI!";
                msg = "Pemenang: " + top->getUsername() +
                      "\nKekayaan: " + fmtMoney(top->getWealth()) +
                      "\n\nKlasemen:";
            }
        } else if (!active.empty()) {
            title = "BATAS GILIRAN TERCAPAI!";
            msg = "Pemenang: " + active[0]->getUsername() +
                  "\nKekayaan: " + fmtMoney(active[0]->getWealth());
        } else {
            title = "PERMAINAN SELESAI";
            msg = "Tidak ada pemenang.";
        }
        for (int i = 0; i < (int)active.size(); i++) {
            msg += "\n" + to_string(i + 1) + ". " + active[i]->getUsername() +
                   " — " + fmtMoney(active[i]->getWealth()) +
                   " | " + to_string(active[i]->getOwnedProperties().size()) + " prop" +
                   " | " + to_string(active[i]->getHandCards().size()) + " kartu";
        }

        PopupState ps;
        ps.type = PopupType::WINNER;
        ps.title = title;
        ps.message = msg;
        ps.options = {"OK"};
        window.showPopup(ps);
        return true;
    }
    return false;
}

// ============================================================
// COM HELPERS
// ============================================================

// Cek apakah pemain dengan ID tertentu adalah COM
static bool isComPlayer(int playerId) {
    return comPlayers.count(playerId) > 0;
}

// Kembalikan difficulty COM (hanya valid jika isComPlayer == true)
static ComputerAI::Difficulty getComDifficulty(int playerId) {
    auto it = comPlayers.find(playerId);
    if (it != comPlayers.end()) return it->second;
    return ComputerAI::Difficulty::EASY;
}

// Inisialisasi comPlayers berdasarkan nama pemain:
//   - Nama == "COM" (case-insensitive) → EASY
//   - Nama == "GOD" (case-insensitive) → HARD
static void setupComPlayers(const std::vector<Player *> &players) {
    comPlayers.clear();
    for (Player *p : players) {
        std::string name = p->getUsername();
        std::string upper = name;
        for (char &c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        // Deteksi prefix: "COM", "COM2", "COM3" → EASY; "GOD", "GOD2", "GOD3" → HARD
        if (upper.rfind("COM", 0) == 0) {
            comPlayers[p->getId()] = ComputerAI::Difficulty::EASY;
        } else if (upper.rfind("GOD", 0) == 0) {
            comPlayers[p->getId()] = ComputerAI::Difficulty::HARD;
        }
    }
}

// ---- Extracted action helpers (digunakan oleh command callbacks DAN COM) ----

static void performRollDice(GameEngine &engine, GameWindow &window,
                             TransactionLogger &logger) {
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
}

static void performEndTurn(GameEngine &engine, GameWindow &window,
                           TransactionLogger &logger) {
    if (!gameStarted) return;
    if (phase != Phase::POST_ROLL && phase != Phase::AWAITING_ACTION) return;
    if (!engine.hasDiceRolled()) return;

    // Jika pemain melewati properti tak bertuan tanpa menekan BELI → lelang
    Player *curP = engine.getCurrentPlayer();
    if (curP && pendingProp &&
        pendingProp->getStatus() == 0 && pendingProp->getOwnerId() == -1) {
        logger.logEvent(engine.getCurrentRound(), curP->getUsername(),
                        LogActionType::AUCTION, "Lewat BELI → Lelang " + pendingProp->getName());
        startAuction(pendingProp, engine, window);
        return; // tunggu lelang selesai lalu pemain klik AKHIRI lagi
    }

    int prevIdx = engine.getCurrentTurnIdx();
    try {
        engine.endTurn();
    } catch (InvalidCommandException &) {
        return;
    } catch (GameStateException &) {}

    // Kurangi durasi festival di semua street (1 giliran per endTurn)
    Board *brd = engine.getBoard();
    for (auto it = festivalDurations.begin(); it != festivalDurations.end(); ) {
        it->second--;
        if (it->second <= 0) {
            Tile *ft = brd->getTileAt(it->first);
            if (ft && ft->isStreet())
                static_cast<StreetTile*>(ft)->clearFestivalEffect();
            it = festivalDurations.erase(it);
        } else {
            ++it;
        }
    }

    lastD1 = 0; lastD2 = 0;
    if (checkGameOver(engine, window)) return;
    newPlayerTurnStarted = (engine.getCurrentTurnIdx() != prevIdx);
    turnJustStarted = true;
    phase = Phase::AWAITING_ACTION;
}

// ---- Pemicu aksi COM ----

static void triggerComAction(GameEngine &engine, GameWindow &window,
                              TransactionLogger &logger) {
    Player *p = engine.getCurrentPlayer();

    // Lelang: cek apakah penawar saat ini adalah COM
    if (phase == Phase::AUCTION_ACTIVE && auctionCtx.active &&
        !auctionCtx.bidders.empty()) {
        Player *bidder = auctionCtx.bidders[auctionCtx.currentIdx];
        if (!isComPlayer(bidder->getId())) return;

        ComputerAI::Difficulty diff = getComDifficulty(bidder->getId());
        int bid1 = max(50, auctionCtx.highestBid + 1);
        int bid2 = max(100, auctionCtx.highestBid + 51);
        int choice = ComputerAI::decideAuction(bidder, auctionCtx.property,
                                               auctionCtx.highestBid,
                                               bid1, bid2, diff);
        handlePopupResponse(choice, engine, window, logger);
        return;
    }

    if (!p || !isComPlayer(p->getId())) return;
    ComputerAI::Difficulty diff = getComDifficulty(p->getId());

    switch (phase) {
    case Phase::AWAITING_ACTION:
        if (!engine.hasDiceRolled())
            performRollDice(engine, window, logger);
        break;

    case Phase::EFFECT_PENDING:
        switch (pending) {
        case Pending::BUY_PROPERTY: {
            int choice = ComputerAI::decideBuyProperty(p, pendingProp, diff);
            handlePopupResponse(choice, engine, window, logger);
            break;
        }
        case Pending::TAX_CHOICE: {
            int choice = ComputerAI::decideTaxChoice(p, pendingTax, diff);
            handlePopupResponse(choice, engine, window, logger);
            break;
        }
        case Pending::FESTIVAL: {
            int choice = ComputerAI::decideFestival(p, diff);
            handlePopupResponse(choice, engine, window, logger);
            break;
        }
        case Pending::JAIL_CHOICE: {
            int fine = 50;
            int choice = ComputerAI::decideJailChoice(p, fine, diff);
            handlePopupResponse(choice, engine, window, logger);
            break;
        }
        case Pending::DROP_CARD: {
            int choice = ComputerAI::decideDropCard(p, pendingNewCard, diff);
            handlePopupResponse(choice, engine, window, logger);
            break;
        }
        default:
            break;
        }
        break;

    case Phase::POST_ROLL:
        // COM: keputusan beli properti jika mendarat di petak tak bertuan
        if (pendingProp && pendingProp->getStatus() == 0 && pendingProp->getOwnerId() == -1) {
            int choice = ComputerAI::decideBuyProperty(p, pendingProp, diff);
            if (choice == 0 && p->getMoney() >= pendingProp->getPrice()) {
                try {
                    *p -= pendingProp->getPrice();
                    pendingProp->setOwnerId(p->getId());
                    pendingProp->setStatus(1);
                    p->addProperty(pendingProp);
                    updatePropertyCounts(p);
                    checkMonopoly(p, engine.getBoard());
                    logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                    LogActionType::BUY,
                                    "Beli " + pendingProp->getName() + " [COM]");
                } catch (...) {}
                pendingProp = nullptr;
            }
            // choice != 0 atau tidak mampu: performEndTurn akan lelang
        }
        // HARD mode: coba bangun rumah/hotel sebelum mengakhiri giliran
        if (diff == ComputerAI::Difficulty::HARD) {
            int maxBuilds = 15; // batas loop untuk mencegah loop tak terbatas
            while (maxBuilds-- > 0) {
                std::string target =
                    ComputerAI::decideBuildTarget(p, engine.getBoard(), diff);
                if (target.empty()) break;
                try {
                    engine.buyBuilding(target);
                    logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                    LogActionType::BUILD, target + " [COM]");
                } catch (...) {
                    break;
                }
            }
        }
        performEndTurn(engine, window, logger);
        break;

    default:
        break;
    }
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
        setupComPlayers(engine.getAllPlayers()); // deteksi & daftarkan COM
        comDelayCounter = 0;
        gameStarted = true;
        window.setScreen(AppScreen::PLAYING);
        phase = Phase::AWAITING_ACTION;
        turnJustStarted = true;
        newPlayerTurnStarted = true;
        lastD1 = 0; lastD2 = 0;
    });

    window.onLoadGame([&](string filename) {
        // Try to load; on fail, show error in text input
        try {
            engine.loadGame(filename);
            setupComPlayers(engine.getAllPlayers()); // deteksi & daftarkan COM
            comDelayCounter = 0;
            gameStarted = true;
            window.setScreen(AppScreen::PLAYING);
            phase = Phase::AWAITING_ACTION;
            turnJustStarted = true;
            newPlayerTurnStarted = true;
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

    // LEMPAR DADU — sekarang mendelegasikan ke performRollDice
    window.onCommand("LEMPAR_DADU", [&]() {
        // Blokir jika pemain saat ini adalah COM (COM bertindak otomatis)
        Player *cp = engine.getCurrentPlayer();
        if (cp && isComPlayer(cp->getId())) return;
        performRollDice(engine, window, logger);
    });

    // AKHIRI GILIRAN — sekarang mendelegasikan ke performEndTurn
    window.onCommand("AKHIRI_GILIRAN", [&]() {
        // Blokir jika pemain saat ini adalah COM
        Player *cp = engine.getCurrentPlayer();
        if (cp && isComPlayer(cp->getId())) return;
        performEndTurn(engine, window, logger);
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
                if (prop->hasBuildings()) continue;
                gadaiCandidates.push_back(prop);
                opts.push_back(prop->getKode() + " - Gadai: " +
                               fmtMoney(prop->getmortgageValue()));
            }
        }
        if (opts.empty()) return;
        opts.push_back("BATAL");
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
        opts.push_back("BATAL");
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
            if (prop->isStreet() && prop->getRentLevel() < 5)
                buildable[prop->getColorGroup()].push_back(
                    static_cast<StreetTile *>(prop));
        }

        map<string, int> totalInGroup;
        for (int i = 1; i <= board->getTotalTiles(); i++) {
            Tile *t = board->getTileAt(i);
            if (t && t->isStreet())
                totalInGroup[t->getColorGroup()]++;
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

    // BELI — beli properti di petak saat ini (jika belum dimiliki, hanya setelah melempar)
    window.onCommand("BELI", [&]() {
        if (!gameStarted) return;
        if (phase != Phase::POST_ROLL) return;
        Player *p = engine.getCurrentPlayer();
        if (!p) return;
        Tile *tile = board->getTileAt(p->getPosition() + 1);
        if (!tile || !tile->isProperty()) return;
        PropertyTile *prop = static_cast<PropertyTile *>(tile);
        if (prop->getStatus() != 0 || prop->getOwnerId() != -1) return; // sudah dimiliki
        pendingProp = prop;
        if (p->getMoney() >= prop->getPrice()) {
            PopupState ps;
            ps.type = PopupType::BUY_PROPERTY;
            ps.title = "BELI PROPERTI";
            ps.message = "Kamu berada di " + prop->getName() + " (" + prop->getKode() + ")\n" +
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
            if (!tile || !tile->isProperty()) continue;
            PropertyTile *pt = static_cast<PropertyTile *>(tile);
            string ownerStr = pt->getOwnerId() == -1 ? "BANK" :
                findOwnerName(pt->getOwnerId(), engine.getAllPlayers());
            string line = "[" + to_string(i) + "] " + tile->getKode() +
                          " - " + propStatusStr(pt->getStatus()) +
                          " (" + ownerStr + ")";
            if (tile->isStreet()) {
                int rl = tile->getRentLevel();
                string bld = rl == 5 ? " | Hotel" : (rl > 0 ? " | " + to_string(rl) + " Rumah" : "");
                line += bld;
                int fm = pt->getFestivalMultiplier();
                if (fm > 1) line += " | Festival x" + to_string(fm);
            }
            lines.push_back(line);
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
            if (tile->isProperty()) {
                PropertyTile *pt = static_cast<PropertyTile *>(tile);
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

    // CETAK PROPERTI — show all players' properties
    window.onCommand("CETAK_PROPERTI", [&]() {
        if (!gameStarted) return;
        auto allPlayers = engine.getAllPlayers();
        vector<string> lines;
        lines.push_back("=== PROPERTI SEMUA PEMAIN ===");
        for (Player *pl : allPlayers) {
            lines.push_back("--- " + pl->getUsername() +
                            " (" + fmtMoney(pl->getMoney()) +
                            " | Kekayaan: " + fmtMoney(pl->getWealth()) + ")" +
                            (pl->getStatus() == PlayerStatus::BANKRUPT ? " [BANGKRUT]" :
                             pl->getStatus() == PlayerStatus::JAILED    ? " [PENJARA]"  : ""));
            if (pl->getOwnedProperties().empty()) {
                lines.push_back("  (kosong)");
            } else {
                for (PropertyTile *prop : pl->getOwnedProperties()) {
                    string info = "  " + prop->getKode() + " - " + prop->getName() +
                                  " [" + propStatusStr(prop->getStatus()) + "]";
                    if (prop->isStreet()) {
                        int rl = prop->getRentLevel();
                        info += rl == 5 ? " Hotel" : (rl > 0 ? " " + to_string(rl) + "H" : "");
                        int fm = prop->getFestivalMultiplier();
                        if (fm > 1) info += " Fest x" + to_string(fm);
                    }
                    info += " | Sewa: " + fmtMoney(prop->calcRent());
                    lines.push_back(info);
                }
            }
        }
        window.showPropertyInfo("PROPERTI SEMUA PEMAIN", lines);
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
        int from = (int)logs.size() > 20 ? (int)logs.size() - 20 : 0;
        for (int i = from; i < (int)logs.size(); i++) {
            auto &le = logs[i];
            lines.push_back("[T" + to_string(le.turn) + "] [" +
                            actionTypeToString(le.actionType) + "] " +
                            le.username + ": " + le.detail);
        }
        if ((int)logs.size() > 20) lines.push_back("... (hanya 20 terakhir ditampilkan)");
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
            SkillCardEffect eff = card->use(*p, engine);

            auto removeUsedCard = [&](int idx) {
                p->setHasUsedCardThisTurn(true);
                p->removeCard(idx);
            };

            switch (eff) {
            case SkillCardEffect::MOVE: {
                int steps = card->getSteps();
                int oldPos = p->getPosition();
                removeUsedCard(ci);
                p->move(steps);
                // Award gaji GO jika melewati GO (posisi baru < posisi lama = wrap)
                if (p->getPosition() < oldPos) {
                    int salary = ConfigLoader::getInstance()->getGoSalary();
                    *p += salary;
                    logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                    LogActionType::GO,
                                    "MoveCard melewati GO, terima " + fmtMoney(salary));
                }
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::CARD, "MoveCard maju " + to_string(steps) + " petak");
                engine.markDiceRolled();
                phase = Phase::ROLLED;
                processTileEffect(engine, window, logger);
                break;
            }
            case SkillCardEffect::DISCOUNT: {
                int pct = card->getDiscountPercent();
                p->setActiveDiscountPercent(pct);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::CARD, "DiscountCard diskon " + to_string(pct) + "%");
                removeUsedCard(ci);
                break;
            }
            case SkillCardEffect::SHIELD: {
                p->setShieldActive(true);
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::CARD, "ShieldCard: imun tagihan giliran ini");
                removeUsedCard(ci);
                break;
            }
            case SkillCardEffect::TELEPORT: {
                // Popup untuk pilih tujuan (semua petak 1–40)
                pendingSkillCard = card;
                vector<string> opts;
                for (int i = 1; i <= board->getTotalTiles(); i++) {
                    Tile *t = board->getTileAt(i);
                    opts.push_back("[" + to_string(i) + "] " + t->getName());
                }
                PopupState ps;
                ps.type = PopupType::BUY_PROPERTY;
                ps.title = "TELEPORT - Pilih Tujuan";
                ps.message = "Pilih petak tujuan:";
                ps.options = opts;
                window.showPopup(ps);
                pending = Pending::TELEPORT_SELECT;
                break;
            }
            case SkillCardEffect::LASSO: {
                // Popup pilih pemain lawan
                lassoTargets.clear();
                vector<string> opts;
                for (Player *pl : engine.getAllPlayers()) {
                    if (pl != p && pl->getStatus() != PlayerStatus::BANKRUPT) {
                        lassoTargets.push_back(pl);
                        opts.push_back(pl->getUsername() + " (petak " +
                                       to_string(pl->getPosition() + 1) + ")");
                    }
                }
                if (lassoTargets.empty()) break;
                pendingSkillCard = card;
                PopupState ps;
                ps.type = PopupType::BUY_PROPERTY;
                ps.title = "LASSO - Pilih Target";
                ps.message = "Tarik pemain lawan ke petakmu saat ini\n(posisi " +
                             to_string(p->getPosition() + 1) + "):";
                ps.options = opts;
                window.showPopup(ps);
                pending = Pending::LASSO_SELECT;
                break;
            }
            case SkillCardEffect::DEMOLITION: {
                // Popup pilih street lawan (termasuk tergadai per QnA Q34)
                demolitionCandidates.clear();
                vector<string> opts;
                for (Player *pl : engine.getAllPlayers()) {
                    if (pl == p || pl->getStatus() == PlayerStatus::BANKRUPT) continue;
                    for (PropertyTile *prop : pl->getOwnedProperties()) {
                        if (!prop->isStreet()) continue;
                        StreetTile *st = static_cast<StreetTile *>(prop);
                        demolitionCandidates.push_back(st);
                        string lvl;
                        if (prop->getStatus() == 2)       lvl = "[TERGADAI]";
                        else if (st->getRentLevel() == 0)  lvl = "Tanah kosong";
                        else if (st->getRentLevel() == 5)  lvl = "Hotel";
                        else                               lvl = to_string(st->getRentLevel()) + " Rumah";
                        opts.push_back(st->getKode() + " [" + pl->getUsername() + "] " + lvl);
                    }
                }
                if (demolitionCandidates.empty()) break;
                pendingSkillCard = card;
                PopupState ps;
                ps.type = PopupType::BUY_PROPERTY;
                ps.title = "DEMOLITION - Pilih Properti";
                ps.message = "Pilih properti lawan untuk dihancurkan 1 bangunan:";
                ps.options = opts;
                window.showPopup(ps);
                pending = Pending::DEMOLITION_SELECT;
                break;
            }
            default:
                logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                LogActionType::CARD, card->getCardName());
                removeUsedCard(ci);
                break;
            }
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
                    newPlayerTurnStarted = true;
                    turnJustStarted = true;
                    continue;
                }

                // Reset discount
                p->setActiveDiscountPercent(0);

                // Draw 1 kartu di awal giliran pemain baru (spec: "awal giliran, semua pemain mendapat 1 kartu acak")
                bool droppingCard = false;
                if (newPlayerTurnStarted) {
                    newPlayerTurnStarted = false;
                    SkillCard *newCard = engine.drawSkillCard();
                    if (newCard) {
                        if ((int)p->getHandCards().size() < 3) {
                            p->addCard(newCard);
                            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                            LogActionType::CARD,
                                            "Dapat kartu awal giliran: " + newCard->getCardName());
                        } else {
                            // Tangan penuh — paksa buang 1 kartu
                            pendingNewCard = newCard;
                            vector<string> opts;
                            for (const SkillCard *c : p->getHandCards())
                                opts.push_back(c->getCardName() + ": " + c->getCardDescription());
                            PopupState ps;
                            ps.type = PopupType::BUY_PROPERTY;
                            ps.title = "BUANG KARTU";
                            ps.message = "Tangan penuh! Pilih kartu untuk dibuang.\n"
                                         "Kartu baru: " + newCard->getCardName() +
                                         "\n" + newCard->getCardDescription();
                            ps.options = opts;
                            window.showPopup(ps);
                            pending = Pending::DROP_CARD;
                            phase = Phase::EFFECT_PENDING;
                            droppingCard = true;
                        }
                    }
                }

                // Jail popup — ditunda jika sedang DROP_CARD
                if (!droppingCard && p->getStatus() == PlayerStatus::JAILED) {
                    int fine = ConfigLoader::getInstance()->getJailFine();
                    if (p->getJailTurnsLeft() == 0) {
                        // Giliran ke-4: wajib bayar denda, tidak ada pilihan
                        try {
                            *p -= fine;
                            p->setStatus(PlayerStatus::ACTIVE);
                            p->setJailTurnsLeft(0);
                            logger.logEvent(engine.getCurrentRound(), p->getUsername(),
                                            LogActionType::UNKNOWN,
                                            "Bayar denda penjara (wajib) " + fmtMoney(fine));
                        } catch (NotEnoughMoneyException &) {
                            auto bkProps = vector<PropertyTile*>(
                                p->getOwnedProperties().begin(), p->getOwnedProperties().end());
                            for (PropertyTile *op : bkProps) {
                                op->setOwnerId(-1); op->setStatus(0);
                                if (op->isStreet()) static_cast<StreetTile*>(op)->demolish();
                                p->removeProperty(op);
                            }
                            p->declareBankruptcy();
                            phase = Phase::POST_ROLL;
                        }
                        // Lanjutkan ke fase lempar dadu (tidak perlu popup)
                    } else {
                        PopupState ps;
                        ps.type = PopupType::JAIL;
                        ps.title = "PENJARA";
                        ps.message = p->getUsername() + " sedang di penjara.\n" +
                                     "Sisa percobaan: " + to_string(p->getJailTurnsLeft()) +
                                     " giliran\nBayar denda " + fmtMoney(fine) +
                                     " atau coba lempar double?";
                        ps.options = {"Bayar Denda " + fmtMoney(fine), "Lempar Dadu"};
                        window.showPopup(ps);
                        pending = Pending::JAIL_CHOICE;
                        phase = Phase::EFFECT_PENDING;
                    }
                }
            }

            // ---- COM Automation Tick ----
            // Periksa apakah perlu memicu aksi COM pada iterasi ini
            if (phase != Phase::GAME_OVER) {
                bool needComAction = false;

                if (phase == Phase::AUCTION_ACTIVE && auctionCtx.active &&
                    !auctionCtx.bidders.empty()) {
                    // Cek apakah penawar lelang saat ini adalah COM
                    needComAction = isComPlayer(
                        auctionCtx.bidders[auctionCtx.currentIdx]->getId());
                } else if (engine.getCurrentPlayer()) {
                    Player *cp = engine.getCurrentPlayer();
                    if (isComPlayer(cp->getId())) {
                        switch (phase) {
                        case Phase::AWAITING_ACTION:
                            needComAction = !engine.hasDiceRolled();
                            break;
                        case Phase::EFFECT_PENDING:
                            // Hanya tangani pending state yang didukung COM
                            needComAction =
                                (pending == Pending::BUY_PROPERTY  ||
                                 pending == Pending::TAX_CHOICE     ||
                                 pending == Pending::FESTIVAL        ||
                                 pending == Pending::JAIL_CHOICE     ||
                                 pending == Pending::DROP_CARD);
                            break;
                        case Phase::POST_ROLL:
                            needComAction = true;
                            break;
                        default:
                            needComAction = false;
                            break;
                        }
                    }
                }

                if (needComAction) {
                    if (comDelayCounter < COM_DELAY) {
                        comDelayCounter++;
                    } else {
                        comDelayCounter = 0;
                        triggerComAction(engine, window, logger);
                    }
                } else {
                    comDelayCounter = 0;
                }
            }

            GameState gs = buildGameState(engine, logger);
            window.updateState(gs);
        }

        window.tick();
    }

    return 0;
}
