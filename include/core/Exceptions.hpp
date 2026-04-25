#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include "Enums.hpp"
#include <exception>
#include <string>

class GameException : public std::exception {
  private:
    GameErrorCode errorCode;
    std::string errorMessage;

  public:
    GameException(GameErrorCode errorCode, const std::string &errorMessage)
        : errorCode(errorCode), errorMessage(errorMessage) {}

    GameErrorCode getErrorCode() const { return errorCode; }

    std::string getErrorMessage() const { return errorMessage; }

    const char *what() const noexcept override { return errorMessage.c_str(); }
};

class NotEnoughMoneyException : public GameException {
  public:
    NotEnoughMoneyException(int have, int need, const std::string &context)
        : GameException(GameErrorCode::NOT_ENOUGH_MONEY,
                        "Uang tidak cukup untuk: " + context +
                            ". Punya: " + std::to_string(have) +
                            ", Butuh: " + std::to_string(need)) {}
};

class MaxCardLimitException : public GameException {
  public:
    MaxCardLimitException()
        : GameException(GameErrorCode::MAX_CARD_LIMIT,
                        "Slot untuk kartu sudah limit! Maksimal 3") {}
};

class InvalidCommandException : public GameException {
  public:
    InvalidCommandException(const std::string command)
        : GameException(GameErrorCode::INVALID_COMMAND,
                        "Command " + command + " tidak dikenali!") {}
};

class InvalidPropertyException : public GameException {
  public:
    InvalidPropertyException(const std::string kode)
        : GameException(GameErrorCode::INVALID_PROPERTY,
                        "Kode petak " + kode + " tidak dikenali!") {}
};

class GameStateException : public GameException {
  public:
    GameStateException(const std::string &context)
        : GameException(GameErrorCode::GAME_STATE_ERROR,
                        "Tidak diperbolehkan melakukan " + context +
                            " sekarang") {}
};

#endif
