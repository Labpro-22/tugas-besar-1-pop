#ifndef CARD_HPP
#define CARD_HPP

#include "../core/Enums.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <stdexcept>
class Player;
class GameEngine;


template <class T>
class CardDeck {
private:
    std::vector<T*> deck;
    std::vector<T*> discardPile;

public:
    CardDeck() = default;

    ~CardDeck() {
        for (T* card : deck)        delete card;
        for (T* card : discardPile) delete card;
        deck.clear();
        discardPile.clear();
    }

    void addCard(T* card) {
        if (card) deck.push_back(card);
    }

    void shuffle() {
        std::random_device rd;
        std::mt19937 rng(rd());
        std::shuffle(deck.begin(), deck.end(), rng);
    }

    T* draw() {
        if (deck.empty()) reshuffleDiscard();
        if (deck.empty()) throw std::runtime_error("CardDeck: deck dan discard pile keduanya kosong.");
        T* card = deck.back();
        deck.pop_back();
        return card;
    }

    void discard(T* card) {
        if (card) discardPile.push_back(card);
    }

    void reshuffleDiscard() {
        for (T* card : discardPile) deck.push_back(card);
        discardPile.clear();
        shuffle();
    }

    bool isEmpty()     const { return deck.empty(); }
    int  size()        const { return static_cast<int>(deck.size()); }
    int  discardSize() const { return static_cast<int>(discardPile.size()); }

    const std::vector<T*>& getDeck()        const { return deck; }
    const std::vector<T*>& getDiscardPile() const { return discardPile; }
};


class SkillCard {
protected:
    std::string   cardName;
    std::string   cardDescription;
    SkillCardType skillType;

public:
    SkillCard(const std::string& name,
              const std::string& description,
              SkillCardType type);
    virtual ~SkillCard();

    virtual SkillCardEffect use(Player& player, GameEngine& engine);

    std::string   getCardName()        const;
    std::string   getCardDescription() const;
    SkillCardType getSkillType()        const;

    virtual std::string getValueString() const = 0;
};



class MoveCard : public SkillCard {
private:
    int steps;

public:
    MoveCard();
    explicit MoveCard(int savedSteps);

    int getSteps() const;

    SkillCardEffect use(Player& player, GameEngine& engine) override;
    std::string getValueString() const override;
};



class DiscountCard : public SkillCard {
private:
    int discountPercent;
    int remainingTurns;

public:
    DiscountCard();
    explicit DiscountCard(int savedPercent, int savedTurns = 1);

    int  getDiscountPercent() const;
    int  getRemainingTurns()  const;
    void decrementTurns();

    SkillCardEffect use(Player& player, GameEngine& engine) override;
    std::string getValueString() const override;
};


class ShieldCard : public SkillCard {
public:
    ShieldCard();
    SkillCardEffect use(Player& player, GameEngine& engine) override;
    std::string getValueString() const override;
};

class TeleportCard : public SkillCard {
public:
    TeleportCard();
    SkillCardEffect use(Player& player, GameEngine& engine) override;
    std::string getValueString() const override;
};


class LassoCard : public SkillCard {
public:
    LassoCard();
    SkillCardEffect use(Player& player, GameEngine& engine) override;
    std::string getValueString() const override;
};



class DemolitionCard : public SkillCard {
public:
    DemolitionCard();
    SkillCardEffect use(Player& player, GameEngine& engine) override;
    std::string getValueString() const override;
};


class ActionCard {
protected:
    std::string cardName;
    std::string cardDescription;

public:
    ActionCard(const std::string& name, const std::string& description);
    virtual ~ActionCard();

    virtual ActionCardEffect execute(Player& player, GameEngine& engine);

    std::string getCardName()        const;
    std::string getCardDescription() const;
};


class ChanceCard : public ActionCard {
protected:
    ChanceCardType type;

public:
    explicit ChanceCard(ChanceCardType cardType);

    ChanceCardType   getType() const;
    ActionCardEffect execute(Player& player, GameEngine& engine) override;
};


class CommunityChestCard : public ActionCard {
protected:
    CommunityChestCardType type;

public:
    explicit CommunityChestCard(CommunityChestCardType cardType);

    CommunityChestCardType getType() const;
    ActionCardEffect execute(Player& player, GameEngine& engine) override;
};

#endif
