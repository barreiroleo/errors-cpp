# errors

<!--toc:start-->
- [errors](#errors)
  - [Basic usage](#basic-usage)
    - [Defining a domain](#defining-a-domain)
    - [Returning and handling errors](#returning-and-handling-errors)
    - [Logging errors](#logging-errors)
    - [Propagating context between layers](#propagating-context-between-layers)
  - [Example](#example)
  - [Building the tests](#building-the-tests)
<!--toc:end-->

A minimal, header-only C++23 error-handling library intended for layered applications. Each module
can define its own error codes. Higher layers can handle errors they know, translate them, or
propagate unknown errors without depending on lower layers.

`errors` is built on top of `std::expected` and `std::error_code`.

## Basic usage

### Defining a domain

```cpp
#include <errors/domain.hpp>

namespace inventory {
// Codes start at 1: std::error_code treats 0 as success.
enum class Code { NotFound = 1, OutOfStock };

inline constexpr auto errors = err::error_list<Code>({
    { .code = Code::NotFound,   .msg = "Product not found" },
    { .code = Code::OutOfStock, .msg = "Not enough stock"  },
});
inline constexpr auto category = "inventory";
} // namespace inventory

ERR_REGISTER_DOMAIN(inventory::category, inventory::errors);

// Syntactic sugar for:
// template <>
// inline constexpr auto err::registered_domain<inventory::Code> =
//     err::Domain { .name = inventory::category, .entries = inventory::errors };
```

### Returning and handling errors

`err::Result<T, Ctx>` holds either a `T` or an error with an optional context of type `Ctx`.

```cpp
#include <errors/error.hpp>
#include "inventory_errors.hpp"

// Shorthand for err::Result<int, std::string_view>
auto available_units() -> err::Result<int>
{
    return err::fail(inventory::Code::NotFound);
    // Or, with a context:
    // return err::fail(inventory::Code::NotFound, "Some optional context");
}

struct FooCtx {
    int a { 0 };
};

auto reserved_units() -> err::Result<int, FooCtx>
{
    return err::fail(inventory::Code::NotFound, FooCtx { .a = 1 });
}

auto validate_order() -> err::Result<void>
{
    if (auto stock = available_units(); !stock) {
        if (stock.error() == inventory::Code::NotFound) {
            // A layer can handle or translate an error it knows.
        }
        return err::forward(stock.error()); // Propagate any error unchanged.
    }

    if (auto reserved = reserved_units(); !reserved) {
        const FooCtx& ctx = *reserved.error().ctx(); // Read the context.
        // ...
    }

    return {}; // Success for Result<void>.
}
```

### Logging errors

`to_string()` formats an error for logging. The context is appended if `Ctx` has a `to_string()`
member or supports `std::format`, otherwise it's omitted.

```cpp
if (auto result = validate_order(); !result) {
    logger.error(result.error().to_string());
    // [inventory:1] Product not found
    // [inventory:1] Product not found: Some optional context
}

// A code outside the domain's list, e.g. cast from an int, gets a fallback message.
auto unknown = err::Error<> { static_cast<inventory::Code>(99) };
logger.error(unknown.to_string());
// [inventory:99] Unknown error
```

### Propagating context between layers

A layer can propagate an error as-is only to a `Result` with the same context type. Otherwise it
converts, drops, replaces or flattens the context; see [Propagating context between
layers](docs/context-propagation.md).

| Intent                         | Call                                       |
|--------------------------------|--------------------------------------------|
| Create an error                | `err::fail(code)` / `err::fail(code, ctx)` |
| Propagate unchanged            | `err::forward(error)`                      |
| Propagate with another context | `err::forward(error, ctx)`                 |
| Propagate without context      | `err::forward(error, err::no_context)`     |

## Example

[`examples/layered`](examples/layered) puts it all together in a three-layer application.

## Building the tests

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Tests use Catch2, fetched with CMake's `FetchContent`.
