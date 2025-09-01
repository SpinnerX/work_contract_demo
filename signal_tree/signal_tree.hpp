#pragma once
#include <cstdint>
#include <signal_tree/level.hpp>
#include <signal_tree/signal_index.hpp>

namespace work_contracts{
    template<uint64_t total_counters, uint64_t bits_per_counter, uint64_t bias_bit = (1ull << 64)>
    struct default_selector{
        inline auto operator()(uint64_t biasFlags, uint64_t counters, uint64_t nextBias = 0){
            if constexpr (total_counters == 1){
                select_bias_hint |= nextBias;
                return 0;
            }
            else{
                static auto constexpr counters_per_half = (total_counters / 2);
                static auto constexpr bits_per_half = (counters_per_half * bits_per_counter);
                static auto constexpr right_bit_mask = ((1ull << bits_per_half) - 1);
                static auto constexpr left_bit_mask = (right_bit_mask << bits_per_half);

                auto const rightCounters = (counters & right_bit_mask);
                auto const leftCounters = (counters & left_bit_mask);
                auto const biasRight = (biasFlags & bias_bit);
                auto chooseRight = ((biasRight && rightCounters) || (leftCounters == 0ull));
                nextBias <<= 1;
                nextBias |= (rightCounters != 0); 
                counters >>= (chooseRight) ? 0 : bits_per_half;
                return ((chooseRight) ? counters_per_half : 0) + default_selector<counters_per_half, bits_per_counter, bias_bit / 2>()(biasFlags, counters & right_bit_mask, nextBias);
            }
        }
    };

    //=====================================================================
    // TODO: hack.  write better when have time
    template<uint64_t N = 64>
    static uint64_t consteval select_tree_size(uint64_t requested){
        constexpr std::array<std::uint64_t, 16> valid{
            64,             // 2^6
            64 << 3,        // 2^9
            64 << 5,        // 2^11
            64 << 7,        // 2^13
            64 << 9,        // 2^15
            64 << 11,       // 2^17
            64 << 12,       // 2^18
            64 << 13,       // 2^19
            64 << 14,       // 2^20
            64 << 15,       // 2^21
            64 << 16,       // 2^22
            64 << 17,       // 2^23
            64 << 18,       // 2^24
            64 << 19,       // 2^25
            64 << 20,       // 2^26
            64 << 21        // 2^27
            };
        for (auto v : valid){
            if (v >= requested){
                return v;
            }
        }

        return (0x800000000000ull >> std::countl_zero(requested));
    }





    //=============================================================================

    template<uint64_t N>
    class tree final {
    private:
        using root_level_traits = level_traits<1, N>;
        using root_level = level<root_level_traits>;
    public:
        static auto constexpr capacity = N;
        static_assert(select_tree_size(capacity) == capacity, "invalid signal_tree capacity");

        std::pair<bool, bool> set(signal_index) noexcept;

        bool empty() const noexcept;

        template<template<uint64_t, uint64_t> class = default_selector>
        std::pair<signal_index, bool> select(uint64_t) noexcept;

    private:
        root_level rootLevel_;
    };
    // end of class tree
}; // end of signal_tree namespace

template<size_t N>
using signal_tree = work_contracts::tree<work_contracts::select_tree_size(N)>;

template<size_t N>
inline std::pair<bool, bool> work_contracts::tree<N>::set(signal_index signalIndex) noexcept{
    return rootLevel_.set(signalIndex);
}

//=============================================================================
// returns true if no leaf nodes are 'set' (root count is zero)
// returns false otherwise
template <std::size_t N>
inline bool work_contracts::tree<N>::empty() const noexcept {
    return rootLevel_.empty();
}


//=============================================================================
// select and return the index of a leaf which is 'set'
// return invalid_signal_index if no leaf is 'set' (empty tree)
template <std::size_t N>
template <template <std::uint64_t, std::uint64_t> class>
inline std::pair<work_contracts::signal_index, bool> work_contracts::tree<N>::select(uint64_t bias) noexcept{
    static int constexpr number_of_bias_bits = (65 - minimum_bit_count(capacity));
    bias = bias << number_of_bias_bits;
    return rootLevel_. template select<work_contracts::default_selector>(bias);
}