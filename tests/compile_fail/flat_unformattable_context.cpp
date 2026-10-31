#include "../inventory_errors.hpp"

#include <errors/error.hpp>
#include <string>

struct Opaque { };

auto check(const err::Error<Opaque>& error) -> err::Result<void, std::string>
{
    return err::fail(error.code(), error.ctx().flat());
}
