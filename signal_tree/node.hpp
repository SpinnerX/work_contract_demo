#pragma once
#include <signal_tree/helper.hpp>
#include <signal_tree/signal_index.hpp>
#include <atomic>
#include <array>
#include <utility>

namespace work_contracts{
    static thread_local uint64_t select_bias_hint = 0;

    // template<size_t N1, size_t N2> requires(is_power_of_two(N1))
    template<size_t N1, size_t N2>
    struct node_traits{
        static auto constexpr tree_capacity = N2;
        static auto constexpr capacity = N1;
        static auto constexpr root_node = (tree_capacity == capacity);
        static auto constexpr number_of_counters = sub_counter_arity_v<capacity>;
        static auto constexpr counter_capacity = capacity / number_of_counters;
        static auto constexpr bits_per_counter = minimum_bit_count(counter_capacity);
    };

    template <typename T> concept node_traits_concept = std::is_same_v<T, node_traits<T::capacity, T::tree_capacity>>;
    template <typename T> concept leaf_node_traits = ((node_traits_concept<T>) && (T::capacity == 64));
    template <typename T> concept non_leaf_node_traits = ((node_traits_concept<T>) && (!leaf_node_traits<T>));
    template <typename T> concept root_node_traits = ((node_traits_concept<T>) && (T::root_node));

    //! @note non-leaf nodes
    //! @note node is a 64-bit integer which represents two (or more) counters
    template<node_traits_concept T>
    class alignas(16) node final {
    public:
        static auto constexpr capacity = T::capacity;
        static auto constexpr tree_capacity = T::tree_capacity;
        static auto constexpr number_of_counters = T::number_of_counters;
        static auto constexpr counter_capacity = T::counter_capacity;
        static auto constexpr bits_per_counter = T::bits_per_counter;
        static auto constexpr counter_mask = (1ull << bits_per_counter) - 1;

        using child_type = node<node_traits<capacity / number_of_counters, tree_capacity>>;
        using bias_flags = std::uint64_t;
        using value_type = std::uint64_t;

        std::pair<bool, bool> set(uint64_t) noexcept;

        bool empty() const noexcept { return (value_ == 0); }

        template<template<uint64_t, uint64_t> class>
        std::pair<signal_index, bool> select(bias_flags) noexcept;

    protected:
        std::atomic<value_type> value_{0};

        // static std::array<uint64_t, number_of_counters> constexpr addened_{
        //     [](size_t ... N)(std::index_sequence<N...>) -> std::array<uint64_t, number_of_counters>{

        //     }
        // };
        static std::array<std::uint64_t, number_of_counters> constexpr addend_{
            []<std::size_t ... N>(std::index_sequence<N ...>) -> std::array<std::uint64_t, number_of_counters>{
                return {(1ull << ((number_of_counters - N - 1) * bits_per_counter)) ...};
            }(std::make_index_sequence<number_of_counters>())
        };
    };
}; // end of signal_tree namespace

template<work_contracts::node_traits_concept T>
inline std::pair<bool, bool> work_contracts::node<T>::set(uint64_t signal_index) noexcept {
    static auto constexpr set_successful = true;
    auto counter_index = signal_index / counter_capacity;

    if constexpr (root_node_traits<T>){
        if constexpr (non_leaf_node_traits<T>){
            return {(value_.fetch_add(addend_[counter_index]) == 0ull), set_successful};
        }
        else{
            auto bit = 0x8000000000000000ull >> counter_index;
            auto prev = value_.fetch_or(bit);
            return {prev == 0, (prev & bit) == 0ull};
        }
    }
    else{
        // not a root node
        if constexpr (non_leaf_node_traits<T>){
            // non-leaf node.  increment correct sub counter
            value_.fetch_add(addend_[counter_index]);
            return {true, set_successful};
        }
        else{
            // leaf node. counters are 1 bit in size
            // set correct counter bit and return true if not already set
            auto bit = 0x8000000000000000ull >> counter_index;
            return {false, (value_.fetch_or(bit) & bit) == 0ull}; 
        }
    }
}

template<work_contracts::node_traits_concept T>
template<template <uint64_t, uint64_t> class selector>
inline std::pair<work_contracts::signal_index, bool> work_contracts::node<T>::select(bias_flags p_BiasFlags) noexcept{
    auto expected = value_.load();
    while (expected){
        auto counterIndex = selector<number_of_counters, bits_per_counter>()(p_BiasFlags, expected);
        if constexpr (non_leaf_node_traits<T>){
            auto desired = expected - addend_[counterIndex];
            if (value_.compare_exchange_strong(expected, desired))
                return {counterIndex, (desired == 0)};
        }
        else{
            auto bit = 0x8000000000000000ull >> counterIndex;
            if (expected = value_.fetch_and(~bit); ((expected & bit) == bit))
                return {counterIndex, (expected == bit)};
        }
    }
    return {invalid_signal_index, false}; 
}