#include "../../include/core/TransactionLogger.hpp"
#include <sstream>

TransactionLogger::TransactionLogger() {}

void TransactionLogger::logEvent(int turn, const string &username,
                                 LogActionType actionType,
                                 const string &detail) {
    logs.push_back({turn, username, actionType, detail});
}

vector<LogEntry> TransactionLogger::getLogs(int limit) const {
    if (limit == -1 || limit >= static_cast<int>(logs.size())) {
        return logs;
    }

    return vector<LogEntry>(logs.end() - limit, logs.end());
}

void TransactionLogger::clearLogs() { logs.clear(); }

string TransactionLogger::serialize() const {
    ostringstream oss;
    oss << logs.size() << "\n";
    for (const auto &log : logs) {
        oss << log.turn << " " << log.username << " "
            << actionTypeToString(log.actionType) << " " << log.detail << "\n";
    }
    return oss.str();
}

void TransactionLogger::deserialize(const string &data) {
    logs.clear();
    istringstream iss(data);
    int count;
    if (!(iss >> count))
        return;

    string line;
    getline(iss, line);

    for (int i = 0; i < count; ++i) {
        getline(iss, line);
        if (line.empty())
            continue;

        istringstream lineStream(line);
        LogEntry entry;
        string actionStr;

        lineStream >> entry.turn >> entry.username >> actionStr;
        entry.actionType = stringToActionType(actionStr);

        getline(lineStream >> ws, entry.detail);

        logs.push_back(entry);
    }
}
