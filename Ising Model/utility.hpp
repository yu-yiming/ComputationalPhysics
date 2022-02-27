#pragma once
#include <random>
#include <type_traits>

/**
 * @brief Generate a random number within a give range.
 * 
 * @tparam T Either a integral type or a floating piont type.
 * @param left The left bound of the range, inclusive.
 * @param right The right bound of the range, exclusive.
 * @return T A random number.
 */
template<typename T, typename U, typename R = std::common_type_t<T, U>>
R randnum(T left, U right) {
    using distribution_t = std::conditional_t<std::is_integral_v<R>, 
                                              std::uniform_int_distribution<R>, std::uniform_real_distribution<R>>;

    std::random_device rd{};
    std::default_random_engine eng(rd());
    distribution_t d(left, right);

    R result{};

    while (true) {
        result = d(eng);
        if (result != static_cast<R>(right)) {
            break;
        }
    }

    return result;
}