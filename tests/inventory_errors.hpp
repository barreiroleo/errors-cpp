#pragma once

#include <cstdint>
#include <errors/domain.hpp>

namespace inventory {
enum class Code : std::uint8_t {
    NotFound = 1,
    OutOfStock,
};

inline constexpr auto errors = err::error_list<Code>({
    { .code = Code::NotFound,   .msg = "Product not found" },
    { .code = Code::OutOfStock, .msg = "Not enough stock"  },
});
inline constexpr auto category = "inventory";
} // namespace inventory

template <>
inline constexpr auto err::registered_domain<inventory::Code> =
    err::Domain { .name = inventory::category, .entries = inventory::errors };
