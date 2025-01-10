#pragma once
#include <cstdint>

namespace work_contracts{
    using signal_index = uint64_t;

    static signal_index constexpr invalid_signal_index(~0ull);
};