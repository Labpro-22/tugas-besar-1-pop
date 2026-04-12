#ifndef CARD_HPP
#define CARD_HPP


#include "core/Enums.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <stdexcept>


template <class T>
class CardDeck {
private:
    std::vector<T*> deck;        // kartu yang belum ditarik
    std::vector<T*> discardPile; // kartu yang sudah dipakai

public:
    
    CardDeck();
    ~CardDeck();

    void addCard(T* card);
    void shuffle();

    // Ambil kartu teratas dari deck.
    T* draw(); 

    // Masukkan kartu ke discard pile abis dipake.
    void discard(T* card);

    // Kocok ulang discard pile menjadi deck baru (deck lama harus sudah kosong).
    void reshuffleDiscard();

   

    bool isEmpty();
    int  size();
    int  discardSize() ;

    // Akses read-only ke isi deck (berguna untuk save/load)
    // const std::vector<T*>& getDeck()        const { return deck; }
    // const std::vector<T*>& getDiscardPile() const { return discardPile; }
};




class Player;
class GameEngine;
class SkillCard {
protected:
    std::string cardName;
    std::string cardDescription;
    SkillCardType skillType;

public:
    SkillCard(const std::string& name,
              const std::string& description,
              SkillCardType type);
    ~SkillCard() ;
    virtual void use(Player& player, GameEngine& engine);

    std::string  getCardName() const ;
    std::string  getCardDescription() const;
    SkillCardType getSkillType()const ;
    virtual std::string getValueString() const = 0;
};


class MoveCard : public SkillCard {
private:
    int steps;

public:
    MoveCard();

    // Konstruktor kalo ada dari save
    explicit MoveCard(int savedSteps);

    int getSteps() const { return steps; }

    void use(Player& player, GameEngine& engine) override;
};


class DiscountCard : public SkillCard {
private:
    int discountPercent; 
    int remainingTurns;  

public:
    DiscountCard();
    explicit DiscountCard(int savedPercent, int savedTurns = 1);

    int getDiscountPercent() const ;
    int getRemainingTurns()  const ;
    void decrementTurns();
    void use(Player& player, GameEngine& engine) override;


};


class ShieldCard : public SkillCard {
public:
    ShieldCard();

    void use(Player& player, GameEngine& engine) override;

};


class TeleportCard : public SkillCard {
public:
    TeleportCard();

    void use(Player& player, GameEngine& engine) override;

};


class LassoCard : public SkillCard {
public:
    LassoCard();
    void use(Player& player, GameEngine& engine) override;
};


class DemolitionCard : public SkillCard {
public:
    DemolitionCard();


    void use(Player& player, GameEngine& engine) override;
    
};



class ActionCard {
protected:
    std::string cardName;
    std::string cardDescription;

public:
    ActionCard(const std::string& name, const std::string& description);

    ~ActionCard();
    virtual void execute(Player& player, GameEngine& engine);

    std::string getCardName() const ;
    std::string getCardDescription() const;
};


class ChanceCard : public ActionCard {
protected:
    ChanceCardType type;

public:

    ChanceCard(ChanceCardType cardType);

    ChanceCardType getType() const;
    void execute(Player& player, GameEngine& engine) override;
};


class CommunityChestCard : public ActionCard {
protected:
    CommunityChestCardType type;

public:
    CommunityChestCard(CommunityChestCardType cardType);
    CommunityChestCardType getType() const;
    void execute(Player& player, GameEngine& engine) override;
};

#endif 
