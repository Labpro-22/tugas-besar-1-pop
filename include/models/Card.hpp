#ifndef CARD_HPP
#define CARD_HPP

#include <string>
#include <vector>

class SkillCard {
protected:
    std::string cardName;
public:
    SkillCard();
    virtual void useSkill() = 0; 
    virtual ~SkillCard();
};

// Generic Class
template <class T>
class CardDeck {
private:
    std::vector<T*> deck;
    T* activeCard; 
public:
    CardDeck();
    void shuffle(); 
    void drawCard(); 
    void returnToDeck(); 
};

#endif