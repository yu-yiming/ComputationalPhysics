#pragma once
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef __APPLE__
#   include <unistd.h>
#elif _WIN32 defined
#   include <direct.h>
#   define chdir _chdir
#endif

#include "ising_model.hpp"

namespace stdf = std::filesystem;

constexpr char const* k_cat = "cat";
constexpr char const* k_cd = "cd";
constexpr char const* k_dir = "dir";
constexpr char const* k_echo = "echo";
constexpr char const* k_evolve = "evolve";
constexpr char const* k_exit = "exit";
constexpr char const* k_hist = "hist";
constexpr char const* k_init = "init";
constexpr char const* k_ls = "ls";
constexpr char const* k_path = "path";
constexpr char const* k_reset = "reset";
constexpr char const* k_show = "show";

inline void println(std::string_view sv, std::ostream& out = std::cout) {
    std::cout << sv << '\n';
}

inline void prompt() {
    auto const non_quote = [](auto arg) {
        return arg != '"';
    };
    std::string&& path = stdf::current_path().filename().string();
    for (auto ch : path | stdv::filter(non_quote)) {
        std::cout << ch;
    }
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

inline bool shell_command(std::string_view line, std::vector<std::string_view> const& argv) {
    auto const match_prefix = [](std::string_view target, std::string_view pattern) {
        if (target.size() < pattern.size()) {
            return false;
        }
        return target.substr(0, pattern.size()) == pattern 
            && (target.size() == pattern.size() || target[pattern.size()] == ' ');
    };

    auto const is_shell_command = argv[0] == k_cat 
                               || argv[0] == k_cd 
                               || argv[0] == k_dir 
                               || argv[0] == k_echo 
                               || argv[0] == k_ls;

    if (is_shell_command) {
        if (argv[0] == k_cd) {
            chdir(argv[1].data());
        }
        else {
            system(line.data());
        }
    }
    return is_shell_command;
}

[[noreturn]]
inline void repl() {
    extern Ising g_model;

    println("REPL started.");
    prompt();
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line == "") {
            prompt();
            continue;
        }
        else if (line == k_exit) {
            std::cout << "Now exit the REPL." << std::endl;
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

        // Do it if it's system commands.
        if (shell_command(line, command)) {
            goto prompt;
        }

        // init [spins_file] [bond_file]
        if (command[0] == k_init) {
            if (command.size() != 3) {
                print_usage();
                goto prompt;
            }
            g_model = make_ising(command[1], command[2]);
            goto prompt;
        }
        else if (command[0] == k_path) {
            auto const path = stdf::absolute(stdf::current_path());
            std::cout << path << '\n';
            goto prompt;
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
            bool show_config = false;
            bool show_state = false;

            if (command.size() == 1) {
                show_energy = show_config = show_state = true;
            }

            for (auto opt : command | stdv::drop(1)) {
                auto const opt_name = opt.substr(1);
                if (opt_name == "e") {
                    show_energy = true;
                }
                else if (opt_name == "c") {
                    show_config = true;
                }
                else if (opt_name == "s") {
                    show_state = true;
                }
            }

            auto const energy = g_model.energy();
            auto const state = g_model.state();
            if (show_config) {
                std::cout << g_model << '\n';
            }
            if (show_energy) {
                std::cout << "The energy of this configuration is: " << energy << '\n';
            }
            if (show_state) {
                std::cout << "The state of this configuration is: " << state << '\n';
            }
        }
        // evolve [sweep_count]
        else if (command[0] == k_evolve) {
            if (command.size() > 3) {
                print_usage();
                goto prompt;
            }
            g_model.stablize();
            
            if (command.size() == 3) {
                auto const sweep_count = std::atoi(command[2].data());
                g_model.markov_chain_monte_carlo([](auto&& self) {}, sweep_count);
                goto prompt;
            }
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


