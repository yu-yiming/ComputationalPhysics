#pragma once
#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <stdexcept>
#include <vector>

#include "typedef.hpp"

extern double beta;

template<typename T>
T rand_in(T left, T right) {
    using distribution_t = std::conditional_t<std::is_integral_v<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;

    std::random_device rd{};
    std::default_random_engine eng(rd());
    distribution_t d(left, right);

    return d(eng);
}

template<typename SpinT, typename EnergyT, typename FieldT>
class BasicIsing {
public:
    using NodeT = int;
    using SpinValue = SpinTrait<SpinT>;

    BasicIsing(std::vector<std::pair<NodeT, FieldT>> const& spins, std::vector<std::tuple<NodeT, NodeT, EnergyT>> const& bonds)
        : m_spins(spins.size()), m_fields(spins.size()), m_neighbors(spins.size()), m_energy(0.0) {

        for (auto& spin : m_spins) {
            spin = random_spin<SpinT>();
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

    void filp(NodeT n, SpinT new_spin) noexcept {
        auto const delta = this->delta(n, new_spin);
        m_spins[n] = new_spin;
        m_energy += delta;
    }

    EnergyT energy() const noexcept {
        return m_energy;
    }

    template<typename F>
    void markov_chain_monte_carlo(F&& callback) {
        auto const k_sweep_limit = 10000;
        auto const k_spin_size = m_spins.size();
        for (int sweep = 0; sweep < k_sweep_limit; ++sweep) {
            for (int i = 0; i < k_spin_size; ++i) {
                auto const spin = rand_in(0, k_spin_size);
                auto const delta = this->delta(spin);
                if (std::exp(-beta * delta) > rand_in(0.0, 1.0)) {
                    this->flip(spin);
                }
            }
            callback(*this);
        }
    }

private:
    std::vector<SpinT> m_spins;
    std::vector<FieldT> m_fields;
    std::vector<std::vector<std::pair<NodeT, EnergyT>>> m_neighbors;
    EnergyT m_energy;
};

template<typename SpinT, typename EnergyT, typename FieldT>
BasicIsing<SpinT, EnergyT, FieldT> make_basic_ising(std::string_view spin_file, std::string_view bond_file) {
    std::ifstream ifs(spin_file);
    if (!ifs) {
        throw spin_file;
    }

    while (ifs.good()) {
        
    }
}

using Ising = BasicIsing<spin_t, energy_t, field_t>;

auto const make_ising = make_basic_ising<spin_t, energy_t, field_t>;
