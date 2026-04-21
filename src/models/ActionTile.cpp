#include "../../include/models/ActionTile.hpp"
//ActionTile

ActionTile::ActionTile(int index, const std::string& code, const std::string& name)
:Tile(index,code,name){}

void ActionTile::onLanded(Player& player){
    triggerEffect(player);
}

//CardTile

CardTile::CardTile(int index, const std::string& code, const std::string& name,
             string type)
             :ActionTile(index,code,name),cardType(type){}

void CardTile::triggerEffect(Player& player){
    if(cardType == "1"){
        applyChanceCard(player);
    }
    else if(cardType == "2"){
        applyChanceCard(player);
    }
}

string CardTile::getCardType(){
    return this->cardType;
}

void CardTile::applyChanceCard(Player& player){
    // TODO : Implementasi apabila player mengambil kartu kesempatan
}

void CardTile::applyCommunityCard(Player& player){
    // TODO : Implementasi apabila player mengambil kartu komunitas
}

// TaxTile

TaxTile::TaxTile(int index, const std::string& code, const std::string& name,
            string type,
            int flatAmount,
            double percentageRate = 0.0)
            :ActionTile(index,code,name),taxType(type),flatAmount(flatAmount),percentageRate(percentageRate){}

void TaxTile::triggerEffect(Player& player){
    if(taxType == "PPH") handlePPH(player);
    else if(taxType == "PBM") handlePBM(player);
}

void TaxTile::handlePPH(Player& player){
    int pilihan = 0;
    if(pilihan == 1){
        // membayar sejumlah flat
    }
    else if (pilihan == 2){
        // membayar persentase total dari kekayaan
    }
}

void TaxTile::handlePBM(Player& player){
    if(player.getMoney() >= flatAmount){
        player -= flatAmount;
    }
    else{
        player.declareBankruptcy();
    }
}

int TaxTile::computeNetWorth(const Player& player) const{
    int total = player.getMoney();
    for(auto* v : player.getOwnedProperties()){
        total += v->getPrice();
    }
    return total;
}

//FestivalTile

FestivalTile::FestivalTile(int index, const std::string& code, const std::string& name)
:ActionTile(index,code,name){}

void FestivalTile::triggerEffect(Player& player){
    applyFestivalToProperty(player);
}

void FestivalTile::applyFestivalToProperty(Player& player){
    auto it = player.getOwnedProperties();
    if(player.getOwnedProperties().empty()){
        return;   
    }
    string pilihan;
    PropertyTile* choose = nullptr;
    // masukin pilihan
    for(auto* v : player.getOwnedProperties()){
        if(v->getKode() == pilihan){
            choose = v;
            break;
        }
    }

    if(!choose) return;
    //TODO : Efek festival pada StreetTile
}

// SpecilTile

SpecialTile::SpecialTile(int index, const std::string& code, const std::string& name)
:ActionTile(index,code,name){}

void SpecialTile::onLanded(Player& player){
    handleArrival(player);
}

//GoTile

GoTile::GoTile(int index, const std::string& code, const std::string& name,
            int salary)
            :SpecialTile(index,code,name),salary(salary){}

void GoTile::handleArrival(Player& player) {
    awardSalary(player);
}
 
void GoTile::awardPassSalary(Player& player) const {
    player += salary;
}
 
void GoTile::awardSalary(Player& player) const {
    player += salary;
}

int GoTile::getSalary() const {
    return salary;
}
 
// JailTile
 
JailTile::JailTile(int index, const std::string& code, const std::string& name, int fine)
    : SpecialTile(index, code, name), fine(fine) {}
 
int JailTile::getFine() const {
    return fine;
}
 
void JailTile::handleArrival(Player& player) {
    if (player.getStatus() == PlayerStatus::JAILED ) {
        // Pemain berstatus tahanan — tampilkan pilihan keluar
        processTurnInJail(player);
    } else {
        // Pemain hanya mampir dari hasil lemparan dadu normal
        cout << "Kamu hanya mampir di Petak Penjara. Tidak ada penalti.\n";
    }
}
 
void JailTile::imprisonPlayer(Player& player) {
    player.goToJail();
    // Pindahkan posisi pemain ke petak penjara (index 11 sesuai papan)
    int steps = abs(11 - player.getPosition());
    player.move(steps);
}
 
void JailTile::releasePlayer(Player& player) {
    // TODO : ubah status dari Jailed ke Active
}
 
void JailTile::processTurnInJail(Player& player) {
    int turnsInJail = player.getJailTurns();
 
    // Giliran ke-4: wajib bayar denda, tidak ada pilihan lain
    if (turnsInJail >= 3) {
        handlePayFine(player);
        return;
    }
    // TODO: opsi 3 - gunakan kartu Bebas dari Penjara (jika punya)
    cout << "Pilihan (1/2): ";
 
    int pilihan;
    cin >> pilihan;
 
    if (pilihan == 1) {
        handlePayFine(player);
    } else if (pilihan == 2) {
        handleRollForDouble(player);
    } else {
        player.decrementJailTurn();
    }
}
 
void JailTile::handlePayFine(Player& player) {
    if (player.getMoney() >= fine) {
        player -= fine;
        releasePlayer(player);
        cout << "Uang kamu sekarang: M" << player.getMoney() << "\n";
        // TODO: GameLogic melanjutkan lemparan dadu normal untuk giliran ini
    } else {
        // TODO: GameLogic::handleBankruptcy(player, -1)
    }
}
 
void JailTile::handleRollForDouble(Player& player) {
    // TODO: GameLogic melempar dadu dan mengecek apakah double
    // Jika double: releasePlayer() lalu gerakkan pemain
    // Jika tidak double: player tidak bergerak, increment jailTurns
    int turnsInJail = player.getJailTurns();
    // Simulasi sementara, entar digantikan GameLogic
    int d1 = rand() % 6 + 1;
    int d2 = rand() % 6 + 1;
 
    if (d1 == d2) {
        releasePlayer(player);
        // TODO: GameLogic::movePlayer(player, d1 + d2)
    } else {
        player.decrementJailTurn();
    }
}
 
// FreeParkingTile
 
FreeParkingTile::FreeParkingTile(int index, const std::string& code, const std::string& name)
    : SpecialTile(index, code, name) {}
 
void FreeParkingTile::handleArrival(Player& player) {
    // Tidak ada efek apapun, hanya rest area
}
 
// GoToJailTile
 
GoToJailTile::GoToJailTile(int index, const std::string& code, const std::string& name)
    : SpecialTile(index, code, name), jailRef(nullptr) {}
 
void GoToJailTile::setJailTile(JailTile* jail) {
    jailRef = jail;
}
 
void GoToJailTile::handleArrival(Player& player) { 
    if (jailRef != nullptr) {
        jailRef->imprisonPlayer(player);
    }
    // TODO: GameLogic::endTurn(player)
}
