#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace err {

// ------------------------------------------------------------------------------------------------
// Error list: the messages of a domain, one Entry per code, validated at compile time.
// ------------------------------------------------------------------------------------------------

template <class Code>
struct Entry {
    Code code;
    std::string_view msg;
};

namespace detail {

// Not constexpr: calling them from a consteval function aborts compilation, and their names show
// up in the diagnostic.
inline void error_code_must_not_be_zero() { }
inline void error_code_must_be_unique() { }

// std::error_code treats 0 as success.
template <class Code, std::size_t N>
constexpr auto has_zero_code(const std::array<Entry<Code>, N>& entries) -> bool
{
    return std::ranges::any_of(entries, [](const Entry<Code>& entry) -> bool {
        return std::to_underlying(entry.code) == 0;
    });
}

template <class Code, std::size_t N>
constexpr auto has_duplicate_codes(const std::array<Entry<Code>, N>& entries) -> bool
{
    return std::ranges::any_of(entries, [&](const Entry<Code>& entry) -> bool {
        return std::ranges::count(entries, entry.code, &Entry<Code>::code) > 1;
    });
}

template <class Code, std::size_t N>
consteval void validate(const std::array<Entry<Code>, N>& entries)
{
    if (has_zero_code(entries)) {
        error_code_must_not_be_zero();
    }
    if (has_duplicate_codes(entries)) {
        error_code_must_be_unique();
    }
}

} // namespace detail

// C array: lets N be deduced from a braced list of designated initializers.
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
template <class Code, std::size_t N>
    requires std::is_enum_v<Code>
consteval auto error_list(const Entry<Code> (&entries)[N]) -> std::array<Entry<Code>, N>
{
    auto list = std::to_array(entries);
    detail::validate(list);
    return list;
}
// NOLINTEND(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

// ------------------------------------------------------------------------------------------------
// Domain: a named error list.
// ------------------------------------------------------------------------------------------------

template <class Code, std::size_t N>
struct Domain {
    // const char* instead of std::string_view: std::error_category::name() must return a
    // null-terminated string.
    const char* name;
    std::array<Entry<Code>, N> entries;

    // Codes outside the list (e.g. cast from an int) get a fallback message.
    [[nodiscard]] constexpr auto message(Code code) const -> std::string_view
    {
        const auto entry = std::ranges::find(entries, code, &Entry<Code>::code);
        return entry != entries.end() ? entry->msg : "Unknown error";
    }
};

// ------------------------------------------------------------------------------------------------
// Registration: maps an enum type to its Domain. RegisteredCode accepts only enums whose
// registered_domain specialization is a Domain of that same enum.
// ------------------------------------------------------------------------------------------------

struct NotRegistered { };

// Specialize for each error enum to register its domain.
template <class Code>
inline constexpr auto registered_domain = NotRegistered { };

namespace detail {

template <class Domain, class Code>
inline constexpr bool is_domain_of = false;

template <class Code, std::size_t N>
inline constexpr bool is_domain_of<Domain<Code, N>, Code> = true;

} // namespace detail

template <class Code>
concept RegisteredCode =
    std::is_enum_v<Code> &&
    detail::is_domain_of<std::remove_cv_t<decltype(registered_domain<Code>)>, Code>;

// ------------------------------------------------------------------------------------------------
// std::error_code integration: one std::error_category per registered domain, backed by its
// name and messages.
// ------------------------------------------------------------------------------------------------

namespace detail {

template <RegisteredCode Code>
class DomainCategory final : public std::error_category {
public:
    [[nodiscard]] auto name() const noexcept -> const char* override
    {
        return registered_domain<Code>.name;
    }

    [[nodiscard]] auto message(int value) const -> std::string override
    {
        return std::string { registered_domain<Code>.message(static_cast<Code>(value)) };
    }
};

} // namespace detail

// std::error_code compares categories by address, so each domain needs a single instance.
template <RegisteredCode Code>
auto category() -> const std::error_category&
{
    static const detail::DomainCategory<Code> instance;
    return instance;
}

template <RegisteredCode Code>
auto make_error_code(Code code) -> std::error_code
{
    return { static_cast<int>(std::to_underlying(code)), category<Code>() };
}

} // namespace err
