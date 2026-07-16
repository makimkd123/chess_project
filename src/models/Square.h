#pragma once

struct Square{
    int file{};
    int rank{};

    bool operator==(const Square&) const = default;
};