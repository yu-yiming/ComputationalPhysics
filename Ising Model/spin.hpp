#pragma once
#include <cstdint>
#include <stdexcept>

#include "utility.hpp"

enum struct spin_t : int8_t {
    k_up, k_down, k_invalid
};

template<typename SpinT>
struct SpinTraits {
    static constexpr double values[] = { 0.0 };
    static constexpr char const* name[] = { "empty" };

    static constexpr std::size_t state_count() noexcept {
        return 1;
    }
    static constexpr SpinT invalid_state() noexcept {
        return SpinT{};
    }
    static constexpr double value_of(SpinT spin) noexcept {
        return 0.0;
    }
    static constexpr char const* name_of(SpinT spin) noexcept {
        return "empty";
    }
    static constexpr SpinT from_value(double val) {
        if (val == 0.0) {
            return SpinT{};
        }
        throw std::invalid_argument("Spin value doesn't match any valid state.");
    }
    static constexpr int index(SpinT spin) {
        return 0;
    }
};

template<>
struct SpinTraits<spin_t> {
    using enum spin_t;

    static constexpr double values[] = { 1.0, -1.0 };
    static constexpr char const* name[] = { "up", "down", "error" };

    static constexpr std::size_t state_count() noexcept {
        return 2;
    }
    static constexpr spin_t invalid_state() noexcept {
        return k_invalid;
    }
    static constexpr double value_of(spin_t spin) {
        return spin == k_up ? 1.0 : spin == k_down ? -1.0 : 0.0;
    }
    static constexpr char const* name_of(spin_t spin) noexcept {
        return name[index(spin)];
    }
    static constexpr spin_t from_value(double val) {
        if (val == 1.0) {
            return k_up;
        }
        else if (val == -1.0) {
            return k_down;
        }
        throw std::invalid_argument("Spin value doesn't match any valid state.");
    }
    static constexpr int index(spin_t spin) {
        return spin == k_up ? 0 : spin == k_down ? 1 : 2;
    }
};

template<typename SpinT>
struct SpinPermutation {
    static inline SpinT values[] = { 0 };
};

template<>
struct SpinPermutation<spin_t> {
    static inline spin_t values[] = { spin_t::k_up, spin_t::k_down };
};

template<typename SpinT>
SpinT random_spin() {
    using STraits = SpinTraits<SpinT>;
    auto const index = randnum(0, STraits::state_count());
    return STraits::from_value(STraits::values[index]);
}

using node_t = int;
using energy_t = double;
using field_t = double;