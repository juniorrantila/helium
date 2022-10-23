#pragma once
#include "Context.h"
#include "TypecheckedExpression.h"
#include <Ty/ErrorOr.h>
#include <Ty/StringBuffer.h>

namespace He {

ErrorOr<StringBuffer> codegen(Context const&,
    TypecheckedExpressions const&);

}
