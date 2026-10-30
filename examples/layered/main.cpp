// Upper layer: the application. It only knows inventory, yet can report storage errors.

#include "inventory.hpp"

#include <print>
#include <string_view>

namespace {

void place_order(std::string_view product, int units)
{
    const auto reserved = inventory::reserve(product, units);
    if (reserved) {
        std::println("{} x{}: reserved", product, units);
        return;
    }

    // A known error: handle it.
    if (reserved.error() == inventory::Code::OutOfStock) {
        std::println("{} x{}: back-ordered ({})", product, units, reserved.error().context());
        return;
    }

    // Any other error, from any layer: log it.
    std::println("{} x{}: failed: {}", product, units, reserved.error().to_string());
}

} // namespace

auto main() -> int
{
    place_order("apple", 2);
    place_order("pear", 5);
    place_order("kiwi", 1);
    place_order("melon", 1);
}
