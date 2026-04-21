#ifndef SAVE_LOAD_MANAGER_HPP
#define SAVE_LOAD_MANAGER_HPP

#include <string>
#include <fstream>

#include "GameEngine.hpp"

class SaveLoadManager
{
private:
    static void savePlayers(std::ofstream& file, const GameEngine& engine);
    static void saveProperties(std::ofstream& file, const GameEngine& engine);
    static void saveDeck(std::ofstream& file, const GameEngine& engine);
    static void saveLogs(std::ofstream& file, const GameEngine& engine);

    static void loadPlayers(std::ifstream& file, GameEngine& engine);
    static void loadProperties(std::ifstream& file, GameEngine& engine);
    static void loadDeck(std::ifstream& file, GameEngine& engine);
    static void loadLogs(std::ifstream& file, GameEngine& engine);
public:
    static bool saveGame(const std::string& fileName, const GameEngine& engine);
    static bool loadGame(const std::string& fileName, GameEngine& engine);
    static bool fileExists(const std::string& fileName);
    
};


#endif