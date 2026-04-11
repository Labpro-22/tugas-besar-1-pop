#ifndef GAME_ENGINE_HPP
#define GAME_ENGINE_HPP

#include <vector>
#include <string>
#include <map>
#include "Tile.hpp"
#include "Player.hpp"

class GameEngine {
private:
    static GameEngine* instance;
    
    std::vector<Tile*> board;            
    std::vector<Player> players;         
    int currentPlayerIndex;
    int diceResult;
    
    GameEngine();

public:
    static GameEngine* getInstance();
    
    ~GameEngine();
    
    // Prosedur game logic tetap tanpa parameter
    void loadConfig(); 
    void startGame();
    void rollDice(); 
    void executeTurn(); 
    void checkWinCondition(); 
};

#endif