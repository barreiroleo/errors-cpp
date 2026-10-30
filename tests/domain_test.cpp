#include "inventory_errors.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <errors/domain.hpp>
#include <string_view>

namespace {
enum class Unregistered : std::uint8_t {
    A = 1,
};

enum class Sample : std::uint8_t {
    Zero = 0,
    A,
    B,
};
constexpr auto entries(Sample first, Sample second) -> std::array<err::Entry<Sample>, 2>
{
    return std::to_array<err::Entry<Sample>>({
        { .code = first,  .msg = "" },
        { .code = second, .msg = "" },
    });
}

namespace shipping {
enum class Code : std::uint8_t {
    Delayed = 1,
};

constexpr auto errors = err::error_list<Code>({
    { .code = Code::Delayed, .msg = "Shipment delayed" },
});
} // namespace shipping
} // namespace

ERR_REGISTER_DOMAIN("shipping", shipping::errors);

static_assert(err::RegisteredCode<shipping::Code>);
static_assert(err::registered_domain<shipping::Code>.message(shipping::Code::Delayed) ==
              "Shipment delayed");

static_assert(!err::detail::has_zero_code(entries(Sample::A, Sample::B)));
static_assert(err::detail::has_zero_code(entries(Sample::Zero, Sample::A)));
static_assert(!err::detail::has_duplicate_codes(entries(Sample::A, Sample::B)));
static_assert(err::detail::has_duplicate_codes(entries(Sample::A, Sample::A)));

static_assert(err::RegisteredCode<inventory::Code>);
static_assert(!err::RegisteredCode<Unregistered>);
static_assert(!err::RegisteredCode<int>);

static_assert(err::registered_domain<inventory::Code>.message(inventory::Code::OutOfStock) ==
              "Not enough stock");
static_assert(err::registered_domain<inventory::Code>.message(static_cast<inventory::Code>(99)) ==
              "Unknown error");

TEST_CASE("make_error_code uses the domain category")
{
    const auto code = err::make_error_code(inventory::Code::NotFound);

    CHECK(code.value() == 1);
    CHECK(std::string_view { code.category().name() } == "inventory");
    CHECK(code.message() == "Product not found");
    CHECK(code.category() == err::category<inventory::Code>());
}
