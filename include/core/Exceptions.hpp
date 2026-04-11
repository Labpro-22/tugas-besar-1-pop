#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <exception>

class NotEnoughMoneyException : public std::exception {
public:
    const char* what() const noexcept override;
};

class MaxCardLimitException : public std::exception {
public:
    const char* what() const noexcept override;
};

#endif