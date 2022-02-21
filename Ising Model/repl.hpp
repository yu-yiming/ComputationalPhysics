#pragma once
#include <iostream>

constexpr char const* k_exit = "exit";

constexpr char const* k_hist = "hist";

constexpr char const* k_init = "init";

inline void println(std::string_view sv) {
    std::cout << sv << '\n';
}

inline void prompt() {
    std::cout << "> ";
}

// TODO
inline void print_usage() {
    std::cout << "Usage: ";
}
