#pragma once

// Lower layer: data access.

#include <cstdint>
#include <errors/domain.hpp>
#include <errors/error.hpp>
#include <format>
#include <string>
#include <string_view>

namespace storage {
enum class Code : std::uint8_t {
    RowNotFound = 1,
    Timeout,
};

inline constexpr auto errors = err::error_list<Code>({
    { .code = Code::RowNotFound, .msg = "Row not found"   },
    { .code = Code::Timeout,     .msg = "Query timed out" },
});
} // namespace storage

ERR_REGISTER_DOMAIN("storage", storage::errors);

namespace storage {

struct QueryCtx {
    std::string table;
    std::string key;

    [[nodiscard]] auto to_string() const -> std::string
    {
        return std::format("{}[{}]", table, key);
    }
};

auto query_stock(std::string_view product) -> err::Result<int, QueryCtx>;

} // namespace storage
