#include "../inventory_errors.hpp"

#include <errors/error.hpp>
#include <string>

struct Opaque { };

auto check(const err::Error<Opaque>& error) -> err::Result<void, std::string>
{
    return err::forward(error, error.ctx().flat());
}
