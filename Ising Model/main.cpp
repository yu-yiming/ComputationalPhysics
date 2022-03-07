#include <iostream>
#include <numbers>
#include <string>
#include <utility>

#include "external-libraries/matplotlibcpp.h"
#include "repl.hpp"

Ising g_model;

double g_beta = 0.1;
field_t g_bond_energy = 1.0;

namespace plt = matplotlibcpp;
using namespace std::numbers;

struct Test {
    auto operator ()(auto&& self) const {
        v.push_back(v.size());
    }
    auto operator ()() const {
        return v;
    }

    mutable std::vector<std::size_t> v;
};

int main() {
    //repl();



    g_model.from_grid(3, 3);
    auto&& s = Ising::record_state;
    auto&& t = Test{};
    g_model.markov_chain_monte_carlo(t);
    auto res = s();
    for (auto k : t()) {
        std::cout << k << '\n';
    }

    //auto&& r = Ising::record<Ising::StateRecorder, Ising::EnergyRecorder, Ising::MagnetizationRecorder>;
    //r(g_model);
    //auto res = r();

    //auto model = make_ising("./data/spins_1.txt", "./data/bonds_1.txt");
    //auto const print = [&model] {
    //    std::cout << model << '\n';
    //    std::cout << "Energy: " << model.energy() << '\n';
    //    std::cout << "State: " << model.state() << '\n';
    //};
    //print();
    //model.markov_chain_monte_carlo(Ising::pass);
    //print();
    //model.markov_chain_monte_carlo(Ising::pass);
    //print();
   // Prepare data.
   //int n = 5000;
   //std::vector<double> x(n), y(n), z(n), w(n, 2);
   //for (int i = 0; i < n; ++i) {
   //    x.at(i) = i * i;
   //    y.at(i) = sin(2 * pi * i / 360.0);
   //    z.at(i) = log(i);
   //}

   //plt::figure_size(1200, 780);
   //plt::plot(x, y);
   //plt::plot(x, w, "r--");
   //plt::named_plot("log(x)", x, z);
   //plt::xlim(0, 1000 * 1000);
   //plt::title("Sample figure");
   //plt::legend();
   //plt::save("./basic.png");
    return 0;
}
