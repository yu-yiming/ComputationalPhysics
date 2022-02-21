#pragma once

enum struct spin_t {
    k_up, k_down, k_invalid
};

template<typename SpinT>
struct SpinTrait {
    static constexpr double values[] = { 0.0 };
    static constexpr std::size_t state_count() noexcept {
        return 1;
    }
    static constexpr double of(SpinT spin) noexcept {
        return 0.0;
    }
    static constexpr SpinT from(double val) {
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
struct SpinTrait<spin_t> {
    using enum spin_t;

    static constexpr double values[] = { 1.0, -1.0 };
    static constexpr std::size_t state_count() noexcept {
        return 2;
    }
    static constexpr spin_t invalid_state() {
        return k_invalid;
    }
    static constexpr double of(spin_t spin) {
        return spin == k_up ? 1.0 : spin == k_down ? -1.0 : 0.0;
    }
    static constexpr spin_t from(double val) {
        if (val == 1.0) {
            return k_up;
        }
        else if (val == -1.0) {
            return k_down;
        }
        throw std::invalid_argument("Spin value doesn't match any valid state.");
    }
    static constexpr int index(spin_t spin) {
        return spin == k_up ? 1 : 0;
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

using node_t = int;
using energy_t = double;
using field_t = double;