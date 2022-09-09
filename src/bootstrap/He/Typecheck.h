#pragma once
#include <Core/ErrorOr.h>
#include <He/Codegen.h>
#include <He/Context.h>
#include <He/Parser.h>
#include <string_view>

namespace He {

struct TypecheckError {
    std::string_view message {};
    u32 offending_expression_index { 0 };

    Core::ErrorOr<void> show(Context const& context) const;
};

using TypecheckResult = Core::ErrorOr<Codegen, TypecheckError>;
TypecheckResult typecheck(Context& context);

}
