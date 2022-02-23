#pragma once
#include <iomanip>
#include <iostream>
#include <string>

#include "ising_model.hpp"

constexpr char const* k_evolve = "evolve";
constexpr char const* k_exit = "exit";
constexpr char const* k_hist = "hist";
constexpr char const* k_init = "init";
constexpr char const* k_show = "show";

inline void println(std::string_view sv, std::ostream& out = std::cout) {
    std::cout << sv << '\n';
}

inline void prompt() {
    std::cout << "> ";
}

// TODO
inline void print_usage() {
#   define TAB "\t"
#   define PADDING1 TAB << std::setw(42) << std::left
#   define PADDING2 std::setw(84) << std::left

    std::cout << "Usage:" << '\n';
    std::cout << PADDING1 << "help" 
              << PADDING2 << "Print the usage." << '\n';
    std::cout << PADDING1 << "init [spins_file] [bonds_file]" 
              << PADDING2 << "Initialize the Ising model from a spins file and bonds file." << '\n';
    std::cout << PADDING1 << "hist ([output_file])"
              << PADDING2 << "Draw histogram on the terminal, or stream it to a local file." << '\n';
    std::cout << PADDING1 << "show [options]"
              << PADDING2 << "Show statistics of the current Ising model." << '\n'
              << PADDING1 << "The options are as follows:" << '\n'
              << TAB PADDING1 << "-e"
              << PADDING2 << "Show the total energy of the configuration." << '\n'
              << TAB PADDING1 << "-f [n]"
              << PADDING2 << "Flip one of the spins" << '\n'
              << TAB PADDING1 << "-s"
              << PADDING2 << "Print the serialized configuration" << '\n';
    std::cout << PADDING1 << "evolve [sweeps]"
              << PADDING2 << "Let the model evolove certain number of sweeps." << '\n';

#   undef PADDING2
#   undef PADDING1
#   undef TAB
}

inline void undefined() {
    std::cerr << "\033[1m\033[31mThis function is not yet implemented" << '\n';
}

[[noreturn]]
inline void repl() {
    extern Ising g_model;

    println("REPL started.");
    prompt();
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line == k_exit) {
            std::cout << std::endl;
            std::exit(0);
        }
        // Parse the line. (we need std::ranges::to !)
        auto view = line | stdv::split(' ')
                         | stdv::transform([](auto&& range) { return std::string_view(&*range.begin(), std::ranges::distance(range)); })
                         | stdv::filter([](auto&& sv) { return sv != ""; });
        std::vector<std::string_view> command(view.begin(), view.end());

        if (command.size() == 0) {
            print_usage();
            goto prompt;
        }
        // init [spins_file] [bond_file]
        else if (command[0] == k_init) {
            if (command.size() != 3) {
                print_usage();
                goto prompt;
            }
            g_model = make_ising(command[1], command[2]);
        }

        // check validity of the global Ising model.
        if (!g_model.valid()) {
            std::cerr << "There's no model or the model is invalid right now. Use init to initialize an Ising model" << '\n';
            goto prompt;
        }

        // hist [output_file]
        if (command[0] == k_hist) {
            if (command.size() > 3) {
                print_usage();
                goto prompt;
            }
            undefined();
        }
        // show [options]
        else if (command[0] == k_show) {
            bool show_energy = false;
            bool show_string = false;
            bool show_flip = false;

            for (auto opt : command | stdv::drop(1)) {
                auto const opt_name = opt.substr(1);
                if (opt_name == "e") {
                    show_energy = true;
                }
                else if (opt_name == "s") {
                    show_string = true;
                }
                else if (opt_name == "f") {
                    show_flip = true;
                }
            }

            auto const energy = g_model.energy();

        }
        // evolve [sweep_count]
        else if (command[0] == k_evolve) {
            if (command.size() > 3) {
                print_usage();
                prompt;
            }
            g_model.stablize();
            g_model.markov_chain_monte_carlo([](auto&& self) {});
        }
        else {
            print_usage();
        }
prompt:
        prompt();
    }
    std::exit(EXIT_SUCCESS);
}


