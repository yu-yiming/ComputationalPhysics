#pragma once
#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "typedef.hpp"

namespace stdv = std::ranges::views;

extern double beta;

/**
 * @brief Generate a random number within a give range.
 * 
 * @tparam T Either a integral type or a floating piont type.
 * @param left The left bound of the range, inclusive.
 * @param right The right bound of the range, exclusive.
 * @return T A random number.
 */
template<typename T>
T randnum(T left, T right) {
    using distribution_t = std::conditional_t<std::is_integral_v<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;

    std::random_device rd{};
    std::default_random_engine eng(rd());
    distribution_t d(left, right);

    return d(eng);
}

template<typename SpinT, typename EnergyT, typename FieldT>
class BasicIsing {
public:
    using NodeT = node_t;
    using SpinValue = SpinTrait<SpinT>;

    BasicIsing(std::vector<std::pair<NodeT, FieldT>> const& spins, std::vector<std::tuple<NodeT, NodeT, EnergyT>> const& bonds)
        : m_spins(spins.size()), m_fields(spins.size()), m_neighbors(spins.size()), m_energy(0.0), m_state(0) {

        auto const base = SpinValue::state_count();

        for (auto& spin : m_spins) {
            spin = random_spin<SpinT>();
            m_state *= base;
            m_state += SpinValue::index(spin);
        }
        for (auto [i, h] : spins) {
            m_fields[i] = h;
            m_energy += SpinValue::of(m_spins[i]) * h;
        }
        for (auto [i, j, e] : bonds) {
            m_neighbors[i].emplace_back(j, e);
            m_neighbors[j].emplace_back(i, e);
            m_energy -= SpinValue::of(m_spins[i]) * SpinValue::of(m_spins[j]) * e;
        }
    }

    EnergyT delta(NodeT n) {
        return delta(n, SpinValue::from(-SpinValue::of(m_spins[n])));
    }

    EnergyT delta(NodeT n, SpinT new_spin) noexcept {
        static std::tuple<NodeT, SpinT, EnergyT> cache;
        auto& [cached_node, cached_spin, cached_energy] = cache;

        if (cached_node == n && cached_spin == new_spin) {
            cache = std::tuple(-1, SpinT{}, EnergyT{});
            return cached_energy;
        }
        cached_node = n;
        cached_spin = new_spin;

        auto const spin_delta = new_spin - m_spins[n];
        auto delta = m_fields[n] * spin_delta;
        for (auto [i, e] : m_neighbors[n]) {
            delta += SpinValue::of(m_spins[i]) * e;
        }
        cached_energy = delta;
        return delta;
    }

    void flip(NodeT n) {
        this->flip(n, SpinValue::from(-SpinValue::of(m_spins[n])));
    }

    void filp(NodeT n, SpinT new_spin) {
        auto const delta = this->delta(n, new_spin);
        m_spins[n] = new_spin;
        m_energy += delta;

        auto const spin_delta = new_spin - m_spins[n];
        m_state += static_cast<int64_t>(std::pow(base, m_spins.size() - n - 1)) * spin_delta;
    }

    EnergyT energy() const noexcept {
        return m_energy;
    }

    int64_t state() const noexcept {
        return m_state;
    }

    template<typename F>
    void markov_chain_monte_carlo(F&& callback) {
        auto const k_sweep_limit = 10000;
        auto const k_spin_size = m_spins.size();

        for (int sweep = 0; sweep < k_sweep_limit; ++sweep) {
            for (int i = 0; i < k_spin_size; ++i) {
                auto const spin = randnum(0, k_spin_size);
                auto const delta = this->delta(spin);
                if (std::exp(-beta * delta) > randnum(0.0, 1.0)) {
                    this->flip(spin);
                }
            }
            callback(*this);
        }
    }

private:
    std::vector<SpinT> const m_spins;
    std::vector<FieldT> const m_fields;
    std::vector<std::vector<std::pair<NodeT, EnergyT>>> const m_neighbors;
    EnergyT m_energy;
    int64_t m_state;
};

template<typename FieldT>
std::vector<std::pair<node_t, FieldT>> read_spin_file(std::string_view spin_file) {
    std::ifstream ifs(spin_file);
    if (!ifs) {
        throw spin_file;
    }

    node_t node;
    FieldT field;
    std::vector<std::pair<node_t, FieldT>> result{};

    while (ifs.good()) {
        try {
            ifs >> node >> field;
        }
        catch (...) {
            throw;
        }
        result.emplace_back(node, field);
    }

    return result;
}

template<typename EnergyT>
std::vector<std::tuple<node_t, node_t, EnergyT>> read_bond_file(std::string_view bond_file) {
    std::ifstream ifs(bond_file);
    if (!ifs) {
        throw bond_file;
    }

    node_t n1, n2;
    EnergyT energy;
    std::vector<std::tuple<node_t, node_t, EnergyT>> result{};

    while (ifs.good()) {
        try {
            ifs >> n1 >> n2 >> energy;
        }
        catch (...) {
            throw;
        }
        result.emplace_back(n1, n2, energy);
    }
    
    return result;
}

template<typename SpinT, typename EnergyT, typename FieldT>
BasicIsing<SpinT, EnergyT, FieldT> make_basic_ising(std::string_view spin_file, std::string_view bond_file) try {
    auto const spins = read_spin_file(spin_file);
    auto const bonds = read_bond_file(bond_file);
    return { spins, bonds };
}
catch (std::string_view filename) {
    std::cerr << "Error opening file " << filename << '\n';
    std::exit(1);
}
catch (std::exception const& e) {
    std::cerr << e.what() << '\n';
    std::exit(1);
}

using Ising = BasicIsing<spin_t, energy_t, field_t>;

auto const make_ising = make_basic_ising<spin_t, energy_t, field_t>;