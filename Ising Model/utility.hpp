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

//template<typename T, typename U>
//struct cons;
//
//template<typename T, typename... Args>
//struct cons<T, std::tuple<Args...>> {
//    using type = std::tuple<T, Args...>;
//};
//
//template<typename T, typename U>
//using cons_t = typename cons<T, U>::type;
//
//template<typename T, typename U>
//struct in;
//
//template<typename T>
//struct in<T, std::tuple<>> {
//    using type = std::false_type;
//};
//
//template<typename T, typename... Tail>
//struct in<T, std::tuple<T, Tail...>> {
//    using type = std::true_type;
//};
//
//template<typename T, typename Head, typename... Tail>
//struct in<T, std::tuple<Head, Tail...>> {
//    using type = typename in<T, Tail...>::type;
//};
//
//template<typename T, typename U>
//using in_t = typename in<T, U>::type;
//
//template<typename... Args>
//struct intersect;
//
//template<typename... Args>
//struct intersect<std::tuple<>, std::tuple<Args...>> {
//    using type = std::false_type;
//};
//
//template<typename Head, typename... Tail, typename... Args>
//struct intersect<std::tuple<Head, Tail...>, std::tuple<Args...>> {
//    using type = std::disjunction<in_t<Head, std::tuple<Args...>>, typename intersect<std::tuple<Tail...>, std::tuple<Args...>>::type>;
//};
//
//template<typename... Args>
//using intersect_t = typename intersect<Args...>::type;
//
//template<template<class...> class Pred, typename T, typename... Args>
//struct filter;
//
//template<template<class...> class Pred, typename... Args>
//struct filter<Pred, std::tuple<>, Args...> {
//    using type = std::tuple<>;
//};
//
//template<template<class...> class Pred, typename Head, typename... Tail, typename... Args>
//struct filter<Pred, std::tuple<Head, Tail...>, Args...> {
//    using type = std::conditional_t<Pred<Head, Args...>,
//        cons_t<Head, typename filter<Pred, std::tuple<Tail...>>::type>,
//        typename filter<Pred, std::tuple<Tail...>>::type>;
//};
//
//template<template<class> class Pred, typename T, typename... Args>
//using filter_t = typename filter<Pred, T, Args...>::type;
//
//template<template<typename...> class C, typename D>
//struct rebind;
//
//template<template<typename...> class C, template<typename...> class D, typename... Args>
//struct rebind<C, D<Args...>> {
//    using type = C<Args...>;
//};
//
//template<template<typename...> class C, typename D>
//using rebind_t = typename rebind<C, D>::type;

template<typename T>
struct transpose {};

template <typename... Ts>
struct transpose<std::tuple<std::vector<Ts>...>> {
    using type = std::vector<std::tuple<Ts...>>;
};

template <typename... Ts>
struct transpose<std::vector<std::tuple<Ts...>>> {
    using type = std::tuple<std::vector<Ts>...>;
};

template<typename... Ts>
using transpose_t = typename transpose<Ts...>::type;

template<typename... Ts>
static inline std::tuple<Ts&...> ith(size_t i, std::vector<Ts>&... vs) {
    return std::tie(vs[i]...);
}

template<typename... Ts>
class Builder {
public:
    Builder(std::tuple<Ts const&...> const& args)
        : m_tuple(args) {}

    std::tuple<Ts const&...>&& tuple() const {
        return std::move(m_tuple);
    }

    template<typename T>
    auto operator ,(Builder const& other) {
        return Builder(std::tuple_cat(m_tuple, other.m_tuple));
    }

private:
    std::tuple<Ts const&...> m_tuple;
};

template<typename... Ts>
Builder(std::tuple<Ts&...> const&) -> Builder<Ts...>;

template<typename... T>
std::tuple<T const&...> ctie(T const&... args) {
    return std::tie(args...);
}
