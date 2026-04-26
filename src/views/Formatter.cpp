#include "../../include/views/Formatter.hpp"
#include "../../include/core/Enums.hpp"
#include "../../include/models/PropertyTile.hpp"

#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace Color {
const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string DIM = "\033[2m";

const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";

const std::string BG_RED = "\033[41m";
const std::string BG_GREEN = "\033[42m";
const std::string BG_YELLOW = "\033[43m";
const std::string BG_BLUE = "\033[44m";
const std::string BG_MAGENTA = "\033[45m";
const std::string BG_CYAN = "\033[46m";
const std::string BG_WHITE = "\033[47m";
} // namespace Color

static std::string fmtMoney(int amount) {
    std::string s = std::to_string(amount);
    int pos = (int)s.length() - 3;
    while (pos > 0) {
        s.insert(pos, ".");
        pos -= 3;
    }
    return "M" + s;
}

static std::string colorGroupAnsi(const std::string &group) {
    if (group == "COKLAT")
        return "\033[38;5;130m";
    if (group == "BIRU_MUDA")
        return "\033[96m";
    if (group == "MERAH_MUDA")
        return "\033[95m";
    if (group == "ORANGE")
        return "\033[38;5;208m";
    if (group == "MERAH")
        return "\033[91m";
    if (group == "KUNING")
        return "\033[93m";
    if (group == "HIJAU")
        return "\033[92m";
    if (group == "BIRU_TUA")
        return "\033[94m";
    if (group == "ABU_ABU")
        return "\033[37m";
    return "\033[37m";
}

static std::string tileColorLabel(const std::string &group) {
    if (group == "COKLAT")
        return "[CK]";
    if (group == "BIRU_MUDA")
        return "[BM]";
    if (group == "MERAH_MUDA")
        return "[PK]";
    if (group == "ORANGE")
        return "[OR]";
    if (group == "MERAH")
        return "[MR]";
    if (group == "KUNING")
        return "[KN]";
    if (group == "HIJAU")
        return "[HJ]";
    if (group == "BIRU_TUA")
        return "[BT]";
    if (group == "ABU_ABU")
        return "[AB]";
    return "[DF]";
}

static std::string padLeft(const std::string &s, int width) {
    if ((int)s.size() >= width)
        return s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

static std::string padRight(const std::string &s, int width) {
    if ((int)s.size() >= width)
        return s.substr(0, width);
    return std::string(width - s.size(), ' ') + s;
}

static void printLine(char ch = '-', int len = 52) {
    std::cout << std::string(len, ch) << "\n";
}

static void printDoubleLine(int len = 52) {
    std::cout << std::string(len, '=') << "\n";
}

void Formatter::printBoard(Board &board, const std::vector<Player> &players,
                           int currentTurn, int maxTurn) {

    std::map<int, std::vector<int>> posMap;
    for (int i = 0; i < (int)players.size(); i++) {
        int pos = players[i].getPosition();
        posMap[pos].push_back(i + 1);
    }

    std::vector<int> jailedPlayers;
    std::vector<int> visitingPlayers;
    for (int i = 0; i < (int)players.size(); i++) {
        if (players[i].getStatus() == PlayerStatus::JAILED)
            jailedPlayers.push_back(i + 1);
        else if (players[i].getPosition() == 11)
            visitingPlayers.push_back(i + 1);
    }

    auto renderCell = [&](int tileIdx) -> std::pair<std::string, std::string> {
        if (tileIdx < 1 || tileIdx > 40)
            return {"          ", "          "};

        Tile *tile = board.getTileAt(tileIdx);
        if (!tile)
            return {"          ", "          "};

        std::string kode = tile->getKode();

        std::string colorLabel = "[DF]";
        std::string colorAnsi = "";
        std::string resetAnsi = "";

        if (tile->isStreet()) {
            colorLabel = tileColorLabel(tile->getColorGroup());
            colorAnsi = colorGroupAnsi(tile->getColorGroup());
            resetAnsi = Color::RESET;
        } else if (tile->isRailroad() || tile->isUtility()) {
            colorLabel = "[AB]";
        }

        std::string line1 =
            colorAnsi + colorLabel + " " + padLeft(kode, 4) + resetAnsi;

        std::string line2 = "";

        if (tile->isProperty()) {
            PropertyTile *prop = static_cast<PropertyTile *>(tile);
            int status = prop->getStatus();
            if (status == 1) {

                int oid = prop->getOwnerId();
                std::string ownerStr = "P" + std::to_string(oid + 1);

                if (tile->isStreet()) {
                    int lvl = tile->getRentLevel();
                    std::string bldg = "";
                    if (tile->hasBuildings()) {
                        if (lvl == 5)
                            bldg = " *";
                        else
                            bldg = " " + std::string(lvl, '^');
                    }
                    line2 = ownerStr + bldg;
                } else {
                    line2 = ownerStr;
                }
            } else if (status == 2) {
                line2 = "[M]";
            }
        }

        auto it = posMap.find(tileIdx);
        if (it != posMap.end()) {
            std::string pions = "";
            for (int pn : it->second) {
                if (tileIdx == 11) {

                    bool jailed = false;
                    for (int jn : jailedPlayers)
                        if (jn == pn) {
                            jailed = true;
                            break;
                        }
                    pions += jailed ? " IN" : " V";
                } else {
                    pions += " (" + std::to_string(pn) + ")";
                }
            }
            line2 += pions;
        }

        std::string line1_display = colorLabel + " " + padLeft(kode, 4);
        std::string line2_display = padLeft(line2, 8);

        return {padLeft(line1_display, 10), padLeft(line2_display, 10)};
    };

    const int W = 10;
    const std::string SEP = "+----------";
    const std::string TOP_BOT =
        SEP + SEP + SEP + SEP + SEP + SEP + SEP + SEP + SEP + SEP + SEP + "+";

    int topRow[] = {21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

    int bottomRow[] = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

    int leftCol[] = {20, 19, 18, 17, 16, 15, 14, 13, 12};

    int rightCol[] = {32, 33, 34, 35, 36, 37, 38, 39, 40};

    auto cell = [&](int idx) { return renderCell(idx); };

    std::cout << TOP_BOT << "\n";

    std::cout << "|";
    for (int idx : topRow) {
        std::cout << cell(idx).first << "|";
    }
    std::cout << "\n";

    std::cout << "|";
    for (int idx : topRow) {
        std::cout << cell(idx).second << "|";
    }
    std::cout << "\n";

    std::cout << TOP_BOT << "\n";

    std::vector<std::string> centerLines;
    {
        std::string title = "================================== ";
        centerLines.push_back("  " + title);

        std::string title2 = "|| NIMONSPOLI || ";
        centerLines.push_back("  " + title2);

        std::string title3 = "================================== ";
        centerLines.push_back("  " + title3);

        std::string turnInfo = "  TURN " + std::to_string(currentTurn) + " / " +
                               std::to_string(maxTurn) + "  ";
        centerLines.push_back("    " + turnInfo);

        centerLines.push_back("  ");

        centerLines.push_back("  ---------------------------------- ");
        centerLines.push_back("  LEGENDA KEPEMILIKAN & STATUS ");
        centerLines.push_back("  P1-P4 : Properti milik Pemain 1-4 ");
        centerLines.push_back("  ^ : Rumah | * : Hotel | [M] : Gadai");
    }

    while ((int)centerLines.size() < 9)
        centerLines.push_back("  ");

    for (int row = 0; row < 9; row++) {

        std::cout << "|" << cell(leftCol[row]).first << "|";

        std::string midLine = centerLines[row];

        int panelW = 9 * (W + 1) - 1;
        if ((int)midLine.size() < panelW)
            midLine += std::string(panelW - midLine.size(), ' ');
        else
            midLine = midLine.substr(0, panelW);
        std::cout << midLine << "|";

        std::cout << cell(rightCol[row]).first << "|\n";

        std::cout << "|" << cell(leftCol[row]).second << "|";
        std::string midLine2 = "";
        if ((int)midLine2.size() < panelW)
            midLine2 += std::string(panelW - midLine2.size(), ' ');
        std::cout << midLine2 << "|";
        std::cout << cell(rightCol[row]).second << "|\n";

        std::cout << "+----------+" << std::string(panelW, ' ')
                  << "+----------+\n";
    }

    std::cout << TOP_BOT << "\n";

    std::cout << "|";
    for (int idx : bottomRow) {
        std::cout << cell(idx).first << "|";
    }
    std::cout << "\n";

    std::cout << "|";
    for (int idx : bottomRow) {
        std::cout << cell(idx).second << "|";
    }
    std::cout << "\n";

    std::cout << TOP_BOT << "\n";
}

void Formatter::printAkta(const PropertyTile &property) {
    std::string colorAnsi = "";
    std::string resetAnsi = Color::RESET;
    std::string jenis = "PROPERTI";
    std::string colorLabel = "";

    if (property.isStreet()) {
        jenis = "STREET";
        colorLabel = property.getColorGroup();
        colorAnsi = colorGroupAnsi(colorLabel);
    } else if (property.isRailroad()) {
        jenis = "RAILROAD";
    } else if (property.isUtility()) {
        jenis = "UTILITY";
    }

    printDoubleLine(36);
    std::cout << "| " << Color::BOLD << "AKTA KEPEMILIKAN" << Color::RESET
              << "                   |\n";
    if (!colorLabel.empty()) {
        std::cout << "| " << colorAnsi << "[" << colorLabel << "] "
                  << property.getKode() << " - " << property.getName()
                  << resetAnsi << "\n";
    } else {
        std::cout << "| [" << jenis << "] " << property.getKode() << " - "
                  << property.getName() << "\n";
    }
    printDoubleLine(36);

    std::cout << "| Harga Beli  : "
              << padRight(fmtMoney(property.getPrice()), 18) << " |\n";
    std::cout << "| Nilai Gadai : "
              << padRight(fmtMoney(property.getmortgageValue()), 18) << " |\n";
    printLine('-', 36);

    if (property.isStreet()) {
        std::cout << "| Sewa (unimproved)   : "
                  << padRight(fmtMoney(property.calcRent(0)), 11) << " |\n";
        std::cout << "| Harga Rumah         : "
                  << padRight(fmtMoney(property.getHouseCost()), 11) << " |\n";
        std::cout << "| Harga Hotel         : "
                  << padRight(fmtMoney(property.getHotelCost()), 11) << " |\n";
    }

    printLine('-', 36);

    int status = property.getStatus();
    std::string statusStr;
    if (status == 0)
        statusStr = "BANK";
    else if (status == 1)
        statusStr =
            "OWNED (P" + std::to_string(property.getOwnerId() + 1) + ")";
    else if (status == 2)
        statusStr = "MORTGAGED [M]";

    std::cout << "| Status : " << padLeft(statusStr, 24) << " |\n";

    if (property.hasBuildings()) {
        std::cout << "| Bangunan: ";
        int lvl = property.getRentLevel();
        if (lvl == 5)
            std::cout << "Hotel";
        else
            std::cout << lvl << " Rumah";
        std::cout << "                          |\n";
    }

    printDoubleLine(36);
}

void Formatter::printProperti(const Player &player) {
    std::cout << "=== Properti Milik: " << player.getUsername() << " ===\n";

    const auto &owned = player.getOwnedProperties();
    if (owned.empty()) {
        std::cout << "Kamu belum memiliki properti apapun.\n";
        return;
    }

    std::map<std::string, std::vector<const PropertyTile *>> groups;
    std::vector<const PropertyTile *> railroads;
    std::vector<const PropertyTile *> utilities;

    for (const PropertyTile *prop : owned) {
        if (prop->isStreet()) {
            groups[prop->getColorGroup()].push_back(prop);
        } else if (prop->isRailroad()) {
            railroads.push_back(prop);
        } else if (prop->isUtility()) {
            utilities.push_back(prop);
        }
    }

    int totalWealth = 0;

    for (auto &[group, props] : groups) {
        std::string colorAnsi = colorGroupAnsi(group);
        std::cout << colorAnsi << "[" << group << "]" << Color::RESET << "\n";
        for (const PropertyTile *prop : props) {
            int status = prop->getStatus();

            std::string bldgStr = "";
            if (prop->hasBuildings()) {
                int lvl = prop->getRentLevel();
                bldgStr =
                    lvl == 5 ? " Hotel" : " " + std::to_string(lvl) + " rumah";
            }

            std::string statusStr = status == 2 ? " MORTGAGED [M]" : " OWNED";

            std::cout << "  - " << prop->getName() << " (" << prop->getKode()
                      << ")" << bldgStr << " " << fmtMoney(prop->getPrice())
                      << statusStr << "\n";
            totalWealth += prop->getPrice();
        }
    }

    if (!railroads.empty()) {
        std::cout << "[STASIUN]\n";
        for (const PropertyTile *prop : railroads) {
            int status = prop->getStatus();
            std::string statusStr = status == 2 ? " MORTGAGED [M]" : " OWNED";
            std::cout << "  - " << prop->getName() << " (" << prop->getKode()
                      << ") " << fmtMoney(prop->getPrice()) << statusStr
                      << "\n";
            totalWealth += prop->getPrice();
        }
    }

    if (!utilities.empty()) {
        std::cout << "[UTILITAS]\n";
        for (const PropertyTile *prop : utilities) {
            int status = prop->getStatus();
            std::string statusStr = status == 2 ? " MORTGAGED [M]" : " OWNED";
            std::cout << "  - " << prop->getName() << " (" << prop->getKode()
                      << ") " << fmtMoney(prop->getPrice()) << statusStr
                      << "\n";
            totalWealth += prop->getPrice();
        }
    }

    std::cout << "Total kekayaan properti: " << fmtMoney(totalWealth) << "\n";
}

void Formatter::printLog(const TransactionLogger &logger, int limit) {
    std::vector<LogEntry> logs = logger.getLogs(limit);

    if (limit == -1) {
        std::cout << "=== Log Transaksi Penuh ===\n";
    } else {
        std::cout << "=== Log Transaksi (" << limit << " Terakhir) ===\n";
    }

    if (logs.empty()) {
        std::cout << "(belum ada log)\n";
        return;
    }

    for (const LogEntry &entry : logs) {
        std::string actionStr = actionTypeToString(entry.actionType);
        std::cout << "[Turn " << std::setw(2) << entry.turn << "] "
                  << std::setw(10) << std::left << entry.username << " | "
                  << std::setw(12) << std::left << actionStr << " | "
                  << entry.detail << "\n";
    }
}

void Formatter::printPanelLikuidasi(const Player &player, int debt) {
    std::cout << Color::BOLD << "=== Panel Likuidasi ===" << Color::RESET
              << "\n";
    std::cout << "Uang kamu saat ini: " << fmtMoney(player.getMoney())
              << " | Kewajiban: " << fmtMoney(debt) << "\n";
    printLine();

    const auto &owned = player.getOwnedProperties();
    if (owned.empty()) {
        std::cout << "Tidak ada properti yang bisa dilikuidasi.\n";
        return;
    }

    int idx = 1;

    std::cout << "[Jual ke Bank]\n";
    for (const PropertyTile *prop : owned) {
        if (prop->getStatus() == 2)
            continue;

        int sellValue = prop->getPrice();
        std::string extra = "";
        if (prop->hasBuildings()) {
            int bldgVal = prop->calcBuildingResaleValue();
            sellValue += bldgVal;
            extra = " (termasuk bangunan: " + fmtMoney(bldgVal) + ")";
        }
        std::cout << "  " << idx++ << ". " << prop->getName() << " ("
                  << prop->getKode() << ") Harga Jual: " << fmtMoney(sellValue)
                  << extra << "\n";
    }

    std::cout << "[Gadaikan]\n";
    for (const PropertyTile *prop : owned) {
        if (prop->getStatus() != 1)
            continue;

        if (prop->hasBuildings())
            continue;

        std::cout << "  " << idx++ << ". " << prop->getName() << " ("
                  << prop->getKode()
                  << ") Nilai Gadai: " << fmtMoney(prop->getmortgageValue())
                  << "\n";
    }

    printLine();
    std::cout << "Pilih aksi (0 jika sudah cukup): ";
}

void Formatter::printBayarSewa(const Player &payer, const Player &owner,
                               const PropertyTile &property, int rentAmount) {
    if (property.getStatus() == 2) {

        std::cout << "Kamu mendarat di " << property.getName() << " ("
                  << property.getKode() << "), milik " << owner.getUsername()
                  << ".\n"
                  << "Properti ini sedang digadaikan [M]. Tidak ada sewa yang "
                     "dikenakan.\n";
        return;
    }

    std::string kondisi = "unimproved";
    if (property.hasBuildings()) {
        int lvl = property.getRentLevel();
        if (lvl == 5)
            kondisi = "Hotel";
        else
            kondisi = std::to_string(lvl) + " rumah";
    }

    std::cout << "Kamu mendarat di " << property.getName() << " ("
              << property.getKode() << "), milik " << owner.getUsername()
              << "!\n"
              << "Kondisi : " << kondisi << "\n"
              << "Sewa    : " << Color::RED << fmtMoney(rentAmount)
              << Color::RESET << "\n"
              << "Uang kamu       : " << fmtMoney(payer.getMoney() + rentAmount)
              << " -> " << fmtMoney(payer.getMoney()) << "\n"
              << "Uang " << owner.getUsername() << " : "
              << fmtMoney(owner.getMoney() - rentAmount) << " -> "
              << fmtMoney(owner.getMoney()) << "\n";
}

void Formatter::printBayarPajak(const Player &player, int amount) {
    std::cout << "Uang kamu: " << fmtMoney(player.getMoney() + amount) << " -> "
              << Color::RED << fmtMoney(player.getMoney()) << Color::RESET
              << "\n";
}

void Formatter::printAuction(const PropertyTile &property,
                             const Player &highestBidder, int highestBid) {
    std::cout << "\nLelang selesai!\n"
              << "Pemenang  : " << Color::GREEN << highestBidder.getUsername()
              << Color::RESET << "\n"
              << "Harga akhir: " << fmtMoney(highestBid) << "\n"
              << "Properti " << property.getName() << " (" << property.getKode()
              << ") kini dimiliki " << highestBidder.getUsername() << ".\n";
}

void Formatter::printFestival(const PropertyTile &property, int oldRent,
                              int newRent, int duration) {
    std::cout << Color::YELLOW << "Efek festival aktif!" << Color::RESET << "\n"
              << "Properti  : " << property.getName() << " ("
              << property.getKode() << ")\n"
              << "Sewa awal  : " << fmtMoney(oldRent) << "\n"
              << "Sewa sekarang : " << Color::YELLOW << fmtMoney(newRent)
              << Color::RESET << "\n"
              << "Durasi    : " << duration << " giliran\n";
}

void Formatter::printHandCards(const Player &player) {
    std::cout << "Daftar Kartu Kemampuan Spesial " << player.getUsername()
              << ":\n";

    const auto &cards = player.getHandCards();
    if (cards.empty()) {
        std::cout << "  (tidak ada kartu)\n";
        return;
    }

    for (int i = 0; i < (int)cards.size(); i++) {
        const SkillCard *card = cards[i];
        std::cout << "  " << (i + 1) << ". " << Color::CYAN
                  << card->getCardName() << Color::RESET << " - "
                  << card->getCardDescription();
        std::string val = card->getValueString();
        if (!val.empty())
            std::cout << " [" << val << "]";
        std::cout << "\n";
    }
    std::cout << "  0. Batal\n";
}

void Formatter::printWinner(const std::vector<Player> &players,
                            bool isBankruptcy) {
    printDoubleLine(52);

    if (isBankruptcy) {
        std::cout << Color::BOLD
                  << "Permainan selesai! (Semua pemain kecuali satu bangkrut)"
                  << Color::RESET << "\n";
        printDoubleLine(52);

        for (const Player &p : players) {
            if (p.getStatus() == PlayerStatus::ACTIVE) {
                std::cout << "Pemain tersisa:\n  - " << p.getUsername() << "\n";
                std::cout << Color::BOLD << Color::GREEN
                          << "Pemenang: " << p.getUsername() << Color::RESET
                          << "\n";
                break;
            }
        }
    } else {
        std::cout << Color::BOLD
                  << "Permainan selesai! (Batas giliran tercapai)"
                  << Color::RESET << "\n";
        printDoubleLine(52);

        std::cout << "Rekap pemain:\n";
        for (const Player &p : players) {
            if (p.getStatus() == PlayerStatus::BANKRUPT)
                continue;
            std::cout << p.getUsername() << "\n"
                      << "  Uang     : " << fmtMoney(p.getMoney()) << "\n"
                      << "  Properti : " << p.getOwnedProperties().size()
                      << "\n"
                      << "  Kartu    : " << p.getHandCards().size() << "\n";
        }
        printLine('-', 52);

        std::vector<const Player *> candidates;
        for (const Player &p : players)
            if (p.getStatus() != PlayerStatus::BANKRUPT)
                candidates.push_back(&p);

        std::sort(candidates.begin(), candidates.end(),
                  [](const Player *a, const Player *b) {
                      if (a->getMoney() != b->getMoney())
                          return a->getMoney() > b->getMoney();
                      if (a->getOwnedProperties().size() !=
                          b->getOwnedProperties().size())
                          return a->getOwnedProperties().size() >
                                 b->getOwnedProperties().size();
                      return a->getHandCards().size() >
                             b->getHandCards().size();
                  });

        std::vector<const Player *> winners;
        if (!candidates.empty()) {
            winners.push_back(candidates[0]);
            for (int i = 1; i < (int)candidates.size(); i++) {
                bool seri =
                    (candidates[i]->getMoney() == candidates[0]->getMoney() &&
                     candidates[i]->getOwnedProperties().size() ==
                         candidates[0]->getOwnedProperties().size() &&
                     candidates[i]->getHandCards().size() ==
                         candidates[0]->getHandCards().size());
                if (seri)
                    winners.push_back(candidates[i]);
                else
                    break;
            }
        }

        if (winners.size() == 1) {
            std::cout << Color::BOLD << Color::GREEN
                      << "Pemenang: " << winners[0]->getUsername()
                      << Color::RESET << "\n";
        } else {
            std::cout << Color::BOLD << Color::YELLOW
                      << "Seri! Pemenang bersama:" << Color::RESET << "\n";
            for (const Player *w : winners)
                std::cout << "  - " << w->getUsername() << "\n";
        }
    }

    printDoubleLine(52);
}

void Formatter::printBankrupt(const Player &player,
                              const std::string &creditor) {
    std::cout << Color::BOLD << Color::RED << player.getUsername()
              << " dinyatakan BANGKRUT!" << Color::RESET << "\n"
              << "Kreditor: " << creditor << "\n";

    if (creditor == "Bank") {
        std::cout << "Uang sisa " << fmtMoney(player.getMoney())
                  << " diserahkan ke Bank.\n"
                  << "Seluruh properti dikembalikan ke status BANK.\n"
                  << "Bangunan dihancurkan — stok dikembalikan ke Bank.\n"
                  << "Properti akan dilelang satu per satu:\n";
        for (const PropertyTile *prop : player.getOwnedProperties()) {
            std::cout << "  -> Lelang: " << prop->getName() << " ("
                      << prop->getKode() << ") ...\n";
        }
    } else {

        std::cout << "Pengalihan aset ke " << creditor << ":\n"
                  << "  - Uang tunai sisa : " << fmtMoney(player.getMoney())
                  << "\n";
        for (const PropertyTile *prop : player.getOwnedProperties()) {
            const std::string &statusStr =
                prop->getStatus() == 2 ? " MORTGAGED [M]" : " OWNED";
            std::string bldg = "";
            if (prop->hasBuildings()) {
                int lvl = prop->getRentLevel();
                bldg = lvl == 5 ? " (Hotel)"
                                : " (" + std::to_string(lvl) + " rumah)";
            }
            std::cout << "  - " << prop->getName() << " (" << prop->getKode()
                      << ")" << bldg << statusStr << "\n";
        }
        std::cout << creditor << " menerima semua aset " << player.getUsername()
                  << ".\n";
    }

    std::cout << player.getUsername() << " telah keluar dari permainan.\n";
}
