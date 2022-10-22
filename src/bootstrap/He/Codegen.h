#pragma once
#include <Core/ErrorOr.h>
#include <Core/StringBuffer.h>
#include "Context.h"
#include "TypecheckedExpression.h"

namespace He {

Core::ErrorOr<Core::StringBuffer> codegen(Context const&,
    TypecheckedExpressions const&);

}
