#include "inventory_errors.hpp"

#include <catch2/catch_test_macros.hpp>
#include <errors/error.hpp>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace {
struct FooCtx {
    int a { 0 };
    int b { -1 };
};

struct Described {
    std::string text;
    [[nodiscard]] auto to_string() const -> std::string { return text; }
};

auto available_units() -> err::Result<int>
{
    return err::fail(inventory::Code::NotFound);
}

auto validate_order() -> err::Result<void>
{
    auto stock = available_units();
    if (!stock) {
        return err::fail(stock.error());
    }
    return { };
}
} // namespace

// Contexts only convert from NoContext.
static_assert(std::is_convertible_v<err::Error<err::NoContext>, err::Error<FooCtx>>);
static_assert(!std::is_constructible_v<err::Error<std::string_view>, err::Error<FooCtx>>);
static_assert(!std::is_constructible_v<err::Error<FooCtx>, err::Error<std::string_view>>);

// Non-owning strings other than std::string_view are rejected as contexts.
static_assert(err::detail::Context<std::string_view>);
static_assert(!err::detail::Context<const char*>);
static_assert(!err::detail::Context<err::StaticString>);

TEST_CASE("Error without context")
{
    const auto result = validate_order();

    REQUIRE_FALSE(result);
    CHECK(result.error() == inventory::Code::NotFound);
    CHECK(inventory::Code::NotFound == result.error());
    CHECK(result.error() != inventory::Code::OutOfStock);
    CHECK(result.error().message() == "Product not found");
    CHECK(result.error().to_string() == "[inventory:1] Product not found");
}

TEST_CASE("Result<void> success")
{
    const auto result = [] -> err::Result<void> { return { }; }();

    CHECK(result);
}

TEST_CASE("String literal context")
{
    const auto error = err::fail(inventory::Code::OutOfStock, "Some optional context").error();

    STATIC_CHECK(std::is_same_v<decltype(error), const err::Error<std::string_view>>);
    CHECK(*error.ctx() == "Some optional context");
    CHECK(error.to_string() == "[inventory:2] Not enough stock: Some optional context");
}

TEST_CASE("Context formatting")
{
    SECTION("to_string member")
    {
        const auto error = err::fail(inventory::Code::NotFound, Described { .text = "described" })
                               .error();
        CHECK(error.to_string() == "[inventory:1] Product not found: described");
    }
    SECTION("std::formattable")
    {
        const auto error = err::fail(inventory::Code::NotFound, std::string { "owned" }).error();
        CHECK(error.to_string() == "[inventory:1] Product not found: owned");
    }
    SECTION("not formattable")
    {
        const auto error = err::fail(inventory::Code::NotFound, FooCtx { .a = 1, .b = -3 }).error();
        CHECK(error.ctx()->a == 1);
        CHECK(error.to_string() == "[inventory:1] Product not found");
    }
}

TEST_CASE("Changing the context")
{
    const auto error = err::Error<FooCtx> { inventory::Code::NotFound, FooCtx { .a = 1 } };

    const err::Result<void> without = err::fail(error.code());
    CHECK(without.error() == inventory::Code::NotFound);
    CHECK(without.error().ctx()->empty());

    const err::Result<void> with = err::fail(error.code(), "while validating");
    CHECK(with.error().to_string() == "[inventory:1] Product not found: while validating");

    const err::Result<void, std::string> owned = err::fail(error.code(), std::string { "owned" });
    CHECK(owned.error().to_string() == "[inventory:1] Product not found: owned");
}

TEST_CASE("Flattening the context")
{
    const auto error = err::Error<Described> { inventory::Code::NotFound,
                                               Described { .text = "described" } };

    const err::Result<void, std::string> flat = err::fail(error.code(), error.ctx().flat());
    CHECK(*flat.error().ctx() == "described");
}

TEST_CASE("Wrapping a std::error_code")
{
    const auto error = err::Error<> { std::make_error_code(std::errc::io_error) };

    CHECK(error.code() == std::errc::io_error);
}
