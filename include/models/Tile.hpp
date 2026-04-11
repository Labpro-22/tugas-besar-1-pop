#ifndef TILE_HPP
#define TILE_HPP

#include <string>

class Tile {
protected:
    int id;
    std::string kode;
    std::string name;
public:
    Tile();
    // Method virtual murni
    virtual void onLanded() = 0; 
    virtual ~Tile();
};

#endif