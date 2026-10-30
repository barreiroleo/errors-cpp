#include "storage.hpp"

#include <errors/error.hpp>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>

namespace storage {

auto query_stock(std::string_view product) -> err::Result<int, QueryCtx>
{
    static const auto stock = std::map<std::string, int, std::less<>> {
        { "apple", 10 },
        { "pear",  1  },
    };

    auto ctx = QueryCtx { .table = "stock", .key = std::string { product } };

    // Simulates a slow query.
    if (product == "melon") {
        return err::fail(Code::Timeout, std::move(ctx));
    }

    const auto row = stock.find(product);
    if (row == stock.end()) {
        return err::fail(Code::RowNotFound, std::move(ctx));
    }
    return row->second;
}

} // namespace storage
