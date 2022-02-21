#include <iostream>
#include <string>

#include "ising_model.hpp"

[[noreturn]]
void repl(Ising& config) {
    std::cout << "REPL started" << '\n';
    std::string line;
    
    while (std::getline(std::cin, line)) {
        
    }
}

int main() {
    auto config = make_ising("spin.txt", "bond.txt");
    repl(config);
    return 0;
}