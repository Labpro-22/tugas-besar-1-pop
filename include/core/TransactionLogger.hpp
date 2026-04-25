#pragma once

#include "Enums.hpp"
#include <string>
#include <vector>

using namespace std;

class LogEntry {
  public:
    int turn;
    string username;
    LogActionType actionType;
    string detail;
};

class TransactionLogger {
  private:
    vector<LogEntry> logs;

  public:
    TransactionLogger();
    void logEvent(int turn, const string &username, LogActionType actionType,
                  const string &detail);
    vector<LogEntry> getLogs(int limit = -1) const;
    void clearLogs();
    string serialize() const;
    void deserialize(const string &data);
};
