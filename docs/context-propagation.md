# Propagating context between layers

A lower layer A returns errors with its own context:

```cpp
struct StockCtx {
    int requested;
    int available;

    [[nodiscard]] auto to_string() const -> std::string
    {
        return std::format("requested {}, available {}", requested, available);
    }
};

auto reserve(int units) -> err::Result<void, StockCtx>
{
    return err::fail(inventory::Code::OutOfStock, StockCtx { .requested = units, .available = 3 });
}
```

A layer B that consumes `reserve()` can only propagate the error as-is if it returns the same
context type. Otherwise it must choose what to do with the context; an error with a different
context doesn't compile.

`forward()` propagates an error. With a second argument, it replaces the context; pass
`err::no_context` to drop it. Either way the code, and so its domain in `to_string()`, is kept.

## B knows A's context

B can propagate it unchanged:

```cpp
auto place_order() -> err::Result<void, StockCtx>
{
    if (auto reserved = reserve(5); !reserved) {
        return err::forward(reserved.error());
    }
    return {};
}
```

Or convert it to its own context:

```cpp
struct OrderCtx {
    int missing;
};

auto place_order() -> err::Result<void, OrderCtx>
{
    if (auto reserved = reserve(5); !reserved) {
        const auto& error = reserved.error();
        const auto& stock = *error.ctx();
        return err::forward(error, OrderCtx { .missing = stock.requested - stock.available });
    }
    return {};
}
```

## B doesn't know A's context

B can drop it:

```cpp
auto place_order() -> err::Result<void>
{
    if (auto reserved = reserve(5); !reserved) {
        return err::forward(reserved.error(), err::no_context);
    }
    return {};
}
```

Or replace it:

```cpp
auto place_order() -> err::Result<void>
{
    if (auto reserved = reserve(5); !reserved) {
        return err::forward(reserved.error(), "while placing the order");
    }
    return {};
}
```

Or flatten it to a `std::string`, if it has a `to_string()` member or supports `std::format`.
Otherwise `ctx().flat()` doesn't compile:

```cpp
auto place_order() -> err::Result<void, std::string>
{
    if (auto reserved = reserve(5); !reserved) {
        const auto& error = reserved.error();
        return err::forward(error, error.ctx().flat()); // "requested 5, available 3"
    }
    return {};
}
```
