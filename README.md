# errors

A minimal, header-only C++23 error-handling library intended for layered applications. Each module
can define its own error codes. Higher layers can handle errors they know, translate them, or
propagate unknown errors without depending on lower layers.

`errors` is built on top of `std::expected` and `std::error_code`.

## Defining a domain

```cpp
#pragma once
#include <errors/domain.hpp>

namespace inventory {
enum class Code { NotFound = 1, OutOfStock };

inline constexpr auto errors = err::error_list<Code>({
    { .code = Code::NotFound,   .msg = "Product not found" },
    { .code = Code::OutOfStock, .msg = "Not enough stock"  },
});
inline constexpr auto category = "inventory";
} // namespace inventory

template <>
inline constexpr auto err::registered_domain<inventory::Code> =
    err::Domain { .name = inventory::category, .entries = inventory::errors };

// Or, equivalently, at global scope:
ERR_REGISTER_DOMAIN(inventory::category, inventory::errors);
```

> [!WARNING]
> `error_list` fails to compile if a code is `0` (success for `std::error_code`) or repeated.

## Returning and handling errors

```cpp
#include <errors/error.hpp>
#include "inventory_errors.hpp"

auto available_units() -> err::Result<int>
{
    return err::fail(inventory::Code::NotFound);
}

auto validate_order() -> err::Result<void>
{
    if (auto stock = available_units(); !stock) {
        if (stock.error() == inventory::Code::NotFound) {
            // A layer can handle or translate an error it knows.
        }
        return err::fail(stock.error()); // Propagate any error unchanged.
    }

    return {}; // Success for Result<void>.
}
```

`to_string()` formats an error for logging:

```cpp
if (auto result = validate_order(); !result) {
    logger.error(result.error().to_string());
    // [inventory:1] Product not found
}

// A code outside the domain's list, e.g. cast from an int, gets a fallback message.
auto unknown = err::Error<> { static_cast<inventory::Code>(99) };
logger.error(unknown.to_string());
// [inventory:99] Unknown error
```

## Context

`err::Result<T, Ctx>` carries an optional context with the error. `Ctx` defaults to
`std::string_view`, and `fail()` deduces it from its argument:

```cpp
auto available_units() -> err::Result<int>
{
    return err::fail(inventory::Code::NotFound, "Some optional context");
    // to_string() appends the context if Ctx has a to_string() member or supports std::format:
    // [inventory:1] Product not found: Some optional context
}

struct FooCtx {
    int a { 0 };
    int b { -1 };
};

auto reserved_units() -> err::Result<int, FooCtx>
{
    return err::fail(inventory::Code::NotFound, FooCtx { .a = 1, .b = -3 });
    // FooCtx can't be formatted, so to_string() omits it:
    // [inventory:1] Product not found
}
```

An error without context, from `err::fail(code)`, can be returned as any `Result`. The context is
available through `error().context()`.

## Propagating context between layers

A lower layer A returns errors with its own context:

```cpp
struct StockCtx {
    int requested;
    int available;
};

auto reserve(int units) -> err::Result<void, StockCtx>
{
    return err::fail(inventory::Code::OutOfStock, StockCtx { .requested = units, .available = 3 });
}
```

A layer B that consumes `reserve()` can only propagate the error as-is if it returns the same
context type. Otherwise it must choose what to do with the context; an error with a different
context doesn't compile.

### B knows A's context

B can propagate it unchanged:

```cpp
auto place_order() -> err::Result<void, StockCtx>
{
    if (auto reserved = reserve(5); !reserved) {
        return err::fail(reserved.error());
    }
    return {};
}
```

Or convert it to its own context:

```cpp
auto place_order() -> err::Result<void, std::string>
{
    if (auto reserved = reserve(5); !reserved) {
        const auto& stock = reserved.error().context();
        return err::fail(reserved.error().with_context(
            std::format("requested {}, available {}", stock.requested, stock.available)));
    }
    return {};
}
```

### B doesn't know A's context

B can drop it:

```cpp
auto place_order() -> err::Result<void>
{
    if (auto reserved = reserve(5); !reserved) {
        return err::fail(reserved.error().without_context());
    }
    return {};
}
```

Or replace it:

```cpp
auto place_order() -> err::Result<void>
{
    if (auto reserved = reserve(5); !reserved) {
        return err::fail(reserved.error().with_context("while placing the order"));
    }
    return {};
}
```

## Example

[`examples/layered`](examples/layered) puts it all together in a three-layer application.

## Building the tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Tests use Catch2, fetched with CMake's `FetchContent`.
