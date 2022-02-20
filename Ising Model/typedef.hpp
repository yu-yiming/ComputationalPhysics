#pragma once

enum struct spin_t {
    k_up, k_down, k_invalid
};

template<typename SpinT>
struct SpinTrait {
    static constexpr double values[] = { 0.0 };
    static constexpr double of(SpinT spin) {
        return 0.0;
    }
    static constexpr SpinT from(double val) {
        if (val == 0.0) {
            return SpinT{};
        }
        throw std::invalid_argument("Spin value doesn't match any valid state.");
    }
};

template<>
struct SpinTrait<spin_t> {
    static constexpr double values[] = { 1.0, -1.0 };
    static constexpr spin_t invalid_state() {
        return spin_t::k_invalid;
    }
    static constexpr double of(spin_t spin) {
        return spin == spin_t::k_up ? 1.0 : spin == spin_t::k_down ? -1.0 : 0.0;
    }
    static constexpr spin_t from(double val) {
        if (val == 1.0) {
            return spin_t::k_up;
        }
        else if (val == -1.0) {
            return spin_t::k_down;
        }
        throw std::invalid_argument("Spin value doesn't match any valid state.");
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
    auto const& arr = SpinPermutation<SpinT>::values;
    std::random_shuffle(&arr[0], &arr[sizeof(arr) / sizeof(SpinT)]);
    return arr[0];
}


using energy_t = double;
using field_t = double;