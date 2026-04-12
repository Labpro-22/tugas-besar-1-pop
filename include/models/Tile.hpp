#pragma once
#include <string>
#include "Player.hpp"
using namespace std;
#include <bits/stdc++.h>

class Tile {
protected:
    int id;
    std::string kode;
    std::string name;
public:
    Tile(int id, string kode, string name);
    virtual void onLanded(Player& player) = 0; 
    virtual ~Tile();
    int getIndex();
    string getKode();
    string getName();

};