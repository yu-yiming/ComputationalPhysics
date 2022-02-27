#include <iostream>
#include <string>
#include <utility>

#include "external-libraries/matplotlibcpp.h"
#include "repl.hpp"

Ising g_model;

double g_beta = 1000;

namespace plt = matplotlibcpp;

int main() {
    // repl();
    // auto model = make_ising("./data/spins_1.txt", "./data/bonds_1.txt");
    // auto const print = [&model] {
    //     std::cout << model << '\n';
    //     std::cout << "Energy: " << model.energy() << '\n';
    //     std::cout << "State: " << model.state() << '\n';
    // };
    // print();
    // model.markov_chain_monte_carlo(Ising::pass);
    // print();
    // model.markov_chain_monte_carlo(Ising::pass);
    // print();
    // Prepare data.
    int n = 5000;
    std::vector<double> x(n), y(n), z(n), w(n,2);
    for(int i=0; i<n; ++i) {
        x.at(i) = i*i;
        y.at(i) = sin(2*M_PI*i/360.0);
        z.at(i) = log(i);
    }

    // Set the size of output image to 1200x780 pixels
    plt::figure_size(1200, 780);
    // Plot line from given x and y data. Color is selected automatically.
    plt::plot(x, y);
    // Plot a red dashed line from given x and y data.
    plt::plot(x, w,"r--");
    // Plot a line whose name will show up as "log(x)" in the legend.
    plt::named_plot("log(x)", x, z);
    // Set x-axis to interval [0,1000000]
    plt::xlim(0, 1000*1000);
    // Add graph title
    plt::title("Sample figure");
    // Enable legend.
    plt::legend();
    // Save the image (file format is determined by the extension)
    plt::save("./basic.png");
    return 0;
}