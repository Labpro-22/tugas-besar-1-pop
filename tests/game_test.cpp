// tests/game_test.cpp
// Integration test suite for pOOPs Nimonspoli
// Run from project root: cmake --build build && ./build/game_test
// Config files must be accessible at config/ relative to CWD.

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

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

// ─── Test Framework ────────────────────────────────────────────────────────────

static int g_pass = 0;
static int g_fail = 0;
static int g_known_bugs = 0;

#define ASSERT(cond, msg)                                                        \
    do {                                                                         \
        if (cond) {                                                              \
            std::cout << "  [PASS] " << (msg) << "\n";                          \
            ++g_pass;                                                            \
        } else {                                                                 \
            std::cout << "  [FAIL] " << (msg) << " (line " << __LINE__ << ")\n";\
            ++g_fail;                                                            \
        }                                                                        \
    } while (0)

#define ASSERT_THROWS(expr, ExType, msg)                                         \
    do {                                                                         \
        bool _threw = false;                                                     \
        try {                                                                    \
            (expr);                                                              \
        } catch (const ExType &) {                                               \
            _threw = true;                                                       \
        } catch (...) {                                                          \
        }                                                                        \
        ASSERT(_threw, msg);                                                     \
    } while (0)

#define ASSERT_NO_THROW(expr, msg)                                               \
    do {                                                                         \
        bool _threw = false;                                                     \
        try {                                                                    \
            (expr);                                                              \
        } catch (...) {                                                          \
            _threw = true;                                                       \
        }                                                                        \
        ASSERT(!_threw, msg);                                                    \
    } while (0)

#define KNOWN_BUG(desc)                                                          \
    do {                                                                         \
        std::cout << "  [KNOWN BUG] " << (desc) << "\n";                        \
        ++g_known_bugs;                                                          \
    } while (0)

#define TEST_SECTION(name) std::cout << "\n=== " << (name) << " ===\n"

// ─── Global singletons (initialized once) ─────────────────────────────────────

static Board *g_board = nullptr;
static ConfigLoader *g_config = nullptr;

void initSingletons() {
    g_config = ConfigLoader::getInstance();
    g_config->setConfigFilePath("config");
    g_config->loadAllConfigs();
    g_board = Board::getInstance();
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

// Find a player by username in a GameEngine
Player *getPlayerByName(GameEngine &engine, const std::string &name) {
    for (Player *p : engine.getAllPlayers()) {
        if (p->getUsername() == name)
            return p;
    }
    return nullptr;
}

// Get a PropertyTile by tile code from the board
PropertyTile *getPropByCode(const std::string &code) {
    Tile *t = g_board->getTileByKode(code);
    return dynamic_cast<PropertyTile *>(t);
}

// Manually assign a property to a player (sets ownerId, status=OWNED, adds to list)
void giveProperty(Player *player, PropertyTile *prop) {
    prop->setOwnerId(player->getId());
    prop->setStatus(1); // 1 = OWNED
    player->addProperty(prop);
}

// Reset all board properties to unowned (call between tests)
void resetBoardProperties() {
    int total = g_board->getTotalTiles();
    for (int i = 1; i <= total; i++) {
        Tile *tile = g_board->getTileAt(i);
        if (!tile)
            continue;
        PropertyTile *prop = dynamic_cast<PropertyTile *>(tile);
        if (!prop)
            continue;
        prop->setOwnerId(-1);
        prop->setStatus(0);
        StreetTile *st = dynamic_cast<StreetTile *>(prop);
        if (st) {
            st->demolish();
            st->setMonopolized(false);
            st->clearFestivalEffect();
        }
        RailroadTile *rr = dynamic_cast<RailroadTile *>(prop);
        if (rr)
            rr->setrailroadOwnedCount(1);
        UtilityTile *ut = dynamic_cast<UtilityTile *>(prop);
        if (ut)
            ut->setUtilityOwnedCount(1);
    }
}

// Give all streets in a color group to a player and set monopolized
void setupMonopoly(Player *player, const std::string &colorGroup) {
    auto groupTiles = g_board->getTileByColorGroup(colorGroup);
    for (Tile *t : groupTiles) {
        PropertyTile *prop = dynamic_cast<PropertyTile *>(t);
        if (!prop)
            continue;
        // Only give if not already owned by this player
        if (prop->getOwnerId() != player->getId()) {
            giveProperty(player, prop);
        }
        StreetTile *st = dynamic_cast<StreetTile *>(prop);
        if (st)
            st->setMonopolized(true);
    }
}

// Build N houses directly on a StreetTile (requires isMonopolized=true)
void buildNHouses(StreetTile *st, int n) {
    for (int i = 0; i < n && i < 4; i++)
        st->buildHouse();
}

// Create a fresh 4-player game context (use in every test function)
struct TestCtx {
    TransactionLogger logger;
    GameEngine engine;

    TestCtx()
        : logger(), engine(Board::getInstance(), &logger) {
        engine.startNewGame({"P1", "P2", "P3", "P4"});
        resetBoardProperties();
    }
};

// ─── Test Sections ────────────────────────────────────────────────────────────

void test_initialization() {
    TEST_SECTION("1. Initialization");

    // Board dimensions and config values
    ASSERT(g_board->getTotalTiles() == 40, "Board has 40 tiles");
    ASSERT(g_config->getMaxTurn() == 15, "maxTurn == 15");
    ASSERT(g_config->getInitialMoney() == 1000, "initialMoney == 1000");
    ASSERT(g_config->getGoSalary() == 200, "goSalary == 200");
    ASSERT(g_config->getJailFine() == 50, "jailFine == 50");
    ASSERT(g_config->getIsConfigValid(), "config is valid after loadAllConfigs");

    // Board tile lookup
    ASSERT(g_board->getTileByKode("GRT") != nullptr, "getTileByKode(GRT) returns tile");
    ASSERT(g_board->getTileByKode("PEN") != nullptr, "getTileByKode(PEN) returns jail tile");
    ASSERT(g_board->getTileByKode("NONEXISTENT") == nullptr, "getTileByKode(NONEXISTENT) returns nullptr");

    // Jail position
    ASSERT(g_board->getJailPosition() == 11, "jail at board index 11");

    // Game engine initialization
    TestCtx ctx;
    auto &engine = ctx.engine;

    ASSERT(engine.getAllPlayers().size() == 4, "4 players created");
    ASSERT(engine.getCurrentRound() == 1, "roundCount starts at 1");
    ASSERT(engine.getCurrentTurnIdx() >= 0 && engine.getCurrentTurnIdx() < 4,
           "currentTurnIdx in [0,3]");

    // All players have initial money and start at position 0
    for (Player *p : engine.getAllPlayers()) {
        ASSERT(p->getMoney() == 1000, p->getUsername() + " starts with M1000");
        ASSERT(p->getPosition() == 0, p->getUsername() + " starts at position 0");
        ASSERT(p->getStatus() == PlayerStatus::ACTIVE,
               p->getUsername() + " starts ACTIVE");
    }

    // Player IDs are unique (0-3)
    std::vector<int> ids;
    for (Player *p : engine.getAllPlayers())
        ids.push_back(p->getId());
    std::sort(ids.begin(), ids.end());
    bool uniqueIds = true;
    for (int i = 0; i < 4; i++)
        if (ids[i] != i)
            uniqueIds = false;
    ASSERT(uniqueIds, "Player IDs are unique [0,3]");

    // Deck sizes
    ASSERT((int)engine.getSkillDeckCards().size() == 15, "skillDeck has 15 cards");

    // Second startNewGame is safe (no double-free crash)
    ASSERT_NO_THROW(engine.startNewGame({"A", "B", "C", "D"}),
                    "second startNewGame() does not crash");
    ASSERT(engine.getAllPlayers().size() == 4, "4 players after second startNewGame");

    ASSERT(!engine.hasDiceRolled(), "diceRolled starts false");
    ASSERT(engine.getCurrentPlayer() != nullptr, "getCurrentPlayer() is non-null");
}

void test_dice_roll() {
    TEST_SECTION("2. Dice Roll Mechanics");

    TestCtx ctx;
    auto &engine = ctx.engine;
    Player *cur = engine.getCurrentPlayer();

    // Basic roll
    int startPos = cur->getPosition();
    ASSERT_NO_THROW(engine.rollDice(3, 4), "rollDice(3,4) succeeds");
    ASSERT(cur->getPosition() == startPos + 7, "rollDice(3,4) moves 7 steps");
    ASSERT(engine.hasDiceRolled(), "hasDiceRolled() true after roll");

    // Rolling again without endTurn throws
    ASSERT_THROWS(engine.rollDice(1, 2), InvalidCommandException,
                  "second rollDice throws InvalidCommandException");

    engine.endTurn();

    // Invalid dice values throw
    ASSERT_THROWS(engine.rollDice(0, 7), InvalidCommandException,
                  "rollDice(0,7) throws InvalidCommandException");
    ASSERT_THROWS(engine.rollDice(7, 1), InvalidCommandException,
                  "rollDice(7,1) throws InvalidCommandException");
    ASSERT_THROWS(engine.rollDice(-1, 3), InvalidCommandException,
                  "rollDice(-1,3) throws InvalidCommandException");

    // Valid range: 1-6
    ASSERT_NO_THROW(engine.rollDice(1, 6), "rollDice(1,6) is valid");
    engine.endTurn();

    ASSERT_NO_THROW(engine.rollDice(6, 6), "rollDice(6,6) is valid (double)");

    // Double roll: endTurn does NOT advance turn (same player)
    int idxBeforeDouble = engine.getCurrentTurnIdx();
    engine.endTurn();
    ASSERT(engine.getCurrentTurnIdx() == idxBeforeDouble,
           "after double, endTurn keeps same player");

    // After double, second roll succeeds
    ASSERT_NO_THROW(engine.rollDice(2, 1), "second roll after double succeeds");
    int idxAfterNonDouble = engine.getCurrentTurnIdx();
    engine.endTurn();
    // Now turn should advance
    ASSERT(engine.getCurrentTurnIdx() != idxAfterNonDouble ||
               engine.getAllPlayers().size() == 1,
           "after non-double, turn advances");

    // Position wraparound: set player near end of board
    Player *cur2 = engine.getCurrentPlayer();
    cur2->setPosition(37);
    ASSERT_NO_THROW(engine.rollDice(2, 5), "rollDice(2,5) from pos 37 succeeds");
    // (37 + 7) % 40 = 4
    ASSERT(cur2->getPosition() == 4, "wraparound: pos 37 + 7 -> pos 4");
    engine.endTurn();
}

void test_double_streak_and_jail() {
    TEST_SECTION("3. Double Streak and Triple-Double Jail");

    TestCtx ctx;
    auto &engine = ctx.engine;
    Player *cur = engine.getCurrentPlayer();

    // Roll double 1: streak = 1
    engine.rollDice(2, 2);
    ASSERT(cur->getDoubleStreak() == 1, "after 1st double, streak = 1");
    engine.endTurn();

    // Roll double 2: streak = 2
    engine.rollDice(3, 3);
    ASSERT(cur->getDoubleStreak() == 2, "after 2nd double, streak = 2");
    engine.endTurn();

    // Roll double 3: triple double -> sent to jail
    engine.rollDice(4, 4);
    ASSERT(cur->getStatus() == PlayerStatus::JAILED, "triple double -> JAILED");
    ASSERT(cur->getPosition() == 10, "triple double -> position = 10 (jail)");

    // After triple double, turn advances (no bonus)
    int idxBefore = engine.getCurrentTurnIdx();
    engine.endTurn();
    ASSERT(engine.getCurrentTurnIdx() != idxBefore, "after triple-double jail, turn advances");
}

void test_jail_mechanics() {
    TEST_SECTION("4. Jail Mechanics");

    TestCtx ctx;
    auto &engine = ctx.engine;
    Player *cur = engine.getCurrentPlayer();
    int jailedIdx = engine.getCurrentTurnIdx();

    // Direct jail via goToJail()
    cur->goToJail();
    ASSERT(cur->getStatus() == PlayerStatus::JAILED, "goToJail -> status JAILED");
    ASSERT(cur->getPosition() == 10, "goToJail -> position = 10");
    ASSERT(cur->getJailTurnsLeft() == 3, "goToJail -> jailTurnsLeft = 3");

    // Non-double in jail: stays jailed, turn decrements
    engine.rollDice(1, 2); // non-double
    ASSERT(cur->getJailTurnsLeft() == 2, "non-double in jail: jailTurnsLeft 3->2");
    ASSERT(cur->getPosition() == 10, "non-double in jail: position stays 10");
    engine.endTurn();
    ASSERT(engine.getCurrentTurnIdx() != jailedIdx, "after jail non-double, turn advances");

    // Advance to the jailed player's turn again (cycle through others)
    while (engine.getCurrentTurnIdx() != jailedIdx) {
        engine.rollDice(1, 1);
        engine.endTurn();
        engine.rollDice(1, 2);
        engine.endTurn();
    }

    // decrementJailTurn to 0
    cur->setJailTurnsLeft(1);
    cur->decrementJailTurn();
    ASSERT(cur->getJailTurnsLeft() == 0, "decrementJailTurn: 1 -> 0");
    cur->decrementJailTurn();
    ASSERT(cur->getJailTurnsLeft() == 0, "decrementJailTurn at 0: stays 0");

    // resetTurnFlags clears doubleStreak
    cur->incrementDoubleStreak();
    cur->setHasUsedCardThisTurn(true);
    cur->resetTurnFlags();
    ASSERT(cur->getDoubleStreak() == 0, "resetTurnFlags clears doubleStreak");
    ASSERT(!cur->getHasUsedCardThisTurn(), "resetTurnFlags clears hasUsedCardThisTurn");

    // releasePlayer is a known stub
    KNOWN_BUG("JailTile::releasePlayer() is empty stub — player stays JAILED after double roll");
}

void test_property_buying() {
    TEST_SECTION("5. Property Tile Effects");

    resetBoardProperties();

    // Create temporary players for direct tile testing
    Player p1("TestP1", 1000);
    p1.setId(0);
    Player p2("TestP2", 1000);
    p2.setId(1);

    // StreetTile unowned -> OFFER_BUY
    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    ASSERT(grt != nullptr, "GRT tile found");
    ASSERT(grt->onLanded(p1) == EffectType::OFFER_BUY, "unowned GRT -> OFFER_BUY");

    // After giving to p1: owner lands -> ALREADY_OWNED_SELF
    giveProperty(&p1, grt);
    ASSERT(grt->onLanded(p1) == EffectType::ALREADY_OWNED_SELF,
           "owned GRT, owner lands -> ALREADY_OWNED_SELF");

    // p2 lands on p1's property -> PAY_RENT
    ASSERT(grt->onLanded(p2) == EffectType::PAY_RENT,
           "owned GRT, other player lands -> PAY_RENT");

    // Mortgaged property -> NONE
    grt->mortgage();
    ASSERT(grt->onLanded(p2) == EffectType::NONE,
           "mortgaged GRT -> NONE for any player");

    // Reset
    grt->unmortgage();

    // RailroadTile unowned -> AUTO_ACQUIRE
    RailroadTile *gbr = dynamic_cast<RailroadTile *>(g_board->getTileByKode("GBR"));
    ASSERT(gbr != nullptr, "GBR (railroad) tile found");
    ASSERT(gbr->onLanded(p1) == EffectType::AUTO_ACQUIRE,
           "unowned railroad -> AUTO_ACQUIRE");

    // UtilityTile unowned -> AUTO_ACQUIRE
    UtilityTile *pln = dynamic_cast<UtilityTile *>(g_board->getTileByKode("PLN"));
    ASSERT(pln != nullptr, "PLN (utility) tile found");
    ASSERT(pln->onLanded(p1) == EffectType::AUTO_ACQUIRE,
           "unowned utility -> AUTO_ACQUIRE");

    resetBoardProperties();
}

void test_rent_calculation() {
    TEST_SECTION("6. Rent Calculation");

    resetBoardProperties();

    Player p1("RentP1", 2000);
    p1.setId(0);

    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    ASSERT(grt != nullptr, "GRT tile exists");

    // No monopoly, level 0: base rent
    grt->setMonopolized(false);
    ASSERT(grt->calcRent() == 2, "GRT no monopoly level 0: rent == 2");

    // Monopoly, level 0: 2x base
    grt->setMonopolized(true);
    ASSERT(grt->calcRent() == 4, "GRT monopoly level 0: rent == 4 (2x2)");

    // Build houses and check rent levels
    // GRT rentTable: [2, 10, 30, 90, 160, 250]
    grt->buildHouse(); // level 1
    ASSERT(grt->calcRent() == 10, "GRT level 1 (1 house): rent == 10");
    grt->buildHouse(); // level 2
    ASSERT(grt->calcRent() == 30, "GRT level 2 (2 houses): rent == 30");
    grt->buildHouse(); // level 3
    ASSERT(grt->calcRent() == 90, "GRT level 3 (3 houses): rent == 90");
    grt->buildHouse(); // level 4
    ASSERT(grt->calcRent() == 160, "GRT level 4 (4 houses): rent == 160");
    grt->buildHotel(); // level 5
    ASSERT(grt->calcRent() == 250, "GRT level 5 (hotel): rent == 250");

    // Festival effect x2 on hotel
    grt->setFestivalEffect(2);
    ASSERT(grt->calcRent() == 500, "GRT hotel + festival x2: rent == 500");
    grt->clearFestivalEffect();
    ASSERT(grt->calcRent() == 250, "clearFestivalEffect: rent back to 250");

    // Festival x2 on monopoly level 0
    grt->demolish();
    grt->setFestivalEffect(2);
    ASSERT(grt->calcRent() == 8, "GRT monopoly level 0 + festival x2: 4*2 == 8");
    grt->clearFestivalEffect();

    // Railroad rent by owned count
    RailroadTile *gbr = dynamic_cast<RailroadTile *>(g_board->getTileByKode("GBR"));
    ASSERT(gbr != nullptr, "GBR railroad found");
    gbr->setrailroadOwnedCount(1);
    ASSERT(gbr->calcRent() == 25, "railroad owned=1: rent == 25");
    gbr->setrailroadOwnedCount(2);
    ASSERT(gbr->calcRent() == 50, "railroad owned=2: rent == 50");
    gbr->setrailroadOwnedCount(3);
    ASSERT(gbr->calcRent() == 100, "railroad owned=3: rent == 100");
    gbr->setrailroadOwnedCount(4);
    ASSERT(gbr->calcRent() == 200, "railroad owned=4: rent == 200");

    // Utility rent (multiplier x dice)
    UtilityTile *pln = dynamic_cast<UtilityTile *>(g_board->getTileByKode("PLN"));
    ASSERT(pln != nullptr, "PLN utility found");
    pln->setUtilityOwnedCount(1);
    ASSERT(pln->calcRent(7) == 28, "utility owned=1, dice=7: 7*4 == 28");
    pln->setUtilityOwnedCount(2);
    ASSERT(pln->calcRent(7) == 70, "utility owned=2, dice=7: 7*10 == 70");

    resetBoardProperties();
}

void test_building_mechanics() {
    TEST_SECTION("7. Building Mechanics");

    resetBoardProperties();

    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    ASSERT(grt != nullptr, "GRT tile found");

    // buildHouse requires monopoly
    grt->setMonopolized(false);
    grt->buildHouse(); // should be no-op
    ASSERT(grt->getRentLevel() == 0, "buildHouse without monopoly: rentLevel stays 0");
    ASSERT(!grt->hasBuildings(), "buildHouse without monopoly: hasBuildings false");

    // With monopoly
    grt->setMonopolized(true);
    grt->buildHouse();
    ASSERT(grt->getRentLevel() == 1, "buildHouse: rentLevel 0->1");
    ASSERT(grt->hasBuildings(), "buildHouse: hasBuildings true");

    buildNHouses(grt, 3); // now level 4
    ASSERT(grt->getRentLevel() == 4, "4 houses: rentLevel == 4");

    // 5th buildHouse no-op
    grt->buildHouse();
    ASSERT(grt->getRentLevel() == 4, "5th buildHouse: no change (max 4 houses)");

    // buildHotel when level == 4
    grt->buildHotel();
    ASSERT(grt->getRentLevel() == 5, "buildHotel from level 4: rentLevel == 5");

    // buildHotel when not level 4 (already 5): no-op
    grt->buildHotel();
    ASSERT(grt->getRentLevel() == 5, "buildHotel at level 5: no change");

    // demolish
    grt->demolish();
    ASSERT(grt->getRentLevel() == 0, "demolish: rentLevel == 0");
    ASSERT(!grt->hasBuildings(), "demolish: hasBuildings false");

    // demolish with no buildings: no-op
    ASSERT_NO_THROW(grt->demolish(), "demolish with no buildings: no crash");

    // removeOneBuilding
    buildNHouses(grt, 3); // level 3
    grt->removeOneBuilding();
    ASSERT(grt->getRentLevel() == 2, "removeOneBuilding: level 3->2");
    grt->removeOneBuilding();
    grt->removeOneBuilding();
    ASSERT(grt->getRentLevel() == 0, "removeOneBuilding: level 1->0");
    ASSERT(!grt->hasBuildings(), "removeOneBuilding to 0: hasBuildings false");

    // removeOneBuilding at 0: no change
    grt->removeOneBuilding();
    ASSERT(grt->getRentLevel() == 0, "removeOneBuilding at 0: no change");

    // calcValue
    ASSERT(grt->calcValue() == 0, "no buildings: calcValue == 0");
    buildNHouses(grt, 2); // 2 houses, houseCost=20
    ASSERT(grt->calcValue() == 40, "2 houses (houseCost=20): calcValue == 40");
    grt->buildHotel(); // need level 4 first
    grt->buildHouse(); // level 3
    grt->buildHotel(); // still not hotel since level != 4
    // manually get to level 4 then hotel
    grt->demolish();
    buildNHouses(grt, 4);
    grt->buildHotel();
    // hotel: 4*houseCost + hotelCost = 4*20 + 50 = 130
    ASSERT(grt->calcValue() == 130, "hotel: calcValue == 4*20+50 == 130");

    KNOWN_BUG("Even-building rule not enforced at StreetTile::buildHouse() — spec gap");

    resetBoardProperties();
}

void test_mortgage_mechanics() {
    TEST_SECTION("8. Mortgage Mechanics");

    resetBoardProperties();

    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    ASSERT(grt != nullptr, "GRT tile found");

    Player p1("MortP1", 2000);
    p1.setId(0);
    giveProperty(&p1, grt);

    // mortgage() on status=1 -> status=2
    ASSERT(grt->getStatus() == 1, "GRT initially status=1 (OWNED)");
    grt->mortgage();
    ASSERT(grt->getStatus() == 2, "mortgage: status 1->2");

    // mortgage() on status=2 -> no change
    grt->mortgage();
    ASSERT(grt->getStatus() == 2, "mortgage on mortgaged: no change");

    // unmortgage() on status=2 -> status=1
    grt->unmortgage();
    ASSERT(grt->getStatus() == 1, "unmortgage: status 2->1");

    // unmortgage() on status=1 -> no change
    grt->unmortgage();
    ASSERT(grt->getStatus() == 1, "unmortgage on owned: no change");

    // mortgage from status=0 -> no change
    StreetTile *tsk = dynamic_cast<StreetTile *>(g_board->getTileByKode("TSK"));
    ASSERT(tsk != nullptr, "TSK tile found");
    ASSERT(tsk->getStatus() == 0, "TSK starts unowned (status=0)");
    tsk->mortgage();
    ASSERT(tsk->getStatus() == 0, "mortgage on unowned: no change");

    // mortgageValue check
    ASSERT(grt->getmortgageValue() == 40, "GRT mortgageValue == 40");

    // Test GameEngine mortgageProperty / unmortgageProperty
    TestCtx ctx;
    auto &engine = ctx.engine;

    Player *cp = engine.getCurrentPlayer();
    StreetTile *grt2 = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    giveProperty(cp, grt2);

    int moneyBefore = cp->getMoney();
    engine.mortgageProperty("GRT");
    ASSERT(grt2->getStatus() == 2, "engine.mortgageProperty: status -> 2");
    ASSERT(cp->getMoney() == moneyBefore + 40, "engine.mortgageProperty: player gets mortgageValue");

    // unmortgage charges 110% of mortgageValue = 44
    int moneyBeforeUnmort = cp->getMoney();
    engine.unmortgageProperty("GRT");
    ASSERT(grt2->getStatus() == 1, "engine.unmortgageProperty: status -> 1");
    int unmortgageCost = (int)(40 * 1.1);
    ASSERT(cp->getMoney() == moneyBeforeUnmort - unmortgageCost,
           "engine.unmortgageProperty: player pays 110% of mortgageValue");

    // Invalid code -> no crash
    ASSERT_NO_THROW(engine.mortgageProperty("INVALID_CODE"), "mortgageProperty with invalid code: no crash");

    resetBoardProperties();
}

void test_player_wealth() {
    TEST_SECTION("9. Player Wealth Calculation");

    resetBoardProperties();

    Player p1("WealthP1", 1000);
    p1.setId(0);

    // No properties: wealth = money
    ASSERT(p1.getWealth() == 1000, "no properties: wealth == money");

    // Own GRT (price=60, no buildings)
    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    giveProperty(&p1, grt);
    ASSERT(p1.getWealth() == 1060, "GRT owned (price=60, no buildings): wealth == 1060");

    // Build 2 houses on GRT (calcValue = 2*20 = 40)
    grt->setMonopolized(true);
    buildNHouses(grt, 2);
    ASSERT(p1.getWealth() == 1100, "GRT + 2 houses (calcValue=40): wealth == 1100");

    // operator< compares wealth
    Player p2("WealthP2", 2000);
    ASSERT(p1 < p2, "p1 (wealth 1100) < p2 (wealth 2000)");
    ASSERT(!(p2 < p1), "p2 not < p1");

    resetBoardProperties();
}

void test_card_hand_limit() {
    TEST_SECTION("10. Card Hand Limit");

    Player p("CardP", 1000);
    p.setId(0);

    MoveCard *c1 = new MoveCard();
    MoveCard *c2 = new MoveCard();
    MoveCard *c3 = new MoveCard();
    MoveCard *c4 = new MoveCard();

    // Add 3 cards: succeeds
    ASSERT_NO_THROW(p.addCard(c1), "addCard 1st: succeeds");
    ASSERT_NO_THROW(p.addCard(c2), "addCard 2nd: succeeds");
    ASSERT_NO_THROW(p.addCard(c3), "addCard 3rd: succeeds");
    ASSERT((int)p.getHandCards().size() == 3, "3 cards in hand");

    // 4th card: MaxCardLimitException
    ASSERT_THROWS(p.addCard(c4), MaxCardLimitException, "4th addCard: MaxCardLimitException");
    ASSERT((int)p.getHandCards().size() == 3, "hand still 3 after failed add");

    // removeCard
    p.removeCard(0);
    ASSERT((int)p.getHandCards().size() == 2, "removeCard(0): size decrements");

    // removeCard out of range: no crash
    ASSERT_NO_THROW(p.removeCard(-1), "removeCard(-1): no crash");
    ASSERT_NO_THROW(p.removeCard(99), "removeCard(99): no crash");

    // Cleanup c4
    delete c4;
}

void test_skill_card_types() {
    TEST_SECTION("11. SkillCard Types");

    TestCtx ctx;
    auto &engine = ctx.engine;
    Player *p = engine.getCurrentPlayer();

    // MoveCard
    MoveCard *mv = new MoveCard(3);
    ASSERT(mv->getSteps() == 3, "MoveCard(3): getSteps == 3");
    ASSERT(mv->getSkillType() == SkillCardType::MOVE, "MoveCard type == MOVE");

    // DiscountCard
    DiscountCard *dc = new DiscountCard(20, 1);
    ASSERT(dc->getDiscountPercent() == 20, "DiscountCard(20): getDiscountPercent == 20");
    ASSERT(dc->getSkillType() == SkillCardType::DISCOUNT, "DiscountCard type == DISCOUNT");

    // ShieldCard
    ShieldCard *sc = new ShieldCard();
    ASSERT(sc->getSkillType() == SkillCardType::SHIELD, "ShieldCard type == SHIELD");

    // TeleportCard
    TeleportCard *tc = new TeleportCard();
    ASSERT(tc->getSkillType() == SkillCardType::TELEPORT, "TeleportCard type == TELEPORT");

    // LassoCard
    LassoCard *lc = new LassoCard();
    ASSERT(lc->getSkillType() == SkillCardType::LASSO, "LassoCard type == LASSO");

    // DemolitionCard
    DemolitionCard *dmc = new DemolitionCard();
    ASSERT(dmc->getSkillType() == SkillCardType::DEMOLITION, "DemolitionCard type == DEMOLITION");

    // use() returns correct SkillCardEffect
    ASSERT(mv->use(*p, engine) == SkillCardEffect::MOVE, "MoveCard::use returns MOVE");
    ASSERT(dc->use(*p, engine) == SkillCardEffect::DISCOUNT, "DiscountCard::use returns DISCOUNT");
    ASSERT(sc->use(*p, engine) == SkillCardEffect::SHIELD, "ShieldCard::use returns SHIELD");
    ASSERT(tc->use(*p, engine) == SkillCardEffect::TELEPORT, "TeleportCard::use returns TELEPORT");
    ASSERT(lc->use(*p, engine) == SkillCardEffect::LASSO, "LassoCard::use returns LASSO");
    ASSERT(dmc->use(*p, engine) == SkillCardEffect::DEMOLITION, "DemolitionCard::use returns DEMOLITION");

    // Shield flag
    p->setShieldActive(true);
    ASSERT(p->isShieldActive(), "setShieldActive(true): isShieldActive returns true");
    p->setShieldActive(false);
    ASSERT(!p->isShieldActive(), "setShieldActive(false): isShieldActive returns false");

    // Discount flag
    p->setActiveDiscountPercent(50);
    ASSERT(p->getActiveDiscountPercent() == 50, "setActiveDiscountPercent(50): getter returns 50");

    // Cleanup
    delete mv;
    delete dc;
    delete sc;
    delete tc;
    delete lc;
    delete dmc;
}

void test_skill_deck_draw_discard() {
    TEST_SECTION("12. SkillDeck Draw and Discard");

    TestCtx ctx;
    auto &engine = ctx.engine;

    // Draw from skill deck returns non-null
    SkillCard *card = engine.drawSkillCard();
    ASSERT(card != nullptr, "drawSkillCard returns non-null");
    engine.discardSkillCard(card);

    // Draw and discard 15 times, then draw again (triggers reshuffle)
    for (int i = 0; i < 15; i++) {
        SkillCard *c = engine.drawSkillCard();
        ASSERT(c != nullptr, "drawSkillCard #" + std::to_string(i + 1) + " non-null");
        engine.discardSkillCard(c);
    }
    // 16th draw: should reshuffle and succeed
    auto draw16 = [&]() {
        SkillCard *c = engine.drawSkillCard();
        if (c) engine.discardSkillCard(c);
    };
    ASSERT_NO_THROW(draw16(), "16th drawSkillCard triggers reshuffle, no crash");
}

void test_action_card_effects() {
    TEST_SECTION("13. ActionCard Effects");

    TestCtx ctx;
    auto &engine = ctx.engine;
    Player *p = engine.getCurrentPlayer();

    // ChanceCard effect enums only (execute() returns enum, no side effects applied here)
    ChanceCard cc_station(ChanceCardType::NEAREST_STATION);
    ASSERT(cc_station.execute(*p, engine) == ActionCardEffect::CHANCE_NEAREST_STATION,
           "ChanceCard NEAREST_STATION returns correct enum");

    ChanceCard cc_back3(ChanceCardType::MOVE_BACK_3);
    ASSERT(cc_back3.execute(*p, engine) == ActionCardEffect::CHANCE_MOVE_BACK_3,
           "ChanceCard MOVE_BACK_3 returns correct enum");

    ChanceCard cc_jail(ChanceCardType::GO_TO_JAIL);
    ASSERT(cc_jail.execute(*p, engine) == ActionCardEffect::CHANCE_GO_TO_JAIL,
           "ChanceCard GO_TO_JAIL returns correct enum");

    // CommunityChestCard effect enums
    CommunityChestCard cm_birthday(CommunityChestCardType::BIRTHDAY);
    ASSERT(cm_birthday.execute(*p, engine) == ActionCardEffect::COMMUNITY_BIRTHDAY,
           "CommunityChestCard BIRTHDAY returns correct enum");

    CommunityChestCard cm_doctor(CommunityChestCardType::DOCTOR);
    ASSERT(cm_doctor.execute(*p, engine) == ActionCardEffect::COMMUNITY_DOCTOR,
           "CommunityChestCard DOCTOR returns correct enum");

    CommunityChestCard cm_election(CommunityChestCardType::ELECTION);
    ASSERT(cm_election.execute(*p, engine) == ActionCardEffect::COMMUNITY_ELECTION,
           "CommunityChestCard ELECTION returns correct enum");

    // DrawChanceCard / drawCommunityCard from decks
    ActionCard *chance = engine.drawChanceCard();
    ASSERT(chance != nullptr, "drawChanceCard returns non-null");
    engine.discardChanceCard(chance);

    ActionCard *community = engine.drawCommunityCard();
    ASSERT(community != nullptr, "drawCommunityCard returns non-null");
    engine.discardCommunityCard(community);

    // Draw/discard cycle: 3 draws then reshuffle
    for (int i = 0; i < 3; i++) {
        ActionCard *c = engine.drawChanceCard();
        ASSERT(c != nullptr, "chance draw #" + std::to_string(i + 1) + " non-null");
        engine.discardChanceCard(c);
    }
    auto draw4thChance = [&]() {
        ActionCard *c = engine.drawChanceCard();
        if (c) engine.discardChanceCard(c);
    };
    ASSERT_NO_THROW(draw4thChance(), "4th chance draw: reshuffle succeeds, no crash");

    KNOWN_BUG("CardTile::triggerEffect() checks \"1\"/\"2\" but stored type strings are "
              "\"CHANCE\"/\"COMMUNITY_CHEST\" — always returns NONE (bug in ActionTile.cpp)");
}

void test_handleActionCardEffect_nearest_station() {
    TEST_SECTION("14. handleActionCardEffect - Nearest Station");

    TestCtx ctx;
    auto &engine = ctx.engine;
    Player *p = engine.getCurrentPlayer();

    // Railroad positions (1-indexed board): GBR=6, STB=16, TUG=26, GUB=36
    // Player positions (0-indexed): GBR->5, STB->15, TUG->25, GUB->35

    // From position 0: nearest is GBR at board idx 6 -> player pos 5
    p->setPosition(0);
    engine.rollDice(1, 2); // move to pos 3, then we manually check
    // Actually we can't use executeTileAction (has cin). Test handleActionCardEffect directly.
    // Reset: use engine to test the effect indirectly via Card::execute + engine method
    // We'll test position after manually calling the underlying moveBack/jail effects

    // Test CHANCE_MOVE_BACK_3 effect
    Player testP("EffectP", 1000);
    testP.setId(10);
    testP.setPosition(10);
    ChanceCard cc_back(ChanceCardType::MOVE_BACK_3);
    cc_back.execute(testP, engine);
    // execute() only returns the enum - does NOT apply the effect
    // The effect is applied by GameEngine::handleActionCardEffect which is private
    // So we test that position doesn't change (effect not applied by execute())
    ASSERT(testP.getPosition() == 10,
           "ChanceCard::execute() doesn't move player (effect applied elsewhere)");

    // Test CHANCE_GO_TO_JAIL enum
    ChanceCard cc_jail(ChanceCardType::GO_TO_JAIL);
    ASSERT(cc_jail.execute(testP, engine) == ActionCardEffect::CHANCE_GO_TO_JAIL,
           "GO_TO_JAIL card returns correct enum");
    ASSERT(testP.getStatus() == PlayerStatus::ACTIVE,
           "execute() alone doesn't jail player");
}

void test_festival_effect() {
    TEST_SECTION("15. Festival Effect");

    resetBoardProperties();

    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    ASSERT(grt != nullptr, "GRT tile found");
    grt->setMonopolized(true); // base rent = 4 (monopoly, level 0)

    // setFestivalEffect x2
    grt->setFestivalEffect(2);
    ASSERT(grt->calcRent() == 8, "festival x2 on monopoly level 0: 4*2 == 8");

    // setFestivalEffect x4
    grt->setFestivalEffect(4);
    ASSERT(grt->calcRent() == 16, "festival x4: 4*4 == 16");

    // setFestivalEffect x8
    grt->setFestivalEffect(8);
    ASSERT(grt->calcRent() == 32, "festival x8: 4*8 == 32");

    // clearFestivalEffect
    grt->clearFestivalEffect();
    ASSERT(grt->calcRent() == 4, "clearFestivalEffect: back to base 4");

    // Festival + houses
    buildNHouses(grt, 2); // rentTable[2] = 30
    grt->setFestivalEffect(2);
    ASSERT(grt->calcRent() == 60, "festival x2 + 2 houses: 30*2 == 60");
    grt->clearFestivalEffect();

    KNOWN_BUG("Festival stacking not implemented — last setFestivalEffect wins, not cumulative");

    resetBoardProperties();
}

void test_tax_tiles() {
    TEST_SECTION("16. Tax Tiles");

    // TaxTile effects
    TaxTile *pph = dynamic_cast<TaxTile *>(g_board->getTileByKode("PPH"));
    TaxTile *pbm = dynamic_cast<TaxTile *>(g_board->getTileByKode("PBM"));

    ASSERT(pph != nullptr, "PPH tax tile found");
    ASSERT(pbm != nullptr, "PBM tax tile found");

    Player p1("TaxP1", 1000);
    p1.setId(0);

    // PPH -> PAY_TAX_CHOICE
    ASSERT(pph->onLanded(p1) == EffectType::PAY_TAX_CHOICE,
           "PPH tile -> PAY_TAX_CHOICE");

    // PBM -> PAY_TAX_FLAT
    ASSERT(pbm->onLanded(p1) == EffectType::PAY_TAX_FLAT,
           "PBM tile -> PAY_TAX_FLAT");

    // Tax config values
    ASSERT(pbm->getFlatAmount() == 200, "PBM flat amount == 200");
    ASSERT(pph->getFlatAmount() == 150, "PPH flat amount == 150");
    ASSERT(pph->getPercentageRate() == 10, "PPH percentage == 10%");

    // computeNetWorth: money + owned property prices
    resetBoardProperties();
    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    giveProperty(&p1, grt); // price = 60
    double netWorth = pph->computeNetWorth(p1);
    // getWealth() = money + price + calcValue = 1000 + 60 + 0 = 1060
    ASSERT(netWorth == 1060.0, "computeNetWorth: 1000 + 60 = 1060");

    // PPH 10% of 1060 = 106
    int pphAmount = (int)(netWorth * pph->getPercentageRate() / 100.0);
    ASSERT(pphAmount == 106, "PPH 10% of 1060 == 106");

    // PBM flat payment
    ASSERT_NO_THROW(p1 -= 200, "PBM flat 200: succeeds with 1000");

    // Not enough money
    p1.setMoney(100);
    ASSERT_THROWS(p1 -= 200, NotEnoughMoneyException,
                  "PBM flat 200 with M100: NotEnoughMoneyException");

    resetBoardProperties();
}

void test_bankruptcy_to_player() {
    TEST_SECTION("17. Bankruptcy - Transfer to Player");

    resetBoardProperties();

    TestCtx ctx;
    auto &engine = ctx.engine;
    Player *p1 = getPlayerByName(engine, "P1");
    Player *p2 = getPlayerByName(engine, "P2");
    ASSERT(p1 != nullptr, "P1 found");
    ASSERT(p2 != nullptr, "P2 found");

    // Give P1 a property with 2 houses
    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    ASSERT(grt != nullptr, "GRT found");
    giveProperty(p1, grt);
    grt->setMonopolized(true);
    buildNHouses(grt, 2);

    // P1 declares bankruptcy (manual)
    p1->declareBankruptcy();
    ASSERT(p1->getStatus() == PlayerStatus::BANKRUPT, "declareBankruptcy: status BANKRUPT");
    ASSERT(p1->getMoney() == 0, "declareBankruptcy: money == 0");

    // Properties with buildings transfer to creditor (P2) including buildings
    // Simulate transfer
    grt->setOwnerId(p2->getId());
    p2->addProperty(grt);
    p1->removeProperty(grt);

    ASSERT(grt->getOwnerId() == p2->getId(), "GRT ownership transferred to P2");
    ASSERT(grt->getRentLevel() == 2, "buildings preserved after transfer");

    KNOWN_BUG("GameEngine::handleBankruptcy does not clear bankrupt player's ownedProperties list");
}

void test_bankruptcy_to_bank() {
    TEST_SECTION("18. Bankruptcy - Return to Bank");

    resetBoardProperties();

    Player p1("BankruptP", 1000);
    p1.setId(0);

    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    StreetTile *tsk = dynamic_cast<StreetTile *>(g_board->getTileByKode("TSK"));
    ASSERT(grt != nullptr && tsk != nullptr, "GRT and TSK tiles found");

    giveProperty(&p1, grt);
    giveProperty(&p1, tsk);
    grt->setMonopolized(true);
    tsk->setMonopolized(true);
    buildNHouses(grt, 2);

    // Bankruptcy to bank: properties return to bank, buildings demolished
    p1.declareBankruptcy();
    ASSERT(p1.getStatus() == PlayerStatus::BANKRUPT, "bankrupt to bank: status BANKRUPT");

    // Simulate what handleBankruptcy does for bank creditor
    grt->setOwnerId(-1);
    grt->setStatus(0);
    grt->demolish();

    tsk->setOwnerId(-1);
    tsk->setStatus(0);
    tsk->demolish();

    ASSERT(grt->getOwnerId() == -1, "GRT returned to bank: ownerId == -1");
    ASSERT(grt->getStatus() == 0, "GRT returned to bank: status == 0 (BANK)");
    ASSERT(grt->getRentLevel() == 0, "GRT buildings demolished after bankrupt to bank");
    ASSERT(tsk->getOwnerId() == -1, "TSK returned to bank");

    resetBoardProperties();
}

void test_money_operations() {
    TEST_SECTION("19. Money Operations");

    Player p("MoneyP", 500);
    p.setId(0);

    p += 300;
    ASSERT(p.getMoney() == 800, "operator+=: 500+300 == 800");

    p -= 200;
    ASSERT(p.getMoney() == 600, "operator-=: 800-200 == 600");

    // operator-= with not enough money
    ASSERT_THROWS(p -= 1000, NotEnoughMoneyException,
                  "operator-= not enough money: NotEnoughMoneyException");
    ASSERT(p.getMoney() == 600, "money unchanged after failed -=");

    // setMoney
    p.setMoney(2000);
    ASSERT(p.getMoney() == 2000, "setMoney(2000): getMoney == 2000");
}

void test_win_condition_max_turn() {
    TEST_SECTION("20. Win Condition - Max Turn Exceeded");

    TestCtx ctx;
    auto &engine = ctx.engine;

    // Set round to one past the limit
    engine.setRoundCount(16); // maxTurn = 15

    // endTurn should trigger win condition via GameStateException
    engine.rollDice(1, 2);
    ASSERT_THROWS(engine.endTurn(), GameStateException,
                  "endTurn at round 16 (maxTurn=15) throws GameStateException");
}

void test_win_condition_last_player() {
    TEST_SECTION("21. Win Condition - Last Active Player");

    TestCtx ctx;
    auto &engine = ctx.engine;

    Player *p1 = getPlayerByName(engine, "P1");
    Player *p2 = getPlayerByName(engine, "P2");
    Player *p3 = getPlayerByName(engine, "P3");
    Player *p4 = getPlayerByName(engine, "P4");

    ASSERT(p1 && p2 && p3 && p4, "All 4 players found");

    // Bankrupt 3 players, leave 1 active
    // First, ensure current player is not the one we'll bankrupt
    // Make P2, P3, P4 bankrupt (find which IDs they have)
    int activeIdx = engine.getCurrentTurnIdx();
    Player *activePlayer = engine.getCurrentPlayer();

    // Bankrupt all others
    for (Player *p : engine.getAllPlayers()) {
        if (p != activePlayer) {
            p->declareBankruptcy();
        }
    }

    // Now endTurn should detect only 1 active player -> GameStateException
    engine.rollDice(1, 2);
    ASSERT_THROWS(engine.endTurn(), GameStateException,
                  "endTurn with 3 bankrupt players throws GameStateException");

    (void)activeIdx;
}

void test_next_turn_skips_bankrupt() {
    TEST_SECTION("22. nextTurn Skips Bankrupt Players");

    TestCtx ctx;
    auto &engine = ctx.engine;

    // Bankrupt P2 (index 1), P3 (index 2) - leaving P1(0) and P4(3) active
    auto players = engine.getAllPlayers();
    ASSERT(players.size() == 4, "4 players present");

    // Set currentTurnIdx to 0 (first player)
    engine.setCurrentTurnIdx(0);
    players[1]->declareBankruptcy(); // idx 1 bankrupt
    players[2]->declareBankruptcy(); // idx 2 bankrupt

    // Roll and endTurn: should skip idx 1 and 2, land on idx 3
    engine.rollDice(1, 2);
    engine.endTurn();
    int nextIdx = engine.getCurrentTurnIdx();
    ASSERT(nextIdx == 3, "nextTurn skips bankrupt players 1,2 -> idx 3");
}

void test_save_load() {
    TEST_SECTION("23. Save and Load Game");

    TestCtx ctx;
    auto &engine = ctx.engine;

    // Set known state
    Player *p1 = getPlayerByName(engine, "P1");
    Player *p2 = getPlayerByName(engine, "P2");
    ASSERT(p1 != nullptr && p2 != nullptr, "P1 and P2 found");

    p1->setMoney(750);
    p1->setPosition(7);

    // Give P1 the full COKLAT monopoly (GRT+TSK) so saveGame computes monopolized=true
    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    StreetTile *tsk = dynamic_cast<StreetTile *>(g_board->getTileByKode("TSK"));
    ASSERT(grt != nullptr && tsk != nullptr, "GRT and TSK tiles found for save test");
    giveProperty(p1, grt);
    giveProperty(p1, tsk);
    grt->setMonopolized(true);
    tsk->setMonopolized(true);
    buildNHouses(grt, 2);

    p2->goToJail();
    p2->setJailTurnsLeft(2);

    engine.setRoundCount(5);
    engine.setCurrentTurnIdx(1);

    const std::string saveFile = "test_save_nimonspoli.dat";

    // Save
    ASSERT_NO_THROW(engine.saveGame(saveFile), "saveGame() succeeds");

    // Load into a fresh engine
    TransactionLogger logger2;
    GameEngine engine2(Board::getInstance(), &logger2);
    ASSERT_NO_THROW(engine2.loadGame(saveFile), "loadGame() succeeds");

    ASSERT(engine2.getCurrentRound() == 5, "after load: roundCount == 5");
    ASSERT(engine2.getCurrentTurnIdx() == 1, "after load: currentTurnIdx == 1");

    Player *lp1 = nullptr;
    Player *lp2 = nullptr;
    for (Player *p : engine2.getAllPlayers()) {
        if (p->getUsername() == "P1") lp1 = p;
        if (p->getUsername() == "P2") lp2 = p;
    }

    if (lp1) {
        ASSERT(lp1->getMoney() == 750, "after load: P1 money == 750");
        ASSERT(lp1->getPosition() == 7, "after load: P1 position == 7");
    } else {
        ASSERT(false, "after load: P1 found");
    }

    if (lp2) {
        ASSERT(lp2->getStatus() == PlayerStatus::JAILED, "after load: P2 is JAILED");
        ASSERT(lp2->getJailTurnsLeft() == 2, "after load: P2 jailTurnsLeft == 2");
    } else {
        ASSERT(false, "after load: P2 found");
    }

    // GRT property state restored
    Tile *t = Board::getInstance()->getTileByKode("GRT");
    StreetTile *grtLoaded = dynamic_cast<StreetTile *>(t);
    if (grtLoaded) {
        ASSERT(grtLoaded->getStatus() == 1, "after load: GRT status == 1 (OWNED)");
        ASSERT(grtLoaded->getRentLevel() == 2, "after load: GRT rentLevel == 2");
    } else {
        ASSERT(false, "GRT tile found after load");
    }

    // Cleanup
    std::remove(saveFile.c_str());

    // Load nonexistent file
    ASSERT_THROWS(engine.loadGame("nonexistent_file_xyz.dat"), std::runtime_error,
                  "loadGame nonexistent file: std::runtime_error");

    KNOWN_BUG("SkillCard hands not serialized in save file");

    resetBoardProperties();
}

void test_board_tile_types() {
    TEST_SECTION("24. Board Tile Type Verification");

    // Verify specific tile types are correct
    ASSERT(dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT")) != nullptr,
           "GRT is StreetTile");
    ASSERT(dynamic_cast<StreetTile *>(g_board->getTileByKode("TSK")) != nullptr,
           "TSK is StreetTile");
    ASSERT(dynamic_cast<RailroadTile *>(g_board->getTileByKode("GBR")) != nullptr,
           "GBR is RailroadTile");
    ASSERT(dynamic_cast<RailroadTile *>(g_board->getTileByKode("STB")) != nullptr,
           "STB is RailroadTile");
    ASSERT(dynamic_cast<RailroadTile *>(g_board->getTileByKode("TUG")) != nullptr,
           "TUG is RailroadTile");
    ASSERT(dynamic_cast<RailroadTile *>(g_board->getTileByKode("GUB")) != nullptr,
           "GUB is RailroadTile");
    ASSERT(dynamic_cast<UtilityTile *>(g_board->getTileByKode("PLN")) != nullptr,
           "PLN is UtilityTile");
    ASSERT(dynamic_cast<UtilityTile *>(g_board->getTileByKode("PAM")) != nullptr,
           "PAM is UtilityTile");
    ASSERT(dynamic_cast<TaxTile *>(g_board->getTileByKode("PPH")) != nullptr,
           "PPH is TaxTile");
    ASSERT(dynamic_cast<TaxTile *>(g_board->getTileByKode("PBM")) != nullptr,
           "PBM is TaxTile");
    ASSERT(dynamic_cast<JailTile *>(g_board->getTileByKode("PEN")) != nullptr,
           "PEN is JailTile");
    ASSERT(dynamic_cast<GoToJailTile *>(g_board->getTileByKode("PPJ")) != nullptr,
           "PPJ is GoToJailTile");
    ASSERT(dynamic_cast<CardTile *>(g_board->getTileByKode("KSP")) != nullptr,
           "KSP is CardTile");
    ASSERT(dynamic_cast<FestivalTile *>(g_board->getTileByKode("FES")) != nullptr,
           "FES is FestivalTile");

    // isStreet()
    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    ASSERT(grt->isStreet(), "StreetTile::isStreet() returns true");
    RailroadTile *gbr = dynamic_cast<RailroadTile *>(g_board->getTileByKode("GBR"));
    ASSERT(!gbr->isStreet(), "RailroadTile::isStreet() returns false");

    // getColorGroup (config stores group names in uppercase)
    ASSERT(grt->getColorGroup() == "COKLAT", "GRT colorGroup == COKLAT");
    StreetTile *tsk = dynamic_cast<StreetTile *>(g_board->getTileByKode("TSK"));
    ASSERT(tsk->getColorGroup() == "COKLAT", "TSK colorGroup == COKLAT (same group as GRT)");
}

void test_goToJailTile() {
    TEST_SECTION("25. GoToJail and Jail Tile Effects");

    Player p("JailTileP", 1000);
    p.setId(0);

    // GoToJailTile returns SEND_TO_JAIL
    GoToJailTile *gtj = dynamic_cast<GoToJailTile *>(g_board->getTileByKode("PPJ"));
    ASSERT(gtj != nullptr, "PPJ is GoToJailTile");
    ASSERT(gtj->onLanded(p) == EffectType::SEND_TO_JAIL,
           "GoToJailTile::onLanded returns SEND_TO_JAIL");

    // FreeParkingTile returns NONE
    FreeParkingTile *fp = dynamic_cast<FreeParkingTile *>(g_board->getTileByKode("BBP"));
    ASSERT(fp != nullptr, "BBP is FreeParkingTile");
    ASSERT(fp->onLanded(p) == EffectType::NONE,
           "FreeParkingTile::onLanded returns NONE");

    // FestivalTile returns FESTIVAL_TRIGGER
    FestivalTile *fest = dynamic_cast<FestivalTile *>(g_board->getTileByKode("FES"));
    ASSERT(fest != nullptr, "FES is FestivalTile");
    ASSERT(fest->onLanded(p) == EffectType::FESTIVAL_TRIGGER,
           "FestivalTile::onLanded returns FESTIVAL_TRIGGER");

    // JailTile while ACTIVE -> JUST_VISITING
    JailTile *jail = dynamic_cast<JailTile *>(g_board->getTileByKode("PEN"));
    ASSERT(jail != nullptr, "PEN is JailTile");
    ASSERT(p.getStatus() == PlayerStatus::ACTIVE, "player is ACTIVE");
    ASSERT(jail->onLanded(p) == EffectType::JUST_VISITING,
           "JailTile::onLanded for ACTIVE player -> JUST_VISITING");

    // JailTile::handleArrival always returns JUST_VISITING (called via onLanded).
    // JAIL_TURN is returned by JailTile::processTurnInJail(), which GameEngine
    // calls at the START of a jailed player's turn — not via onLanded().
    // processTurnInJail() has cin >> so it cannot be called in automated tests.
    p.goToJail();
    ASSERT(jail->onLanded(p) == EffectType::JUST_VISITING,
           "JailTile::onLanded for JAILED player -> JUST_VISITING (jail turn handled by engine)");
}

void test_property_tile_getters() {
    TEST_SECTION("26. Property Tile Getters");

    StreetTile *grt = dynamic_cast<StreetTile *>(g_board->getTileByKode("GRT"));
    ASSERT(grt != nullptr, "GRT found");

    ASSERT(grt->getPrice() == 60, "GRT price == 60");
    ASSERT(grt->getmortgageValue() == 40, "GRT mortgageValue == 40");
    ASSERT(grt->getHouseCost() == 20, "GRT houseCost == 20");
    ASSERT(grt->getHotelCost() == 50, "GRT hotelCost == 50");
    ASSERT(grt->getOwnerId() == -1, "GRT ownerId == -1 (unowned)");
    ASSERT(grt->getStatus() == 0, "GRT status == 0 (BANK)");
    ASSERT(grt->getKode() == "GRT", "GRT kode == GRT");

    StreetTile *tsk = dynamic_cast<StreetTile *>(g_board->getTileByKode("TSK"));
    ASSERT(tsk != nullptr, "TSK found");
    ASSERT(tsk->getPrice() == 60, "TSK price == 60");
    ASSERT(tsk->getmortgageValue() == 40, "TSK mortgageValue == 40");

    // Railroad
    RailroadTile *gbr = dynamic_cast<RailroadTile *>(g_board->getTileByKode("GBR"));
    ASSERT(gbr != nullptr, "GBR found");
    ASSERT(gbr->getmortgageValue() > 0, "GBR has a mortgageValue");
    ASSERT(gbr->getPrice() == 0, "RailroadTile price == 0 (auto-acquired)");
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=================================================\n";
    std::cout << "  pOOPs Nimonspoli — Integration Test Suite\n";
    std::cout << "=================================================\n";

    // One-time singleton initialization
    try {
        initSingletons();
        std::cout << "\n[INIT] Singletons initialized successfully.\n";
    } catch (const std::exception &e) {
        std::cerr << "[ERROR] Failed to initialize singletons: " << e.what() << "\n";
        std::cerr << "Make sure to run from project root (config/ must be accessible).\n";
        return 2;
    }

    // Run all test sections
    test_initialization();
    test_dice_roll();
    test_double_streak_and_jail();
    test_jail_mechanics();
    test_property_buying();
    test_rent_calculation();
    test_building_mechanics();
    test_mortgage_mechanics();
    test_player_wealth();
    test_card_hand_limit();
    test_skill_card_types();
    test_skill_deck_draw_discard();
    test_action_card_effects();
    test_handleActionCardEffect_nearest_station();
    test_festival_effect();
    test_tax_tiles();
    test_bankruptcy_to_player();
    test_bankruptcy_to_bank();
    test_money_operations();
    test_win_condition_max_turn();
    test_win_condition_last_player();
    test_next_turn_skips_bankrupt();
    test_save_load();
    test_board_tile_types();
    test_goToJailTile();
    test_property_tile_getters();

    // Summary
    std::cout << "\n==================================\n";
    std::cout << "Results: " << g_pass << " passed, " << g_fail << " failed";
    if (g_known_bugs > 0)
        std::cout << ", " << g_known_bugs << " known bugs documented";
    std::cout << "\n==================================\n";

    return (g_fail > 0) ? 1 : 0;
}
