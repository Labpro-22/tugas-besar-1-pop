#include "../../include/core/Auction.hpp"

#include <algorithm>
#include <iostream>

using namespace std;

// ── Konstruktor ───────────────────────────────────────────────────────────

// Objek Auction hanya valid jika punya properti target dan minimal 1 penawar.
// Jika activeBidders kosong saat konstruksi, run() langsung return tanpa efek.
Auction::Auction(PropertyTile *property, const vector<Player *> &initialBidders)
    : targetProperty(property), activeBidders(initialBidders),
      currentHighestBid(0), currentHighestBidder(nullptr),
      currentBidderIndex(0) {}

bool Auction::placeBid(Player *bidder, int amount) {
    if (amount <= currentHighestBid)
        return false;
    if (bidder->getMoney() < amount)
        return false;

    currentHighestBid = amount;
    currentHighestBidder = bidder;
    return true;
}

void Auction::passTurn(Player *bidder) {
    auto it = find(activeBidders.begin(), activeBidders.end(), bidder);
    if (it == activeBidders.end())
        return;

    int removedIdx = (int)(it - activeBidders.begin());
    activeBidders.erase(it);

    if (removedIdx < currentBidderIndex)
        currentBidderIndex--;

    // Wrap index agar tidak out-of-bounds setelah erase
    if (!activeBidders.empty())
        currentBidderIndex = currentBidderIndex % (int)activeBidders.size();
}

void Auction::delegateProperty() {
    if (!currentHighestBidder || !targetProperty)
        return;

    *currentHighestBidder -= currentHighestBid;

    // Set kepemilikan properti ke pemenang lelang
    targetProperty->setOwnerId(currentHighestBidder->getId());
    targetProperty->setStatus(1); // 1 = OWNED

    currentHighestBidder->addProperty(targetProperty);
}

void Auction::run() {
    if (activeBidders.empty() || !targetProperty)
        return;

    cout << "\n=== LELANG DIMULAI: " << targetProperty->getName() << " ===\n";

    // Loop selama masih ada lebih dari 1 penawar aktif
    while ((int)activeBidders.size() > 1) {
        // Wrap aman — passTurn sudah menjaga index valid
        currentBidderIndex = currentBidderIndex % (int)activeBidders.size();
        Player *cur = activeBidders[currentBidderIndex];

        cout << "\n[LELANG] Giliran   : " << cur->getUsername()
             << "\n[LELANG] Bid saat ini: Rp" << currentHighestBid;
        if (currentHighestBidder)
            cout << " (oleh " << currentHighestBidder->getUsername() << ")";
        cout << "\n[LELANG] Kas Anda  : Rp" << cur->getMoney()
             << "\nMasukkan bid (0 = pass): ";

        int amount = 0;
        cin >> amount;

        if (amount <= 0) {
            passTurn(cur);
            cout << cur->getUsername() << " mengoper.\n";
            // Index sudah dikoreksi oleh passTurn — JANGAN increment lagi
        } else if (placeBid(cur, amount)) {
            cout << cur->getUsername() << " bid Rp" << amount << ".\n";
            currentBidderIndex =
                (currentBidderIndex + 1) % (int)activeBidders.size();
        } else {
            cout << "Bid tidak valid (harus > Rp" << currentHighestBid
                 << " dan uang cukup).\n";
            currentBidderIndex =
                (currentBidderIndex + 1) % (int)activeBidders.size();
        }
    }

    // Kasus khusus: 1 penawar tersisa tapi BELUM PERNAH ada yang bid.
    // Ini terjadi jika semua pemain lain pass sebelum ada bid pertama,
    // sehingga pemain terakhir ini mendapat kesempatan terakhir.
    if ((int)activeBidders.size() == 1 && currentHighestBidder == nullptr) {
        Player *last = activeBidders[0];
        cout << "\n[LELANG] Hanya " << last->getUsername()
             << " tersisa. Masukkan bid (0 = mundur): ";
        int amount = 0;
        cin >> amount;
        if (amount > 0 && last->getMoney() >= amount) {
            currentHighestBid = amount;
            currentHighestBidder = last;
        }
    }

    // Umumkan hasil
    if (currentHighestBidder) {
        cout << "\n=== LELANG SELESAI ==="
             << "\nPemenang : " << currentHighestBidder->getUsername()
             << "\nHarga    : Rp" << currentHighestBid << "\n";
        delegateProperty();
    } else {
        cout << "\n=== LELANG SELESAI ==="
             << "\nTidak ada penawar. Properti tetap di Bank.\n";
    }
}
