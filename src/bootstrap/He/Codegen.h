#pragma once
#include <Core/ErrorOr.h>
#include <Core/StringBuffer.h>
#include <He/Context.h>
#include <He/TypecheckedExpression.h>

namespace He {

Core::ErrorOr<Core::StringBuffer> codegen(Context const&,
    TypecheckedExpressions const&);

}
