#pragma once
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef __APPLE__
#   include <unistd.h>
#elif _WIN32
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
constexpr char const* k_grid = "grid";
constexpr char const* k_help = "help";
constexpr char const* k_hist = "hist";
constexpr char const* k_init = "init";
constexpr char const* k_ls = "ls";
constexpr char const* k_path = "path";
constexpr char const* k_reset = "reset";
constexpr char const* k_show = "show";
constexpr char const* k_time = "time";

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
    std::cerr << "\033[1m\033[31mThis function is not yet implemented\033[0m" << '\n';
}


/**
 * @brief Try parse and exec the line as a shell command.
 * @param line The line input.
 * @param argv Arguments of the line input.
 * @return Whether the line is executed as a shell command.
*/
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
            int res = chdir(argv[1].data());
            if (res < 0) {
                perror("chdir: ");
            }
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

    std::vector<energy_t> energy_record{};
    std::vector<int64_t> states_record{};
    std::vector<double> magnetization_record{};
    

    println("REPL started.");
    std::string line;
    
    while (true) {
        prompt();
        std::getline(std::cin, line);

        // Trim the line. (we probably need std::ranges::views::trim)
        auto line_view = line | stdv::drop_while(isspace)
                              | stdv::reverse
                              | stdv::drop_while(isspace)
                              | stdv::reverse;
        auto line_sv = std::string_view(&*line_view.begin(), stdr::distance(line_view));
        if (line_sv == "") {
            continue;
        }
        else if (line_sv == k_exit) {
            std::cout << "Now exit the REPL." << std::endl;
            std::exit(0);
        }
        else if (line_sv == k_help) {
            print_usage();
            continue;
        }
        else if (line_sv == k_path) {
            auto const path = stdf::absolute(stdf::current_path());
            std::cout << path << '\n';
            continue;
        }

        // Parse the line. (we need std::ranges::to!)
        auto words_view = line_view | stdv::split(' ')
                                    | stdv::transform([](auto&& range) { return std::string_view(&*range.begin(), stdr::distance(range)); })
                                    | stdv::filter([](auto&& sv) { return sv != ""; });


        auto options_view = words_view | stdv::filter([](auto&& sv) { return sv.size() >= 2 && sv.substr(0, 2) == "--"; });
        std::vector<std::string_view> options(options_view.begin(), options_view.end());

        bool record_time = false;
        auto const now = std::chrono::high_resolution_clock::now;
        decltype(now()) time{};
        decltype(now() - now()) delta_time{};
        for (auto const& opt : options) {
            auto opt_name = opt.substr(2);
            if (opt_name == k_time) {
                record_time = true;
            }
        }
#       define TIME_GUARD_START do {    \
            if (record_time) {          \
                time = now();           \
            }                           \
        } while (0)
#       define PRINT_TIME_SPENT do {          \
            std::cout << "Operation spent: " << std::chrono::duration_cast<std::chrono::milliseconds>(delta_time).count() << "ms" << '\n'; \
        } while (0)
#       define TIME_GUARD_STOP do {           \
            if (record_time) {                \
                delta_time = now() - time;    \
            PRINT_TIME_SPENT;                 \
            }                                 \
        } while (0)

#       define TIME_GUARD(...) do {           \
            TIME_GUARD_START;                 \
            __VA_ARGS__;                      \
            TIME_GUARD_STOP;                  \
        } while (0)


        auto command_view = words_view | stdv::filter([options = std::as_const(options)](auto&& sv) { return stdr::find(options, sv) == options.cend(); });
        std::vector<std::string_view> command(command_view.begin(), command_view.end());

        // Do it if it's system commands.
        if (shell_command(line, command)) {
            continue;
        }

        // init [spins_file] [bond_file]
        if (command[0] == k_init) {
            if (command.size() != 3) {
                print_usage();
                continue;
            }
            TIME_GUARD(g_model = make_ising(command[1], command[2]));
            continue;
        }
        // grid [row_ct] ?[col_ct]
        else if (command[0] == k_grid) {
            if (command.size() < 2) {
                print_usage();
                continue;
            }
            using enum std::errc;

            node_t row_ct{}, col_ct{};
            auto const& sv_1 = command[1];
            auto const [p1, e1] = std::from_chars(sv_1.data(), sv_1.data() + sv_1.size(), row_ct);

            if (e1 == invalid_argument) {
                std::cout << "Invalid node!" << '\n';
                continue;
            }
            else if (e1 == result_out_of_range) {
                std::cout << "Grid is too big!" << '\n';
                continue;
            }

            // if column count is not specified, use row count.
            if (command.size() == 2) {
                col_ct = row_ct;
            }
            else {
                auto const& sv_2 = command[2];
                auto const [p2, e2] = std::from_chars(sv_2.data(), sv_2.data() + sv_2.size(), col_ct);

                if (e2 == std::errc::invalid_argument) {
                    std::cout << "Invalid node!" << '\n';
                    continue;
                }
                else if (e1 == std::errc::result_out_of_range || e2 == std::errc::result_out_of_range) {
                    std::cout << "Grid is too big!" << '\n';
                    continue;
                }
            }

            TIME_GUARD(g_model = Ising::from_grid(row_ct, col_ct, g_bond_energy));
            continue;
        }

        // check validity of the global Ising model.
        if (!g_model.valid()) {
            std::cerr << "There's no model or the model is invalid right now. Use init to initialize an Ising model" << '\n';
            continue;
        }

        // hist [output_file]
        if (command[0] == k_hist) {
            if (command.size() > 3) {
                print_usage();
                continue;
            }
            undefined();
        }
        // show [options]
        else if (command[0] == k_show) {
            bool show_energy = false;
            bool show_config = false;
            bool show_state = false;
            bool show_mag = false;

            if (command.size() == 1) {
                show_energy = show_config = show_state = show_mag = true;
            }

            TIME_GUARD_START;
            for (auto const& opt : command | stdv::drop(1)) {
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
                else if (opt_name == "m") {
                    show_mag = true;
                }
            }

            auto const energy = g_model.energy();
            auto const state = g_model.state();
            auto const mag = g_model.magnetization();
            if (show_config) {
                std::cout << g_model << '\n';
            }
            if (show_energy) {
                std::cout << "The energy of this configuration is: " << energy << '\n';
            }
            if (show_state) {
                std::cout << "The state of this configuration is: " << state << '\n';
            }
            if (show_mag) {
                std::cout << "The magnetization of this configuration is: " << mag << '\n';
                std::cout << "The magnetization squared of this configuration is: " << mag * mag << '\n';
            }
            TIME_GUARD_STOP;
        }
        // evolve [sweep_count] [options]
        else if (command[0] == k_evolve) {
            if (command.size() < 2) {
                print_usage();
                continue;
            }
            TIME_GUARD_START;
            g_model.stablize();

            bool record_energy = false;
            bool record_state = false;
            bool record_magnetization = false;

            for (auto const& opt : command | stdv::drop(1)) {
                auto const opt_name = opt.substr(1);
                if (opt_name == "e") {
                    record_energy = true;
                }
                else if (opt_name == "s") {
                    record_state = true;
                }
                else if (opt_name == "m") {
                    record_magnetization = true;
                }
            }
            
            if (command.size() == 2) {
                auto const sweep_count = std::atoi(command[1].data());
                g_model.markov_chain_monte_carlo(Ising::pass, sweep_count);
                continue;
            }
            else {
                g_model.markov_chain_monte_carlo(Ising::pass);
            }
            TIME_GUARD_STOP;
        }
        else {
            print_usage();
        }
    }
    std::exit(EXIT_SUCCESS);

#   undef TIME_GUARD_START
#   undef TIME_GUARD_STOP
}


