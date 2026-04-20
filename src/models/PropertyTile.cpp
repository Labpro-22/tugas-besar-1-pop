#include "../../include/models/PropertyTile.hpp"

PropertyTile:: PropertyTile(int index, const std::string& code, const std::string& name, int buyPrice, int mortgageValue)
: Tile(index,code,name), price(buyPrice), mortgageValue(mortgageValue), ownerId(-1),status(0){}

void PropertyTile:: onLanded(Player& player){
    if(this->status == 1){
        bool found = false;
        for(auto v : player.getOwnedProperties()){
            if(v->ownerId == this->ownerId){
                found = true;
                break;
            }
        }
        if(!found){
            payRent();
        }
    }
}

void PropertyTile:: payRent(){
}

StreetTile::StreetTile(int index, const std::string& code, const std::string& name,
               const std::string& colorGroup,
               int buyPrice, int mortgageValue,
               const std::vector<int>& rentTable,
               int houseCost, int hotelCost)
               :PropertyTile(index, code, name, buyPrice, mortgageValue){}

int StreetTile:: calcRent(int diceRoll) const{
    int cur = 1;
    if(isMonopolized && hasBuilding){ // isMonopolized dan hasBuilding butuh implementasi
        cur =  2 * rentTable[0];
    }
    else{
        cur =  rentTable[rentLevel];
    }
    if(isFestivalEffectActive){
        cur *= festivalEffectMultiplier;
    }
    return cur;
}

void StreetTile:: onLanded(Player& player){
    if(this->status == 0){
        bool beli = false;
        // Impelemen : Menampilkan akta kepemilikan
        // Implemen : Menawarkan kepada pemain
        if(beli){
            if(player.getMoney() <= this->price){
                // Player belum mengimplementasikan pengurangan money
                this->status = 1;
            }
        }
    }
}

void StreetTile:: sellTobank(Player& owner){
    // ini kayaknya redundan sama mortgage dari PropertyTile
}

void StreetTile:: buildHouse(){
    if(isMonopolized){
        // perlu implemen kelas baru kalo gitu, colorGroup, supaya lebih mudah dalam pengerjaan, 
        // bisa build House jika udah monopoli semua tile di 1 colorGroup yang sama,
        // pembangunan juga harus dilakukan setara
    }
}

void StreetTile::buildHotel(){}

void StreetTile::demolish(){}

bool StreetTile:: hasBuildings(){return this->hasBuilding;}

int StreetTile::getHouseCost(){return this->houseCost;}

int StreetTile::getHotelCost(){return this->hotelCost;}

string StreetTile::getColorGroup() const {return this->colorGroup;}

void StreetTile::setMonopolized(bool val){this->isMonopolized = val;}

int StreetTile::calcBuildingResaleValue() const{
    return (hotelCost + houseCost) * 0.5;
}

RailroadTile::RailroadTile(int index, const std::string& code, const std::string& name,
                 int mortgageValue,
                 const map<int, int>& rentByCount)
                 :PropertyTile(index,code, name,0,mortgageValue), rentByCount(rentByCount){}

int RailroadTile::calcRent(int diceRoll = 0) const {
    int pemilik = this->ownerId;
    // untuk pemilik railroad ini, cari berpaa banyak railroad yg dimilikinya, misalkan aja x
    // maka, jawabannya adalah rentByCount[x]
    
}