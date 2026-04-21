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

void PropertyTile:: mortgage(){
    if(status != 1){
        return;
    }
    status = 2;
}

void PropertyTile:: unmortgage(){
    if(status != 2){
        return;
    }
    status = 1;
}

void PropertyTile:: sellTobank(Player& owner){
    ownerId = -1;
    status = 0;
}

int PropertyTile::getmortgageValue() { return mortgageValue;}
int PropertyTile::getPrice() { return price; }
int PropertyTile::getOwnerId() { return ownerId; }
int PropertyTile::getStatus() { return status; }

//Street Tile
StreetTile::StreetTile(int index, const std::string& code, const std::string& name,
               const std::string& colorGroup,
               int buyPrice, int mortgageValue,
               const std::vector<int>& rentTable,
               int houseCost, int hotelCost)
               :PropertyTile(index, code, name, buyPrice, mortgageValue),colorGroup(colorGroup),rentTable(rentTable)
               ,houseCost(houseCost), hotelCost(hotelCost), rentLevel(0){}

int StreetTile:: calcRent(int diceRoll) const{
    int cur = 0;
    if (rentLevel == 0) {
        if (isMonopolized) {
            cur = 2 * rentTable[0]; // Monopoli tanpa bangunan = sewa dasar x2
        } else {    
            cur = rentTable[0];     // Sewa dasar biasa
        }
    } else {
        // rentLevel 1-5 sesuai indeks rentTable (1=H1, 2=H2, 3=H3, 4=H4, 5=Hotel)
        cur = rentTable[rentLevel];
    }
 
    // Terapkan efek festival jika aktif
    if (isFestivalEffectActive) {
        cur *= festivalEffectMultiplier;
    }
    return cur;

}

void StreetTile:: onLanded(Player& player){
    if(this->status == 0){
        bool beli = false;
        // TODO : Menampilkan akta kepemilikan
        // TODO : Menawarkan kepada pemain
        if(beli){
            if(player.getMoney() >= this->price){
                player -= price;
                this->status = 1;
            }
        }
        else{
            // TODO : Masuk ke sistem lelang
        }
    }
    else if(this->status == 1){
        // TODO : Jika ownerId != IdPlayer, Sewa dilakukan secara otomatis.
    }
    
}

void StreetTile:: sellTobank(Player& owner){
    // ini kayaknya redundan sama mortgage dari PropertyTile
}

void StreetTile:: buildHouse(){
    if(!isMonopolized){
        return;
    }
    if(rentLevel >= 4){
        return;
    }
    // perlu implemen kelas baru kalo gitu, colorGroup, supaya lebih mudah dalam pengerjaan, 
    // bisa build House jika udah monopoli semua tile di 1 colorGroup yang sama,
    // pembangunan juga harus dilakukan setara
    // atau bisa juga pengecekan ini dilakukan di gamelogic.
    rentLevel++;
    hasBuilding = true;
    // TODO : kurangi uang owner sebesar houseCost
}

void StreetTile::buildHotel(){
    if(!isMonopolized) return;
    if(rentLevel != 4) return;
    // TODO : GaneLogic harus memastikan bahwa bahwa setiap tile udah memiki rentLevel 4
    rentLevel = 5;
    // TODO : kurangi uang owner

}

void StreetTile::demolish(){
    if(!hasBuilding) return;
    rentLevel = 0;
    hasBuilding = false;
}

bool StreetTile:: hasBuildings(){return this->hasBuilding;}

int StreetTile::getHouseCost(){return this->houseCost;}

int StreetTile::getHotelCost(){return this->hotelCost;}

string StreetTile::getColorGroup() const {return this->colorGroup;}

void StreetTile::setMonopolized(bool val){this->isMonopolized = val;}

int StreetTile::calcBuildingResaleValue() const{
    if(!hasBuilding) return 0;
    int total = 0;
    if(rentLevel == 5) return (4 * houseCost) + hotelCost;
    else return rentLevel * houseCost;
}

// RailroadTile

RailroadTile::RailroadTile(int index, const std::string& code, const std::string& name,
                 int mortgageValue,
                 const map<int, int>& rentByCount)
                 :PropertyTile(index,code, name,0,mortgageValue), rentByCount(rentByCount){}

int RailroadTile::calcRent(int diceRoll = 0) const {
    auto it = rentByCount.find(railroadOwnedCount);
    if(it != rentByCount.end()){
        return it->second;
    }
    return 0;
    // untuk pemilik railroad ini, cari berpaa banyak railroad yg dimilikinya, misalkan aja x
    // maka, jawabannya adalah rentByCount[x]
}

void RailroadTile::onLanded(Player& player){
    if (status == 0) {
        // BANK - kepemilikan otomatis tanpa beli/lelang
        status  = 1;
 
    } else if (status == 1) {
        if (ownerId != player.getId()) {
            int rent = calcRent();
            if (player.getMoney() >= rent) {
                    player -= rent;
                    // TODO: tambahkan ke saldo owner lewat GameLogic
            } else {
                // TODO: panggil GameLogic::handleBankruptcy(player, ownerId)
            }
        }
 
    } else if (status == 2) {
        // properti sedang digadaikan
    }
}

int RailroadTile::getRailroadOwnedCount(){
    return this->railroadOwnedCount;
}

void RailroadTile::setrailroadOwnedCount(int count){
    railroadOwnedCount = count;
}


// UtilityTile

UtilityTile::UtilityTile(int index, const std::string& code, const std::string& name,
                         int mortgageValue,
                         const std::map<int, int>& multiplierByCount)
    : PropertyTile(index, code, name, 0, mortgageValue),
      multiplierByCount(multiplierByCount) {}
 
int UtilityTile::calcRent(int diceroll) const {
    auto it = multiplierByCount.find(utilityOwnedCount);
    if(it != multiplierByCount.end()){
        return diceroll * it->second;
    }
    return 0;
}

void UtilityTile:: onLanded(Player& player){
    if(status == 0){
        status = 1;
    }
    else if(status == 1){
        if(ownerId != player.getId()){
            int rent = calcRent(lastDiceRoll);
            if(player.getMoney() >= rent){
                player -= rent;
                // TODO : tambahkan saldo owner
            }
            else{
                // Todo : Bangkrut?;
            }
        }
    }
    else{
        // lagi digadaikan, ga terjadi apa "
    }
}

void UtilityTile::setUtilityOwnedCount(int count){
    this->utilityOwnedCount = count;
}

void UtilityTile::setLastDiceRoll(int dice){
    this->utilityOwnedCount = dice;
}

int UtilityTile::getUtilityOwnedCount(){
    return this->utilityOwnedCount;
}

int UtilityTile::getLastDiceRoll(){
    return this->lastDiceRoll;
}