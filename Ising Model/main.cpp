#include <iostream>
#include <string>
// #include <Python/Python.h>

#include "ising_model.hpp"
#include "repl.hpp"

static Ising gs_model;

[[noreturn]]
void repl() {
    println("REPL started.");
    prompt();
    std::string line;
    
    while (std::getline(std::cin, line)) {
        if (line == k_exit) {
            std::cout << std::endl;
            std::exit(0);
        }
        auto view = line | stdv::split(' ')
                         | stdv::transform([](auto&& range) { return std::string_view(&*range.begin(), std::ranges::distance(range)); })
                         | stdv::filter([](auto&& sv) { return sv != ""; });
        std::vector<std::string_view> command(view.begin(), view.end());

        if (command.size() == 0) {
            print_usage();
            continue;
        }

        // init [spins_file] [bond_file]
        if (command[0] == k_init) {
           if (command.size() != 3) {
               print_usage();
               continue;
           }
           // TODO
        }
        // hist [output_file]
        else if (command[0] == k_hist) {
            if (command.size() > 3) {
                print_usage();
                continue;
            }
        }
        // evolve [sweep_count]
        else if (command[0] == k_evolve) {
            if (command.size() > 3) {
                print_usage();
                continue;
            }
            gs_model.stablize();
            gs_model.markov_chain_monte_carlo([](auto&& self) {});
        }

        prompt();
    }
}

int main() {
    return 0;
}