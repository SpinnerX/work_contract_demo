#pragma once
#include <bit>
#include <cstdint>
#include <concepts>

namespace work_contracts{
    static auto constexpr bits_per_byte = 8ull;


    //=========================================================================
    static inline constexpr auto minimum_bit_count
    (
        std::unsigned_integral auto value
    )
    {
        return ((sizeof(value) * bits_per_byte) - std::countl_zero(value));
    }


    //=========================================================================
    static inline constexpr auto minimum_power_of_two
    (
        std::unsigned_integral auto value
    )
    {
        return (1ull << minimum_bit_count(value - 1));
    }


    //=========================================================================
    static inline consteval auto is_power_of_two
    (
        std::unsigned_integral auto value
    )
    {
        return (std::popcount(value) == 1);
    }
};