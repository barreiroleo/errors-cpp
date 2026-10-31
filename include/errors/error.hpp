#pragma once

// Errors and results: Error pairs a std::error_code with an optional context, Result is a
// std::expected of it, and fail() builds the value a function returns on failure.

#include <errors/domain.hpp>

#include <concepts>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace err {

// ------------------------------------------------------------------------------------------------
// Context types: NoContext for errors created without one, and StaticString to build
// std::string_view contexts that are guaranteed not to dangle.
// ------------------------------------------------------------------------------------------------

struct NoContext { };

// A string in static storage. The consteval constructor only accepts constant expressions, so a
// view of a runtime buffer (e.g. a std::string) does not compile.
class StaticString {
public:
    constexpr StaticString() = default;

    // Implicit: lets a string literal be passed wherever a StaticString is expected.
    // NOLINTBEGIN(*-explicit-constructor,*-explicit-conversions,*-array-to-pointer-decay)
    template <class T>
        requires std::convertible_to<const T&, std::string_view>
    consteval StaticString(const T& text) : view_ { text }
    { }
    // NOLINTEND(*-explicit-constructor,*-explicit-conversions,*-array-to-pointer-decay)

    constexpr explicit operator std::string_view() const { return view_; }

private:
    std::string_view view_;
};

namespace detail {

// Non-owning strings: fail() stores them as std::string_view, via StaticString.
template <class T>
concept StringReference =
    std::same_as<std::decay_t<T>, std::string_view> ||
    std::same_as<std::decay_t<T>, StaticString> || std::same_as<std::decay_t<T>, const char*> ||
    std::same_as<std::decay_t<T>, char*>;

// Any plain object type, except non-owning strings other than std::string_view.
template <class Ctx>
concept Context =
    std::is_object_v<Ctx> && !std::is_array_v<Ctx> && std::same_as<Ctx, std::remove_cv_t<Ctx>> &&
    (!StringReference<Ctx> || std::same_as<Ctx, std::string_view>);

// What Error's constructors take to build a Ctx: std::string_view must come from a StaticString.
template <class Ctx>
using ContextArg = std::conditional_t<std::same_as<Ctx, std::string_view>, StaticString, Ctx>;

template <class Ctx>
concept HasToString = requires(const Ctx& ctx) {
    { ctx.to_string() } -> std::convertible_to<std::string>;
};

template <class Ctx>
concept FormattableContext = HasToString<Ctx> || std::formattable<Ctx, char>;

// Returns an empty string if the context can't be formatted.
template <class Ctx>
auto context_to_string(const Ctx& ctx) -> std::string
{
    if constexpr (HasToString<Ctx>) {
        return ctx.to_string();
    }
    else if constexpr (std::formattable<Ctx, char>) {
        return std::format("{}", ctx);
    }
    else {
        return { };
    }
}

} // namespace detail

// ------------------------------------------------------------------------------------------------
// ContextHandle: an error's context, returned by Error::ctx().
// ------------------------------------------------------------------------------------------------

template <detail::Context Ctx>
class ContextHandle {
public:
    explicit ContextHandle(detail::ContextArg<Ctx> ctx = { }) : ctx_ { std::move(ctx) } { }

    [[nodiscard]] auto operator->() const -> const Ctx* { return &ctx_; }
    [[nodiscard]] auto operator*() const -> const Ctx& { return ctx_; }

    [[nodiscard]] auto flat() const -> std::string
        requires detail::FormattableContext<Ctx>
    {
        return detail::context_to_string(ctx_);
    }

private:
    Ctx ctx_;
};

// ------------------------------------------------------------------------------------------------
// Error: an error code and its context. Errors with different contexts don't convert into each
// other, except from NoContext; use fail(error.code(), ...) to change it.
// ------------------------------------------------------------------------------------------------

template <detail::Context Ctx = std::string_view>
class Error {
public:
    explicit Error(std::error_code code, detail::ContextArg<Ctx> ctx = { })
        : code_ { code }, ctx_ { std::move(ctx) }
    { }

    template <RegisteredCode Code>
    explicit Error(Code code, detail::ContextArg<Ctx> ctx = { })
        : Error { make_error_code(code), std::move(ctx) }
    { }

    // Adds an empty context. A template, so that it never acts as Error<NoContext>'s copy
    // constructor.
    template <std::same_as<NoContext> Other>
        requires(!std::same_as<Ctx, NoContext> && std::default_initializable<Ctx>)
    // NOLINTNEXTLINE(*-explicit-constructor,*-explicit-conversions)
    Error(const Error<Other>& other) : code_ { other.code() }
    { }

    [[nodiscard]] auto code() const -> std::error_code { return code_; }
    [[nodiscard]] auto ctx() const -> const ContextHandle<Ctx>& { return ctx_; }
    [[nodiscard]] auto message() const -> std::string { return code_.message(); }

    // "[domain:value] message: context". The context is omitted if it can't be formatted (see
    // detail::context_to_string) or formats as an empty string.
    [[nodiscard]] auto to_string() const -> std::string
    {
        auto text = std::format("[{}:{}] {}", code_.category().name(), code_.value(), message());
        if (const auto ctx_text = detail::context_to_string(*ctx_); !ctx_text.empty()) {
            text += ": " + ctx_text;
        }
        return text;
    }

    template <RegisteredCode Code>
    [[nodiscard]] auto operator==(Code code) const -> bool
    {
        return code_ == make_error_code(code);
    }

private:
    std::error_code code_;
    [[no_unique_address]] ContextHandle<Ctx> ctx_;
};

// ------------------------------------------------------------------------------------------------
// Result and fail(): fail() deduces the context type from its argument. String literals and other
// non-owning strings become std::string_view; anything else is stored by value.
// ------------------------------------------------------------------------------------------------

template <class T, class Ctx = std::string_view>
using Result = std::expected<T, Error<Ctx>>;

// Taking a std::error_code lets a layer change an error's context while keeping its code, e.g.
// fail(error.code()) drops the context and fail(error.code(), ctx) replaces it.
inline auto fail(std::error_code code) -> std::unexpected<Error<NoContext>>
{
    return std::unexpected(Error<NoContext> { code });
}

inline auto fail(std::error_code code, StaticString ctx)
    -> std::unexpected<Error<std::string_view>>
{
    return std::unexpected(Error<std::string_view> { code, ctx });
}

template <class Ctx>
    requires(!detail::StringReference<Ctx>)
auto fail(std::error_code code, Ctx&& ctx) -> std::unexpected<Error<std::decay_t<Ctx>>>
{
    return std::unexpected(Error<std::decay_t<Ctx>> { code, std::forward<Ctx>(ctx) });
}

template <RegisteredCode Code>
auto fail(Code code) -> std::unexpected<Error<NoContext>>
{
    return fail(make_error_code(code));
}

template <RegisteredCode Code>
auto fail(Code code, StaticString ctx) -> std::unexpected<Error<std::string_view>>
{
    return fail(make_error_code(code), ctx);
}

template <RegisteredCode Code, class Ctx>
    requires(!detail::StringReference<Ctx>)
auto fail(Code code, Ctx&& ctx) -> std::unexpected<Error<std::decay_t<Ctx>>>
{
    return fail(make_error_code(code), std::forward<Ctx>(ctx));
}

// Propagates an error unchanged: only to a Result with the same context type.
template <class Ctx>
auto fail(Error<Ctx> error) -> std::unexpected<Error<Ctx>>
{
    return std::unexpected(std::move(error));
}

} // namespace err
