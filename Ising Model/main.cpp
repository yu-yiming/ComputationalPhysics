#include <iostream>
#include <string>
#include <utility>
// #include <Python/Python.h>

#include "repl.hpp"

Ising g_model;

double g_beta = 0.42;

int main() {
    repl();
    return 0;
}