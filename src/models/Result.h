#pragma once
#include <string>
#include <utility>

template <typename T>
struct Result {
    bool success = false;
    T value{};
    std::string message;

    static Result<T> Success(T value, std::string message = "") {
        return Result<T>{true, std::move(value), std::move(message)};
    }

    static Result<T> Failure(std::string message) {
        return Result<T>{false, T{}, std::move(message)};
    }
};