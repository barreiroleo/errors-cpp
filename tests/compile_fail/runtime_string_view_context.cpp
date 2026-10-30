#include "../inventory_errors.hpp"

#include <errors/error.hpp>
#include <string>
#include <string_view>

auto check(const std::string& text) -> err::Result<void>
{
    return err::fail(inventory::Code::NotFound, std::string_view { text });
}
