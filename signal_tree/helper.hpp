#pragma once
// #include <bit>
#include <signal_tree/bit.hpp>
#include <cstdint>

namespace work_contracts{
    template<size_t, size_t>
    struct sub_counter_arity;

    template<size_t counter_capacity>
    struct sub_counter_arity<counter_capacity, 0>{
        static auto constexpr value = 0;
    };

    template<size_t counter_capacity, size_t N>
    struct sub_counter_arity{
        static auto constexpr bits_per_counter = (64ull - std::countl_zero(counter_capacity / N));
        static auto constexpr bits_required = (bits_per_counter * N);
        static auto constexpr value = (bits_required <= 64) ? N : 
                sub_counter_arity<counter_capacity, N / 2>::value;
    };

    template<size_t counter_capacity>
    static auto constexpr sub_counter_arity_v = sub_counter_arity<counter_capacity, 64>::value;

    // inline bool is_power_of_two(size_t value) {
    //     if(value <= 0){
    //         return false;
    //     }

    //     return (value & (value - 1)) == 0;
    // }

    // static int constexpr minimum_bit_count(size_t counter_capacity){
    //     return ceil(log2(counter_capacity + 1));
    // }

    // static bool 

    // these will be type rich types in near future (from actual speaker)
    using tree_index = uint64_t;
};