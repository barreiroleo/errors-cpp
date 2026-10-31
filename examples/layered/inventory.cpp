#include "inventory.hpp"

#include "storage.hpp"

#include <errors/error.hpp>
#include <format>
#include <string>
#include <string_view>

namespace inventory {

auto reserve(std::string_view product, int units) -> err::Result<void, std::string>
{
    const auto stock = storage::query_stock(product);
    if (!stock) {
        const auto& error = stock.error();

        // An error this layer can explain better: translate it to its own domain.
        if (error == storage::Code::RowNotFound) {
            return err::fail(Code::NotFound, std::string { product });
        }

        // Any other error: propagate it, flattening the context to this layer's std::string.
        return err::fail(error.code(), error.ctx().flat());
    }

    if (*stock < units) {
        return err::fail(Code::OutOfStock,
                         std::format("{} requested, {} available", units, *stock));
    }
    return { };
}

} // namespace inventory
