#include <errors/domain.hpp>

enum class Code {
    A = 0,
    B,
};

constexpr auto errors = err::error_list<Code>({
    { .code = Code::A, .msg = "A" },
    { .code = Code::B, .msg = "B" },
});
