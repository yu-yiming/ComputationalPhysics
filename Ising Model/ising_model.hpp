#pragma once
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <tuple>
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

extern energy_t g_bond_energy;

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
    using This = BasicIsing;

    struct Empty {
        auto operator ()(BasicIsing<SpinT, EnergyT, FieldT> const& self) const noexcept {
            // do nothing
        }
        auto operator ()() const {
            // do nothing
        }
    };

    struct EnergyRecorder {
        auto operator ()(BasicIsing<SpinT, EnergyT, FieldT> const& self) const {
            m_energies.push_back(self.m_energy);
        }
        auto operator ()() const {
            auto result = std::move(m_energies);
            m_energies.clear();
            return result;
        }
    private:
        mutable std::vector<EnergyT> m_energies;
    };

    struct StateRecorder {
        auto operator ()(BasicIsing<SpinT, EnergyT, FieldT> const& self) const {
            m_states.push_back(self.m_state);
        }
        auto operator ()() const {
            auto result = std::move(m_states);
            m_states.clear();
            return result;
        }
    private:
        mutable std::vector<int64_t> m_states;
    };

    struct MagnetizationRecorder {
        using MagnetizationT = double;
        auto operator ()(BasicIsing<SpinT, EnergyT, FieldT> const& self) const {
            m_magnetizations.push_back(self.magnetization());
        }
        auto operator ()() const {
            auto result = std::move(m_magnetizations);
            m_magnetizations.clear();
            return result;
        }

    private:
        mutable std::vector<double> m_magnetizations;
    };

    template<class... Rs>
    struct Recorder : Rs... {
        auto operator ()(BasicIsing<SpinT, EnergyT, FieldT> const& self) const {
            (Rs::operator ()(self), ...);
        }
        auto operator ()() const {
            auto const tup = (Builder(ctie(Rs::operator ()())), ...).tuple();
            auto result = transpose_t<decltype(tup)>(std::get<0>(tup).size());
            std::apply(
                [&result](auto const&... vs) {
                    for (auto i = 0; i < result.size(); ++i) {
                        result[i] = ith(i, vs...);
                    }
                }, tup);
            return result;
        }
    };

    static inline auto const pass = Empty{};
    static inline auto const record_state = StateRecorder{};
    static inline auto const record_energy = EnergyRecorder{};
    static inline auto const record_magnetization = MagnetizationRecorder{};
    template<class... Rs>
    static inline auto const record = Recorder<Rs...>{};

    // TODO
    // Combine recorders.
    //template<class... Rs>
    //static auto make_recorder(Rs&&... rs) {
    //    using R = rebind_t<
    //        Recorder, 
    //        filter_t<
    //            intersect_t, std::tuple<Rs...>, std::tuple<StateRecorder, EnergyRecorder, MagnetizationRecorder>>>;
    //    return R{};
    //}

    static This from_grid(node_t ct, EnergyT bond_energy = 0.0) {
        return from_grid(ct, ct, bond_energy);
    }

    /**
     * @brief Get a simple lattice model
     * @param row_ct 
     * @param col_ct 
     * @param bond_energy 
     * @return 
    */
    static This from_grid(node_t row_ct, node_t col_ct, EnergyT bond_energy = 0.0) {
        auto const total_ct = row_ct * col_ct;
        std::vector<std::pair<node_t, FieldT>> spins(total_ct);
        for (node_t i{}; auto& [n, f] : spins) {
            n = ++i;
            f = FieldT{};
        }

        std::vector<std::tuple<node_t, node_t, EnergyT>> bonds{};

        auto const right = [col_ct](node_t n) {
            auto const result = n + 1;
            if (result % col_ct == 0) {
                return -1;
            }
            return result;
        };
        auto const down = [col_ct, total_ct](node_t n) {
            auto const result = n + col_ct;
            if (result >= total_ct) {
                return -1;
            }
            return result;
        };

        for (node_t i = 0; i < total_ct; ++i) {
            auto const r = right(i);
            if (r >= 0) {
                bonds.emplace_back(i + 1, r + 1, bond_energy);
            }
            auto const d = down(i);
            if (d >= 0) {
                bonds.emplace_back(i + 1, d + 1, bond_energy);
            }
        }

        return { spins, bonds };
    }

    BasicIsing() noexcept
        : m_valid(false) {}

    BasicIsing(This const& other) = delete;

    BasicIsing(This&& other) = default;

    This& operator =(This const& other) = delete;

    This& operator =(This&& other) = default;

    BasicIsing(std::vector<std::pair<node_t, FieldT>> const& spins, std::vector<std::tuple<node_t, node_t, EnergyT>> const& bonds)
        : m_spins(spins.size()), m_fields(spins.size()), m_neighbors(spins.size()), m_energy(0.0), m_state(0), m_valid(true) {

        this->initialize(spins, bonds);
    }

    /**
     * @brief Initialize the model from vectors of config data.
     * Note that the node are specified by 1-indexed integers in accordance with the config files.
     * @param spins The field information of the model.
     * @param bonds The bond information
    */
    void initialize(std::vector<std::pair<node_t, FieldT>> spins, std::vector<std::tuple<node_t, node_t, EnergyT>> const& bonds) {
        auto const base = STraits::state_count();
        
        // make sure the spins are sorted by node number.
        std::sort(
            spins.begin(), spins.end(), 
            [](auto const& p1, auto const& p2) {
                return p1.first > p2.first ? false : p1.first < p2.first || p1.second < p2.second;
            });

        // the count of spins is the largest among their numbers.
        auto const spin_count = std::accumulate(
            spins.cbegin(), spins.cend(), node_t{},
            [](auto max, auto const& pair) { return std::max(max, pair.first); });

        m_spins.resize(spin_count);
        m_fields.resize(spin_count);
        m_neighbors.resize(spin_count);

        // initialize the spins with random direction.
        for (auto& spin : m_spins) {
            spin = random_spin<SpinT>();
            m_state *= base;
            m_state += STraits::index(spin);
            m_sum += STraits::value_of(spin);
        }
        // initialize fields of the spins.
        for (auto [i, h] : spins) {
            --i;
            m_fields[i] = h;
            m_energy += STraits::value_of(m_spins[i]) * h;
        }
        // initialize bonds between the spins.
        for (auto [i, j, e] : bonds) {
            --i; --j;
            m_neighbors[i].emplace_back(j, e);
            m_neighbors[j].emplace_back(i, e);
            m_energy -= STraits::value_of(m_spins[i]) * STraits::value_of(m_spins[j]) * e;
        }
        m_valid = true;
    }

    bool valid() const noexcept {
        return m_valid;
    }

    /**
     * @brief Return the change of energy if certain spin is flipped.
     * Note that this might be illegal for some spin types.
     * @param n The node represented by a 0-indexed integer.
     * @return The energy difference.
     */
    EnergyT delta(node_t n) noexcept {
        return delta(n, STraits::from_value(-STraits::value_of(m_spins[n])));
    }

    /**
     * @brief Return the change of energy if certain spin is changed to another direction.
     * This is a result-cached function so that consecutive calls with same inputs are trivial.
     * @param n The node represented by a 0-indexed integer.
     * @param new_spin The new spin.
     * @return The energy difference.
     */
    EnergyT delta(node_t n, SpinT new_spin) noexcept {
        static auto cache = std::tuple(node_t{ -1 }, SpinT{}, EnergyT{});
        auto& [cached_node, cached_spin, cached_energy] = cache;

        if (cached_node == n && cached_spin == new_spin) {
            return cached_energy;
        }
        cached_node = n;
        cached_spin = new_spin;

        auto const spin_delta = STraits::value_of(new_spin) - STraits::value_of(m_spins[n]);
        auto delta = m_fields[n] * spin_delta;
        for (auto [i, e] : m_neighbors[n]) {
            delta += STraits::value_of(m_spins[i]) * e * spin_delta;
        }
        cached_energy = delta;
        return delta;
    }

    void flip(node_t n) {
        this->flip(n, STraits::from_value(-STraits::value_of(m_spins[n])));
    }

    void flip(node_t n, SpinT new_spin) {
        auto const delta = this->delta(n, new_spin);
        auto const spin_delta = STraits::value_of(new_spin) - STraits::value_of(m_spins[n]);
        auto const base = STraits::state_count();

        m_spins[n] = new_spin;
        m_energy += delta;
        m_state += static_cast<int64_t>(std::pow(base, m_spins.size() - n - 1) * spin_delta);
        m_sum += spin_delta;
    }

    EnergyT energy() const noexcept {
        return m_energy;
    }

    int64_t state() const noexcept {
        return m_state;
    }

    double magnetization() const noexcept {
        return m_sum / m_spins.size();
    }

    /**
     * @brief Stablize the system by performing MCMC several times first.
     */
    void stablize() {
        auto const k_stable_sweep_ct = 10;
        this->markov_chain_monte_carlo(pass, k_stable_sweep_ct);
    }

    /**
     * @brief Perform the Metropolis-Hastings algorithm which uses Markov chain Monte Carlo (MCMC) method.
     * It basically choose a random spin and flip at some chance during one of multiple sweeps.
     * @tparam F A callback type.
     * @param callback Moniter the model object and do something every sweep, e.g. record the energy of the system.
     * @param sweep_limit The count of sweeps.
    */
    template<typename F>
    void markov_chain_monte_carlo(F&& callback, int sweep_limit = 1000) {
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
        os << "--------------------------------------------------------------" << '\n'
           << "                            Spins                             " << '\n'
           << "--------------------------------------------------------------" << '\n';
        for (auto spin : ising.m_spins) {
            os << ++ct << " : " << std::setw(8) << std::left << STraits::name_of(spin);
            if (ct % 5 == 0) {
                os << '\n';
            }
        }
        os << '\n';
        ct = 0;
        os << "--------------------------------------------------------------" << '\n'
           << "                            Fields                            " << '\n'
           << "--------------------------------------------------------------" << '\n';
        for (auto f : ising.m_fields) {
            os << ++ct << " : " << std::setw(8) << std::left << f;
            if (ct % 5 == 0) {
                os << '\n';
            }
        }
        os << '\n';
        ct = 0;
        os << "--------------------------------------------------------------" << '\n'
           << "                            Bonds                             " << '\n'
           << "--------------------------------------------------------------" << '\n';
        std::vector<std::tuple<node_t, node_t, EnergyT>> bonds;
        node_t i{};
        auto const form_bonds = [&i](auto&& v) {
            ++i;
            return v | stdv::transform([i = i - 1](auto&& pair) {
                auto const [min, max] = std::minmax(i, pair.first);
                return std::tuple(min, max, pair.second);
            });
        };
        auto view = ising.m_neighbors | stdv::transform(form_bonds)
                                      | stdv::join;
        node_t last_i = -1, last_j = -1;
        for (auto [i, j, e] : view) {
            if (i != last_i || j != last_j) {
                os << "(" << std::setw(2) << std::left << i << ", "
                          << std::setw(2) << std::right << j << ") : " 
                          << std::setw(6) << std::left << e;
            }
            if (++ct % 4 == 0) {
                os << '\n';
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
    double m_sum;
    bool m_valid;
};

template<typename FieldT>
std::vector<std::pair<node_t, FieldT>> read_spin_file(std::string_view spin_file) {
    // We need more support for std::string_view !!!!!!!
    std::ifstream ifs{};
    // std::string_view is dangerous since it's not null-terminated!
    ifs.open(std::string(spin_file));
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
    // We need more support for std::string_view !!!!!!!
    std::ifstream ifs{};
    ifs.open(std::string(bond_file));
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