#pragma once
#include <iomanip>
#include <iostream>
#include <string>

constexpr char const* k_exit = "exit";

constexpr char const* k_hist = "hist";

constexpr char const* k_init = "init";

constexpr char const* k_evolve = "evolve";

inline void println(std::string_view sv) {
    std::cout << sv << '\n';
}

inline void prompt() {
    std::cout << "> ";
}


// TODO
inline void print_usage() {
#   define TAB "\t"
#   define PADDING1 TAB << std::setw(42) << std::left
#   define PADDING2 std::setw(84) << std::internal

    std::cout << "Usage:" << '\n';
    std::cout << PADDING1 << "help" 
              << PADDING2 << "Print the usage." << '\n';
    std::cout << PADDING1 << "init [filename] [filename]" 
              << PADDING2 << "Initialize the Ising model from a spins file and bonds file." << '\n';
    std::cout << PADDING1 << "hist ([filename])"
              << PADDING2 << "Draw histogram on the terminal, or stream it to a local file." << '\n';
    std::cout << PADDING1 << "show [option]"
              << PADDING2 << "Show statistics of the current Ising model." << '\n'
              << PADDING1 << "The options are as follows:" << '\n'
              << TAB PADDING1 << "-e"
              << PADDING2 << "Show the total energy of the configuration." << '\n'
              << TAB PADDING1 << "-f [n]"
              << PADDING2 << "Flip one of the spins" << '\n'
              << TAB PADDING1 << "-s"
              << PADDING2 << "Print the serialized configuration" << '\n';

#   undef PADDING2
#   undef PADDING1
#   undef TAB
}

