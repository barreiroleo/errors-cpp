# Propagating context between layers

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

## B knows A's context

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

## B doesn't know A's context

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
