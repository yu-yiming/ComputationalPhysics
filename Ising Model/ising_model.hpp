#pragma once
#include <algorithm>
#include <cmath>
#include <fstream>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "spin.hpp"
#include "utility.hpp"

namespace stdv = std::ranges::views;
namespace stdr = std::ranges;

/**
 * @brief A constant related with T, the temperature value_of the system.
 * g_beta = 1/(kT), where k is the Boltzmann's constant. We'd prefer to let k = 1 (in some proper unit)
 */
extern double g_beta;

/**
 * @brief A basic Ising model that has a graph structure value_of "weighed" vertices and edges.
 * 
 * @tparam SpinT Enumeration type value_of spin; see spin.hpp.
 * @tparam EnergyT Energy type; usually double.
 * @tparam FieldT Field type; usually double.
 */
template<typename SpinT, typename EnergyT, typename FieldT>
class BasicIsing {
public:
    using STraits = SpinTraits<SpinT>;

    BasicIsing() noexcept
        : m_valid(false) {}

    BasicIsing(std::vector<std::pair<node_t, FieldT>> const& spins, std::vector<std::tuple<node_t, node_t, EnergyT>> const& bonds)
        : m_spins(spins.size()), m_fields(spins.size()), m_neighbors(spins.size()), m_energy(0.0), m_state(0), m_valid(true) {

        this->initialize(spins, bonds);
    }

    void initialize(std::vector<std::pair<node_t, FieldT>> const& spins, std::vector<std::tuple<node_t, node_t, EnergyT>> const& bonds) {
        auto const base = STraits::state_count();

        auto const spin_count = spins.size();
        m_spins.resize(spin_count);
        m_fields.resize(spin_count);
        m_neighbors.resize(spin_count);

        for (auto& spin : m_spins) {
            spin = random_spin<SpinT>();
            m_state *= base;
            m_state += STraits::index(spin);
        }
        for (auto [i, h] : spins) {
            m_fields[i] = h;
            m_energy += STraits::value_of(m_spins[i]) * h;
        }
        for (auto [i, j, e] : bonds) {
            m_neighbors[i].emplace_back(j, e);
            m_neighbors[j].emplace_back(i, e);
            m_energy -= STraits::value_of(m_spins[i]) * STraits::value_of(m_spins[j]) * e;
        }
        m_valid = true;
    }

    bool valid() const noexcept {
        return m_valid;
    }

    EnergyT delta(node_t n) {
        return delta(n, STraits::from_value(-STraits::value_of(m_spins[n])));
    }

    EnergyT delta(node_t n, SpinT new_spin) noexcept {
        static std::tuple<node_t, SpinT, EnergyT> cache;
        auto& [cached_node, cached_spin, cached_energy] = cache;

        if (cached_node == n && cached_spin == new_spin) {
            return cached_energy;
        }
        cached_node = n;
        cached_spin = new_spin;

        auto const spin_delta = STraits::value_of(new_spin) - STraits::value_of(m_spins[n]);
        auto delta = m_fields[n] * spin_delta;
        for (auto [i, e] : m_neighbors[n]) {
            delta += STraits::value_of(m_spins[i]) * e;
        }
        cached_energy = delta;
        return delta;
    }

    void flip(node_t n) {
        this->flip(n, STraits::from_value(-STraits::value_of(m_spins[n])));
    }

    void flip(node_t n, SpinT new_spin) {
        auto const delta = this->delta(n, new_spin);
        m_spins[n] = new_spin;
        m_energy += delta;

        auto const spin_delta = STraits::value_of(new_spin) - STraits::value_of(m_spins[n]);
        auto const base = STraits::state_count();
        m_state += static_cast<int64_t>(std::pow(base, m_spins.size() - n - 1)) * spin_delta;
    }

    EnergyT energy() const noexcept {
        return m_energy;
    }

    int64_t state() const noexcept {
        return m_state;
    }

    /**
     * @brief Stablize the system by performing MCMC several times first.
     */
    void stablize() {
        auto const k_stable_sweep_ct = 10;
        this->template markov_chain_monte_carlo([](auto&& self){}, k_stable_sweep_ct);
    }

    template<typename F>
    void markov_chain_monte_carlo(F&& callback, int sweep_limit = 10000) {
        auto const k_sweep_limit = sweep_limit;
        auto const k_spin_size = m_spins.size();

        for (int sweep = 0; sweep < k_sweep_limit; ++sweep) {
            for (int i = 0; i < k_spin_size; ++i) {
                auto const spin = randnum(0, k_spin_size);
                auto const delta = this->delta(spin);
                if (std::exp(-g_beta * delta) > randnum(0.0, 1.0)) {
                    this->flip(spin);
                }
            }
            callback(*this);
        }
    }

    template<typename SpinU, typename EnergyU, typename FieldU>
    friend std::ostream& operator <<(std::ostream& os, BasicIsing<SpinU, EnergyU, FieldU> const& ising) {
        using STraits = SpinTraits<SpinT>;
        int ct{};
        os << "----------------------------------------------" << '\n'
           << "                    Spins                     " << '\n'
           << "----------------------------------------------" << '\n';
        for (auto spin : ising.m_spins) {
            os << ++ct << " : " << std::setw(8) << std::left << STraits::name_of(spin);
            if (ct >= 16) {
                os << '\n';
                ct = 0;
            }
        }
        ct = 0;
        os << "----------------------------------------------" << '\n'
           << "                   Fields                     " << '\n'
           << "----------------------------------------------" << '\n';
        for (auto f : ising.m_fields) {
            os << ++ct << " : " << std::setw(8) << std::left << f;
            if (ct >= 32) {
                os << '\n';
                ct = 0;
            }
        }
        ct = 0;
        os << "----------------------------------------------" << '\n'
           << "                   Bonds                      " << '\n'
           << "----------------------------------------------" << '\n';
        std::vector<std::tuple<node_t, node_t, EnergyT>> bonds;
        node_t i{};
        auto const form_bonds = [&i](auto&& v) {
            ++i;
            return v | stdv::transform([i = i - 1](auto&& pair) {
                auto const min = std::min(i, pair.first);
                auto const max = std::max(i, pair.first);
                return std::tuple(min, max, pair.second);
            });
        };
        auto view = ising.m_neighbors | stdv::transform(form_bonds)
                                      | stdv::join;
        node_t last_i = -1, last_j = -1;
        for (auto [i, j, e] : view) {
            if (i != last_i || j != last_j) {
                os << "(" << i << ", " << j << ") : " << e;
            }
            if (ct >= 8) {
                os << '\n';
                ct = 0;
            }
        }

        return os;
    }

private:
    std::vector<SpinT> m_spins;
    std::vector<FieldT> m_fields;
    std::vector<std::vector<std::pair<node_t, EnergyT>>> m_neighbors;
    EnergyT m_energy;
    int64_t m_state;
    bool m_valid;
};

template<typename FieldT>
std::vector<std::pair<node_t, FieldT>> read_spin_file(std::string_view spin_file) {
    std::ifstream ifs(spin_file.data());
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
    std::ifstream ifs(bond_file.data());
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
std::string to_string(BasicIsing<SpinT, EnergyT, FieldT> const& model) {
    std::ostringstream oss{};
    oss << model;
    return oss.str();
}


template<typename SpinT, typename EnergyT, typename FieldT>
BasicIsing<SpinT, EnergyT, FieldT> make_basic_ising(std::string_view spin_file, std::string_view bond_file) try {
    auto const spins = read_spin_file<FieldT>(spin_file);
    auto const bonds = read_bond_file<FieldT>(bond_file);
    return { spins, bonds };
}
catch (std::string_view filename) {
    std::cerr << "Error opening file " << filename << '\n';
    std::exit(EXIT_FAILURE);
}
catch (std::exception const& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
}

using Ising = BasicIsing<spin_t, energy_t, field_t>;

auto const make_ising = make_basic_ising<spin_t, energy_t, field_t>;