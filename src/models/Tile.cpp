#include "../../include/models/Tile.hpp"

Tile::Tile(int id, string kode, string name) : id(id), kode(kode), name(name) {}

int Tile::getIndex() const { return this->id; }

string Tile::getKode() const { return this->kode; }

string Tile::getName() const { return this->name; }